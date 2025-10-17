# LitePB - Lightweight Protocol Buffers for Embedded C++

[![CI](https://github.com/jethome-iot/litepb/actions/workflows/ci.yml/badge.svg)](https://github.com/jethome-iot/litepb/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

LitePB is a high-performance, zero-dependency Protocol Buffers implementation specifically designed for embedded systems and resource-constrained environments. Built from the ground up for efficiency, LitePB provides complete Protocol Buffers serialization functionality with minimal overhead, making it ideal for microcontrollers, IoT devices, and edge computing applications.

**Key Highlights:**
- 🚀 **Zero External Dependencies** - Pure C++ implementation, no third-party libraries required
- 🔧 **Embedded-First Design** - Optimized for resource-constrained environments
- 📦 **Complete Proto Support** - Full Proto2/Proto3 compatibility with 100% wire format interoperability
- 🎯 **Type-Safe Code Generation** - Python-based generator creates efficient, type-safe C++ code
- ⚡ **High Performance** - Efficient parsing with minimal copies, compile-time optimizations, minimal allocations
- 🏗️ **Dual Build System** - Supports both PlatformIO (embedded) and CMake (general C++)

## Production Status

✅ **Production Ready** - All 186 tests passing (168 PlatformIO unit tests + 18 interoperability tests)  
✅ **100% Wire Format Compatibility** - Full interoperability with standard Protocol Buffers (protoc)  
✅ **Battle-Tested** - Extensively tested on embedded platforms and native systems  

## Features

### Complete Protocol Buffers Support

**Fully Supported:**
- ✅ Proto2 and Proto3 syntax
- ✅ All scalar types: `int32`, `int64`, `uint32`, `uint64`, `sint32`, `sint64`, `fixed32`, `fixed64`, `sfixed32`, `sfixed64`, `float`, `double`, `bool`, `string`, `bytes`
- ✅ Enums with custom values
- ✅ Nested messages and packages
- ✅ Repeated fields (packed and unpacked)
- ✅ Map fields with arbitrary key/value types
- ✅ Oneof fields with proper variant handling
- ✅ Optional fields (explicit and implicit)
- ✅ Default values for Proto2
- ✅ Field presence tracking

**Limitations:**
- ❌ Self-referential/recursive message types (e.g., `message Node { repeated Node children; }`)
- ❌ Groups (deprecated in protobuf)
- ❌ Extensions (rarely used, complex feature)

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+)
- Python 3.7+ (for code generation)
- Protocol Buffers compiler (`protoc`) - automatically downloaded if not present

### Quick Install

```bash
# Clone the repository
git clone https://github.com/jethome-iot/litepb.git
cd litepb

# The generator script auto-installs Python dependencies on first run
./litepb_gen --help
```

### PlatformIO Integration

Add to your `platformio.ini`:

```ini
[env:your_environment]
lib_deps = 
    https://github.com/jethome-iot/litepb.git
```

### CMake Integration

```cmake
# Add LitePB as a subdirectory
add_subdirectory(path/to/litepb/cmake)

# Link against LitePB
target_link_libraries(your_target PRIVATE LitePB::litepb)

# Use the generator (LITEPB_GENERATOR variable is automatically available)
add_custom_command(
    OUTPUT generated/message.pb.h generated/message.pb.cpp
    COMMAND ${LITEPB_GENERATOR} ${CMAKE_CURRENT_SOURCE_DIR}/message.proto 
            -o ${CMAKE_CURRENT_BINARY_DIR}/generated
    DEPENDS message.proto
)
```

## Quick Start

### Basic Serialization

**1. Define your message (person.proto):**
```protobuf
syntax = "proto3";

message Person {
    string name = 1;
    int32 age = 2;
    repeated string emails = 3;
    
    enum PhoneType {
        MOBILE = 0;
        HOME = 1;
        WORK = 2;
    }
    
    message PhoneNumber {
        string number = 1;
        PhoneType type = 2;
    }
    
    repeated PhoneNumber phones = 4;
}
```

**2. Generate C++ code:**
```bash
./litepb_gen person.proto -o generated/
```

**3. Use in your application:**
```cpp
#include "generated/person.pb.h"
#include "litepb/litepb.h"

int main() {
    // Create and populate message
    Person person;
    person.name = "Alice Smith";
    person.age = 30;
    person.emails.push_back("alice@example.com");
    person.emails.push_back("alice.smith@work.com");
    
    Person::PhoneNumber phone;
    phone.number = "555-1234";
    phone.type = Person::PhoneType::MOBILE;
    person.phones.push_back(phone);
    
    // Serialize to bytes
    litepb::BufferOutputStream output;
    if (!litepb::serialize(person, output)) {
        // Handle error
        return 1;
    }
    
    // Transmit or save the data
    send_data(output.data(), output.size());
    
    // Later, deserialize received data
    uint8_t received_data[256];
    size_t received_size = receive_data(received_data, sizeof(received_data));
    
    litepb::BufferInputStream input(received_data, received_size);
    Person decoded;
    if (!litepb::parse(decoded, input)) {
        // Handle error
        return 1;
    }
    
    // Access decoded data
    std::cout << "Name: " << decoded.name << std::endl;
    std::cout << "Age: " << decoded.age << std::endl;
    
    return 0;
}
```

## Build Systems

LitePB supports both **PlatformIO** (for embedded development) and **CMake** (for general C++ projects):

- **PlatformIO** - First-class support via `library.json` configuration
- **CMake** - Static library build with proper installation and packaging support
- **Code Generator** - Works seamlessly with both build systems

## API Documentation

### Core Serialization API

```cpp
namespace litepb {
    // Main serialization functions
    template<typename T>
    bool serialize(const T& msg, OutputStream& stream);
    
    template<typename T>
    bool parse(T& msg, InputStream& stream);
    
    template<typename T>
    size_t byte_size(const T& msg);
}
```

### Stream Interfaces

```cpp
// Dynamic buffer (recommended for most uses)
litepb::BufferOutputStream output;
litepb::BufferInputStream input(data, size);

// Fixed-size buffer (for embedded systems)
litepb::FixedOutputStream<256> output;
litepb::FixedInputStream<256> input(data, size);

// Custom streams (implement these interfaces)
class OutputStream {
    virtual bool write(const uint8_t* data, size_t size) = 0;
    virtual size_t position() const = 0;
};

class InputStream {
    virtual bool read(uint8_t* data, size_t size) = 0;
    virtual bool skip(size_t size) = 0;
    virtual size_t position() const = 0;
    virtual size_t available() const = 0;
};
```

## Examples

### PlatformIO Examples

Located in `examples/cpp/platformio/serialization/`:

- **[Basic Example](examples/cpp/platformio/serialization/basic/)** - Simple person message serialization
- **[All Types Showcase](examples/cpp/platformio/serialization/all_types/)** - Demonstrates all supported protobuf types

### CMake Examples

Located in `examples/cpp/cmake/`:

- **[Basic Serialization](examples/cpp/cmake/basic_serialization/)** - Simple person message serialization
- **[All Types](examples/cpp/cmake/all_types/)** - Comprehensive type showcase

## Testing

LitePB includes a comprehensive test suite ensuring reliability and compatibility:

```bash
# Run PlatformIO unit tests (168 tests)
pio test

# Run interoperability tests (18 tests)
./scripts/run_interop_tests.sh

# Run PlatformIO examples
./scripts/run_platformio_examples.sh

# Run CMake examples
./scripts/run_cmake_examples.sh
```

### Test Coverage

- **Core Tests** - Stream operations, varint encoding, wire format
- **Serialization Tests** - All protobuf types and edge cases
- **Interoperability Tests** - Wire format compatibility with protoc (CTest-based)
- **Platform Tests** - Native and embedded platform validation
- **Examples** - Working examples for both PlatformIO and CMake

## Code Generator

The LitePB code generator features a modular, extensible architecture:

- **Language-Agnostic Parsing** - Core parser works for all target languages
- **Pluggable Backends** - Easy to add new language support (TypeScript, Dart, etc.)
- **Full Proto Support** - Proto2/Proto3 with all standard features
- **Self-Installing** - Automatically manages Python dependencies

```bash
# Generate C++ code
./litepb_gen message.proto -o output_dir/

# With include paths
./litepb_gen message.proto -I proto/ -o output/

# With namespace prefix
./litepb_gen message.proto -o output/ --namespace-prefix=my_namespace
```

## License

LitePB is released under the MIT License. See [LICENSE](LICENSE) for details.

Copyright (c) 2025 JetHome LLC

## Support

- **Issues**: [GitHub Issues](https://github.com/jethome-iot/litepb/issues)
- **Examples**: See `examples/` directory for working code samples

## Acknowledgments

LitePB leverages the Protocol Buffers specification by Google. While fully compatible with the protobuf wire format, LitePB is an independent implementation optimized for embedded systems.
