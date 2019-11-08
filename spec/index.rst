.. Pirate documentation master file, created by
   sphinx-quickstart on Mon Sep 16 21:42:19 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

The Pirate C and C++ Extensions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Overview
========

The Pirate C and C++ extensions allow developers to partition their
software into a set of software enclaves.  These enclaves can
communicate with each other using unidirectional communication
channels, and can be mapped to different execution environments that
may provide stronger isolation and integrity guarantees than
traditional operating system process or hypervisor VM guarantees.
This provides a higher level of assurance that the runtime maintains
confidentiality and integrity requirements than traditional
architectures.  Our approach allows developers to use attributes to
declarare enclaves and channels in the source code that can then be
leveraged by the build process to map these virtual enclaves to
hardware architectures tailored to the application security and
performance requirements.

In addition to the enclave annotations, our extensions can attach
attributes to declarations in the source code.  These attributes will
be collected during the build process and collected into the final
binary.  Attributes will initially be used for defining and
associating sensitivity to levels to declarations in the program, and
the language extensions will allow one to define custom sensitivity
levels and inclusion relationships between sensitivity levels.  One
can then check the sensitivity levels of artifacts within the binary
prior to deploying it on a system.  This will not prevent users from
intentionslly mislabeling sensitive information in the code as
non-sensitive, but will prevent the build system from inadverently
incorporating sensitive code or data on systems not approved for
access to information with that level of sensitivity.

This technology should be usable across multiple enclave technologies,
and make it easy for system developers to change how multi-enclave
code is mapped into the physical hardware architecture.  Spliting a
given capability across enclaves will almost always require developer
involvement, but mapping multiple enclaves defined in the source code
to the same physical proceessor will not require changes to the
application.  Furthermore, we envision that on many hardware enclave
technologies developers or system admistrators will have the ability
to insert transformations such as filters between enclaves without
changing the application logic.


An extensible attribute system will be provided so that the developers
can annotate that environment requirements.  For example, the code may
require specific features are available in the hardware prior to
execution or that certain code or data is highly sensitive, and not
for disclosure to untrusted hardware or software.  This will allow the
build process and deployment tools to check that required hardware is
available in the system during the application build process, or
during deployment for software designed to run in multiple
environments.

The PIRATE language extensions are designed to facilitate the
development of multi-enclave systems via common interfaces and modest
compiler extensions to support annotations.  These annotations will
not prevent buffer overflows, return-oriented-programming exploits or
other security vulnerabilities in application code.  Rather they are
intended to help developers decompose applications into multiple
computing enclaves that may be isolated from each other via physical
isolation boundaries, virtual machines or processor-specific
technologies such as Intel SGX and ARM TrustZone.

We should note that the current extensions in this document are
orthogonal to writing software guards that check information prior to
sending it over a channel.  Application developers are encouraged to
implement such checks as needed, but should be aware that our system
does not change the the fundamental lack of memory safety in C and C++
applications.  Adversaries may be able exploit bugs in the application
or supporting libraries to tamper with data and cause checks to
operate incorrectly, or even inject their own functionality into the
program and bypass checks.  Application developers can mitigate
against these attacks through a variety of other means such as use of
safe languages, compiler-inserted protections, and secure development
practices.

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


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   code_annotations
   elf_extensions
