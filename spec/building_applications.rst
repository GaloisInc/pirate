Building Enclaves
^^^^^^^^^^^^^^^^^

[Note: This documentation is still under development, and highly subject to change.]

After compiling one or more C source files into object files using
enclave-aware compilers, one can generate an executable that runs the
enclaves by running passing ``--enclave name,name,..`` to ``lld``
along with other linker options and object files.  This will result in
`lld` producing an executable that establishes the communication
channels and launches each of the enclave main function at startup.
This capability is intended for testing purposes, and does not
provide physical security protections between enclaves.  A version
of ``lld`` with these protection guarantees will be developed once
suitable hardware is available.

If ``lld`` does not find the main function for one of the enclaves,
then an error will be reported.
