#include <errno.h>
#include <pal/pal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma pirate enclave declare(plugin_app)

#define FATAL(...) do{fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE);}while(0)

int __attribute__((pirate_enclave_main("plugin_app"))) main(void)
{
    int err;

    int fd = get_pal_fd();
    if(fd < 0)
        FATAL("Couldn't find PAL_FD. Are we running under PAL?\n");

    if((err = pal_send_resource_request(fd, "foo", "my_foo", 0)))
        FATAL("Error sending resource request: %s\n",
                err > 0 ? pal_strerror(err) : strerror(err));

    pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);
    if(pal_recv_env(fd, &env, 0))
        FATAL("Error receiving resource: %s\n",
                err > 0 ? pal_strerror(err) : strerror(err));

    pal_env_iterator_t it = pal_env_iterator_start(&env);
    char my_string[1024];
    pal_env_iterator_strncpy(my_string, it, sizeof my_string);
    /* ^ Alternatively:
     *      char *my_string = pal_env_iterator_strdup(it);
     *      ...
     *      free(my_string);
     */

    it = pal_env_iterator_next(it);
    if(pal_env_iterator_size(it) != sizeof(double))
        FATAL("Error deserializing `my_double': Size should be %lu, not %u\n",
                sizeof(double), pal_env_iterator_size(it));
    double my_double = *(double*)pal_env_iterator_data(it);

    it = pal_env_iterator_next(it);
    if(pal_env_iterator_size(it) != sizeof(short))
        FATAL("Error deserializing `my_double': Size should be %lu, not %u\n",
                sizeof(short), pal_env_iterator_size(it));
    short my_short = *(short*)pal_env_iterator_data(it);

    pal_free_env(&env);

    printf("my_string = %s\nmy_double = %f\nmy_short = %d\n",
        my_string, my_double, my_short);

    return EXIT_SUCCESS;
}
