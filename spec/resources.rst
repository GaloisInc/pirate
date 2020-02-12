Managed Resources
=================

Pirate provides facilities for the setting up communication channels
and other resources prior to enclave execution.  This is done by
defining resources in the program source files, and separately
defining a application configuration file that describes how those
resources are configured for a particular environment.  This allows
the application or library developer to be agnostic to how
communication channels and other resources are ultimately configured,
and allows integrators with the ability to reconfigure an application
with needing to modify the program source.

Inter-enclave communication channels are the most common resource that
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

A configuration file has three top-level keys: ``executables``,
``resources``, and ``config``. The ``executables`` key contains a list
of ``executable`` objects, each of which defines an executable to run;
the ``resources`` key contains a list of ``resource`` objects,
describing resources to be initialized by the runner; and the
``config`` key contains an object with options for runner
configuration.

``executables``
    A list of ``executable`` objects.

``resources``
    A list of ``resource`` objects.
    
``config``
    A ``config`` object with information on runner configuration.

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
    
``clear_env``
    A boolean value describing whether the runner should clear the
    environment when running the executable. If this is ``true``, the
    program's environment will contain only the keys specified in
    ``environment``. Otherwise it will inherit environment variables from
    the runner.

[Note: Although these are the only fields here at present, in the future,
we could use other fields to specify how the executable should be run,
e.g., which user it should run as. We could even have systemd-style socket
activation for some executables.]

All ``resource`` objects have a ``name`` field described below, and will
typically have other fields depending on the type of the resource in
the source (e.g. See `Channel Resources` for a list).

``name``
    The user-defined name of the resource. This should match the name the
    user gave the resource in source-file annotations.

The application initialization will report an error if the YAML file
contains a resource object with a name that is not in any enclave, or
if an enclave contains a resource that does not appear in the
configuration file.  The runner will also fail if a resource with an
unsupported type is found, or if the same resource name is associated
with incompatible source types or parameters (e.g., a channel with
datagram semantics in one enclave and stream semantics in another
enclave).

A ``config`` object has the following fields:

``log_level``
    How much logging information the runner should produce:
    
    ``default``
        Print only fatal errors.
        
    ``info``
        Additionally print warnings and informative messages.
        
    ``debug``
        Print copious information about the runner's operation.

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
        path: /var/run/tts/app_to_proxy.sock
      - name: proxy_to_signserv_1
        type: udp_socket
        local:
          host: 10.0.0.1
          port: 9001
        remote:
          host: os2.localdomain
          port: 9002
      - name: proxy_to_signserv_2
        type: device
        path: /dev/ttyS0


Channel Resources
-----------------

Channels represent communication channels between enclaves or between
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

   extern const int clockFD
   __attribute__((gaps_resource(channel_clock, fd_channel)));

``gaps_channel``
  This indicates that the channel is represented as a GAPS ``libpirate``
  channel.  GAPS channels are a library that can communicate across
  a wide variety of channels, and perform transforms prior to transmitting
  messages to an underlying character device.

.. code-block:: c

   extern const int clockGCD
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

``unidirectional``
   This is an attribute indicating if the POSIX unidirectional
   semantics in :doc:`unidirectional_channels` are allowed.
   Valid options are ``true`` and ``false``.
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
    Permissible types are as follows:

    ``tcp_socket``
        A TCP socket channel. A remote hostname or IP address and port must
        be provided using the ``remote`` field (see below).

    ``udp_socket``
        A UDP socket channel. A remote hostname or IP address and port must
        be provided using the ``remote`` field (see below).

    ``unix_socket``
        A Unix socket channel. A filepath may be provided using the
        ``path`` field.

    ``pipe``
        A Linux named-pipe channel. A filepath may be provided using the
        ``path`` field.

    ``device``
        A character-device channel. A device path must be provided using
        the ``path`` field.

    ``shmem``
        A POSIX shared-memory libpirate channel, intended for benchmarking.
        The size of the shared-memory buffer may be specified using the
        ``buffer`` field. See the libpirate documentation for more
        information.

    ``uio_device``
        A Userspace IO shared-memory channel. See the libpirate
        documentation for more information.

