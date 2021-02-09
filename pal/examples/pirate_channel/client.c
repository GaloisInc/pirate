#include <libpirate.h>
#include <stdio.h>
#include <pal/pal.h>
#include <errno.h>

#pragma pirate enclave declare(client)

int clientStartChan __attribute__((
  pirate_resource("client_start", "client"),
  pirate_resource_param("permissions", "writeonly")
));

struct pirate_resource_param params[] = {
    {
        .prp_name = "permissions",
        .prp_value = "writeonly"
    }
};

struct pirate_resource __start_pirate_res_string[] = {};
asm(".global __stop_pirate_res_string");
asm(".set __stop_pirate_res_string, __start_pirate_res_string");
struct pirate_resource __start_pirate_res_boolean[] = {};
asm(".global __stop_pirate_res_boolean");
asm(".set __stop_pirate_res_boolean, __start_pirate_res_boolean");
struct pirate_resource __start_pirate_res_integer[] = {};
asm(".global __stop_pirate_res_integer");
asm(".set __stop_pirate_res_integer, __start_pirate_res_integer");
struct pirate_resource __start_pirate_res_file[] = {};
asm(".global __stop_pirate_res_file");
asm(".set __stop_pirate_res_file, __start_pirate_res_file");
struct pirate_resource __start_pirate_res_pirate_channel[] = {
    { .pr_name = "client"
    , .pr_obj = &clientStartChan
    , .pr_params = params
    , .pr_params_len = 1
    }
};
asm(".global __stop_pirate_res_pirate_channel");
asm(".set __stop_pirate_res_pirate_channel, __start_pirate_res_pirate_channel+32");

int main(int argc, char** argv) __attribute__((pirate_enclave_main("client"))) {
    printf("Hello from client %d\n", clientStartChan);
    if (pirate_write(clientStartChan, "Hello", 5) == -1) {
        printf("Pirate write failed %d.\n", errno);
        return -1;
    }

    return 0;
}