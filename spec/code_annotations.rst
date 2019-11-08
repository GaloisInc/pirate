PIRATE Annotations
^^^^^^^^^^^^^^^^^^

The PIRATE annotations language allows one to define enclaves, which
are execution environments designed to perform various operations that
may involve specific hardware, and communicate with other components
via channels.

Enclave Annotations
===================

Enclave support is controlled via the following pragmas and attributes.

.. code-block:: c

                #pragma enclave declare(<enclave_name>)

This pragma declares an enclave with the given name.  The enclave name
is a string that uniquely identifies the enclave within the program.


.. code-block:: c

                __attribute__((enclave_main(<enclave_name>)))

This attribute may be attached to function definitions, and indicates
that the function provides the main method for the enclave with the
given name.  For initial implementation purposes, we require that the
function this is attached to has the signature `void f(void)` (e.g.,
neither accepts arguments or returns).

.. code-block:: c

                __attribute__((enclave_only(<enclave_name>)))

This attribute may be attached to global variables, function
declarations, and function definitions.  On a function, it indicates
that the function may be only executed or referenced on the enclave
with the given name.  On a global variable, it indicates the variable
may only be referenced (i.e., have its address taken or be read/written)
on that enclave.

Enclave Communication
---------------------

Enclaves can communicate between named channels that are created at
enclave startup.  Each channel allows communication between a pair of
channels.  Channels are declared via global variables, and the PIRATE
toolchain will be responsible for ensuring that channels are correctly
initialized, and only accessible to the correct enclaves.  By declaring
channels in this way, the PIRATE development toolchain is aware of
the enclave communication architecture, and is able to map it to
different architectures.

.. code-block:: c

  const enclave_send_channel_t sender
  __attribute__((enclave_send_channel(<channel>, <enclave>)));

This declares that **sender** is a send channel with the name **channel**
that is visible in the enclave **enclave**.

.. code-block:: c

  const enclave_receive_channel_t receiver
  __attribute__((enclave_receive_channel(<channel>, <enclave>)));

This declares that **receiver** is a send channel with the name **channel**
that is visible in the enclave **enclave**.

TODO: Describe channel API.

Building specific enclaves
--------------------------

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

Sensitivity Annotations
=======================

Sensitivity annotations are used to define different sensitivity
levels which can be ordered.

.. code-block:: c

                #pragma sensitive declare(<new>)
                #pragma sensitive declare(<new>, <level>)

This declares a sensitivity level ``<new>``.  If additional argument is
provided, it must be a previously declared level, and this indicates that
``<new>`` is considered more sensitive than ``<level>``.

.. code-block:: c

                __attribute__((sensitive(<level>)))

This attribute may be attached to declaration in the program,
including function declarations and definitions, typedefs, compound
types, variables, statements, enumerated elements and fields of
struct, union and classes (classes are C++ only).  It is used to
indicate that the data is considered to have the given sensitivity
level.  Multiple annotations may be added to a single declaration if
there is no single highest level of sensitivity affecting a
declaration.

TODO: Discuss the semantics of sensitivity levels and how propagation
checking works.

.. code-block:: c

                #pragma sensitive push(<level>, <level>, ...)
                #pragma sensitive pop

This pragma indicates that all declarations between the ``push`` and
``pop`` pragmas are annotated with the given levels provided to
``push``.  The semantics are the same as if each declaration had the
``sensitive`` attribute, and this is simply provided for convenience
in files that contain many declarations that require sensitivity
levels.

.. code-block:: c

                #pragma enclave trusted(<enclave>, <level>)

This indicates that code running on the given enclave is permitted
access to information marked with the given sensitivity level.  In the
absense of such an annotation, the linker will report errors if the
enclave named ``<enclave>`` depends on any information with the given
level.  Adding this annotation, implicitly adds permission for the
enclave to access information marked as less sensitive than the given
level.  On GAPS-enabled architectures, the linker will verify that
trusted enclaves are mapped to hardware approved for access to
information with the given sensitivity level.
