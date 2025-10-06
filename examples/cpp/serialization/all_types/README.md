# LitePB All Types Showcase

## Overview

This comprehensive example demonstrates every Protocol Buffers field type supported by LitePB. It serves as both a reference implementation and a validation test for the complete type system, showing proper usage of all scalar types, complex types, and advanced features.

## What This Example Demonstrates

- **All Scalar Types**: int32, int64, uint32, uint64, sint32, sint64, bool, string, bytes
- **Fixed-Width Types**: fixed32, fixed64, sfixed32, sfixed64, float, double
- **Complex Types**: enums, nested messages, repeated fields, maps, oneofs
- **Wire Format Encoding**: How different types are encoded on the wire
- **Proto2 vs Proto3**: Differences in default values and field presence
- **Memory Efficiency**: Optimal encoding for different data patterns

## Prerequisites

- C++17 compatible compiler
- LitePB library installed
- Protocol Buffers compiler (`protoc`)

## Building and Running

### Using PlatformIO (Recommended)

```bash
# From this directory
pio run -t exec

# Or from project root
cd examples/cpp/serialization/all_types
pio run -t exec

# Build for specific platforms
pio run -e native -t exec     # Native platform
pio run -e esp32 -t upload     # ESP32
pio run -e stm32 -t upload     # STM32
```

### Manual Build

```bash
# Generate C++ from proto file
../../../../litepb_gen proto/showcase.proto -o generated/

# Compile
g++ -std=c++17 -I../../../../cpp/include src/main.cpp -o all_types_example

# Run
./all_types_example
```

## Code Walkthrough

### Proto Definition (proto/showcase.proto)

The showcase proto demonstrates all supported types:

```protobuf
syntax = "proto3";

message TypeShowcase {
    // Scalar types
    int32 field_int32 = 1;
    int64 field_int64 = 2;
    uint32 field_uint32 = 3;
    uint64 field_uint64 = 4;
    sint32 field_sint32 = 5;      // Efficient for negative numbers
    sint64 field_sint64 = 6;      // Uses zigzag encoding
    bool field_bool = 7;
    string field_string = 8;
    bytes field_bytes = 9;
    
    // Fixed-width types
    fixed32 field_fixed32 = 10;   // Always 4 bytes
    fixed64 field_fixed64 = 11;   // Always 8 bytes
    sfixed32 field_sfixed32 = 12; // Signed fixed 32-bit
    sfixed64 field_sfixed64 = 13; // Signed fixed 64-bit
    float field_float = 14;       // 32-bit IEEE 754
    double field_double = 15;     // 64-bit IEEE 754
    
    // Enum type
    enum Status {
        UNKNOWN = 0;
        ACTIVE = 1;
        INACTIVE = 2;
        PENDING = 3;
    }
    Status field_enum = 16;
    
    // Nested message
    message NestedMessage {
        string name = 1;
        int32 value = 2;
    }
    NestedMessage field_nested = 17;
    
    // Repeated fields
    repeated int32 field_repeated_int32 = 18;
    repeated string field_repeated_string = 19;
    repeated NestedMessage field_repeated_nested = 20;
    
    // Map fields
    map<string, int32> field_map_string_int32 = 21;
    map<int32, NestedMessage> field_map_int32_nested = 22;
    
    // Oneof field
    oneof field_oneof {
        string oneof_string = 23;
        int32 oneof_int32 = 24;
        NestedMessage oneof_nested = 25;
    }
}
```

### Implementation Details (src/main.cpp)

#### Step 1: Populating All Fields

