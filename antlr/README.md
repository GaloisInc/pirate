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

- PIRATE_UNIT_TEST. Enables the unit tests. googletest library is required.

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
./CDRGenerator --target [c|cpp] < [input idl file] > [output C or C++ file]
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

generates the following C code,

```
#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


struct position {
    double x __attribute__((aligned(8)));
    double y __attribute__((aligned(8)));
    double z __attribute__((aligned(8)));
};

struct position_wire {
    unsigned char x[8] __attribute__((aligned(8)));
    unsigned char y[8] __attribute__((aligned(8)));
    unsigned char z[8] __attribute__((aligned(8)));
};

static_assert(sizeof(struct position) == sizeof(struct position_wire),
    "size of struct position not equal to wire protocol struct");

void encode_position(struct position* input, struct position_wire* output) {
    uint64_t field_x;
    uint64_t field_y;
    uint64_t field_z;
    memcpy(&field_x, &input->x, sizeof(uint64_t));
    memcpy(&field_y, &input->y, sizeof(uint64_t));
    memcpy(&field_z, &input->z, sizeof(uint64_t));
    field_x = htobe64(field_x);
    field_y = htobe64(field_y);
    field_z = htobe64(field_z);
    memcpy(&output->x, &field_x, sizeof(uint64_t));
    memcpy(&output->y, &field_y, sizeof(uint64_t));
    memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void decode_position(struct position_wire* input, struct position* output) {
    uint64_t field_x;
    uint64_t field_y;
    uint64_t field_z;
    memcpy(&field_x, &input->x, sizeof(uint64_t));
    memcpy(&field_y, &input->y, sizeof(uint64_t));
    memcpy(&field_z, &input->z, sizeof(uint64_t));
    field_x = be64toh(field_x);
    field_y = be64toh(field_y);
    field_z = be64toh(field_z);
    memcpy(&output->x, &field_x, sizeof(uint64_t));
    memcpy(&output->y, &field_y, sizeof(uint64_t));
    memcpy(&output->z, &field_z, sizeof(uint64_t));
}
```

or the following C++ code,

```
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace pnt {

    struct position {
        double x __attribute__((aligned(8)));
        double y __attribute__((aligned(8)));
        double z __attribute__((aligned(8)));
    };

    struct position_wire {
        unsigned char x[8] __attribute__((aligned(8)));
        unsigned char y[8] __attribute__((aligned(8)));
        unsigned char z[8] __attribute__((aligned(8)));
    };

    static_assert(sizeof(struct position) == sizeof(struct position_wire),
        "size of struct position not equal to wire protocol struct");
}

namespace pirate {
#ifndef _PIRATE_SERIALIZATION_H
#define _PIRATE_SERIALIZATION_H
    template <typename T>
    struct Serialization {
        static void toBuffer(T const& val, std::vector<char>& buf);
        static T fromBuffer(std::vector<char> const& buf);
    };
#endif // _PIRATE_SERIALIZATION_H

    template<>
    struct Serialization<struct pnt::position> {
        static void toBuffer(struct pnt::position const& val, std::vector<char>& buf) {
            buf.resize(sizeof(struct pnt::position));
            struct pnt::position_wire* output = (struct pnt::position_wire*) buf.data();
            const struct pnt::position* input = &val;
            uint64_t field_x;
            uint64_t field_y;
            uint64_t field_z;
            memcpy(&field_x, &input->x, sizeof(uint64_t));
            memcpy(&field_y, &input->y, sizeof(uint64_t));
            memcpy(&field_z, &input->z, sizeof(uint64_t));
            field_x = htobe64(field_x);
            field_y = htobe64(field_y);
            field_z = htobe64(field_z);
            memcpy(&output->x, &field_x, sizeof(uint64_t));
            memcpy(&output->y, &field_y, sizeof(uint64_t));
            memcpy(&output->z, &field_z, sizeof(uint64_t));
        }

        static struct pnt::position fromBuffer(std::vector<char> const& buf) {
            struct pnt::position retval;
            const struct pnt::position_wire* input = (const struct pnt::position_wire*) buf.data();
            struct pnt::position* output = &retval;
            if (buf.size() != sizeof(struct pnt::position)) {
                static const std::string error_msg =
                    std::string("pirate::Serialization::fromBuffer() for pnt::position type did not receive a buffer of size ") +
                    std::to_string(sizeof(struct pnt::position));
                throw std::length_error(error_msg);
            }
            uint64_t field_x;
            uint64_t field_y;
            uint64_t field_z;
            memcpy(&field_x, &input->x, sizeof(uint64_t));
            memcpy(&field_y, &input->y, sizeof(uint64_t));
            memcpy(&field_z, &input->z, sizeof(uint64_t));
            field_x = be64toh(field_x);
            field_y = be64toh(field_y);
            field_z = be64toh(field_z);
            memcpy(&output->x, &field_x, sizeof(uint64_t));
            memcpy(&output->y, &field_y, sizeof(uint64_t));
            memcpy(&output->z, &field_z, sizeof(uint64_t));
            return retval;
        }
    };
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
 - `@round` specifies the floating-point value will be rounded to the nearest integer using round half to even mode

### Dependencies

[ANTLR](https://github.com/antlr/antlr4) is available under the BSD 3-clause license.

[ostream_indenter](https://github.com/spacemoose/ostream_indenter/) is available under the LGPL license.
