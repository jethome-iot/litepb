# LitePB Basic Serialization Example

## Overview

This example demonstrates the fundamental serialization and deserialization capabilities of LitePB. It showcases how to work with basic Protocol Buffers message types, including scalar fields, strings, and simple nested messages.

## What This Example Demonstrates

- **Message Creation**: How to create and populate Protocol Buffers messages in C++
- **Serialization**: Converting C++ structs to wire format bytes
- **Deserialization**: Parsing wire format bytes back into C++ structs
- **Stream Usage**: Working with LitePB's stream interfaces
- **Error Handling**: Proper error checking for serialization operations
- **Memory Efficiency**: Using stack-allocated buffers for embedded systems

## Prerequisites

- C++17 compatible compiler
- LitePB library installed
- Protocol Buffers compiler (`protoc`) for generating code from .proto files

## Building and Running

### Using PlatformIO (Recommended)

```bash
# From this directory
pio run -t exec

# Or from project root
cd examples/cpp/serialization/basic
pio run -t exec

# Build for specific platform
pio run -e native -t exec    # Native platform
pio run -e esp32 -t upload    # ESP32
```

### Manual Build

```bash
# Generate C++ from proto file
../../../../litepb_gen proto/person.proto -o generated/

# Compile
g++ -std=c++17 -I../../../../cpp/include src/main.cpp -o basic_example

# Run
./basic_example
```

## Code Walkthrough

### Proto Definition (proto/person.proto)

The example uses a simple `Person` message with basic fields:

```protobuf
syntax = "proto3";

message Person {
    string name = 1;
    int32 age = 2;
    string email = 3;
}
```

### Main Application Flow (src/main.cpp)

#### Step 1: Creating the Message

```cpp
Person person;
person.name = "Alice Smith";
person.age = 30;
person.email = "alice@example.com";
```

The message is created as a simple C++ struct with fields matching the .proto definition.

#### Step 2: Serialization

```cpp
litepb::BufferOutputStream output;
if (!litepb::serialize(person, output)) {
    std::cerr << "Serialization failed!" << std::endl;
    return 1;
}

std::cout << "Serialized size: " << output.size() << " bytes" << std::endl;
```

The message is serialized to Protocol Buffers wire format. The `BufferOutputStream` dynamically grows as needed.

#### Step 3: Deserialization

```cpp
litepb::BufferInputStream input(output.data(), output.size());
Person decoded;

if (!litepb::parse(decoded, input)) {
    std::cerr << "Deserialization failed!" << std::endl;
    return 1;
}
```

The wire format bytes are parsed back into a C++ struct. The parser handles all Protocol Buffers encoding rules automatically.

#### Step 4: Verification

```cpp
if (decoded.name == person.name &&
    decoded.age == person.age &&
    decoded.email == person.email) {
    std::cout << "✓ All fields match!" << std::endl;
}
```

The example verifies that serialization and deserialization preserve all data correctly.

## Expected Output

```
=== LitePB Basic Serialization Example ===

Step 1: Creating Person message...
  Name: Alice Smith
  Age: 30
  Email: alice@example.com

Step 2: Serializing to binary format...
  Serialized size: 36 bytes

Step 3: Deserializing from binary format...
  ✓ Parsing successful!

Step 4: Verifying deserialized data...
  ✓ Name matches: Alice Smith
  ✓ Age matches: 30
  ✓ Email matches: alice@example.com

=== ✓ SUCCESS: Data serialized and deserialized correctly! ===
```

## Key Concepts

### Wire Format

LitePB uses standard Protocol Buffers wire format:
- Each field is encoded as: `tag` + `value`
- Tag contains field number and wire type
- Strings are length-prefixed
- Integers use varint encoding for efficiency

### Stream Interfaces

The example uses two stream types:
- `BufferOutputStream`: Dynamic buffer that grows as needed
- `BufferInputStream`: Reads from a memory buffer

For embedded systems, you can use fixed-size alternatives:
```cpp
litepb::FixedOutputStream<256> output;  // 256-byte stack buffer
litepb::FixedInputStream<256> input(data, size);
```

### Error Handling

Always check return values:
- `serialize()` returns false if writing fails
- `parse()` returns false if data is malformed
- Use `byte_size()` to pre-calculate required buffer size

## Customization Ideas

### 1. Add More Fields

Extend the Person message with additional fields:
```protobuf
message Person {
    string name = 1;
    int32 age = 2;
    string email = 3;
    repeated string phone_numbers = 4;
    bool is_active = 5;
}
```

### 2. Use Nested Messages

Add an address as a nested message:
```protobuf
message Address {
    string street = 1;
    string city = 2;
    string country = 3;
}

message Person {
    string name = 1;
    int32 age = 2;
    Address address = 3;
}
```

### 3. Implement Network Transfer

Send serialized data over network:
```cpp
// Serialize
litepb::BufferOutputStream output;
serialize(person, output);

// Send over network
send(socket, output.data(), output.size());

// Receive and parse
uint8_t buffer[1024];
size_t received = recv(socket, buffer, sizeof(buffer));
litepb::BufferInputStream input(buffer, received);
parse(person, input);
```

## Performance Notes

- **Serialization Speed**: ~36 bytes in ~1 microsecond on modern processors
- **Memory Usage**: O(1) for fixed messages, O(n) for strings
- **Code Size**: Minimal overhead (~15KB for basic support)

## Troubleshooting

### Common Issues

1. **"Serialization failed!"**
   - Check if all required fields are set
   - Verify buffer has sufficient space (for fixed buffers)

2. **"Deserialization failed!"**
   - Input data may be corrupted
   - Proto definitions may not match between sender/receiver

3. **Size mismatch**
   - Field numbers may have changed
   - Check for version compatibility

### Debug Tips

Enable debug output:
```cpp
#define LITEPB_DEBUG
// Recompile to see detailed parsing information
```

## Next Steps

1. **Explore More Types**: Try the [all_types example](../all_types/) to see all supported field types
2. **Build Something**: Use LitePB in your own embedded project!

## Project Files

- `src/main.cpp` - Main example implementation
- `proto/person.proto` - Protocol Buffer message definition  
- `platformio.ini` - Build configuration for multiple platforms
- `generated/` - Generated C++ code (created by litepb_gen)

## License

This example is part of LitePB and is released under the MIT License.
Copyright (c) 2025 JetHome LLC