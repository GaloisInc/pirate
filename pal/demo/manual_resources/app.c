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

    puts("Getting PAL_FD from environment...");
    if((fd = get_pal_fd()) < 0) {
        fputs("Couldn't find PAL_FD in environment. "
                "Are we running under pal?\n", stderr);
        return EXIT_FAILURE;
    }
    printf("PAL_FD = %d\n", fd);

    puts("Requesting boolean resource \"my_boolean\"...");
    if((res = get_boolean_res(fd, "my_boolean", &my_boolean)))
        printf("failed to get \"my_boolean\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else
        printf("my_boolean = %s\n", my_boolean ? "true" : "false");

    puts("Requesting integer resource \"my_integer\"...");
    if((res = get_integer_res(fd, "my_integer", &my_integer)))
        printf("failed to get \"my_integer\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else
        printf("my_integer = %ld\n", my_integer);

    puts("Requesting string resource \"my_string\"...");
    if((res = get_string_res(fd, "my_string", &my_string)))
        printf("failed to get \"my_string\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else {
        printf("my_string = %s\n", my_string);
        free(my_string);
    }

    puts("Requesting file resource \"my_file\"...");
    if((res = get_file_res(fd, "my_file", &my_file)))
        printf("failed to get \"my_file\": %s\n",
                res > 0 ? "Resource format incorrect" : strerror(errno));
    else {
        char buf[16] = {0};
        if(read(my_file, buf, sizeof(buf) - 1) < 0)
            printf("Failed to read from \"my_file\": %s\n", strerror(errno));
        else
            printf("my_file = %s\n", buf);
        close(my_file);
    }

    close(fd);

    return EXIT_SUCCESS;
}
