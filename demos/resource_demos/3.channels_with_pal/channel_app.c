#include <errno.h>
#include <libpirate.h>
#include <pal/pal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma pirate enclave declare(channel_app1)
#pragma pirate enclave declare(channel_app2)

pirate_channel pcd
    __attribute__((pirate_resource("write_end", "channel_app1")))
    __attribute__((pirate_resource_param("permissions", "writeonly", "channel_app1")))
    __attribute__((pirate_resource("read_end", "channel_app2")))
    __attribute__((pirate_resource_param("permissions", "readonly", "channel_app2")));

#define TRY(_action) do{ \
    if(_action) { \
        perror(#_action); \
        exit(EXIT_FAILURE); \
    } \
}while(0)

int __attribute__((pirate_enclave_main("channel_app1"))) app1_main(void)
{
    char buf[] = "PING";
    ssize_t sz;

    printf("APP1:\t\tSending %s to APP2\n", buf);
    TRY((sz = pirate_write(pcd, buf, sizeof buf)) < 0);
    printf("APP1:\t\tWrote %ld bytes\n", sz);

    return EXIT_SUCCESS;
}

int __attribute__((pirate_enclave_main("channel_app2"))) app2_main(void)
{
    char buf[64];
    ssize_t sz;

    printf("APP2:\t\tWaiting for data\n");
    TRY((sz = pirate_read(pcd, buf, sizeof buf)) < 0);
    printf("APP2:\t\tGot %s from MY_APP1\n", buf);

    return EXIT_SUCCESS;
}
