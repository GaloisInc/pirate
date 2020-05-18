#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pirate enclave declare(orange)

typedef int int_resource __attribute__((pirate_resource_type("int")));

int_resource
    foo __attribute__((pirate_resource("foo", "orange"))) = 1,
    bar __attribute__((pirate_resource("bar", "orange"))) = 2,
    zip __attribute__((pirate_resource("zip", "orange")))
        __attribute__((pirate_resource_param("wiz", "woz")))
        __attribute__((pirate_resource_param("zap", "wow"))) = 3;

struct pirate_resource_param {
    char *prp_name;
    char *prp_value;
};

struct pirate_resource {
    char *pr_name;
    void *pr_obj;
    struct pirate_resource_param *pr_params;
    uint64_t pr_params_len;
} __attribute__((packed));

extern struct pirate_resource __start_pirate_res_int[];
extern struct pirate_resource __stop_pirate_res_int[];

int __attribute((pirate_enclave_main("orange")))
orange_main(void) {
    size_t i, j;

    for(i = 0; &__start_pirate_res_int[i] < __stop_pirate_res_int; ++i) {
        struct pirate_resource *pr = &__start_pirate_res_int[i];

        printf("resource %lu: %s = %d\n", i, pr->pr_name, *(int *)pr->pr_obj);
        for(j = 0; j < pr->pr_params_len; ++j) {
            struct pirate_resource_param *prp = &pr->pr_params[j];

            printf("\t\"%s\" = \"%s\"\n", prp->prp_name, prp->prp_value);
        }
    }

    return 0;
}
