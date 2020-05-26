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

   typedef <c-type> <type-name> __attribute__((pirate_resource_type("<resource_type>")))
   
This typedef declares a named resource type and the corresponding
C type. The resource type name will determine the name of the generated
ELF sections and will enable resource loading libraries to find resources
of this type. As a short-hand this attribute could be declared directly
on a global variable declaration.

.. code-block:: c

    __attribute__((pirate_resource("<name>", "<enclave>")))
    
This attribute on a global variable declares that the variable
is associated with the resource named ``<name>`` in ``<enclave>``.
This global variable should be annotated with a `pirate_resource_type`
attribute described above.

Some resources will appear in multiple enclaves. This attribute can be
provided once for each applicable enclave, and each occurence can optionally
use a unique resource name.


.. code-block:: c

   __attribute__((pirate_resource_param("<param_name>", "<param_value>" [, "<enclave_name>"])))

This attribute provides a compile-time configuration attribute for
a resource that is discoverable by resource loader libraries in a
generated ELF section.

The optional ``enclave_name`` field allows resource parameters to be specific
to the named enclave. Omitting this field causes the paramter to apply to
all enclaves.

In contrast to resource configuration parameters found in runtime configuration
files, these parameters are appropriate for settings that are relied on by
the source code to be set. For example, channels might set the ``permissions``
parameter to indicate if the file descriptor should be read only, write only,
or read-write.

.. code-block:: c
  typedef const char * string_resource __attribute((pirate_resource_type("str")));
  
  string_resource connection_string
    __attribute__((
      pirate_resource("server_settings", "blue"),
      pirate_resource_param("encryption", "yes")
    ));

This example declares a new resource type known as ``str`` to the resource
system, implemented using a ``const char *`` in C, named ``server_settings``
in the resource system, only appearing in the ``blue`` enclave, using a
single key-value entry setting ``encryption`` to ``yes`` in the resource
system.


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

``pirate_channel``
  This indicates that the channel is represented as a ``libpirate``
  channel. This library enables communication across a wide variety
  of channels and perform transforms prior to transmitting
  messages to an underlying character device. The following

.. code-block:: c

   pirate_channel clockFD
   __attribute__((
     pirate_resource("channel_clock", "blue"),
     pirate_resource_param("permissions", "readonly")
   ));
   
   
Parameters
^^^^^^^^^^

The following resource parameters are supported by channel resources:

``permissions``
  The read/write permissions of the specified enclave for this channel
  descriptor. Currently supported options are ``readonly`` and
  ``writeonly``. This option is currently mandatory for
  ``pirate_channel`` resources, since libpirate does not support
  ``O_RDWR``.
  
[BH: Although only ``O_RDONLY`` and ``O_WRONLY`` are supported in
libpirate, whether a channel is truly unidirectional depends on the
channel type. We might want to support a ``unidirectional`` parameter
that causes an error to be thrown if a non-unidirectional channel
type is chosen in the config.]


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


Resource Configuration
----------------------

This section describes the YAML configuration file that captures
information needed to startup one or more enclaves and initialize all
the Pirate managed resources.  A separate application runner will be
needed for each independent machine running enclaves, and although not
required, one can use multiple application runners on the same machine
to, for example, startup processes as different users.

A configuration file has three top-level keys: ``enclaves``,
``resources``, and ``config``. The ``enclaves`` key contains a list
of ``enclave`` objects, each of which defines an executable to run;
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
    be relative to the location of the config file. If this is omitted,
    the runner will assume that the executable file's name is the same
    as the ``name`` field, and that it is present in the same directory
    as the config file.

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
unsupported type is found [TODO: or if the same resource name is
associated with incompatible source types or parameters (e.g., a
channel with datagram semantics in one enclave and stream semantics
in another enclave).]

[BH: Currently the structure of the ``contents`` field is not ideal.
The YAML parser does not distinguish between types for the purposes
of parsing the ``contents`` field, so declaring a ``string`` resource
with an ``integer_value`` will parse just fine. Although this error
will be caught by the handler when the resource gets configured, it
would be much better to make the parsing of ``contents`` depend on
the ``type`` field. This would also allow for a simpler schema, where
``contents`` was a YAML string for ``string`` resources, a YAML
boolean for ``boolean`` resources, a YAML object for ``pirate_channel``
resources, etc. This would also make it much easier to support plugins
for resource types. This is a limitation of libcyaml, not of YAML
itself. We could fix it by adding support for (tagged) unions.]
        
Simple Resources
^^^^^^^^^^^^^^^^

To ease application configuration, the following simple resource types
are available:

``boolean``
  ``contents`` contains the single field ``boolean_value``.

``integer``
  ``contents`` contains the single field ``integer_value``.

``string``
  ``contents`` contains the single field ``string_value``.
    
``file``
  A file resource to be opened by the launcher and passed to the
  application on startup. The ``contents`` stanza contains the
  following fields:
  
  ``file_path``
    Absolute path to the file to open. [TODO: Support paths relative
    to the location of the config file.]
    
  ``file_flags``
    Flags for the resulting file descriptor. The portable values in
    ``open(2)`` are supported, with the same meanings.

Pirate Channels
^^^^^^^^^^^^^^^

The ``pirate_channel`` resource type is an automatically opened
libpirate descriptor. The only common field is ``channel_type``. Which
additional fields are present depends on the value of this field. See
the libpirate documentation for detailed descriptions of each option
and the semantics of the different types. Unsupported fields for a
``channel_type`` will be ignored.

``channel_type``
    The Permissible types are as follows:
    
    ``device``
        An arbitrary device file to send/receive messages on. Fields:
        ``path``, ``iov_length``.
        
    ``pipe``
        A Unix pipe. Fields: ``path``, ``iov_length``.
        
    ``unix_socket``
        A Unix domain socket. Fields: ``path``, ``iov_length``,
        ``buffer_size``.

    ``tcp_socket``
        A TCP socket channel. Fields: ``iov_length``, ``buffer_size``,
        ``host``, ``port``.

    ``udp_socket``
        A UDP socket channel. Fields: ``iov_length``, ``buffer_size``,
        ``host``, ``port``.

    ``shmem``
        A POSIX shared-memory channel. Fields: ``path``, ``buffer_size``.
        
    ``udp_shmem``
        A POSIX shared-memory channel using UDP for transport. Fields:
        ``path``, ``buffer_size``, ``packet_size``, ``packet_count``.

    ``uio``
        A Userspace IO shared-memory channel. Fields: ``path``, ``region``.
        
    ``serial``
        A serial device. Fields: ``path``, ``baud``.
    
    ``mercury``
        A Mercury TA1 device. Fields: An object ``session``, containing the
        subfields ``level``, ``src_id``, ``dst_id``, ``messages``, and
        ``id``.
    
    ``ge_eth``
        A GE TA1 device. Fields: ``host``, ``port``, ``mtu`` ``message_id``.

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
        type: pirate_channel
        ids:
          - tts_app/to_proxy
          - tts_proxy/to_app
        contents:
            channel_type: unix_socket
            path: /var/run/tts/app_to_proxy.sock
      - name: proxy_to_signserv_1
        type: pirate_channel
        ids:
          - tts_proxy/to_signserv_1
        contents:
            channel_type: udp_socket
            host: example.lan # destination host
            port: 9001        # destination port
      - name: proxy_to_signserv_2
        type: pirate_channel
        ids:
          - tts_proxy/to_signserv_2
        contents:
            channel_type: device
            path: /dev/ttyS0
