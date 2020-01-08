Enclave Resources
=================

The Pirate framework simplifies the initialization and configuration
of communication channels and other resources available to Enclaves.
It does this by offering language extensions that emit externally
visible information about such resources, and a custom program launch
facility that can configure resources shared across enclaves within
the same execution environment.  Thus the Pirate runtime provides
dependency injection facilities tailored to the needs of multi-enclave
execution, and thus makes it easier for one to configure an
application for a particular runtime environment without rebuilding
from source.

Inter-enclave communication chanenls are the most common resource that
the Pirate framework will manage and the focus of our initial
demonstration system.  However, we envision this framework will be
useful for initializing other types of information, and plan to
eventually provide a plugin framework for defining custom resource
types to manage.  Our framework likewise does not prevent application
developers from initializing resources themselves.

This rest of this section describes the general process for declaring
resources to be managed in source code (see `Resource Declarations`_)
and how to specify their runtime configuration (see `Resource
Configuration`_).  We conclude with a reference section describing the
available parameters for channels (see `Channel Resources`_).

Resource Declarations
---------------------

Resources managed by Pirate are defined by declaring global variables
annotated with the ``gaps_resource`` attribute described below.
Resources have a name and type with the type provides information
about the program variable needed to initialize it.  Many resources
may need additional information, and this can be provided via
parameters specified using pragmas.  These parameters are used to, for
example, describe whether a channel is a stream or series of
datagrams, and whether the channel can be read from, written to, or
both.

.. code-block:: c

  const var_type var  __attribute__((gaps_resource(<name>, <type>)));

This attribute on a global variable declares that ``var`` is a variable
with program type ``var_type`` that is associated with the resource
having the externally visible name ``<name>``.  Furthermore,
this associates a type with the resource so that the runtime understands
what type of value is being initialized.  Multiple enclaves may have
resources with the same name, but this represents that the underlying
resource is the same.

.. code-block:: c

   #pragma resource <resource_name> <param_name> <param_value>

This pragma declares that the parameter assignment
``<param_name>=<param_value>`` should be provided to the resource with
name ``<resource_name>``.  This is typically used to provide
additional information from the source file about the resource being
configured.  For example, communication channels can set the
``permissions`` parameter to indicate if the file descriptor should be
read only, write only, or read-write.

Resource Configuration
----------------------

This section will describe how resources configurations are specified
in the application runner runtime.  The Pirate application runner is
an executable that accepts a YAML configuration file that captures
information needed to startup one or more enclaves and initialize all
the Pirate managed resources.  A separate application runner will be
needed for each independent machine running enclaves, and although not
required, one can use multiple application runners on the same machine
to, for example, startup processes as different users.

A configuration file has two top-level keys: ``executables`` and
``resources``. The ``executables`` key contains a list of ``executable``
objects, each of which defines an executable to run, and a ``resources`` key
that contains a list of ``resource`` objects.

``executables``
    A list of ``executable`` objects.

``resources``
    A list of ``resource`` objects.

An ``executable`` object has the following fields:

``path``
    The path to the executable. This may be an absolute path, or it may
    be relative to the location of the config file.

``arguments``
    A list of strings to pass to the executable as positional arguments.
    This key may be omitted if no arguments need to be passed.

``environment``
    A set of key-value pairs to add to the executable's environment. This
    key may be omitted if no environment variables are needed.

[Note: Although these are the only fields here at present, in the future,
we could use other fields to specify how the executable should be run,
e.g., which user it should run as. We could even have systemd-style socket
activation for some executables.]

All ``resource`` objects have a ``name`` field described below, and will
typically have other fields depending on the type of the resource in
the source (e.g. See `Channel Resources` for a list of channels).

``name``
    The user-defined name of the resource. This should match the name the
    user gave the resource in source-file annotations.  Resources with the
    same name in multiple enclaves

The application initialization will report an error if the YAML file
contains a resource object with a name that is not in any enclave, or
if an enclave contains a resource that does not appear in the
configuration file.  The runner will also fail if a resource with an
unsupported type is found, or if the same resource name is associated
with incompatible source types or parameters (e.g., a channel with
datagram semantics in one enclave and stream semantics in another
enclave).

Example
^^^^^^^

Suppose we have a `trusted timestamp`_ application separated into three
executables: tts_app, tts_proxy, and tts_signserv (collectively called tts),
modeimplementing the application, proxy, and signing server, respectively. The
application has a channel to the proxy, and the proxy has two (for
illustration's sake) channels to the signing server.

.. _`trusted timestamp`: timestamp_demo.rst

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
        arguments: ["--flag1", "--flag2"]
      - path: tts_proxy
        environment:
          VAR1: value1
          VAR2: value2
    resources:
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


Channel Resources
-----------------

Channels represent communiction channels between enclaves or between
an enclave and the outside world.  In Pirate, we are careful to
distinguish between the underlying message transport mechanism and the
API used by the Enclave to send and receive messages.  We expect that
in many cases, end-users will be porting existing services or
applications to use Pirate, and will already have a preferred API for
their application to send and receive messages, but those users will
still like to be able to consider different transport mechanisms.

Channels as a concept correspond to one of two resource types in
source code:

``fd_channel``
  This indicates that the channel is represented as a POSIX file
  descriptor.  For example, the following code declares a file
  descriptor ``channel_clock``:

.. code-block:: c

   const int clockFD
   __attribute__((gaps_resource(channel_clock, fd_channel)));

``gaps_channel``
  This indicates that the channel is represented as a GAPS ``libpirate``
  channel.  GAPS channels are a library that can communicate across
  a wide variety of channels, and perform transforms prior to transmitting
  messages to an underlying character device.

.. code-block:: c

   const int clockFD
   __attribute__((gaps_resource(channel_clock, gaps_channel)));

File Descriptor Channels
^^^^^^^^^^^^^^^^^^^^^^^^

The ``fd_channel`` resource type is used to denote a file descriptor
that is used for message passing.  With file-descriptor resources, the
source level attributes are used to indicate requirements of file
descriptor message semantics while the runtime configuration file
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
   This is an attribute indicating if the POSIX unidirectional
   constraints is allowed.   Valid options are ``true`` and ``false``.
   If this attribute is omitted, it is assumed ``unidirectional=false``.

GAPS Channels
^^^^^^^^^^^^^

This section is still under development.

Runtime configuration
^^^^^^^^^^^^^^^^^^^^^

To promote interoperability between the different source types, all
channels types use similiar runtime configuration fields in the YAML
``resource`` objects.  Channels resource objects have the following
fields:

``name``
    The user-defined name of the resource.

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
