#include <pal/envelope.h>
#include <pal/pal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

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
