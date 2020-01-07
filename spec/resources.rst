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

YAML schema
-----------

A configuration file has two top-level keys: ``executables`` and
``resources``. The ``executables`` key contains a list of ``executable``
objects, each of which defines an executable to run. The ``resources`` key is
an object with a key for each kind of resource. [Note: Currently, the only key
is ``fd_channels``.]

``executables``
    A list of ``executable`` objects.

``resources``
    An object, with a key for each kind of resource.

An ``executable`` object has the following fields:

``path``
    The path to the executable. This may be an absolute path, or it may
    be relative to the location of the config file.

[Note: Although ``path`` is the only field here at present, in the future, we
could use other fields to specify how the executable should be run, e.g.,
which user it should run as. We could even have systemd-style socket
activation for some executables.]

A ``resources`` object has the following fields:

``fd_channels``
    A list of ``fd_channel`` objects with run-time configuration
    for the channels specified in the executables.

An ``fd_channel`` object has the following fields:

``name``
    The user-defined name of the channel. This should match the name the
    user gave the channel in source-file annotations.

``type``
    Permissible types are ``socket`` for a network socket,
    ``unix_socket`` for a Unix socket, and ``serial`` for a serial device.

``path``
    If ``type`` is ``unix_socket``, this is the filesystem path to use
    for the socket. It will be created if it does not exist. If ``type`` is
    ``serial``, this is the path to the serial device. It is an error to
    include this key if ``type`` is ``socket``.

``local``
    The local address to bind to for network sockets, in the form
    ``<ip>:<port>``. It is an error to include this key if ``type`` is not
    ``socket``.

``remote``
    The remote address to connect to for network sockets, in the form
    ``<ip>:<port>``. It is an error to include this key if ``type`` is not
    ``socket``.

``rate``
    The baud rate for serial channels. This may be omitted, in which case
    a default rate of 9600 will be used. It is an error to include this key if
    ``type`` is not ``serial``.

Example
=======

Suppose we have a `trusted timestamp`_ application separated into three
executables: tts_app, tts_proxy, and tts_signserv (collectively called tts),
implementing the application, proxy, and signing server, respectively. The
application has a channel to the proxy, and the proxy has two (for
illustration's sake) channels to the signing server.

.. `trusted timestamp`_ timestamp_demo.rst

Further suppose we want a configuration where the application and the proxy
run on the same machine. They communicate with each other using a Unix
socket, and with the signing server using a network socket for one channel and
a serial device for the other. Graphically, the configuration looks like as
follows::

    +-----------------------------------+ +-------------------+
    | OS 1                              | | OS 2              |
    |  +---------+       +-----------+  | |  +--------------+ |
    |  | tts_app | <---> | tts_proxy | <-1-> | tts_signserv | |
    |  +---------+       |           | <-2-> |              | |
    |                    +-----------+  | |  +--------------+ |
    +-----------------------------------+ +-------------------+

The configuration file ``os_1.yml`` might look like this:

.. code-block:: yaml

    executables:
      - path: tts_app
      - path: tts_proxy
    resources:
      fd_channels:
        - name: app_to_proxy
          type: unix_socket
          path: /var/run/tts/app_to_proxy
        - name: proxy_to_signserv_1
          type: socket
          local: 10.0.0.1:9001
          remote: 10.0.0.2:9002
        - name: proxy_to_signserv_2
          type: serial
          path: /dev/ttyS0
          rate: 115200


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

Note. The above documentation is being worked on, and we will likely
need to provide additional information for serials.

Example
-------

TODO: Give illustration of resource annotation usage.
