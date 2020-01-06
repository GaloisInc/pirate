LLD demo
--------

This is a C source file for generating a relocatable ELF with the extensions
described [here](https://github.com/GaloisInc/pirate-annotations/blob/master/spec/elf_extensions.rst)
using verbose and unweildy inline assembly. Compile it with

```sh
clang -c -ffunction-sections -fdata-sections lld-demo.c
```

Note that the result will not link with out-of-the-box lld, because it has no
`main()`. (Calling a function `main()` is allowed, but not required, because
the `enclave_main` annotation is used to resolve the reference in `_start`.)
With a GAPS-aware lld, it can be linked for two enclaves (called `enclave_foo`
and `enclave_bar`) as follows:

```sh
ld.lld -enclave foo_enclave -o lld-demo-foo lld-demo.o \
    /usr/lib/x86_64-linux-gnu/crt*.o /usr/lib/x86_64-linux-gnu/libc.so
    -dynamic-linker /lib64/ld-linux-x86-64.so.2
ld.lld -enclave foo_enclave -o lld-demo-bar lld-demo.o \
    /usr/lib/x86_64-linux-gnu/crt*.o /usr/lib/x86_64-linux-gnu/libc.so
    -dynamic-linker /lib64/ld-linux-x86-64.so.2
```

With a GAPS-aware `readelf`, the either of the following commands will print out
the GAPS sections in an object file (in GNU or LLVM format, respectively):

```sh
llvm-readelf --gaps-info lld-demo.o
# or
llvm-readobj --gaps-info lld-demo.o
```

**BUG:** Creating sections in this way fails to correcty fill in the `sh_entsize`
fields in the corresponding section-header entries. As far as I can tell, this
is a limitation of the `.section` assembler directive, so there is no way to fix
it while still using inline assembly to create sections. Fortunately, lld seems
to ignore this field, but other tools may complain. A GAPS-aware compiler would,
of course, not have this limitation.
