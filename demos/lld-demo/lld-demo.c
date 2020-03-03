char foo_msg[]
//__attribute__((capability(foo_sensitivity)))
    = "foo\n";

static char bar_msg[]
    = "bar\n";

//__attribute__((capability(foo_capability)))
//__attribute__((enclave_main(foo_enclave)))
int main(void) {
    __asm__(
        "mov $1, %%rax\n\t"
        "mov $1, %%rdi\n\t"
        "mov %0, %%rsi\n\t"
        "mov $4, %%rdx\n\t"
        "syscall"
        :
        : "r" (foo_msg)
        : "%rax", "%rdi", "%rsi", "%rdx"
    );
    return 0;
}

//__attribute__((capability(common_sensitivity)))
//__attribute__((enclave_only(bar_enclave)))
static void bar_sub(void) {
    __asm__(
        "mov $1, %%rax\n\t"
        "mov $1, %%rdi\n\t"
        "mov %0, %%rsi\n\t"
        "mov $4, %%rdx\n\t"
        "syscall"
        :
        : "r" (bar_msg)
        : "%rax", "%rdi", "%rsi", "%rdx"
    );
}

//__attribute__((enclave_main(bar_enclave)))
int bar_main(void) {
    __asm__(
        "mov $1, %%rax\n\t"
        "mov $1, %%rdi\n\t"
        "mov %0, %%rsi\n\t"
        "mov $4, %%rdx\n\t"
        "syscall"
        :
        : "r" (bar_msg)
        : "%rax", "%rdi", "%rsi", "%rdx"
    );
    bar_sub();
    return 0;
}

/*
#pragma enclave declare(enclave_foo)
#pragma enclave declare(enclave_bar)

#pragma capability declare(common_sensitivity)
#pragma capability declare(foo_sensitivity, common_sensitivity)
#pragma capability declare(bar_sensitivity, common_sensitivity)
#pragma capability declare(foo_capability)

#pragma enclave capability(enclave_foo, foo_sensitivity)
#pragma enclave capability(enclave_bar, bar_sensitivity)
#pragma enclave capability(enclave_foo, foo_capability)
*/

__asm__(
    "\n"
    ".set .L_bar_msg_symtab_index, 2\n"
    ".set .L_bar_sub_symtab_index, 3\n"
    ".set .L_bar_main_symtab_index, 8\n"
    ".set .L_foo_main_symtab_index, 9\n"
    ".set .L_foo_msg_symtab_index, 10\n"
    "\n"
    "    .section    .pirate.strtab,\"\",@progbits\n"
    "    .byte   0\n"
    ".set .L_foo_enclave_name, .-.pirate.strtab\n"
    "    .string \"foo_enclave\"\n"
    ".set .L_bar_enclave_name, .-.pirate.strtab\n"
    "    .string \"bar_enclave\"\n"
    ".set .L_foo_sensitivity_name, .-.pirate.strtab\n"
    "    .string \"foo_sensitivity\"\n"
    ".set .L_foo_capability_name, .-.pirate.strtab\n"
    "    .string \"foo_capability\"\n"
    ".set .L_bar_sensitivity_name, .-.pirate.strtab\n"
    "    .string \"bar_sensitivity\"\n"
    ".set .L_common_sensitivity_name, .-.pirate.strtab\n"
    "    .string \"common_sensitivity\"\n"
    "\n"
    "    .p2align 2\n"
    "    .section    .pirate.captab,\"\",@progbits\n"
    "    .long   0\n"
    ".set .L_foo_msg_requirements_list, (.-.pirate.captab) >> 2\n"
    "    .long   .L_foo_sensitivity\n"
    "    .long   0\n"
    ".set .L_foo_capability_list, (.-.pirate.captab) >> 2\n"
    "    .long   .L_foo_sensitivity\n"
    "    .long   .L_foo_capability\n"
    "    .long   0\n"
    ".set .L_bar_capability_list, (.-.pirate.captab) >> 2\n"
    "    .long   .L_bar_sensitivity\n"
    "    .long   0\n"
    ".set .L_bar_sub_requirements_list, (.-.pirate.captab) >> 2\n"
    "    .long   .L_common_sensitivity\n"
    "    .long   0\n"
    ".set .L_foo_main_requirements_list, (.-.pirate.captab) >> 2\n"
    "    .long   .L_foo_capability\n"
    "    .long   0\n"
    "\n"
    "    .p2align 3\n"
    "    .section    .pirate.enclaves,\"\",@progbits\n"
    "    .zero   16\n"
    ".set .L_foo_enclave, (.-.pirate.enclaves) / 16\n"
    "    .quad   .L_foo_enclave_name\n"
    "    .long   .L_foo_capability_list\n"
    "    .word   .L_foo_main_symtab_index\n"
    "    .word   0\n"
    ".set .L_bar_enclave, (.-.pirate.enclaves) / 16\n"
    "    .quad   .L_bar_enclave_name\n"
    "    .long   .L_bar_capability_list\n"
    "    .word   .L_bar_main_symtab_index\n"
    "    .word   0\n"
    "\n"
    "    .p2align 3\n"
    "    .section    .pirate.capabilities,\"\",@progbits\n"
    "    .zero   16\n"
    ".set .L_foo_sensitivity, (.-.pirate.capabilities) / 16\n"
    "    .quad   .L_foo_sensitivity_name\n"
    "    .long   .L_common_sensitivity\n"
    "    .zero   4\n"
    ".set .L_foo_capability, (.-.pirate.capabilities) / 16\n"
    "    .quad   .L_foo_capability_name\n"
    "    .long   0\n"
    "    .zero   4\n"
    ".set .L_bar_sensitivity, (.-.pirate.capabilities) / 16\n"
    "    .quad   .L_bar_sensitivity_name\n"
    "    .long   .L_common_sensitivity\n"
    "    .zero   4\n"
    ".set .L_common_sensitivity, (.-.pirate.capabilities) / 16\n"
    "    .quad   .L_common_sensitivity_name\n"
    "    .long   0\n"
    "    .zero   4\n"
    "\n"
    "    .p2align 2\n"
    "    .section    .pirate.symreqs,\"\",@progbits\n"
    ".set .L_bar_sub_symreq, (.-.pirate.symreqs) / 8\n"
    "    .long   .L_bar_sub_requirements_list\n"
    "    .long   .L_bar_enclave\n"
    "    .word   .L_bar_sub_symtab_index\n"
    "    .word   0\n"
    ".set .L_foo_main_symreq, (.-.pirate.symreqs) / 8\n"
    "    .long   .L_foo_main_requirements_list\n"
    "    .long   0\n"
    "    .word   .L_foo_main_symtab_index\n"
    "    .word   0\n"
    ".set .L_foo_msg_symreq, (.-.pirate.symreqs) / 8\n"
    "    .long   .L_foo_msg_requirements_list\n"
    "    .long   0\n"
    "    .word   .L_foo_msg_symtab_index\n"
    "    .word   0\n"
);
