ELF extensions
^^^^^^^^^^^^^^

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Overview
--------

Pirate extends the ELF relocatable object-file format with information
about enclaves, capabilities, and channels.  These extensions allow
the linker to emit separate executables for each enclave in a way that
adheres to the annotations applied to the source code.

For each enclave, we provide a name, used when specifying which
enclaves to generate executables for; a main function, in the form of
the index of a ``.symtab`` entry; and a list of capabilities the
enclave offers.  For each symbol in ``.symtab``, we provide a list of
required capabilities, and optionally, the enclave that is allowed to
run or access the symbol.

Capabilities are identified internally by the index of their entry in
``.pirate.captab`` and externally by their user-defined name, which
must be unique within the compilation unit.  Extended capabilities are
capabilities that extend a previous capability, and these contain the
index of a parent capability, with the intended meaning that any
enclave with the extended capability also has its parent.  Lists of
required or provided capabilities are specified as strings in
``.pirate.captab``, ending in ``CAP_NULL``.

Special Sections
----------------

We add five new special sections to the relocatable ELF format. Like
Linux special sections, these are identified by their section names,
so the ``sh_type`` fields in their section headers should be set to
``SHT_NULL``.

``.pirate.enclaves``
    An array of ``Elf64_Pirate_enc`` listing the names, capabilities
    and main functions of each enclave declared in the source file.
    The first entry is unused and corresponds to ``ENC_UNDEF``.

``.pirate.symreqs``
    An array of ``Elf64_Pirate_req`` associating symbols with
    specific required capabities (and/or) tied to specific
    enclaves.

``.pirate.capabilities``
    An array of ``Elf64_Pirate_cap`` listing the capabilities defined
    in the source file. The first entry is unused and corresponds to
    ``CAP_NULL``.

``.pirate.captab``
    A vector of arrays of ``Elf64_Word`` indices into
    ``.pirate.capabilities``, each terminated by ``CAP_NULL``. Offset
    zero contains a 0 to signify an empty capacity list.

``.pirate.strtab``
    A vector of zero-terminated strings to hold the names
    of enclaves and capabilities.  Offset zero contains a null byte,
    to signify the empty string.
    
In addition, for each resource type declared in the source file using
the ``pirate_resource_type declare`` pragma, we define a section that
appears in both the relocatable and executable elf formats. In the
executable ELF, it is loaded into the `text` segment, along with
``.rodata``.

``.pirate.res.<resource_type>.<enclave_name>``
   An array of ``Elf64_Pirate_res``. The data in this struct is handled
   by relocations, to be filled in by the linker. The data to be
   relocated into each field is stored in ``.rodata``. The
   concatenation of all the arrays for a given resource type will be
   made available to the named enclave.
   
