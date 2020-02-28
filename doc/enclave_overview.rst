Enclaves and Capabilities
^^^^^^^^^^^^^^^^^^^^^^^^^

The PIRATE enclave capability annotations are intended to help
developers partition complex applications into a set of individual
components.  The goal is to assist developers in making secure
resilient applications that minimize the attack surface of sensitive
information in the system.

Users can use these annotations to define enclaves and capabilities.
*Enclaves* are specific execution environments that have their own
startup `main` function and typically execute without sharing memory
with other system components.  *Capabilities* are attributes that can
be attached to enclaves as well as source code and variables within
the application.  When attributed to enclaves, capabilities represent
features or authorities the enclave is expected to support.  When
attributed to code or data, capabilities represent features any
enclave that includes this code is expected to have.

The capability model is designed to allow one to capture requirements
of code while still allowing that code to run on any enclave that
satisfies the requirements.  This could be used to express that code
needs specific hardware or other feature like the ability to run
kernel mode instructions without requiring the user tie that code to a
specific named enclave.  The capability model can also be used to
restrict sensitive code or data.  For example, one could describe the
ability to run code as a *capability*, and then achieve the effect of
marking code as sensitive by giving it that capability requirement.

We first describe the annotations for enclaves, and then give
annotations for capabilities.

Enclave Annotations
===================

Enclave support is controlled via the following pragmas and attributes.

.. code-block:: c

                #pragma pirate enclave declare(<enclave_name>)

This pragma declares an enclave with the given name.  The enclave name
is a string that uniquely identifies the enclave within the program.


.. code-block:: c

                __attribute__((pirate_enclave_main(<enclave_name>)))

This attribute may be attached to function definitions, and indicates
that the function provides the main method for the enclave with the
given name.  For initial implementation purposes, we require that the
function this is attached to has the signature `void f(void)` (e.g.,
neither accepts arguments or returns).

.. code-block:: c

                __attribute__((pirate_enclave_only(<enclave_name>)))

This attribute may be attached to global variables, function
declarations, and function definitions.  On a function, it indicates
that the function may be only executed or referenced on the enclave
with the given name.  On a global variable, it indicates the variable
may only be referenced (i.e., have its address taken or be read/written)
on that enclave.

Capability Annotations
=======================

Capability annotations are used to define capabilities in code, and to
annotate enclaves and code with capabilities.


Our capability model includes a simple form of inheritance, and one
may define both new capabilities and extended capabilities.  Extended
capabilities encompass a parent capability, and annotating an enclave
with an extended capability implicitly indicates it has the parent
capability while annotating code with an extended capability
implicitly indicates that it requires the


.. code-block:: c

                #pragma pirate capability declare(<new>)
                #pragma pirate capability declare(<new>, <parent>)

This declares a capability ``<new>``.  If the additional argument
``<parent>`` is provided, it must be a previously declared level, and
this indicates that ``<new>`` extends the ``<parent>`` capability.

.. code-block:: c

                __attribute__((pirate_capability(<level>)))

This attribute may be attached to declaration in the program,
including function declarations and definitions, typedefs, compound
types, variables, statements, enumerated elements and fields of
struct, union and classes (classes are C++ only).  It is used to
indicate that the data requires the given capability.  Multiple
annotations may be added to a single declaration.

.. code-block:: c

                #pragma pirate capability push(<level>, <level>, ...)
                #pragma pirate capability pop

This pragma indicates that all declarations between the ``push`` and
``pop`` pragmas are annotated with the given levels provided to
``push``.  The semantics are the same as if each declaration had the
``capability`` attribute, and this is simply provided for convenience
in files that contain many declarations with shared capability
requirements.

.. code-block:: c

                #pragma pirate enclave capability(<enclave>, <capability>)

This indicates that code running on the given enclave has the given
capability.  In the absence of such an annotation, the linker will
report errors if the enclave named ``<enclave>`` depends on any
information with the given level.  If ``<capability>`` is an extended
capability, this recursively adds any parent capabilities.
