Managed Resources
=================

Pirate provides facilities for the setting up communication channels
and other resources prior to enclave execution.  This is done by
defining resources in the program source files, and separately
defining an application configuration file that describes how those
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

``args``
    A list of strings to pass to the executable as positional arguments.
    This key may be omitted if no arguments need to be passed.

``env``
    A set of strings of the form ``key=value`` to add to the
    executable's environment.  This key may be omitted if no environment
    variables are needed.
    
A ``config`` object has the following fields:

``log_level``
    How much logging information the runner should produce:
    
    ``default``
        Print only fatal errors.
        
    ``info``
        Additionally print warnings and informative messages.
        
    ``debug``
        Print copious information about the runner's operation.

``plugin_directory``
    The directory PAL should look for resource plugins in (see `Resource
    Plugins`_). By default, this is
    ``/usr/local/lib/pirate/pal/plugins``. If the directory does not
    exist, a warning will be emitted.

A ``resource`` object has, at a minimum, ``name``, ``ids``, and
``type`` fields, as described below. Additionally, it has a
``value`` field, which varies depending on the ``type``.

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
    
``value``
    An object whose contents depend on the ``type`` field (see below).

The application initialization will report an error if the YAML file
contains a resource object with a name that is not in any enclave, or
if an enclave contains a resource that does not appear in the
configuration file.  The runner will also fail if a resource with an
unsupported type is found.
        
Simple Resources
^^^^^^^^^^^^^^^^

To ease application configuration, the following simple resource types
are available:

``boolean``
  ``value`` is a YAML boolean value

``integer``
  ``value`` is a YAML numeric value

``string``
  ``value`` is a YAML string value
    
``file``
  A file resource to be opened by the launcher and passed to the
  application on startup. ``value`` is an object with the
  following fields:
  
  ``file_path``
    Absolute path to the file to open. [TODO: Support paths relative
    to the location of the config file.]
    
  ``file_flags``
    Flags for the resulting file descriptor. The portable values in
    ``open(2)`` are supported, with the same meanings.

Pirate Channels
^^^^^^^^^^^^^^^

The ``pirate_channel`` resource type is an automatically opened libpirate
descriptor. ``value`` is an object. The only common field is ``channel_type``.
Which additional fields are present depends on the value of this field. See the
libpirate documentation for detailed descriptions of each option and the
semantics of the different types. Unsupported fields for a ``channel_type``
will be ignored.

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

    enclaves:
      - name: tts_app
        path: /usr/bin/tts_app
        args: ["--flag1", "--flag2"]
      - name: tts_proxy
        path: /usr/bin/tts_proxy
        env: ["VAR1=value1", "VAR2=value2"]
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
        value:
            channel_type: unix_socket
            path: /var/run/tts/app_to_proxy.sock
      - name: proxy_to_signserv_1
        type: pirate_channel
        ids:
          - tts_proxy/to_signserv_1
        value:
            channel_type: udp_socket
            host: example.lan # destination host
            port: 9001        # destination port
      - name: proxy_to_signserv_2
        type: pirate_channel
        ids:
          - tts_proxy/to_signserv_2
        value:
            channel_type: device
            path: /dev/ttyS0


Pirate Application Launcher (PAL)
---------------------------------

The Pirate launcher allows multiple enclaves to be run as a single
application and handles runtime configuration of resources such as
channels. The executables to be run and the configuration of their
resources are supplied using a YAML configuration using the schema
described below, which must be supplied to the launcher as its sole
command-line argument, e.g. ``pal os_1.yml``.
Alternatively, the path to the launcher may be added to the top of
the YAML configuration file in a shebang, e.g.
``#!/usr/bin/pal``.

PAL Operation
^^^^^^^^^^^^^

When PAL is run, it reads the YAML configuration file supplied as its
only argument. If no argument is supplied, or if the configuration file
cannot be read for any reason, PAL will exit and report an error. All
fields of the configuration file (see `Resource Configuration`_) are
read on startup, with the exception of the value of each resource, which
is parsed when that resource is requested by an application. This is
because resource values are configurable (see `Resource Plugins`_).

The apps specified in the ``enclaves`` stanza are started in the order
they are specified in the configuration file (see `Resource
Configuration`_). PAL will exit when all apps have exited. Note that
this can lead to some unintuitive behavior when the apps PAL starts
comprise a CLI client along with its server: Interacting with the CLI
will work as expected, but when the client quits, the application will
appear to hang. This is because the server is still running in the
foreground. Pressing `ctrl-c` will halt the application as normal.

Each application receives a file descriptor that it may use to request
resources from PAL (see `Getting Resources from PAL`_). The environment
variable `PAL_FD` contains the file-descriptor number. The application
requests a resource by sending the resource name and type to PAL. A
type-specific handler then runs in PAL to fill in data from the
configuration file, and sends the resulting object to PAL. In addition,
the handler can open file descriptors and send them to the app.

Resource Plugins
^^^^^^^^^^^^^^^^

Each resource type used in the configuration file and requested by an
app must have a handler within PAL to serialize the data. Some common
resource types, such as ``string``, ``boolean``, and ``integer`` have
resource types that are compiled into PAL. If new resource types are
desired, however, their handlers may be created using PAL's plugin
system.

A plugin for a type called <resource_type> consists of a shared object
called ``<resource_type>.so`` placed in the plugin directory. By
default, the plugin directory is ``/usr/local/lib/pirate/pal/plugins``,
but a different plugin directory may be specified as
``plugin_directory`` in the ``config`` stanza of the configuration file.
The only symbol required is a function of type
``pal_resource_handler_t``.  The following typedef appears in
``pal/resource.h``:

