===================
LLVM Pirate changes
===================

In order to support the pirate extensions, changes to clang and LLVM were needed
across many layers. LLVM in structured with many frontend languages implemented
against a library with many code generation backends. Adding the pirate features
requires changes through all of these layers and beyond into the linker.

Adding pragmas
--------------

**Pragma parser**

- Updated parser to call actions on Sema object

**Semantic actions**

- Updated to record enclaves, capabilities, and links on AST context

Adding attributes
-----------------

**Attrs.td**

- Describe attributes, their targets arguments for enclaves, capabilities, and resources.

**AttrDocs.td**

- Add documentation for new attributes

**SemaDeclAttr.cpp**

- Process arguments
- Validate enclave and capabilities used against AST context lists
- Apply attributes to AST nodes

Propagating attributes into C/C++ AST
-------------------------------------

**PropagateEnclaves.cpp**

- Compute dependency graph of AST
- Compute transitive closure of capability restrictions
- Propagate restrictions to AST nodes

Propagating attributes from C/C++ AST to LLVM AST
-------------------------------------------------

**CodeGenModule.cpp**

- Gather resource names, types, enclaves, parameters from AST nodes for global variables
- Emit resource description data structures into custom output sections
- Add LLVM metadata to LLVM generated code corresponding to enclave restrictions on C/C++ AST

**CodeGenFunction.cpp**

- Gather resource names, types, enclaves, parameters from AST nodes for functions
- Add LLVM metadata to LLVM generated code corresponding to enclave restrictions on C/C++ AST

Propagating information from LLVM AST to object files
-----------------------------------------------------

**AsmPrinter.cpp**

- Gather symbols, enclave metadata and capability metadata LLVM objects in module
- Register enclave and capability data on output streamer

**MCObjectStreamer.cpp**

- Track lists of symbols, enclaves, and capabilities to be emitted into final object

**ELFObjectWriter.cpp**

- Compute final symbol indexes for enclave and capability restrictions
- Emit ELF sections into object file with all gathered enclave and capability data

Propagating information from object files to executable
-------------------------------------------------------
