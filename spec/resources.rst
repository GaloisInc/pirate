Enclave Resources
-----------------

The PIRATE framework offers language extensions to declare global
values that are initialized by the framework at startup.  This makes
it easier for one to configure an application for a particular runtime
environment without rebuilding from source.  The most common use for
these resources will be initializing inter-enclave communication
channels, but one can use this more general resource framework for
initializing other types of information.  Application deverlopers can
of course initialize resources and channels directly in their code;
the PIRATE framework is intended primarily as a mechanism for enabling
end-users to configure the enclaves for particular computing
environments.

This rest of this section describes how to declare custom resource
types, associate variable declarations to specific resource names and
types, and write initialization scripts that tell the Pirate
application runtime environment how to initialize the application.

Resource Declarations
---------------------

Each global variable that is initialized by the PIRATE runtime should
be annotated with an attribute that provides an externally visible
name to the variable along with a name denoting the type of resource
(see ResourceTypes_).  In addition, each resource type may have
parameters that is provided to the application runner to initialzie
the application.

.. code-block:: c

  const var_type var  __attribute__((resource(<resource_name>, <response_type>)));

This attribute on a global variable declares that ``var`` is a variable
with type ``var_type`` that is exported under the name ``<resource_name>``
to be externally visible to the Pirate runtime.  Moreover, the
resource will be initialized using the initializer for the resource
``<resource_type>``.

.. code-block:: c

   #pragma resource <resource_name> <param_name> <param_value>

This pragma declares that the parameter assignment
``<param_name>=<param_value>`` should be provided to the resource with
name ``<resource_name>``.  This is typically used to configure semantics
assumptions about the resource.  For example file descriptor resources
for POSIX channels have attributes indicating if the file descriptor
should be read only, write only, or read-write.

.. _ResourceTypes:

Runtime Resource Configuration
------------------------------

This section will decribe how resources configurations are specified
in the application runner runtime.  The PIRATE application runner is
an executable that accepts a YAML configuration file that captures
information needed to startup one or more enclaves and initialize all
the PIRATE managed resources.  A separate application runner will be
needed for each independent machine running enclaves, and although not
required, one can use multiple application runners on the same machine
to, for example, startup processes as different users.

We will soon be describing the format for the YAML configuration file.


Resource Types
--------------

File Descriptor
===============

The ``fd_channel`` resource is used to denote a file descriptor that
is used for message passing.  With file-descriptor resources, the
source level attributes are used to indicate requirements of file
descriptor message semantics while the runtime configurtion file
specifies the actual mechanism used to establish the channel.

The following attributes may appear in the source file annotations.

``permissions``
   This is a required attribute indicates the permissions
   for sending or receiving on a channel.  Valid options are ``readonly``,
   ``writeonly``, and ``readwrite``.

``mode``
   This attribute affects whether the channel is viewed as individual
   datagrams or a contiguous stream of bytes.  Valid options are ``datagram``
   and ``stream``.

``unidirectional``
   This is an attribute indicating if the POSIX unidirectionality
   constraints is allowed.   Valid options are ``true`` and ``false``.
   If this attribute is omited, it is assumed ``unidirectional=false``.

The YAML runtime configuration file uses the following attributes for
``fd_channel`` resources

``type``
   This indicates the type of file descriptor used.  Valid options
   are ``unix_socket``, ``socket``, and ``serial``.

``file``
   For unix sockets (``type=unix_socket``), this indicates the name to
   use for the socket file.  For serial devices this is the file.

``ip``
  For socket channels, this indicates the IP address to use for creating
  the socket.  The port should be appended (e.g. ``127.0.0.1:500``).

Note. The above documentation is being worked on, and we will likely
need to provide additional information for serials.

Example
-------

TODO: Give illustration of resource annotation usage.