```cpp
TypeShowcase showcase;

// Scalar types with edge cases
showcase.field_int32 = -2147483648;        // Min int32
showcase.field_int64 = 9223372036854775807; // Max int64
showcase.field_uint32 = 4294967295;        // Max uint32
showcase.field_uint64 = 18446744073709551615ULL; // Max uint64
showcase.field_sint32 = -1000;             // Negative (zigzag encoded)
showcase.field_sint64 = -1000000;          // Negative (zigzag encoded)
showcase.field_bool = true;
showcase.field_string = "Hello, LitePB! 你好！🚀"; // Unicode support
showcase.field_bytes = {0x00, 0xFF, 0x42}; // Binary data

// Fixed-width types
showcase.field_fixed32 = 0xDEADBEEF;       // Hex constant
showcase.field_fixed64 = 0xCAFEBABEDEADBEEFULL;
showcase.field_sfixed32 = -123456;
showcase.field_sfixed64 = -9876543210;
showcase.field_float = 3.14159f;           // Pi
showcase.field_double = 2.718281828459045; // Euler's number

// Enum
showcase.field_enum = TypeShowcase::Status::ACTIVE;

// Nested message
showcase.field_nested.name = "nested";
showcase.field_nested.value = 42;

// Repeated fields
showcase.field_repeated_int32 = {1, 2, 3, 4, 5};
showcase.field_repeated_string = {"alpha", "beta", "gamma"};

// Map fields
showcase.field_map_string_int32["one"] = 1;
showcase.field_map_string_int32["two"] = 2;
showcase.field_map_string_int32["three"] = 3;

// Oneof (only one can be set)
showcase.field_oneof = "oneof_active";  // Sets oneof_string variant
```

#### Step 2: Serialization

```cpp
litepb::BufferOutputStream output;
if (!litepb::serialize(showcase, output)) {
    std::cerr << "Serialization failed!" << std::endl;
    return 1;
}

std::cout << "Serialized size: " << output.size() << " bytes" << std::endl;

// Display hex dump for analysis
for (size_t i = 0; i < output.size() && i < 32; ++i) {
    printf("%02X ", output.data()[i]);
}
```

#### Step 3: Deserialization and Verification

```cpp
litepb::BufferInputStream input(output.data(), output.size());
TypeShowcase decoded;

if (!litepb::parse(decoded, input)) {
    std::cerr << "Deserialization failed!" << std::endl;
    return 1;
}

// Verify all fields
assert(decoded.field_int32 == showcase.field_int32);
assert(decoded.field_int64 == showcase.field_int64);
assert(decoded.field_float == showcase.field_float);
assert(decoded.field_string == showcase.field_string);
assert(decoded.field_enum == showcase.field_enum);
assert(decoded.field_repeated_int32.size() == 5);
assert(decoded.field_map_string_int32.size() == 3);
// ... verify all fields ...
```

## Expected Output

```
=== LitePB All Types Showcase ===

Step 1: Creating TypeShowcase message with all field types...

Step 2: Displaying original data...
Scalar Types:
  int32: -2147483648 (min)
  int64: 9223372036854775807 (max)
  uint32: 4294967295 (max)
  uint64: 18446744073709551615 (max)
  sint32: -1000 (zigzag encoded)
  sint64: -1000000 (zigzag encoded)
  bool: true
  string: "Hello, LitePB! 你好！🚀"
  bytes: 3 bytes [00 FF 42]

Fixed-Width Types:
  fixed32: 0xDEADBEEF
  fixed64: 0xCAFEBABEDEADBEEF
  sfixed32: -123456
  sfixed64: -9876543210
  float: 3.14159
  double: 2.718281828459045

Complex Types:
  enum: ACTIVE (1)
  nested message: {name: "nested", value: 42}
  repeated int32: [1, 2, 3, 4, 5]
  repeated string: ["alpha", "beta", "gamma"]
  map<string,int32>: {one: 1, two: 2, three: 3}
  oneof: string variant = "oneof_active"

Step 3: Serializing to binary format...
  Serialized size: 335 bytes
  Hex: 08 80 80 80 80 F8 FF FF FF FF 01 10 FF FF FF ...

Step 4: Deserializing from binary format...
  ✓ Parsing successful!

Step 5: Verifying all field types...
  ✓ All scalar types match
  ✓ All fixed-width types match
  ✓ Enum value matches
  ✓ Nested message matches
  ✓ All repeated fields match (5 + 3 + 2 items)
  ✓ All map entries match (3 + 2 items)
  ✓ Oneof variant matches

=== ✓ SUCCESS: All types verified correctly! ===
```

## Wire Format Details

### Encoding Characteristics

