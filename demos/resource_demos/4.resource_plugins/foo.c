#include <pal/envelope.h>
#include <pal/resource.h>

int foo_resource_handler(pal_env_t *env, const struct app *app,
        pal_yaml_subdoc_t *rsc)
{
    char my_string[1024];
    double my_double;
    short my_short;

    pal_yaml_subdoc_find_static_string(my_string, sizeof my_string, rsc,
            false, 1, PAL_MAP_FIELD("my_string"));
    pal_yaml_subdoc_find_double(&my_double, rsc,
            true, 2, PAL_MAP_FIELD("my_numbers"),
                     PAL_MAP_FIELD("my_double"));
    pal_yaml_subdoc_find_int16(&my_short, rsc,
            true, 2, PAL_MAP_FIELD("my_numbers"),
                     PAL_MAP_FIELD("my_short"));

    if(pal_yaml_subdoc_error_count(rsc) > 0)
        return -1;

    if(pal_add_to_env(env, my_string, strlen(my_string)))
        return -1;
    if(pal_add_to_env(env, &my_double, sizeof my_double))
        return -1;
    if(pal_add_to_env(env, &my_short, sizeof my_short))
        return -1;

    return 0;
}
