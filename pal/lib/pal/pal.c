#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pal/pal.h"
#include <libpirate.h>
#include "unistd.h"
#include <stdarg.h>

int vasprintf(char **ret, const char *format, va_list ap);
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

static
void debugLog(const char* fmt, ...) {
    char* s;
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(&s, fmt, ap);
    va_end(ap);
    if (r < 0) {
        const char* msg = "LOG FAILED.\n";
        write(STDERR_FILENO, msg, strlen(msg));
    }
    write(STDERR_FILENO, s, strlen(s));
}


// FIXME: Do we want to sort these in clang so we can search smarter here?
const char *lookup_pirate_resource_param(struct pirate_resource *pr, const char *name) {
    for (size_t i = 0; i < pr->pr_params_len; ++i) {
        if (!strcmp(pr->pr_params[i].prp_name, name))
            return pr->pr_params[i].prp_value;
    }

    return NULL;
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
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "string", name, 0)))
        ;
    if((res = pal_recv_env(fd, &env, 0)))
        ;
    else if(env.type != PAL_RESOURCE)
        res = 1;
    else if(!(*outp = pal_env_iterator_strdup(pal_env_iterator_start(&env))))
        res = -errno;

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

int get_pirate_channel_cfg(int fd, const char *name, char **outp)
{
    int res = 0;
    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);

    if((res = pal_send_resource_request(fd, "pirate_channel", name, 0)))
        ;
    if((res = pal_recv_env(fd, &env, 0)))
        ;
    else if(env.type != PAL_RESOURCE)
        res = 1;
    else if(!(*outp = pal_env_iterator_strdup(pal_env_iterator_start(&env))))
        res = -errno;

    pal_free_env(&env);

    return res;
}

/*
 * Automatic resource initializers
 */

typedef int (*get_func_t)(int fd, const char *name, void *datap);

static void init_resources_common(const char *type, get_func_t get_func,
        struct pirate_resource *start, struct pirate_resource *stop)
{
    int fd;
    struct pirate_resource *pr;

    if(start == stop)
        return; // No resources present

    if((fd = get_pal_fd()) < 0) {
        fputs("PAL resources declared, but no PAL_FD present in environment. "
                "Are we running with PAL?\n", stderr);
        exit(1);
    }

    for(pr = start; pr < stop; ++pr) {
        int err;

        if(!pr || !pr->pr_name || !pr->pr_obj)
            fprintf(stderr, "Invalid pirate resource section for `%s'\n",
                    type);

        if((err = get_func(fd, pr->pr_name, pr->pr_obj))) {
            fprintf(stderr, "Fatal error getting %s resource %s: %s\n",
                    type, pr->pr_name,
                    err > 0 ? pal_strerror(err) : strerror(-err));
            exit(1);
        }
    }
}

extern struct pirate_resource __start_pirate_res_string[];
extern struct pirate_resource __stop_pirate_res_string[];

void __attribute__((constructor)) init_string_resources()
{
    init_resources_common("string", (get_func_t)&get_string_res, __start_pirate_res_string, __stop_pirate_res_string);
}

extern struct pirate_resource __start_pirate_res_integer[];
extern struct pirate_resource __stop_pirate_res_integer[];

void __attribute__((constructor)) init_integer_resources()
{
    init_resources_common("integer", (get_func_t)&get_integer_res,
            __start_pirate_res_integer, __stop_pirate_res_integer);
}

extern struct pirate_resource __start_pirate_res_boolean[];
extern struct pirate_resource __stop_pirate_res_boolean[];

void __attribute__((constructor)) init_boolean_resources()
{
    init_resources_common("boolean", (get_func_t)&get_boolean_res,
            __start_pirate_res_boolean, __stop_pirate_res_boolean);
}

extern struct pirate_resource __start_pirate_res_file[];
extern struct pirate_resource __stop_pirate_res_file[];

void __attribute__((constructor)) init_file_resources()
{
    init_resources_common("file", (get_func_t)&get_file_res,
            __start_pirate_res_file, __stop_pirate_res_file);
}

extern struct pirate_resource __start_pirate_res_pirate_channel[];
extern struct pirate_resource __stop_pirate_res_pirate_channel[];

void __attribute__((constructor)) init_pirate_channel_resources()
{
    struct pirate_resource *start = __start_pirate_res_pirate_channel;
    struct pirate_resource *stop = __stop_pirate_res_pirate_channel;

    if (start == stop)
        return; // No resources present

    int fd = get_pal_fd();
    if (fd < 0) {
        debugLog("PAL resources declared, but no PAL_FD present in environment.\n"
                 "Are we running with PAL?\n");
        exit(1);
    }

    bool errors = false;
    for (struct pirate_resource *pr = start; pr < stop; ++pr) {
        //int perms = O_RDWR;
        if (!pr || !pr->pr_name || !pr->pr_obj) {
            debugLog("Invalid pirate resource section for `pirate_channel'\n");
            errors = true;
            continue;
        }
 
        char *cfg;
        int err = get_pirate_channel_cfg(fd, pr->pr_name, &cfg);        
        if (err) {
            debugLog("Fatal error getting pirate_channel %s: %s\n", pr->pr_name,
                     err > 0 ? pal_strerror(err) : strerror(-err));
            errors = true;
            continue;
        }

        const char *permstr = lookup_pirate_resource_param(pr, "permissions");
        if (!permstr) {
            debugLog("Missing permissions on %s.\n", pr->pr_name);
            errors = true;
            continue;
        }

        int perms;
        if (!strcmp(permstr, "readonly")) {
            perms = O_RDONLY;
        } else if (!strcmp(permstr, "writeonly")) {
            perms = O_WRONLY;
        } else {
            debugLog("Expected permissions of \"readonly\" or \"writeonly\" on %s: Found %s\n", pr->pr_name, permstr);
            errors = true;
            continue;
        }

        int* chanPtr = pr->pr_obj;
        *chanPtr = pirate_open_parse(cfg, perms);
        if (*chanPtr < 0) {
            debugLog("Fatal error opening pirate_channel %s: %s\n", pr->pr_name, strerror(errno));
            errors = true;
        }

        free(cfg);
    }
    if (errors) exit(-1);
}
