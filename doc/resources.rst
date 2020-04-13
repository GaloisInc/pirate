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

Resources managed by Pirate are created by declaring external global
variables annotated with the ``gaps_resource`` attribute described
below.  Resources have a name, a type, and an associated enclave.
The name is used in the runner configuration file (see `Resource
Configuration`_), the type indicates to the runner and the
application how the resource should be configured, and the enclave
name determines which enclave the corresponding object should be
included in. The resource can be made available to multiple enclaves
by annotating variable multiple times. The linker will define each
variable annotated in this way, so the corresponding variable
definition should not appear in the source.

.. code-block:: c

  extern <c_type> var
    __attribute__((pirate_resource("<name1>", "<resource_type>", "<enclave1>")))
    __attribute__((pirate_resource("<name2>", "<resource_type>", "<enclave2>")));

This attribute on a global variable declares that ``var`` is a
variable with C type ``<c_type>`` that is associated with the
resource named ``<name1>`` in ``<enclave1>`` and ``<name2>`` in
``<enclave2>``. Furthermore, this associates a type with the
resource so that the runtime understands what type of value is
being initialized.

Before a resource type can be used, it must be declared. This
declaration specifies the name of a resource type, the type it is
allowed to annotate.  Annotating a variable of a different type
causes the compiler to emit an error.

.. code-block:: c

   #pragma pirate_resource_type declare("<resource_type>", <c_type>)
   
This pragma declares a resource type that can be applied (only) to
variables with the type ``<c_type>``.  In addition, it will cause the
linker to define a symbol ``__<resource_type>_gaps_resources`` that
points to an array of ``struct gaps_resource`` listing all resources
of the given type annotated in the source (see ``ELF Extensions``).

Many resources may need additional information, and this can be
provided via parameters specified using pragmas.  These parameters
are used to, for example, describe whether a channel is a stream or
series of datagrams, and whether the channel can be read from,
written to, or both.

.. code-block:: c

   #pragma pirate_resource_param <resource_name> <param_name> <param_value>

This pragma indicates that the parameter assignment
``<param_name>=<param_value>`` should be provided to the resource
with name ``<resource_name>``.  This is typically used to provide
additional information from the source file about the resource being
configured.  For example, communication channels can set the
``permissions`` parameter to indicate if the file descriptor should be
read only, write only, or read-write.


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

   extern int clockFD
   __attribute__((gaps_resource(channel_clock, fd_channel)));

``gaps_channel``
  This indicates that the channel is represented as a GAPS ``libpirate``
  channel.  GAPS channels are a library that can communicate across
  a wide variety of channels, and perform transforms prior to transmitting
  messages to an underlying character device.

.. code-block:: c

   extern int clockGCD
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
   
[NOTE: Isn't ``unidirectional`` redundant, since it's implied by
``readonly`` or ``writeonly``?]

GAPS Channels
^^^^^^^^^^^^^

[This section is under development.]


Pirate Launcher
---------------

The Pirate launcher allows multiple enclaves to be run as a single
application and handles runtime configuration of resources such as
channels. The executables to be run and the configuration of their
resources are supplied using a YAML configuration using the schema
described below, which must be supplied to the launcher as its sole
command-line argument, e.g. ``pirate-launcher os_1.yml``.
Alternatively, the path to the launcher may be added to the top of
the YAML configuration file in a shebang, e.g.
``#!/usr/bin/pirate-launcher``.

Runner Internals
^^^^^^^^^^^^^^^^

[This section is under development.]


Resource Configuration
----------------------

This section will describe the YAML configuration file that captures
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

``enclaves``
    A list of ``enclave`` objects.

``resources``
    A list of ``resource`` objects.
    
``config``
    A ``config`` object with information on runner configuration.

An ``enclave`` object has the following fields:

``name``
    The name of this enclave corresponding to resource namespaces in
    the ``resources`` top-level array. This must be unique among all
    entries in the ``enclaves`` list, and it should match the enclave
    name the executable was given when it was linked.

``path``
    The path to the executable. This may be an absolute path, or it may
    be relative to the location of the config file.

``arguments``
    A list of strings to pass to the executable as positional arguments.
    This key may be omitted if no arguments need to be passed.

``environment``
    A set of key-value pairs to add to the executable's environment.
    This key may be omitted if no environment variables are needed.
    
A ``config`` object has the following fields:

