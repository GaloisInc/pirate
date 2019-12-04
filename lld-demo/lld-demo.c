char
//__attribute__((sensitive(foo_sensitivity)))
foo_msg[] = "foo\n";

static char
bar_msg[] = "bar\n";

//__attribute__((needs_capability(foo_capability)))
//__attribute__((enclave_main(foo_enclave)))
int foo_main(void) {
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

//__attribute__((sensitive(common_sensitivity)))
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

#pragma enclave trusted(enclave_foo, foo_sensitivity)
#pragma enclave trusted(enclave_bar, bar_sensitivity)
#pragma enclave capable(enclave_foo, foo_capability)

#pragma declare sensitivity(common_sensitivity)
#pragma declare sensitivity(foo_sensitivity, common_sensitivity)
#pragma declare sensitivity(bar_sensitivity, common_sensitivity)
#pragma declare capability(foo_capability)
*/

__asm__(
    "\n"
    ".set .L_bar_msg_symtab_index, 2\n"
    ".set .L_bar_sub_symtab_index, 3\n"
    ".set .L_bar_main_symtab_index, 8\n"
    ".set .L_foo_main_symtab_index, 9\n"
    ".set .L_foo_msg_symtab_index, 10\n"
    "\n"
    "    .section    .gaps.strtab,\"\",@progbits\n"
    "    .byte   0\n"
    ".set .L_foo_enclave_name, .-.gaps.strtab\n"
    "    .string \"foo_enclave\"\n"
    ".set .L_bar_enclave_name, .-.gaps.strtab\n"
    "    .string \"bar_enclave\"\n"
    ".set .L_foo_sensitivity_name, .-.gaps.strtab\n"
    "    .string \"foo_sensitivity\"\n"
    ".set .L_foo_capability_name, .-.gaps.strtab\n"
    "    .string \"foo_capability\"\n"
    ".set .L_bar_sensitivity_name, .-.gaps.strtab\n"
    "    .string \"bar_sensitivity\"\n"
    ".set .L_common_sensitivity_name, .-.gaps.strtab\n"
    "    .string \"common_sensitivity\"\n"
    "\n"
    "    .section    .gaps.captab,\"\",@progbits\n"
    "    .long   0\n"
    ".set .L_foo_msg_requirements_list, (.-.gaps.captab) >> 2\n"
    "    .long   .L_foo_sensitivity\n"
    "    .long   0\n"
    ".set .L_foo_capability_list, (.-.gaps.captab) >> 2\n"
    "    .long   .L_foo_sensitivity\n"
    "    .long   .L_foo_capability\n"
    "    .long   0\n"
    ".set .L_bar_capability_list, (.-.gaps.captab) >> 2\n"
    "    .long   .L_bar_sensitivity\n"
    "    .long   0\n"
    ".set .L_bar_sub_requirements_list, (.-.gaps.captab) >> 2\n"
    "    .long   .L_common_sensitivity\n"
    "    .long   0\n"
    ".set .L_foo_main_requirements_list, (.-.gaps.captab) >> 2\n"
    "    .long   .L_foo_capability\n"
    "    .long   0\n"
    "\n"
    "    .section    .gaps.enclaves,\"\",@progbits\n"
    "    .zero   16\n"
    ".set .L_foo_enclave, (.-.gaps.enclaves) / 16\n"
    "    .quad   .L_foo_enclave_name\n"
    "    .long   .L_foo_capability_list\n"
    "    .word   .L_foo_main_symtab_index\n"
    "    .word   0\n"
    ".set .L_bar_enclave, (.-.gaps.enclaves) / 16\n"
    "    .quad   .L_bar_enclave_name\n"
    "    .long   .L_bar_capability_list\n"
    "    .word   .L_bar_main_symtab_index\n"
    "    .word   0\n"
    "\n"
    "    .section    .gaps.capabilities,\"\",@progbits\n"
    "    .zero   16\n"
    ".set .L_foo_sensitivity, (.-.gaps.capabilities) / 16\n"
    "    .quad   .L_foo_sensitivity_name\n"
    "    .long   .L_common_sensitivity\n"
    "    .zero   4\n"
    ".set .L_foo_capability, (.-.gaps.capabilities) / 16\n"
    "    .quad   .L_foo_capability_name\n"
    "    .long   0\n"
    "    .zero   4\n"
    ".set .L_bar_sensitivity, (.-.gaps.capabilities) / 16\n"
    "    .quad   .L_bar_sensitivity_name\n"
    "    .long   .L_common_sensitivity\n"
    "    .zero   4\n"
    ".set .L_common_sensitivity, (.-.gaps.capabilities) / 16\n"
    "    .quad   .L_common_sensitivity_name\n"
    "    .long   0\n"
    "    .zero   4\n"
    "\n"
    "    .section    .gaps.symreqs,\"\",@progbits\n"
    ".set .L_bar_sub_symreq, (.-.gaps.symreqs) / 8\n"
    "    .long   .L_bar_sub_requirements_list\n"
    "    .long   .L_bar_enclave\n"
    "    .word   .L_bar_sub_symtab_index\n"
    "    .word   0\n"
    ".set .L_foo_main_symreq, (.-.gaps.symreqs) / 8\n"
    "    .long   .L_foo_main_requirements_list\n"
    "    .long   0\n"
    "    .word   .L_foo_main_symtab_index\n"
    "    .word   0\n"
    ".set .L_foo_msg_symreq, (.-.gaps.symreqs) / 8\n"
    "    .long   .L_foo_msg_requirements_list\n"
    "    .long   0\n"
    "    .word   .L_foo_msg_symtab_index\n"
    "    .word   0\n"
);
