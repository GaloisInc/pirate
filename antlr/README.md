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

The directory `generated-src` is not used by the build system.
The build script will regenerate those files from the ANTLR grammar.
Those files are included in the git repository to make it
easier for development. The Visual Studio Code ANTLR plugin can
be configured with the following settings:

```
"antlr4.generation": {
    "mode": "external",
    "language": "Cpp",
    "outputDir": "generated-src",
    "listeners": true,
    "visitors": true
},
```

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
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = htobe64(x);
    y = htobe64(y);
    z = htobe64(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
}

void decode_position(struct position* input, struct position* output) {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = be64toh(x);
    y = be64toh(y);
    z = be64toh(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
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
 - [x] enum
 - [x] 1-dimensional array
 - [x] N-dimensional array
 - [x] struct
 - [x] union
 - [x] module
 - [x] type references
 - [ ] nested struct
 - [ ] nested union
 - [ ] nested module
 - [ ] type references across namespaces
 - [x] annotations

### Supported Annotations
 
 - `@min(value)` specifies a minimum value for the annotated element
 - `@max(value)` specifies a minimum value for the annotated element
 - `@range(min=value,max=value)` specifies a minimum and maximum value for the annotated element
 - `@round` specifies the float-point value will be rounded to the nearest integer using round half to even mode

### Dependencies

[ANTLR](https://github.com/antlr/antlr4) is available under the BSD 3-clause license.

[ostream_indenter](https://github.com/spacemoose/ostream_indenter/) is available under the LGPL license.