``log_level``
    How much logging information the runner should produce:
    
    ``default``
        Print only fatal errors.
        
    ``info``
        Additionally print warnings and informative messages.
        
    ``debug``
        Print copious information about the runner's operation.

A ``resource`` object has, at a minimum, ``name``, ``ids``, and
``type`` fields, as described below. Additionally, it has a
``contents`` field, which varies depending on the ``type``.

``name``
    The name of this resource as it will appear in launcher debug
    messages.

``ids``
    A list of strings of the form ``<enclave_name>/<resource_name>``.
    The ``<enclave_name>`` must match the ``name`` field of one of
    the entries in ``enclaves``. The ``<resource_name>`` is the
    identifier the application will use to request this resource from
    the launcher. If the resource is annotated in the source file,
    the ``<resource_name>`` should match the corresponding field in
    the annotation. Both fields must be valid C identifiers.
    
``type``
    The type of this resource. If this resource is annotated in the
    source, this must correspond to the ``<resource_type>`` in the
    annotation.
    
``contents``
    An object whose contents depend on the ``type`` field (see below).

The application initialization will report an error if the YAML file
contains a resource object with a name that is not in any enclave, or
if an enclave contains a resource that does not appear in the
configuration file.  The runner will also fail if a resource with an
unsupported type is found, or if the same resource name is associated
with incompatible source types or parameters (e.g., a channel with
datagram semantics in one enclave and stream semantics in another
enclave).
        
Simple Resources
^^^^^^^^^^^^^^^^

To ease application configuration, the following simple resource types
are available:

``boolean``
    Contents contains the single field ``boolean_value``.

``integer``
    Contents contains the single field ``integer_value``.

``string``
    Contents contains the single field ``string_value``.

GAPS Channels and FD Channels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To promote interoperability, resources of type ``gaps_channel`` and
``fd_channel`` use the same runtime configuration fields in the YAML
``contents`` objects:

``channel_type``
    The Permissible types are as follows:

    ``tcp_socket``
        A TCP socket channel. The ``left`` and/or ``right`` fields
        must be filled out with port and address/hostname information.

    ``udp_socket``
        A UDP socket channel. The ``left`` and/or ``right`` fields
        must be filled out with port and address/hostname information.

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
        
``left`` / ``right``
    These fields are present only for resources with ``channel_type`` equal
    to ``tcp_socket`` or ``udp_socket``. They represent the two endpoints
    of such a connection. They contain the following fields:
    
    ``id``
      A string of the form ``<enclave_name>/<resource_name>`` identifying
      which enclave possesses this endpoint. The string must be present in
      the ``resource`` object's ``ids`` field, as well.
    
    ``dst_host`` and ``dst_port``
      The IP address / hostname and port that this endpoint should connect
      to. To specify the source hostname or port, use the opposite endpoint.

``path``
    The contents of this field differs depending on the ``channel_type``
    field as follows:

    * If ``type`` is ``unix_socket`` or ``path``, this is the path to the
      file to be created or used. This may be an absolute path, or relative
      to the location of the configuration file.
    * If ``type`` is ``device``, this is the path to the device to be used.

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
      - name: tts_app
        path: /usr/bin/tts_app
        arguments: ["--flag1", "--flag2"]
      - name: tts_proxy
        path: /usr/bin/tts_proxy
        environment:
          VAR1: value1
          VAR2: value2
        resources:
          - name: proxy_to_signserv_1
            type: gaps_channel
            local:
              host: 10.0.0.1
              port: 9001
            remote:
              host: os2.localdomain
              port: 9002
    resources:
      - name: app_to_proxy
        type: gaps_channel
        ids:
          - tts_app/to_proxy
          - tts_proxy/to_app
        contents:
            channel_type: unix_socket
            path: /var/run/tts/app_to_proxy.sock
      - name: proxy_to_signserv_1
        type: gaps_channel
        ids:
          - tts_proxy/to_signserv_1
        contents:
            channel_type: udp_socket
            left:
                id: tts_proxy/to_signserv_1
                dst_host: example.lan
                dst_port: 9001
            right:
                dst_port: 9002 # The local port on tts_proxy
      - name: proxy_to_signserv_2
        type: gaps_channel
        ids:
          - tts_proxy/to_signserv_2
        contents:
            channel_type: device
            path: /dev/ttyS0
