#include <errno.h>
#include <limits.h>
#include <pal/pal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma pirate enclave declare(my_app)

bool my_boolean;
int64_t my_integer;
char *my_string;
int my_file;

int __attribute__((pirate_enclave_main("my_app"))) main(void)
{
    int fd, res;

    puts("MY_APP:\t\tGetting PAL_FD from environment...");
    if((fd = get_pal_fd()) < 0) {
        fputs("MY_APP:\t\tCouldn't find PAL_FD in environment. "
                "Are we running under pal?\n", stderr);
        return EXIT_FAILURE;
    }
    printf("MY_APP:\t\tPAL_FD = %d\n", fd);

    puts("MY_APP:\t\tRequesting boolean resource \"my_boolean\"...");
    if((res = get_boolean_res(fd, "my_boolean", &my_boolean)))
        printf("MY_APP:\t\tfailed to get \"my_boolean\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else
        printf("MY_APP:\t\tmy_boolean = %s\n", my_boolean ? "true" : "false");

    puts("MY_APP:\t\tRequesting integer resource \"my_integer\"...");
    if((res = get_integer_res(fd, "my_integer", &my_integer)))
        printf("MY_APP:\t\tfailed to get \"my_integer\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else
        printf("MY_APP:\t\tmy_integer = %ld\n", my_integer);

    puts("MY_APP:\t\tRequesting string resource \"my_string\"...");
    if((res = get_string_res(fd, "my_string", &my_string)))
        printf("MY_APP:\t\tFailed to get \"my_string\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else {
        printf("MY_APP:\t\tmy_string = %s\n", my_string);
        free(my_string);
    }

    puts("MY_APP:\t\tRequesting file resource \"my_file\"...");
    if((res = get_file_res(fd, "my_file", &my_file)))
        printf("MY_APP:\t\tFailed to get \"my_file\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else {
        char buf[16] = {0};
        if(read(my_file, buf, sizeof(buf) - 1) < 0)
            printf("MY_APP:\t\tFailed to read from \"my_file\": %s\n",
                    strerror(errno));
        else
            printf("MY_APP:\t\tmy_file = %s\n", buf);
        close(my_file);
    }

    close(fd);

    return EXIT_SUCCESS;
}
