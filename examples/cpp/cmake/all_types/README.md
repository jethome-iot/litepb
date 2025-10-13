# All Types Showcase Example (CMake)

This comprehensive example demonstrates **ALL** Protocol Buffer types supported by LitePB using CMake.

## What This Example Shows

This example showcases serialization and deserialization of:

### Scalar Types
- `int32`, `int64` - Variable-length encoding
- `uint32`, `uint64` - Unsigned variable-length encoding  
- `sint32`, `sint64` - Zigzag encoding for signed integers (efficient for negative numbers)
- `bool` - Boolean values
- `string` - UTF-8 text strings
- `bytes` - Binary data

### Fixed-Width Types
- `fixed32`, `fixed64` - Always 4 or 8 bytes (efficient for large values)
- `sfixed32`, `sfixed64` - Signed fixed-width integers

### Floating-Point Types
- `float` - 32-bit floating point
- `double` - 64-bit floating point

### Complex Types
- **Enums** - Enumerated values with named constants
- **Nested Messages** - Messages within messages
- **Repeated Fields** - Arrays/lists (packed and unpacked)
- **Map Fields** - Key-value dictionaries (string→int32, string→message)
- **Oneof** - Union types (only one field can be set at a time)

## Building and Running

### Prerequisites

- CMake 3.14 or higher
- C++11 compatible compiler
- LitePB generator (`litepb_gen` in the project root)

### Build Instructions

```bash
# From this directory
cmake -B build
cmake --build build

# Run the example
./build/all_types
```

### Expected Output

The program will:
1. Create a `TypeShowcase` message with ALL protobuf field types populated
2. Display the original data for each type category
3. Serialize the complete message to binary format
4. Show the serialized bytes in hexadecimal
5. Deserialize the bytes back to a message
6. Verify each field type matches the original data
7. Report success or failure for each category

## Project Structure

```
all_types/
├── proto/
│   └── showcase.proto    # Comprehensive proto with all types
├── src/
│   └── main.cpp         # Example application with verification
├── CMakeLists.txt       # CMake build configuration
└── README.md           # This file
```

## How It Works

1. **showcase.proto** defines a `TypeShowcase` message with fields of every protobuf type
2. **CMakeLists.txt** automatically invokes `litepb_gen` to generate C++ code
3. The generated code (`showcase.pb.h` and `showcase.pb.cpp`) is placed in `build/generated/`
4. The executable is linked with the LitePB static library
5. **main.cpp** demonstrates round-trip serialization with comprehensive verification

## Learning Points

This example is perfect for:
- Understanding wire format for different protobuf types
- Learning how LitePB handles complex nested structures
- Seeing the binary representation of protobuf messages
- Verifying compatibility with standard Protocol Buffers
