ELF extensions
^^^^^^^^^^^^^^

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Overview
--------

[Note: depending on what channels look like and whether they need to
be initialized in the start routine, an additional section or sections
may need to be added for them here.]

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
level is also authorized at the parent level (I.e., the parent is less
sensitive than the child).  Lists of required or provided
capabilities/sensitivities are specified as strings in ``.capstrtab``,
ending in ``CAP_NULL``.

Special Sections
----------------

We add five new special sections to the relocatable ELF format. Like
Linux special sections, these are identified by their section names,
so the ``sh_type`` fields in their section headers should be set to
``SHT_NULL``.

``.gaps.enclaves``
    An array of ``Elf64_GAPS_enclave`` listing the names, capabilities
    and entrypoints of each enclave declared in the source file. The
    first entry is unused and corresponds to ``ENC_UNDEF``. This
    section may appear only in relocatable ELFs.

``.gaps.symreq``
    An array of ``Elf64_GAPS_symreq`` indexed parallel to ``.symtab``.
    This section lists the capabilities/sensitivities and enclave (if
    any) required by the ``.symtab`` entry with the same index. This
    section may appear only in relocatable ELFs.

``.gaps.captab``
    An array of ``Elf64_GAPS_cap`` listing the
    capabilities/sensitivities defined in the source file. The first
    entry is unused and corresponds to ``CAP_NULL``. This section may
    appear only in relocatable ELFs.

``.gaps.capstrtab``
    A vector of arrays of indices into ``.gaps.captab``, each
    terminated by ``CAP_NULL``. This section may appear only in
    relocatable ELFs.

Structures
----------

[Note: We may want to add placeholder fields to take advantage of the
empty space left by alignment.]

.. code-block:: c

                typedef struct {
                    Elf64_Addr enc_name;
                    Elf64_Addr enc_cap;
                    Elf64_Word enc_entry;
                } Elf64_GAPS_enclave;

``enc_name``
    The address of a string-table entry for the user-defined name of
    the enclave.

``enc_cap``
    The address of a string of capability indices in
    ``.gaps.capstrtab``, indicating capabilities and sensitivity
    permissions for this enclave.

``enc_entry``
    The index of the entry in ``.symtab`` to be used as an entrypoint
    for this enclave.

.. code-block:: c

                typedef struct {
                    Elf64_Addr sr_cap;
                    Elf64_Word sr_enc;
                } Elf64_GAPS_symreq;

``sr_cap``
    The address of a string of capability indices in
    ``.gaps.capstrtab`` indicating capability and sensitivity
    requirements for the ``.symtab`` entry with the corresponding
    index.

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