[NOTE: We could potentially use ``.pirate.res.resource_type``, without
an enclave, to indicate that the given resource is linked into all
enclaves, and that the name is the same among all of them. Not sure if
that's useful.]

Structures
----------

``Elf64_Pirate_enc``
====================

Encodes information about an enclave

.. code-block:: c

                typedef struct {
                    Elf64_Word enc_name;
                    Elf64_Word enc_cap;
                    Elf64_Half enc_main;
                    Elf64_Half enc_padding;
                } Elf64_Pirate_enc;

``enc_name``
    The offset of a ``.pirate.strtab`` entry for the user-defined name
    of the enclave.

``enc_cap``
    The starting index of a string of capability indices in
    ``.pirate.captab``, indicating capabilities for this enclave.

``enc_main``
    The index of the entry in ``.symtab`` to be used as the main
    function for this enclave.

``Elf64_Pirate_cap``
==================

Encodes information about capabilities.

.. code-block:: c

                typedef struct {
                    Elf64_Word cap_name;
                    Elf64_Word cap_parent;
                } Elf64_Pirate_cap;

``cap_name``
    The offset of a ``.pirate.strtab`` entry for the name of this
    capability.

``cap_parent``
    If this entry represents an extended capability, then this stores
    the index of the parent capability.  Otherwise, this should be set
    to ``CAP_NULL``.

``Elf64_Pirate_req``
==================

Encodes information about the capabilities and/or enclave
attributes of a symbol.

.. code-block:: c

                typedef struct {
                    Elf64_Word req_cap;
                    Elf64_Word req_enc;
                    Elf64_Half req_sym;
                    Elf64_Half req_padding;
                } Elf64_Pirate_req;

``req_cap``
    The offset of a string of capability indices in ``.pirate.captab``
    indicating capability for the ``.symtab`` entry with the
    corresponding index.

``req_enc``
    If this symbol was annotated with ``enclave_only(e)``, the index
    into ``.pirate.enclaves`` of the enclave ``e``. Otherwise, this
    should be set to ``ENC_UNDEF``.

``req_sym``
    The symtab index of the symbol with these requirements.

``struct pirate_resource`` and ``Elf64_Pirate_res``
===============================================

Encodes information about a Pirate-initialized resource. With the
exception of ``pr_sym``, all fields should be zero in the relocatable
ELF, to be filled in using relocations. ``pr_sym`` should be zero and
is ignored in the executable ELF. The two different structs represent
the application's view and the toolchain's view of the data,
respectively.

.. code-block:: c

                struct pirate_resource {
                    char *pr_name;
                    void *pr_obj;
                    struct pirate_resource_param *pr_params;
                    uint64_t pr_params_len;
                } __attribute__((packed));
                
                typedef struct {
                    Elf64_Addr pr_name;
                    Elf64_Addr pr_obj;
                    Elf64_Addr pr_params;
                    Elf64_XWord pr_params_len;
                } Elf64_Pirate_res;

``pr_name``
    The name of the resource.
    
``pr_obj``
    A pointer the the object this resource corresponds to.

``pr_param``
    An array of ``struct pirate_resource_param`` storing
    key-value pairs representing static resource configuration.
    
``pr_params_len``
    The number of ``struct pirate_resource_param`` pointed to
    by ``pr_params_len``.

``struct pirate_resource_param``
==============================

Encodes information about a parameter for a Pirate-initialized
resource.

.. code-block:: c

                struct pirate_resource_param {
                    char *prp_name;
                    char *prp_value;
                };

``prp_name``
    The name of the parameter.

``prp_value``
    The value of the parameter.
    
Linker-defined Symbols
----------------------

For each entry in the ``.pirate.res.<resource_type>.<enclave>`` sections
of a relocatable ELF, the linker defines a symbol corresponding to the
undefined symbol indexed in its ``pr_sym`` field.

In addition , for each resource type, the linker defines a start and end
symbol that a library or application can use to access an array of
resources of that type:

.. code-block:: c

               struct pirate_resource __start_pirate_res_<resource_type>[];
               struct pirate_resource __stop_pirate_res_<resource_type>[];

A library or application can gain access to this array by including
the ``pirate_resources.h`` header file and declaring an ``extern``
variable with the appropriate name and type:

.. code-block:: c

               #include <pirate_resources.h>

               extern struct pirate_resource __start_pirate_res_<resource_type>[];
               extern struct pirate_resource __stop_pirate_res_<resource_type>[];

Linking Examples
----------------

Compiling and linking an executable with two enclaves, ``foo`` and
``baz``, given two annotated source files ``zip.c`` and ``zap.c``,
can be achieved as follows:

.. code-block:: sh

               clang -ffunction-sections -fdata-sections -c zip.c zap.c
               ld.lld -dynamic-linker /lib64/ld-linux-x86-64.so.2 \
                   /usr/lib/x86_64-linux-gnu/crt*.o \
                   /usr/lib/x86_64-linux-gnu/libc.so \
                   -o foo -enclave foo zip.o zap.o
               ld.lld -dynamic-linker /lib64/ld-linux-x86-64.so.2 \
                   /usr/lib/x86_64-linux-gnu/crt*.o \
                   /usr/lib/x86_64-linux-gnu/libc.so \
                   -o bar -enclave bar zip.o zap.o