.. code-block:: c
    /* A resource handler should inspect the supplied
     * `pal_yaml_subdoc_t` fill in `env`, which is guaranteed to point
     * to a `pal_env_t` initialized with `EMPTY_PAL_ENV(PAL_RESOURCE)`.
     *
     * The return value should be 0 if the environment was created
     * successfully. Otherwise, -1 should be returned, in which case
     * `env` will not be inspected.
     */
    typedef int (pal_resource_handler_t)(pal_env_t *env,
            const struct app *app, pal_yaml_subdoc_t *rsc);

The type ``pal_env_t`` can be manipulated with functions from
``pal/pal.h``, whereas ``pal_yaml_subdoc_t`` can be manipulated using
functions from ``pal/resource.h``. The following is an example of a
simple resource handler:

.. code-block:: c
    #include <pal/envelope.h>
    #include <pal/resource.h>

    int foo_resource_handler(pal_env_t *env, const struct app *app,
            pal_yaml_subdoc_t *rsc)
    {
        char my_string[1024];
        double my_double;
        short my_short;

        pal_yaml_subdoc_find_static_string(my_string, sizeof my_string, rsc,
                false, 1, PAL_MAP_FIELD("my_string"));
        pal_yaml_subdoc_find_double(&my_double, rsc,
                true, 2, PAL_MAP_FIELD("my_numbers"),
                         PAL_MAP_FIELD("my_double"));
        pal_yaml_subdoc_find_int16(&my_short, rsc,
                true, 2, PAL_MAP_FIELD("my_numbers"),
                         PAL_MAP_FIELD("my_short"));

        if(pal_yaml_subdoc_error_count(rsc) > 0)
            return -1;

        if(pal_add_to_env(env, my_string, strlen(my_string)))
            return -1;
        if(pal_add_to_env(env, &my_double, sizeof my_double))
            return -1;
        if(pal_add_to_env(env, &my_short, sizeof my_short))
            return -1;

        return 0;
    }

The ``pal_yaml_subdoc_find_*`` functions are used to parse the stanza
corresponding to the ``value`` field of the current resource. The depth
and path arguments to these functions are relative to this field.
``pal_yaml_subdoc_error_count`` returns the number of errors encountered
in the parsing. If ``foo_resource_handler`` returns an non-zero value,
these errors will be printed out, and PAL will halt. Finally,
``pal_add_to_env`` is used to serialize the parsed values.

This would parse a configuration file with a resource stanza like the
one below. Note that ``my_string`` is an optional field whereas the
other fields are required, as specified by the boolean fields of the
``pal_yaml_subdoc_find_*`` functions. In addition, ``my_double`` and
``my_short`` are nested within an object called ``my_numbers``.

.. code-block:: yaml
    enclaves:
    - name: plugin_app
    resources:
    - name: my_foo_resource
      type: foo
      ids: 
      - plugin_app/my_foo
      value:
        my_string: I'm a string
        my_numbers:
          my_double: 2.718281
          my_short: 9001

The above could be placed into a file called ``foo.c`` and compiled as
shown below. The resulting shared object, ``foo.so`` should be placed in
``/usr/local/lib/pirate/pal/plugins``, or whichever directory PAL is
configured to look in for plugins.

.. code-block:: sh
    clang -shared -fPIC -o foo.so foo.c

Getting Resources from PAL
^^^^^^^^^^^^^^^^^^^^^^^^^^

Several resource types, such as ``boolean``, ``integer``, ``string``,
and ``pirate_channel`` are loaded into the application automatically if
they are declared using Pirate resource annotations (see `Declaring
Resources`_). Resources can also be loaded manually, using functions
declared in ``pal/pal.h`` and exported in ``libpal.so``.

The built-in resource types listed above can be loaded manually using
the functions ``get_<resource_type>_res``. See ``pal/pal.h`` for
details.

Loading custom resources, such as the one we designed in `Resource
Plugins`_, can be done using the functions in ``pal/envelope.h``.
Loading the example ``foo`` resource above would look like the
following:

.. code-block:: c
    #include <pal/pal.h>
    #include <stdlib.h>

    #pragma pirate enclave declare(plugin_app)

    int __attribute__((pirate_enclave_main("plugin_app"))) main(void)
    {
        int fd = get_pal_fd();

        pal_send_resource_request(fd, "foo", "my_foo", 0);

        pal_env_t env = EMPTY_PAL_ENV(PAL_NO_TYPE);
        pal_recv_env(fd, &env, 0);

        pal_env_iterator_t it = pal_env_iterator_start(&env);
        char *my_string = pal_env_iterator_strdup(it);

        it = pal_env_iterator_next(it);
        double my_double = *(double*)pal_env_iterator_data(it);

        it = pal_env_iterator_next(it);
        short my_short = *(short*)pal_env_iterator_data(it);

        pal_free_env(&env);

        // Application code goes here

        return EXIT_SUCCESS;
    }

The file descriptor is retrieved using ``get_pal_fd``. If this function
returns a value less than zero, the program was probably launched
without PAL. The application sends a resource request to PAL using
``pal_send_resource_request``. This causes PAL to look for a handler
corresponding to the type ``foo`` (the one we wrote above) and a
resource called ``my_foo`` in the configuration file.

The data serialized by the handler is received using ``pal_recv_env``,
and the data is deserialized using the ``pal_env_iterator_*`` functions.
Note that the data is deserialized in the same order
``foo_resource_handler`` serialized it in. Note also that serialized
string data will **not** be zero-terminated, so it's easiest to handle
it using ``pal_env_iterator_strdup`` or ``pal_env_iterator_strncpy``.

See ``demos/resource_demos/4.resource_plugins`` for the full code of the
above example, including error checking (omitted here for clarity).
