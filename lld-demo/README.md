LLD demo
--------

This is a C source file for generating a relocatable ELF with the extensions
described [here](https://github.com/GaloisInc/pirate-annotations/blob/master/spec/elf_extensions.rst)
using verbose and unweildy inline assembly. Compile it with

```c
clang -c -save-temps -ffunction-sections -fdata-sections lld-demo.c
```

Note that the result will not link, because it has no `main()`. This is where
a GAPS-aware `lld` comes in. (Coming soon!).
