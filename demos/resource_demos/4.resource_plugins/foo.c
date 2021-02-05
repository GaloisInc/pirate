#include <pal/envelope.h>
#include <pal/resource.h>

int foo_resource_handler(pal_env_t *env, const struct app *app,
        pal_context_t *ctx, pal_config_node_t* root)
{
    char my_string[1024];
    double my_double;
    short my_short;
    pal_config_static_string(my_string, sizeof my_string, ctx, root, false, "my_string", 0);
    pal_config_double(&my_double, ctx, root, true, "my_numbers", "my_double", 0);
    pal_config_int16(&my_short, ctx, root, true, "my_numbers", "my_short", 0);

    if(pal_error_count(ctx) > 0)
        return -1;

    if(pal_add_to_env(env, my_string, strlen(my_string)))
        return -1;
    if(pal_add_to_env(env, &my_double, sizeof my_double))
        return -1;
    if(pal_add_to_env(env, &my_short, sizeof my_short))
        return -1;

    return 0;
}
