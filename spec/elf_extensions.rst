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
``.gaps.captab`` and externally by their user-defined name, which must
be unique within the compilation unit.  Extended capabilities are
capabilities that extend a previous capability, and these contain the
index of a parent capability, with the intended meaning that any enclave
with the extended capability also has its parent.  Lists of required or
provided capabilities are specified as strings in ``.gaps.captab``,
ending in ``CAP_NULL``.

Special Sections
----------------

We add six new special sections to the relocatable ELF format. Like
Linux special sections, these are identified by their section names,
so the ``sh_type`` fields in their section headers should be set to
``SHT_NULL``.

``.gaps.enclaves``
    An array of ``Elf64_GAPS_enc`` listing the names, capabilities
    and main functions of each enclave declared in the source file.
    The first entry is unused and corresponds to ``ENC_UNDEF``.

``.gaps.symreqs``
    An array of ``Elf64_GAPS_req`` associating symbols with
    specific required capabities (and/or) tied to specific
    enclaves.

``.gaps.capabilities``
    An array of ``Elf64_GAPS_cap`` listing the capabilities defined
    in the source file. The first entry is unused and corresponds to
    ``CAP_NULL``.

``.gaps.captab``
    A vector of arrays of ``Elf64_Word`` indices into
    ``.gaps.capabilities``, each terminated by ``CAP_NULL``. Offset
    zero contains a 0 to signify an empty capacity list.

``.gaps.strtab`` A vector of zero-terminated strings to hold the names
    of enclaves and capabilities.  Offset zero contains a null byte,
    to signify the empty string.

``.gaps.res``
    An array of ``Elf64_GAPS_res`` listing the resources to be managed
    by GAPS in the object file.

``.gaps.res.params``
    An array of ``Elf64_GAPS_res_param`` values listing the parameters
    for resources.  Parameters are a contiguous sequence of
    ``Elf64_GAPS_res_param`` values terminated by a ``Elf64_GAPS_res_param`` struct
    with an empty name (i.e., ``param_name = 0``).

``.gaps.res.strtab``
    A string table consisting of vector of zero-terminated strings to
    hold the names of resource names and types along with resource
    parameter names and values.  Offset zero contains a null byte, to
    signify the empty string.

Structures
----------

``Elf64_GAPS_enc``
==================

Encodes information about an enclave

.. code-block:: c

                typedef struct {
                    Elf64_Addr enc_name;
                    Elf64_Word enc_cap;
                    Elf64_Half enc_main;
                    Elf64_Half enc_padding;
                } Elf64_GAPS_enc;

``enc_name``
    The offset of a ``.gaps.strtab`` entry for the user-defined name
    of the enclave.

``enc_cap``
    The starting index of a string of capability indices in
    ``.gaps.captab``, indicating capabilities for this enclave.

``enc_main``
    The index of the entry in ``.symtab`` to be used as the main
    function for this enclave.

``Elf64_GAPS_cap``
==================

Encodes information about capabilities.

.. code-block:: c

                typedef struct {
                    Elf64_Addr cap_name;
                    Elf64_Word cap_parent;
                    Elf64_Word cap_padding;
                } Elf64_GAPS_cap;

``cap_name``
    The offset of a ``.gaps.strtab`` entry for the name of this
    capability.

``cap_parent``
    If this entry represents an extended capability, then this stores
    the index of the parent capability.  Otherwise, this should be set
    to ``CAP_NULL``.

``Elf64_GAPS_req``
==================

Encodes information about the capabilities and/or enclave
attributes of a symbol.

.. code-block:: c

                typedef struct {
                    Elf64_Word req_cap;
                    Elf64_Word req_enc;
                    Elf64_Half req_sym;
                    Elf64_Half req_padding;
                } Elf64_GAPS_req;

``req_cap``
    The offset of a string of capability indices in ``.gaps.captab``
    indicating capability for the ``.symtab`` entry with the
    corresponding index.

``req_enc``
    If this symbol was annotated with ``enclave_only(e)``, the index
    into ``.gaps.enclaves`` of the enclave ``e``. Otherwise, this
    should be set to ``ENC_UNDEF``.

``req_sym``
    The symtab index of the symbol with these requirements.

``Elf64_GAPS_res``
==================

Encodes information about a PIRATE initialized resource

 .. code-block:: c

                typedef struct {
                    Elf64_Word res_name;
                    Elf64_Word res_type;
                    Elf64_Word res_param;
                    Elf64_Half res_sym;
                    Elf64_Half res_padding;
                } Elf64_GAPS_res;

``res_name``
    The offset of a ``.gaps.res.strtab`` entry for the user-defined name
    of the resource.

``res_type``
    The offset of a ``.gaps.res.strtab`` entry for the type
    of the resource.

``res_param``
    The index into the ``.gaps.res.params`` array for the first
    parameter for this resource.

``res_sym``
    The index into ``.symtab`` identifying the variable
    this resource is associated with.

``Elf64_GAPS_res_param``
========================

Encodes information about a parameter for a PIRATE initialized resource.

 .. code-block:: c

                typedef struct {
                    Elf64_Word param_name;
                    Elf64_Word param_value;
                } Elf64_GAPS_res_param;

``param_name``
    The offset of a ``.gaps.res.strtab`` entry for the name
    of the resource parameter.  If this is ``0``, then this
    terminates the parameter list for the current resource.

``param_value``
    The offset of a ``.gaps.res.strtab`` entry for the value of
    of the resource parameter.

Linking Examples
----------------

[Note: It would be nice to have an example with more meaningful file
and enclave names]

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
