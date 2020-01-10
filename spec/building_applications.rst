Building Enclaves
^^^^^^^^^^^^^^^^^

[Note: This documentation is still under development, and highly subject to change.]

After compiling one or more C source files into object files using
enclave-aware compilers, one can generate an executable that runs the
enclaves by running passing ``--enclave name`` to ``lld``
along with other linker options and object files.  This will result in
`lld` producing an executable that establishes the communication
channels and launches each of the enclave main function at startup.
This capability is intended for testing purposes, and does not
provide physical security protections between enclaves.  A version
of ``lld`` with these protection guarantees will be developed once
suitable hardware is available.

If ``lld`` does not find the main function for one of the enclaves,
then an error will be reported.

Building LLVM
---------------

Assumed tools: `git`, `cmake`, `ninja`, C compiler

.. code-block:: sh

    # clone the repository
    $ git clone git@github.com:GaloisInc/pirate-llvm

    # make a build directory and build
    $ mkdir llvm-ninja
    $ cd llvm-ninja
    $ cmake -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
      -DLLVM_ENABLE_PROJECTS=clang ../private-llvm/llvm
    $ ninja

    # leave build directory
    $ cd ..

    # build a trivial example
    $ llvm-ninja/bin/clang -ffunction-sections -fdata-sections \
      --target=x86_64-pc-linux-elf -c enclave.c

    # see that the example worked
    $ llvm-ninja/bin/llvm-readobj --gaps-info enclave.o
    
    # link the example to produce an executable for the alpha enclave
    $ ld.lld -dynamic-linker /lib64/ld-linux-x86-64.so.2 \
      /usr/lib/x86_64-linux-gnu/crt*.o \
      /usr/lib/x86_64-linux-gnu/libc.so \
      -o enclave_alpha -enclave alpha enclave.o

Trivial Example: `enclave.c`
----

.. code-block:: c

    #pragma capability declare(red)
    #pragma enclave declare(alpha)
    #pragma enclave capability(alpha, red)

    void alphamain(void) __attribute__((gaps_enclave_main("alpha")));
    void alphamain(void) { }

    void onalpha(void)
      __attribute__((gaps_enclave_only("alpha")))
      __attribute__((gaps_capability("red")));
    void onalpha(void) { }
