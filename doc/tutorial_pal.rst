Word Filter With PAL Resources
==============================

In this tutorial, we will adapt the program described in the `word filter
tutorial <./tutorial.rst>`_ to use Pirate channels whose configuration and
initialization are handled by the Pirate Application Launcher (PAL). The
resulting source is available at
https://github.com/GaloisInc/pirate/blob/master/demos/word_filter/0.enclaves/filter.c

Importing and Linking Against libpal
------------------------------------

In a normal installation, ``pal.h`` will be placed somewhere in a subdirectory
``pal`` of some standard include directory (e.g., ``/usr/local/include``). We
add the following line to the includes in the word filter's source file.

.. code-block:: c

    #include <pal/pal.h>

We also need to link against ``libpal.so`` as well. In a normal installation,
this is placed in a standard library directory (e.g., ``/usr/local/lib``), so
adding ``-l pal`` to the compile line or similar will suffice.

Using Pirate Resources
----------------------

The main change we will make to the original source is to change the
declaration of the channel descriptors to be global and to use types the
Pirate resource type :code:`pirate_channel`, which is a typedef of int that
uses Pirate annotations:

.. code-block:: c

    pirate_channel ui_to_host
        __attribute__((pirate_resource("ui_to_host", "filter_ui")))
        __attribute__((pirate_resource_param("permissions", "writeonly", "filter_ui")))
        __attribute__((pirate_resource("ui_to_host", "filter_host")))
        __attribute__((pirate_resource_param("permissions", "readonly", "filter_host")));
    pirate_channel host_to_ui
        __attribute__((pirate_resource("host_to_ui", "filter_host")))
        __attribute__((pirate_resource_param("permissions", "writeonly", "filter_host")))
        __attribute__((pirate_resource("host_to_ui", "filter_ui")))
        __attribute__((pirate_resource_param("permissions", "readonly", "filter_ui")));

The :code:`pirate_resource` annotations make the resource visible and specify
its name in each enclave. In this case each resource has the same name in both
enclaves (:code:`ui_to_host` and :code:`host_to_ui`). The
:code:`pirate_resource_param` annotations set parameters on the resources in
each enclave, in this case the permissions.

The above changes will cause libpal to initialize the two channels in each
enclave automatically, so we no longer need the manual initialization step in
the two main functions:

.. code-block:: c

    int ui(void)
    __attribute__((pirate_enclave_main("filter_ui")))
    {
      // Manual initialization was here

      puts("UI:\tConnected");

      // snip

      for(;;) {
        // snip

        transmit(ui_to_host, line, len + /*null-term*/1);
        receive(host_to_ui, &line, &line_sz);

        // snip
      }

      return 0;
    }

    int host(void)
    __attribute__((pirate_enclave_main("filter_host")))
    {
      // Manual initialization was here

      puts("Host:\tConnected");

      // snip

      for (;;) {
        size_t len = receive(ui_to_host, &line, &line_sz);

        // snip

        transmit(host_to_ui, line, len);

        // snip
      }

      return 0;
    }

Now we compile the two enclaves as described in the `word filter tutorial
<./tutorial.rst>`_. Below, we assume they have been compiled into executables
called ``filter_ui_pal`` and ``filter_host_pal``.

Configuration
-------------

The final step is to create a configuration file that tells PAL the
executables to launch and how to configure the resources they declare. We
create a YAML file in the same directory as the executables we compiled in the
last section called ``word_filter_with_pal.yaml``.

Under the heading ``enclaves``, we specify which executables to run as
follows:

.. code-block:: yaml

    enclaves:
        - name: filter_ui_pal
        - name: filter_host_pal

Each entry in the :code:`enclaves` list indicates a separate executable to run.
The :code:`name` field is an identifier used by PAL to correlate enclaves with
resources. The :code:`path` field (not present) tells PAL the location of the
executable as an absolute or relative path. Since the :code:`path` field is
absent, PAL will use the :code:`name` value instead. If the executables needed
arguments or environment variables to be specified, we could do that in their
entries as well. See the `resources section <resources.rst>_` for more
information.

We place the information needed to configure the application's channels in the
:code:`resources` section. For example, the :code:`ui_to_host` channel is
configured for both enclaves with the following entry:

.. code-block:: yaml

    - name: ui_to_host
      ids: [ "filter_ui_pal/ui_to_host", "filter_host_pal/ui_to_host" ]
      type: pirate_channel
      contents:
          channel_type: pipe
          path: /tmp/filter_ui_to_host

The :code:`name` field is an identifier used by PAL in error messages.
Although it corresponds to the name given to the resources in the above source
annotations in this case, it does not need to in general.

The :code:`ids` field indicates which enclaves have access to this resource
configuration and what the annotated name of the resource is in each enclave.
The format is :code:`<enclave_name>/<resource_name>`, where
:code:`<enclave_name>` is the :code:`name` field specified in the
:code:`enclaves` section, and :code:`<resource_name>` is the first argument of
:code:`pirate_resource` as annotated in the source.

The :code:`type` field indicates the resource type, and the subfields of
:code:`contents` vary depending on its value. For more information about
resource types, see the `resources section <resources.rst>_`.

Running the Application
-----------------------

To run the application, execute ``/path/to/pal word_filter_with_pal.yaml``.
Both executables will be launched and configured according to the contents of
the YAML file.