| Type | Wire Type | Encoding | Size |
|------|-----------|----------|------|
| int32/int64 | Varint | Variable-length | 1-10 bytes |
| uint32/uint64 | Varint | Variable-length | 1-10 bytes |
| sint32/sint64 | Varint | Zigzag + variable | 1-10 bytes |
| bool | Varint | 0 or 1 | 1 byte |
| fixed32/sfixed32/float | Fixed32 | Little-endian | 4 bytes |
| fixed64/sfixed64/double | Fixed64 | Little-endian | 8 bytes |
| string/bytes | Length-delimited | Length + data | Variable |
| nested message | Length-delimited | Length + message | Variable |

### Optimization Tips

1. **Use sint32/sint64 for negative numbers**: More efficient encoding
2. **Use fixed32/fixed64 for values often > 2^28**: Avoids varint overhead
3. **Pack repeated fields**: Proto3 does this by default
4. **Order fields by frequency**: Put common fields first
5. **Use oneof for mutually exclusive fields**: Saves space

## Advanced Features

### Working with Maps

Maps are encoded as repeated key-value pairs:
```cpp
// Setting map values
showcase.field_map_string_int32["key"] = 100;

// Iterating over maps
for (const auto& [key, value] : decoded.field_map_string_int32) {
    std::cout << key << ": " << value << std::endl;
}
```

### Using Oneofs

Oneofs ensure only one field is set:
```cpp
// Setting a oneof
showcase.field_oneof = TypeShowcase::OneofString{"text"};
// or
showcase.field_oneof = TypeShowcase::OneofInt32{42};

// Checking which variant
if (std::holds_alternative<TypeShowcase::OneofString>(decoded.field_oneof)) {
    auto& str = std::get<TypeShowcase::OneofString>(decoded.field_oneof);
    std::cout << "Oneof contains string: " << str.value << std::endl;
}
```

### Repeated Field Patterns

```cpp
// Reserve capacity for efficiency
showcase.field_repeated_int32.reserve(1000);

// Bulk operations
std::vector<int32_t> data = generate_data();
showcase.field_repeated_int32.insert(
    showcase.field_repeated_int32.end(),
    data.begin(), data.end()
);

// Packed encoding (automatic in Proto3)
// Reduces size from N*(tag+value) to tag+length+(N*value)
```

## Performance Characteristics

| Operation | Performance |
|-----------|------------|
| Serialize 335 bytes | ~2-3 μs (x86-64) |
| Deserialize 335 bytes | ~3-4 μs (x86-64) |
| Memory overhead | ~2x message size |
| Code size impact | ~30KB total |

## Testing Strategies

### Boundary Testing

```cpp
// Test minimum values
msg.field_int32 = std::numeric_limits<int32_t>::min();
msg.field_int64 = std::numeric_limits<int64_t>::min();

// Test maximum values
msg.field_uint32 = std::numeric_limits<uint32_t>::max();
msg.field_uint64 = std::numeric_limits<uint64_t>::max();

// Test empty collections
msg.field_repeated_int32.clear();
msg.field_map_string_int32.clear();
```

### Compatibility Testing

```cpp
// Test forward compatibility (unknown fields are skipped)
// Test backward compatibility (missing fields use defaults)
// Test wire format compatibility with protoc
```

## Troubleshooting

### Common Issues

1. **Large serialized size**
   - Check if using appropriate types (sint vs int for negatives)
   - Verify repeated fields are packed
   - Consider field ordering

2. **Deserialization failures**
   - Verify proto definitions match
   - Check for data corruption
   - Ensure buffer sizes are adequate

3. **Type mismatches**
   - Proto field numbers must not change
   - Wire types must be compatible
   - Check for version skew

## Next Steps

1. **Study the Implementation**: Examine generated code in `generated/`
2. **Try RPC**: See [RPC example](../../rpc/litepb_rpc/) for remote calls
3. **Optimize Your Messages**: Apply learnings to your own protos
4. **Benchmark**: Compare performance with your use case
5. **Contribute**: Found an issue? Submit a PR!

## Project Files

- `src/main.cpp` - Comprehensive example implementation
- `proto/showcase.proto` - Complete type showcase definition
- `platformio.ini` - Multi-platform build configuration
- `generated/` - Generated C++ code (created by litepb_gen)

## License

This example is part of LitePB and is released under the MIT License.
Copyright (c) 2025 JetHome LLC