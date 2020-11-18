#include <libpirate.h>
#include <string.h>
#include <stdint.h>

#include "handlers.h"
#include "log.h"
#include "yaml.h"

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

int cstring_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    int ret = 0;
    char *s = NULL;

    if(pal_yaml_subdoc_find_string(&s, sd,
                true, 1, PAL_MAP_FIELD("string_value")))
        ret = -1;

    else if(pal_add_to_env(env, s, strlen(s)))
        ret = -1;

    free(s);
    return ret;
}

int int64_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    int64_t n;

    if(pal_yaml_subdoc_find_int64(&n, sd,
                true, 1, PAL_MAP_FIELD("integer_value")))
        return -1;

    if(pal_add_to_env(env, &n, sizeof n))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    bool b;

    if(pal_yaml_subdoc_find_bool(&b, sd,
                true, 1, PAL_MAP_FIELD("boolean_value")))
        return -1;

    if(pal_add_to_env(env, &b, sizeof b))
        return -1;

    return 0;
}

int file_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    char *path = NULL;
    int flags = O_RDWR;
    int ret = 0;
    int fd;

    pal_yaml_enum_schema_t fflags_schema[] = {
        {"O_RDONLY",    O_RDONLY},
        {"O_WRONLY",    O_WRONLY},
        {"O_RDWR",      O_RDWR},

        {"O_APPEND",    O_APPEND},
        {"O_ASYNC",     O_ASYNC},
        {"O_CLOEXEC",   O_CLOEXEC},
        {"O_CREAT",     O_CREAT},
        {"O_DIRECTORY", O_DIRECTORY},
        {"O_DSYNC",     O_DSYNC},
        {"O_EXCL",      O_EXCL},
        {"O_NOCTTY",    O_NOCTTY},
        {"O_NOFOLLOW",  O_NOFOLLOW},
        {"O_NONBLOCK",  O_NONBLOCK},
        {"O_SYNC",      O_SYNC},
        {"O_TRUNC",     O_TRUNC},
        PAL_YAML_ENUM_END
    };

    pal_yaml_subdoc_find_string(&path, sd,
                true, 1, PAL_MAP_FIELD("file_path"));
    pal_yaml_subdoc_find_flags(&flags, fflags_schema, sd,
                true, 1, PAL_MAP_FIELD("file_flags"));
    if(pal_yaml_subdoc_error_count(sd) > 0)
        ret = -1;

    else if((fd = open(path, flags)) < 0)
        ret = -1;
    // TODO: Allow file creation modes to be set?
    // TODO: Allow paths relative to config?

    else if(pal_add_fd_to_env(env, fd))
        ret = -1;

    free(path);
    return ret;
}
