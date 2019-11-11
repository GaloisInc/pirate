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
enclaves to generate executables for; an entrypoint, in the form of
the index of a ``.symtab`` entry; and a list of capapilities and
sensitivity authorizations the enclave offers.  For each symbol in
``.symtab``, we provide a list of required capabilities and
sensitivity authorizations, and optionally, the enclave that is
allowed to run or access the symbol.

Note that capabilities and sensitivities are handled identically: Both
are identified internally by the index of their entry in ``.captab``
and externally by their user-defined name, which must be unique within
the compilation unit.  Sensitivities optionally contain the index of a
parent sensitivity, meaning that an enclave authorized at the child
level is also authorized at the parent level (i.e., the parent is less
sensitive than the child).  Lists of required or provided
capabilities/sensitivities are specified as strings in ``.capstrtab``,
ending in ``CAP_NULL``.

Special Sections
----------------

We add six new special sections to the relocatable ELF format. Like
Linux special sections, these are identified by their section names,
so the ``sh_type`` fields in their section headers should be set to
``SHT_NULL``.

``.gaps.enclaves``
    An array of ``Elf64_GAPS_enclave`` listing the names, capabilities
    and entrypoints of each enclave declared in the source file. The
    first entry is unused and corresponds to ``ENC_UNDEF``.

``.gaps.reqtab``
    An array of ``Elf64_GAPS_symreq`` indexed parallel to ``.symtab``.
    This section lists the capabilities/sensitivities and enclave (if
    any) required by the ``.symtab`` entry with the same index.

``.gaps.capabilities``
    An array of ``Elf64_GAPS_cap`` listing the
    capabilities/sensitivities defined in the source file. The first
    entry is unused and corresponds to ``CAP_NULL``.

``.gaps.captab``
    A vector of arrays of ``Elf64_Half`` indices into ``.gaps.captab``,
    each terminated by ``CAP_NULL``. Offset zero contains a 0 to
    signify an empty capacity list.

``.gaps.strtab``
    A vector of zero-terminated strings to hold the names of enclaves,
    sensitivities, and capabilities. Offset zero contains a null byte,
    to signify the empty string.

``.gaps.channels``
    An array of ``Elf64_GAPS_channel`` listing the channels declared
    in the source file.

Structures
----------

[Note: We may want to add placeholder fields to take advantage of the
empty space left by alignment.]

.. code-block:: c

                typedef struct {
                    Elf64_Addr enc_name;
                    Elf64_Word enc_cap;
                    Elf64_Word enc_entry;
                } Elf64_GAPS_enclave;

``enc_name``
    The offset of a ``.gaps.strtab`` entry for the user-defined name
    of the enclave.

``enc_cap``
    The offset of a string of capability indices in ``.gaps.captab``,
    indicating capabilities and sensitivity permissions for this
    enclave.

``enc_entry``
    The index of the entry in ``.symtab`` to be used as an entrypoint
    for this enclave.

.. code-block:: c

                typedef struct {
                    Elf64_Word sr_cap;
                    Elf64_Word sr_enc;
                } Elf64_GAPS_symreq;

``sr_cap``
    The address of a string of capability indices in ``.gaps.captab``
    indicating capability and sensitivity requirements for the
    ``.symtab`` entry with the corresponding index.

``sr_enc``
    If this symbol was annotated with ``enclave_only(e)``, the index
    into ``.gaps.enclaves`` of the enclave ``e``. Otherwise, this
    should be set to ``ENC_UNDEF``.

.. code-block:: c

                typedef struct {
                    Elf64_Addr cap_name;
                    Elf64_Word cap_parent;
                } Elf64_GAPS_cap;

``cap_name``
    The address of a string-table entry for the name of this
    capability or sensitivity.

``cap_parent``
    If this entry reoresents a sensitivity, the level that this
    sensitivity was declared as being more sensitive than in the
    source file. Otherwise, thisshould be set to ``CAP_NULL``.

 .. code-block:: c

                typedef struct {
                    Elf64_Addr chan_initdata;
                    Elf64_Word chan_source;
                    Elf64_Word chan_sink;
                } Elf64_GAPS_channel;

``chan_data``
    A pointer to the blob of data needed to initialize the channel.

``chan_source``
    The index into ``.gaps.enclaves`` indicating the enclave
    authorized to send on this channel.

``chan_sink``
    The index into ``.gaps.enclaves`` indicating the enclave
    authorized to receive on this channel.

Global symbols
--------------

Executable ELFs linked by a GAPS-aware linker will define the
following global symbols:

.. code-block:: c

                void **__gaps_receive_channels;
                void **__gaps_send_channels;

``__gaps_receive_channels``
    A ``NULL``-terminated array of pointers to blobs of initialization
    data for each receive channel the executable's enclave is
    authorized to access.

``__gaps_send_channels``
    A ``NULL``-terminated array of pointers to blobs of initialization
    data for each send channel the executable's enclave is authorized
    to access.

A GAPS-aware linker is responsible for populating these from
``.gaps.channels``. Meanwhile, the GAPS runtime will define a
constructor in ``.init`` that references these global symbols to
initialize all send and receive channels.
