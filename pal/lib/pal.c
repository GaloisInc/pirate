#include <errno.h>
#include <limits.h>
#include <pal/pal.h>
#include <pal/resource_types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

static void add_to_env(pal_env_t *env, const void *data, size_t data_size)
{
    memcpy(&env->buf[env->size], data, data_size);
    env->size += data_size;
}

int pal_add_to_env(pal_env_t *env, const void *data, pal_env_size_t data_size)
{
    size_t new_size = env->size + sizeof data_size + data_size;

    if(new_size > PAL_MSG_MAX)
        return PAL_ERR_TOOBIG;

    // Allocate buffer in 1k chunks as needed
    if(new_size > env->buf_size) {
        size_t new_buf_size = (new_size + 1023) % 1024;
        char *new_buf = realloc(env->buf, new_buf_size);
        if(!new_buf)
            return -errno;
        env->buf = new_buf;
        env->buf_size = new_buf_size;
    }

    add_to_env(env, &data_size, sizeof data_size);
    add_to_env(env, data, data_size);

    return 0;
}

void pal_free_env(pal_env_t *env)
{
    free(env->buf);
    env->buf = NULL;
    env->buf_size = 0;
    env->size = 0;
}

#define MSGHDR(_iovs, _iovs_count) { \
        .msg_name = NULL, \
        .msg_namelen = 0, \
        .msg_iov = (_iovs), \
        .msg_iovlen = (_iovs_count), \
        .msg_control = NULL, \
        .msg_controllen = 0, \
        .msg_flags = 0, \
}

int pal_send_env(int sock, pal_env_t *env, int flags)
{
    struct iovec iovs[] = {
        {
            .iov_base = &env->type,
            .iov_len = sizeof env->type,
        },
        {
            .iov_base = &env->size,
            .iov_len = sizeof env->size,
        },
        {
            .iov_base = env->buf,
            .iov_len = env->size,
        },
    };
    struct msghdr msg = MSGHDR(iovs, sizeof(iovs) / sizeof(*iovs));

    size_t total_sent = iovs[0].iov_len + iovs[1].iov_len + iovs[2].iov_len;
    if(sendmsg(sock, &msg, flags) != total_sent)
        return -errno;

    return 0;
}

static size_t msghdr_size(struct msghdr *msg)
{
    size_t i, res = 0;

    for(i = 0; i < msg->msg_iovlen; ++i)
        res += msg->msg_iov[i].iov_len;

    return res;
}

int pal_recv_env(int sock, pal_env_t *env, int flags)
{
    ssize_t bytes;
    pal_env_t new_env;
    struct iovec iovs[] = {
        {
            .iov_base = &new_env.type,
            .iov_len = sizeof new_env.type,
        },
        {
            .iov_base = &new_env.size,
            .iov_len = sizeof new_env.size,
        },
        {
            .iov_base = NULL,
            .iov_len = 0,
        },
    };
    struct msghdr msg = MSGHDR(iovs, sizeof(iovs) / sizeof(*iovs));

    bytes = recvmsg(sock, &msg, MSG_PEEK | flags);
    if(!bytes)
        return PAL_ERR_EMPTY;
    else if(bytes != msghdr_size(&msg))
        return PAL_ERR_BADREQ;

    if(new_env.size > PAL_MSG_MAX)
        return PAL_ERR_TOOBIG; // Length field exceeds max
    if(!(new_env.buf = malloc(new_env.size)))
        return -errno;
    iovs[2].iov_base = new_env.buf;
    iovs[2].iov_len = new_env.buf_size = new_env.size;

    if(recvmsg(sock, &msg, flags) != msghdr_size(&msg))
        return -errno;

    *env = new_env;
    return 0;
}

int pal_send_resource_request(int sock,
        const char *type, const char *name, int flags)
{
    int res;
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE_REQUEST);

    if((res = pal_add_to_env(&env, type, strlen(type))))
        ;
    else if((res = pal_add_to_env(&env, name, strlen(name))))
        ;
    else if((res = pal_send_env(sock, &env, flags)))
        ;

    pal_free_env(&env);
    return res;
}

/* Duplicate a non-zero-terminated string containing `size` characters.
 */
static void *strdupnz(const char *src, size_t size)
{
    char *dst = malloc(size + 1);
    if(!dst)
        return NULL;

    memcpy(dst, src, size);
    dst[size] = '\0';

    return dst;
}

int pal_recv_resource_request(int sock, char **typep, char **namep, int flags)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);
    pal_env_iterator_t type_it, name_it, end_it;
    char *type = NULL, *name = NULL;
    int res = 0;

    if((res = pal_recv_env(sock, &env, flags)))
        return res;

    end_it = pal_env_iterator_end(&env);

    if((type_it = pal_env_iterator_start(&env)) >= end_it)
        res = PAL_ERR_BADREQ; // No resource type present
    else if(!(type = strdupnz(pal_env_iterator_data(type_it),
                              pal_env_iterator_size(type_it))))
        res = -errno;
    else if((name_it = pal_env_iterator_next(type_it)) >= end_it)
        res = PAL_ERR_BADREQ; // No resource name present
    else if(!(name = strdupnz(pal_env_iterator_data(name_it),
                              pal_env_iterator_size(name_it))))
        res = -errno;

    pal_free_env(&env);
    if(res) {
        free(type);
        free(name);
    } else {
        *namep = name;
        *typep = type;
    }

    return res;
}

/*
 * Application resource getters
 */

int get_pal_fd()
{
    long res;
    char *fdstr, *endptr;

    if(!(fdstr = getenv("PAL_FD")))
        return -1;

    errno = 0;
    res = strtol(fdstr, &endptr, 10);

    if(errno || *endptr || res > INT_MAX || res < 0)
        return -1;

    return res;
}

int get_boolean_res(int fd, const char *name, bool *outp)
{
    int res = 0;
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "boolean", name, 0)))
        ;
    else if((res = pal_recv_env(fd, &env, 0)))
        ;
    else {
        pal_env_iterator_t it = pal_env_iterator_start(&env);

        if(pal_env_iterator_size(it) != sizeof(*outp))
            res = 1;
        else
            memcpy(outp, pal_env_iterator_data(it), sizeof(*outp));
    }

    pal_free_env(&env);

    return res;
}

int get_integer_res(int fd, const char *name, int64_t *outp)
{
    int res = 0;
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "integer", name, 0)))
        ;
    else if((res = pal_recv_env(fd, &env, 0)))
        ;
    else {
        pal_env_iterator_t it = pal_env_iterator_start(&env);

        if(pal_env_iterator_size(it) != sizeof(*outp))
            res = 1;
        else
            memcpy(outp, pal_env_iterator_data(it), sizeof(*outp));
    }

    pal_free_env(&env);

    return res;
}

int get_string_res(int fd, const char *name, char **outp)
{
    int res = 0;
    size_t size;
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "string", name, 0)))
        ;
    if((res = pal_recv_env(fd, &env, 0)))
        ;
    else {
        pal_env_iterator_t it = pal_env_iterator_start(&env);

        size = pal_env_iterator_size(it);
        if(!(*outp = malloc(size + 1)))
            res = -errno;
        else {
            memcpy(*outp, pal_env_iterator_data(it), size);
            (*outp)[size] = '\0';
        }
    }

    pal_free_env(&env);

    return res;
}
