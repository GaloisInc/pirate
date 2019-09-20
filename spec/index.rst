.. Pirate documentation master file, created by
   sphinx-quickstart on Mon Sep 16 21:42:19 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

The Pirate C and C++ Extensions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Overview
========

The Pirate C and C++ extensions allow developers to define software
enclaves that run in isolation of other code in the application.
Enclaves can communicate with each other using unidirectional
communication channels.  The enclaves and channels are all defined
by names in the source code, and these names can be used to map
applications to specific processors and hardware channels on
GAPS-enabled architectures.

In addition to the enclave annotations, our extensions will allow
users the ability to define sensitivity levels that can be
associated with declarations in the program.  These sensitivity
levels will be propagated from declarations to the code using those
declaraations, and can be used to derive requirements on GAPS-enabled
architectures and summarize what sensitivity levels are associated
with code in the application.  These levels will prevent the user from
inadverently incorporating sensitive code or data on systems not
approved for access to information with that level of sensitivity.

We should note that the current extensions in this document do not
include any libraries or facilities to assist with runtime
manipulation of sensitive data.  In enclaves that have access to
information at different sensitivity levels, one may need this to, for
example, attach labels to specific data, and ensure that data is
correctly labeled before being sent across a channel.  This tracking
is not included as general purpose C compilers lack to generate
programs with physically provable integrity gurantees on runtime
labels.  We plan to revisit this issue as the GAPS hardware
capabilities are developed.

We envision that GAPS-enabled architectures will provide manifests
that provide constraints on how the software-defined enclaves may be
mapped to physical components of the platform.  The details still need
to be worked out, but we anticipate this will include information
about each processor architecture available on the platform, what
sensitivity levels are allowed in enclaves compiled to a processor,
and what unidirectional channels are available.  We anticipate that
for many applications, there will be a need to map multiple separate
enclaves to the same processor, map multiple channels to the same
hardware channel, and perhaps allow one processor to control the
startup and shutdown of enclaves on other processors.  When multiple
enclaves and channels are mapped to the same underlying hardware, the
isolation guarantees will lack physical guarantees, and so we will
need mechanisms to ensure physical separation is achieved when
required.

Programming Model
-----------------

TODO: Refine this into prose

* GAPS systems are partitioned into one or more enclaves
  that communicate using unidirectional channels.

* Enclaves and channels are named via attributes in the
  code so that they may be mapped during linking to specific
  GAPS hardware features.

* Sensitive data is labeled via source annotations as well
  so that the compiler can ensure that sensitive code and
  data values in the source is not included in binaries
  generated for processors not approved for access to that
  data.

* The annotations language may be attached to specific declarations in
  a fine-grained way so that developers can identify as specifically
  as possible the exact information needing protection.

* The annotations language allows users to develop their own
  sensitivity levels rather than building in a fixed vocabulary.

* Sensitivity levels do not imply a particular layout or TA1
  board configuration, but rather describe requirements on
  the compiled target that code is deployed to.

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

Inferred vs Explicit Propagation
--------------------------------

TODO: Compare and constrast having the processor implicitly
propagating sensitive information vs requiring explicit annotations
for each declaration.  Discuss further the implications of requiring
declarations are annotated vs only requiring definitions are
annotated.

-

.. Indices and tables
.. ==================

.. * :ref:`genindex`
.. * :ref:`modindex`
.. * :ref:`search`
