#include <stdio.h>
#include <pal/pal.h>
#include <libpirate.h>

#pragma pirate enclave declare(server)

int clientStartChan __attribute__((
  pirate_resource("client_start", "server"),
  pirate_resource_param("permissions", "readonly")
));

struct pirate_resource_param params[] = {
    {
        .prp_name = "permissions",
        .prp_value = "readonly"
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

int main(int argc, char** argv) __attribute__((pirate_enclave_main("server"))) {
    printf("Hello from server %d.\n", clientStartChan);        
    char buffer[128];
    int r = pirate_read(clientStartChan, buffer, sizeof(buffer) - 1);
    if (r == -1) {
        printf("Pirate read failed.\n");
        return -1;
    }
    buffer[r+1] = 0;
    printf("Received: %d %s\n", r, buffer);
    return 0;
}