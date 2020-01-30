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
      -DLLVM_ENABLE_PROJECTS=clang\;lld ../pirate-llvm/llvm
    $ ninja clang lld llvm-readobj

    # leave build directory
    $ cd ..

    # build a trivial example
    $ llvm-ninja/bin/clang -ffunction-sections -fdata-sections \
      --target=x86_64-pc-linux-elf -c enclave.c

    # see that the example worked
    $ llvm-ninja/bin/llvm-readobj --gaps-info enclave.o
    
    # link the example to produce an executable for each enclave
    $ llvm-ninja/bin/clang enclave.o -o enclave_alpha \
      -fuse-ld=lld -Xlinker -enclave -Xlinker alpha
    $ llvm-ninja/bin/clang enclave.o -o enclave_beta \
      -fuse-ld=lld -Xlinker -enclave -Xlinker beta

Trivial Example: `enclave.c`
----

.. code-block:: c

    #include <stdio.h>

    #pragma capability declare(red)
    #pragma enclave declare(alpha)
    #pragma enclave declare(beta)
    #pragma enclave capability(alpha, red)

    void onalpha(void)
      __attribute__((gaps_enclave_only("alpha")))
      __attribute__((gaps_capability("red")))
    {
            printf("running on alpha\n");
    }

    void
    alphamain(void)
      __attribute__((gaps_enclave_main("alpha")))
    {
            onalpha();
            printf("alpha started\n");
    }

    void
    betamain(void)
      __attribute__((gaps_enclave_main("beta")))
    {
            printf("beta started\n");
    }
