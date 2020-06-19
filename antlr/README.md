# IDL code generator

This is a code generator for a subset of the IDL specification language.

Common Data Representation (CDR) is used to represent structured or primitive
data types passed as arguments or results during remote invocations on Common
Object Request Broker Architecture (CORBA) distributed objects. The Interface
Definition Language (IDL) is a specification language that is used to describe
the CORBA data representation and component APIs in a language-independent way.

We use a subset of CDR and IDL to describe the representation of gaps messages.
Specifically, we restrict gaps messages to be described using IDL with only a
sequence of type declarations surrounded by (possibly-nested) module declarations.
We do not adopt the messaging or encapsulation sections of the CORBA specification.
We apply the CDR byte order rules by assuming network byte order in all cases.
We apply the standard CDR alignment rules.

We further restrict the subset of IDL to type declarations that have a constant size.
Type declarations with a constant size are the primitive types (characters, bytes,
booleans, integers, floats, enums), structs, unions, and arrays. Strings, sequences,
and recursive types would be excluded. Strings can be represented using character
arrays with trailing null characters.

### Build Instructions

You will need the ANTLR4 C++ runtime to run the code generator.
On OS X and Windows a pre-built C++ runtime is available. On Linux
systems you will need to compile from source. The links to the
binaries and source are at https://www.antlr.org/download.html.
Scroll down to "C++ Target". When building from source on Linux
you will need to use the CMake flag "-DANTLR4_INSTALL=1". If
you are modifying the IDL code generator we also recommend using
the flag "-DCMAKE_BUILD_TYPE=Debug".

You will also need to download the Complete ANTLR 4.8 Java binaries
jar. There is a link to the jar from the ANTLR download page above.

This project uses CMake. The following CMake flags are required:

- ANTLR4_MODULE_PATH. The path to the ANTLR4 runtime CMake modules.
- ANTLR4_JAR_LOCATION. The path to the ANTLR4 jar.

The following CMake flags are optional:

- PIRATE_UNIT_TESTS. Enables the unit tests. googletest library is required.

### Usage

```
./CDRGenerator < [input idl file] > [output C file]
```

### Example

The following IDL specification generates the C output below.

```
module PNT {
	struct Position {
		double x, y, z;
	};
};
```

generates,

```
#include <endian.h>
#include <stdint.h>

struct position {
    double x __attribute__((aligned(8)));
    double y __attribute__((aligned(8)));
    double z __attribute__((aligned(8)));
};

void encode_position(struct position* input, struct position* output) {
    uint64_t x = *(uint64_t*) &input->x;
    uint64_t y = *(uint64_t*) &input->y;
    uint64_t z = *(uint64_t*) &input->z;
    x = htobe64(x);
    y = htobe64(y);
    z = htobe64(z);
    output->x = *(double*) &x;
    output->y = *(double*) &y;
    output->z = *(double*) &z;
}

void decode_position(struct position* input, struct position* output) {
    uint64_t x = *(uint64_t*) &input->x;
    uint64_t y = *(uint64_t*) &input->y;
    uint64_t z = *(uint64_t*) &input->z;
    x = be64toh(x);
    y = be64toh(y);
    z = be64toh(z);
    output->x = *(double*) &x;
    output->y = *(double*) &y;
    output->z = *(double*) &z;
}

```


### Tests

The regression test suite will run the code generator on a set of pre-defined
IDL grammars and compares the generated code to the expected good output. The
expected good output of the regression tests are compiled as a sanity check.
The valgrind target will run the regression test suite under the valgrind leak
detection tool.

### Features Implemented

 - [x] float
 - [x] double
 - [ ] long double
 - [x] short
 - [x] long
 - [x] long long
 - [x] unsigned short
 - [x] unsigned long
 - [x] unsigned long long
 - [x] char
 - [x] boolean
 - [x] octet
 - [ ] enum
 - [ ] 1-dimensional array
 - [ ] N-dimensional array
 - [x] struct
 - [ ] union
 - [x] module
 - [ ] nested struct
 - [ ] nested union
 - [ ] nested module
 - [ ] annotations
