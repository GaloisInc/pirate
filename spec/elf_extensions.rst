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
the index of a ``.symtab`` entry; and a list of capabilities the
enclave offers.  For each symbol in ``.symtab``, we provide a list of
required capabilities, and optionally, the enclave that is allowed to
run or access the symbol.

Note that capabilities are handled identically: Both are identified
internally by the index of their entry in ``.gaps.captab`` and
externally by their user-defined name, which must be unique within the
compilation unit.  Extended capabilities are capabilities that extend
a previous capability, and these contain the index of a parent
capability.  Lists of required or provided capabilities are specified
as strings in ``.gaps.strtab``, ending in ``CAP_NULL``.

Special Sections
----------------

We add six new special sections to the relocatable ELF format. Like
Linux special sections, these are identified by their section names,
so the ``sh_type`` fields in their section headers should be set to
``SHT_NULL``.

``.gaps.enclaves``
    An array of ``Elf64_GAPS_enc`` listing the names, capabilities
    and entrypoints of each enclave declared in the source file. The
    first entry is unused and corresponds to ``ENC_UNDEF``.

``.gaps.symreqs``
    An array of ``Elf64_GAPS_req`` indexed parallel listing the
    capabilities and enclave (if any) required by the
    ``.symtab`` entry with the same index.

``.gaps.capabilities``
    An array of ``Elf64_GAPS_cap`` listing the
    capabilities defined in the source file. The first
    entry is unused and corresponds to ``CAP_NULL``.

``.gaps.captab``
    A vector of arrays of ``Elf64_Half`` indices into
    ``.gaps.capabilities``, each terminated by ``CAP_NULL``. Offset
    zero contains a 0 to signify an empty capacity list.

``.gaps.strtab`` A vector of zero-terminated strings to hold the names
    of enclaves and capabilities.  Offset zero contains a null byte,
    to signify the empty string.

[Note: Channels are still in the discussion phase, and highly subject to change.]

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
                    Elf64_Half enc_entry;
                    Elf64_Half enc_padding;
                } Elf64_GAPS_enc;

``enc_name``
    The offset of a ``.gaps.strtab`` entry for the user-defined name
    of the enclave.

``enc_cap``
    The offset of a string of capability indices in ``.gaps.captab``,
    indicating capabilities for this enclave.

``enc_entry``
    The index of the entry in ``.symtab`` to be used as an entrypoint
    for this enclave.

.. code-block:: c

                typedef struct {
                    Elf64_Word req_cap;
                    Elf64_Word req_enc;
                    Elf64_Half req_sym;
                    Elf64_Half req_padding;
                } Elf64_GAPS_req;

``req_cap``
    The address of a string of capability indices in ``.gaps.captab``
    indicating capability for the ``.symtab`` entry with the
    corresponding index.

``req_enc``
    If this symbol was annotated with ``enclave_only(e)``, the index
    into ``.gaps.enclaves`` of the enclave ``e``. Otherwise, this
    should be set to ``ENC_UNDEF``.

``req_sym``
    The symtab index of the symbol with these requirements.

.. code-block:: c

                typedef struct {
                    Elf64_Addr cap_name;
                    Elf64_Word cap_parent;
                    Elf64_Word cap_padding;
                } Elf64_GAPS_cap;

``cap_name``
    The address of a string-table entry for the name of this
    capability.

``cap_parent``
    If this entry represents an extended capability, then this stores
    the index of the parent capability.  Otherwise, this should be set
    to ``CAP_NULL``.

[Note: Channels are still in the discussion phase, and highly subject to change.]

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