``path``
    The contents of this field differs depending on the ``type`` field as
    follows:

    * If ``type`` is ``unix_socket`` or ``path``, this is the path to the
      file to be created or used. This may be an absolute path, or relative
      to the location of the configuration file.
    * If ``type`` is ``device``, this is the path to the device to be used.

``local``
    An object representing the local address to bind to for a channel of
    type ``tcp_socket`` or ``udp_socket``. This is ignored and may be
    omitted for ``gaps_channel`` resources. It has the following fields:

    ``host``
        A hostname or IP address.

    ``port``
        A port number.

``remote``
    An object representing the remote address to connect to for a channel
    of type ``tcp_socket`` or ``udp_socket``, with the following fields:

    ``host``
        A hostname or IP address.

    ``port``
        A port number. This is ignored and may be omitted for
        ``gaps_channel`` resources.

``buffer``
    The size of the shared-memory buffer for channels of type ``shmem`` or
    the buffer size for channels of type ``unix_socket``. It is an error to
    include this field for any other type of device.

``packet_size``
    The size of a packet for channels of type ``shmem``. It is an error to
    include this field for any other type of device.

``iov_length``
    The length of an iovector for libpirate channels.

``rate``
    The baud rate for serial channels. This may be omitted, in which case
    a default rate of 9600 will be used.


GAPS Runner
-----------

The GAPS runner allows multiple GAPS executables to be run as a single
application and handles runtime configuration of resources such as
channels. The executables to be run and the configuration of their
resources are supplied using a YAML configuration using the schema
described above, which must be supplied to the runner as its sole
command-line argument, e.g. ``gaps-run os_1.yml``. Alternatively, the
path to the runner may be added to the top of the YAML configuration
file in a shebang, e.g. ``#!/usr/bin/gaps-run``.

Runner Internals
^^^^^^^^^^^^^^^^

[NOTE: This section is under development and may change.]

Upon execution, the runner parses its configuration file and, for each
file in the ``executables`` section, compiles a list of resources to
be configured by reading that file's ``.gaps.res`` section. It then
attempts to match each resource found in this way with one in the
``resources`` section of the configuration file by comparing
``res_name`` with the name field in the YAML. If any resource in
``.gaps.res`` lacks a YAML resource configuration, the runner reports
an error. However, since not all resources will be present in all
executables, extraneous resources mentioned in the YAML do not cause
an error.

Once all resource information has been gathered, the runner iterates
through each resource, consulting its table of resource handlers for
one that matches the type name given in ``res_type``. If no handler
is found, the handler reports an error. Otherwise, the handler is
called to fill in the information that will be copied into the
executable at the annotated symbol when it is run. The runner
additionally checks to ensure that the symbol size in the executable's
symtab matches the expected size for a resource of the given type,
reporting an error otherwise.

Finally, the runner calls ``PTRACE_TRACEME`` and calls ``exec`` on the
file supplying it with any arguments or environment variables given in
the configuration. Before calling ``PTRACE_DETACH`` and allowing the
executable to run, it writes the data supplied by the handler into the
executable at the annotated symbol.

Resource Initialization
^^^^^^^^^^^^^^^^^^^^^^^

The linker supports resource initialization for any resource type that
was declared with an associated config type. It does so by exposing an
array ``<cfg_type> *__gaps_res_<resource_type>`` for each such resource
type. The config object of each resource annotated with the
corresponding type is pointed to by an element of the array. E.g.,
``struct gaps_channel_cfg *__gaps_res_gaps_channel`` is an array of
pointers to the ``struct gaps_channel_cfg`` config objects associated
with each resource annotated with the resource type ``gaps_channel``.

Using the resource-pointer arrays exposed in this way, a library can
declare a program constructor that iterates through the resource
objects of a given type. Since this occurs after the runner has
written configuration data to them, the constructor can read this
data and perform whatever resource initialization is required.
