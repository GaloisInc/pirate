#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pal/pal.h>
#include <pal/resource_types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define alen(_arr) (sizeof(_arr) / sizeof(*_arr))

static const char *errstrs[] = {
    "Success",
    "Bad request",
    "Empty message",
    "Message too big",
    "Too many file descriptors",
    "Bad control message",
};

_Static_assert(alen(errstrs) == _PAL_ERR_MAX,
        "Number of error strings must match number of errors");

const char *pal_strerror(int err)
{
    static char unknown_err[64];

    if(err >= alen(errstrs)) {
        snprintf(unknown_err, sizeof unknown_err, "Unknown %d\n", err);
        return unknown_err;
    }

    return errstrs[err];
}

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

int pal_add_fd_to_env(pal_env_t *env, int fd)
{
    if(env->fds_count >= PAL_FDS_MAX)
        return PAL_ERR_FTOOBIG;

    env->fds[env->fds_count++] = fd;

    return 0;
}

void pal_free_env(pal_env_t *env)
{
    free(env->buf);
    env->buf = NULL;
    env->buf_size = 0;
    env->size = 0;
}

void pal_close_env_fds(pal_env_t *env)
{
    int i;
    for(i = 0; i < env->fds_count; ++i)
        if(env->fds[i] >= 0)
            close(env->fds[i]);
}

static size_t msghdr_size(struct msghdr *msg)
{
    size_t i, res = 0;

    for(i = 0; i < msg->msg_iovlen; ++i)
        res += msg->msg_iov[i].iov_len;

    return res;
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
            .iov_base = &env->fds_count,
            .iov_len = sizeof env->fds_count,
        },
        {
            .iov_base = env->buf,
            .iov_len = env->size,
        },
    };
    struct msghdr msg = MSGHDR(iovs, sizeof(iovs) / sizeof(*iovs));

    char buf[CMSG_SPACE(sizeof(int[env->fds_count]))];
    if(env->fds_count > 0) {
        msg.msg_control = buf;
        msg.msg_controllen = sizeof(buf);

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int[env->fds_count]));

        memcpy(CMSG_DATA(cmsg), env->fds, sizeof(int[env->fds_count]));
    }

    size_t total_sent = msghdr_size(&msg);
    if(sendmsg(sock, &msg, flags) != total_sent)
        return -errno;

    return 0;
}

/* Try to read the `type`, `length`, and `fds_count` fields of a `pal_env_t`
 * from a socket using `MSG_PEEK` and allocate a buffer for the environment
 * data. The `msg_iov` field is assumed to point to an appropriate number of
 * `struct iovec`.
 *
 * Returns 0 on success. Otherwise returns a suitable return value for
 * `pal_recv_env`.
 */
static int peek_env(int sock, pal_env_t *env, struct msghdr *msg, int flags)
{
    size_t iovs_count = msg->msg_iovlen;
    struct iovec *iovs = msg->msg_iov;

    assert(iovs_count == 4);

    iovs[0].iov_base = &env->type;
    iovs[0].iov_len = sizeof env->type;

    iovs[1].iov_base = &env->size;
    iovs[1].iov_len = sizeof env->size;

    iovs[2].iov_base = &env->fds_count;
    iovs[2].iov_len = sizeof env->fds_count;

    iovs[3].iov_base = NULL;
    iovs[3].iov_len = 0;

    ssize_t bytes = recvmsg(sock, msg, MSG_PEEK | flags);
    if(!bytes)
        return PAL_ERR_EMPTY;
    else if(bytes != msghdr_size(msg))
        return PAL_ERR_BADREQ;

    if(env->size > PAL_MSG_MAX)
        return PAL_ERR_TOOBIG; // Length field exceeds max
    if(!(env->buf = malloc(env->size)))
        return -errno;
    iovs[3].iov_base = env->buf;
    iovs[3].iov_len = env->buf_size = env->size;

    if(env->fds_count > PAL_FDS_MAX)
        return PAL_ERR_FTOOBIG;

    return 0;
}

int pal_recv_env(int sock, pal_env_t *env, int flags)
{
    int res;
    pal_env_t new_env;
    struct iovec iovs[4];
    struct msghdr msg = MSGHDR(iovs, alen(iovs));

    if((res = peek_env(sock, &new_env, &msg, flags))) {
        char buf[65536];
        recv(sock, buf, sizeof buf, flags); // Clear out peeked datagram
        // FIXME: The above may cause problems on resource limited
        // architectures. Problem: We want to clear out erroneous datagrams,
        // but only clear out the first such datagram.
        return res;
    }

    char cbuf[CMSG_SPACE(sizeof(int[new_env.fds_count]))];
    msg.msg_control = sizeof cbuf > 0 ? cbuf : NULL;
    msg.msg_controllen = sizeof cbuf;

    if(recvmsg(sock, &msg, flags) != msghdr_size(&msg))
        return -errno;

    if(msg.msg_flags & MSG_CTRUNC)
        return PAL_ERR_BADCTRL;

    struct cmsghdr *cmsg;
    for(cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
        if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            memcpy(new_env.fds, CMSG_DATA(cmsg),
                    sizeof(int[new_env.fds_count]));

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
    else if(env.type != PAL_RESOURCE)
        res = 1;
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
    else if(env.type != PAL_RESOURCE)
        res = 1;
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
    else if(env.type != PAL_RESOURCE)
        res = 1;
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

int get_file_res(int fd, const char *name, int *outp)
{
    int res = 0;
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "file", name, 0)))
        ;
    else if((res = pal_recv_env(fd, &env, 0)))
        ;
    else if(env.type != PAL_RESOURCE)
        res = 1;
    else if(env.fds_count != 1)
        res = 1;
    else
        *outp = env.fds[0];

    pal_free_env(&env);

    return res;
}
