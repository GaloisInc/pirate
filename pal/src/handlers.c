#include <string.h>
#include <stdint.h>

#include "handlers.h"

int cstring_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const char *s = rsc->r_contents.cc_string_value;

    if(!s)
        return -1;

    if(pal_add_to_env(env, s, strlen(s)))
        return -1;

    return 0;
}

int int64_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const int64_t *n = rsc->r_contents.cc_integer_value;

    if(!n)
        return -1;

    if(pal_add_to_env(env, n, sizeof(*n)))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const bool *b = rsc->r_contents.cc_boolean_value;

    if(!b)
        return -1;

    if(pal_add_to_env(env, b, sizeof(*b)))
        return -1;

    return 0;
}

struct handler_table_entry handler_table[HANDLER_TABLE_MAX] = {
    { "boolean", &bool_resource_handler },
    { "string",  &cstring_resource_handler },
    { "integer", &int64_resource_handler },
    { NULL,      NULL },
};
