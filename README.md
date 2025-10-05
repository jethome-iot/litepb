# LitePB - Lightweight Protocol Buffers for C++

[![CI](https://github.com/jethome-iot/litepb/actions/workflows/ci.yml/badge.svg)](https://github.com/jethome-iot/litepb/actions)

## Overview

LitePB is a lightweight Protocol Buffers implementation for C++ designed for embedded systems (ESP32, STM32, ARM Cortex-M) and native platforms (Linux, macOS, Windows). It provides:

- **Python-based code generator** (`./litepb_gen`) - Converts .proto files to minimal C++ code
- **Zero-dependency C++17 runtime** - Serialization/deserialization library
- **Async RPC layer** - Complete peer-to-peer RPC implementation

## Status

✅ **Production Ready** - 149/149 tests passing (139 PlatformIO + 10 interop tests)
✅ **100% Protocol Buffers Compatibility** - Full wire format interoperability with protoc

## Features

### Protocol Buffers Support

**Syntax:**
- Proto2 and Proto3
- All scalar types (int32, uint64, float, double, bool, string, bytes, etc.)
- Enums, nested messages, packages
- Repeated fields, maps, oneofs
- Optional fields (explicit and implicit)

**Not Supported:**
- Self-referential/recursive types (e.g., `message Node { repeated Node children; }`)

### RPC Layer

- **Async & Non-blocking** - Callback-based API, single-threaded event loop
- **Peer-to-Peer** - Fully bidirectional, any node can initiate requests
- **Transport Agnostic** - UART, TCP, UDP, LoRa, or custom transports
- **Type-Safe** - Generated client/server stubs from .proto definitions
- **Fire-and-Forget Events** - One-way messages without response tracking
- **Zero Dependencies** - C++17 standard library only

See [RPC Implementation Guide](docs/rpc.md) for complete documentation.

## Quick Start

### Installation

```bash
# Clone repository
git clone https://github.com/jethome-iot/litepb.git
cd litepb

# Generate C++ from .proto files
./litepb_gen proto/simple.proto -o generated/

# With imports
./litepb_gen proto/myfile.proto -I proto/imports -o generated/
```

### Basic Serialization

```cpp
#include "generated/simple.pb.h"
#include "litepb/litepb.h"

// Create message
MyMessage msg;
msg.name = "LitePB";
msg.value = 42;

// Serialize
litepb::BufferOutputStream output;
litepb::serialize(msg, output);

// Deserialize
litepb::BufferInputStream input(output.data(), output.size());
MyMessage decoded;
litepb::parse(decoded, input);
```

### RPC Example

**Define Service:**
```protobuf
syntax = "proto3";
package example;
import "litepb/rpc_options.proto";

message Request {
    int32 value = 1;
}

message Response {
    int32 result = 1;
}

service Calculator {
    rpc Add(Request) returns (Response) {
        option (litepb.rpc.method_id) = 1;
    }
}
```

**Server (using generated stubs):**
```cpp
#include "calculator.pb.h"
#include "litepb/rpc/channel.h"
#include "your_transport.h"

// Implement server interface
class MyCalculator : public CalculatorServer {
public:
    litepb::Result<Response> Add(const Request& req) override {
        litepb::Result<Response> result;
        result.value.result = req.value + 10;
        result.error.code = litepb::RpcError::OK;
        return result;
    }
};

YourTransport transport;
litepb::RpcChannel channel(transport, 1, 5000);

// Register service
MyCalculator calc;
register_calculator(channel, calc);

// Event loop
while (true) {
    channel.process();
}
```

**Client (using generated stubs):**
```cpp
YourTransport transport;
litepb::RpcChannel channel(transport, 2, 5000);

// Create client stub
CalculatorClient client(channel);

Request req;
req.value = 32;

client.Add(req, [](const litepb::Result<Response>& result) {
    if (result.ok()) {
        std::cout << "Result: " << result.value.result << std::endl;
    }
});

// Event loop
while (true) {
    channel.process();
}
```

## Project Structure

```
litepb/
├── litepb_gen              # Entry point (bash wrapper, auto-installs deps)
├── generator/              # Python code generator
│   ├── cpp_generator.py    # Main generator (Jinja2 templates)
│   ├── proto_parser.py     # Invokes protoc to parse .proto
│   └── type_mapper.py      # Maps protobuf → C++ types
│
├── include/litepb/         # Runtime library headers
│   ├── core/
│   │   ├── streams.h       # Stream interfaces
│   │   ├── proto_reader.h  # Wire format reading
│   │   └── proto_writer.h  # Wire format writing
│   ├── rpc/
│   │   ├── channel.h       # RPC core engine
│   │   ├── transport.h     # Transport abstraction
│   │   ├── error.h         # Error types
│   │   ├── framing.h       # Message framing
│   │   └── addressing.h    # Address constants
│   └── litepb.h            # Main include
│
├── src/litepb/             # Runtime implementation
│   ├── core/
│   │   ├── proto_reader.cpp
│   │   └── proto_writer.cpp
│   └── rpc/
│       ├── channel.cpp
│       └── framing.cpp
│
├── proto/                  # Example .proto files
├── examples/               # Example applications
│   └── rpc/litepb_rpc/ # Complete RPC example
├── tests/                  # All tests
│   ├── platformio/        # PlatformIO tests (Unity framework)
│   └── interop/           # Standalone interop tests (protoc compatibility)
└── docs/                   # Documentation
    └── rpc.md              # RPC implementation guide
```

## Architecture

### Code Generator Pipeline

1. **Proto Parser** - Invokes `protoc` to parse .proto files
2. **Type Mapper** - Maps protobuf types to C++ types
3. **C++ Generator** - Uses Jinja2 templates to generate code
4. **Output** - `.pb.h` (structs) and `.pb.cpp` (serializers)

### Runtime Library

4-layer architecture:

1. **Stream Layer** - `InputStream`/`OutputStream` abstractions
2. **Protocol Layer** - Varint, fixed, length-delimited wire format
3. **Data Layer** - Plain C++ structs with `std::optional`, `std::variant`, `std::unordered_map`
4. **Serialization Layer** - Template specializations `litepb::Serializer<T>`

### Design Principles

- Zero external dependencies
- No exceptions, no RTTI
- No hidden allocations
- Zero-copy parsing where possible
- Compile-time template-based serialization

## Testing

```bash
# Run all PlatformIO tests
pio test

# Run specific test
pio test -e test_core

# Run interoperability tests (protoc compatibility)
./tests/interop/run_tests.sh

# Verbose output
pio test -vvv

# Filter by pattern
pio test -f "*test_enums*"
```

**Test Suite:**
- Comprehensive test coverage across all features
- PlatformIO tests for core, RPC, protocol features, and examples
- Interoperability tests validating wire format compatibility with protoc C++
- Unity test framework for embedded platform testing
- Coverage reports generated in `tmp/coverage/` directory

## Examples

### Basic Serialization

Simple "hello world" example showing message encoding/decoding in `examples/serialization/basic/`:

```bash
cd examples/serialization/basic
pio run -t exec
```

Shows: Creating messages, serialization to bytes, deserialization, and data verification.

### All Types Showcase

Comprehensive demonstration of ALL Protocol Buffer types in `examples/serialization/all_types/`:

```bash
cd examples/serialization/all_types
pio run -t exec
```

Demonstrates: All scalar types, fixed-width types, floats, enums, nested messages, repeated fields, maps, and oneofs.

### Sensor RPC Example

Complete peer-to-peer RPC implementation in `examples/rpc/litepb_rpc/`:

- Bidirectional communication
- Fire-and-forget events
- Multiple transport examples (TCP, UART, UDP)
- Comprehensive tests

```bash
cd examples/rpc/litepb_rpc

# Run example
pio run -e litepb_rpc -t exec

# Run tests (from main project directory)
cd ../../..
pio test -e test_sensor_loopback
pio test -e test_sensor_service
pio test -e test_sensor_integration
```

See `examples/rpc/litepb_rpc/README.md` for detailed documentation.

## Platform Support

**Embedded:**
- ESP32
- STM32
- Arduino
- ARM Cortex-M

**Native:**
- Linux
- macOS
- Windows

**Build Systems:**
- PlatformIO (configured)
- CMake (straightforward integration)
- Make (straightforward integration)

**C++ Standard:**
- C++17 minimum
- C++20 compatible

## Dependencies

**Build-time:**
- `protoc` (Protocol Buffers compiler)
- Python 3.7+ with auto-installed packages:
  - `protobuf==5.28.0`
  - `Jinja2==3.1.4`

**Runtime:**
- C++17 standard library only (no external dependencies)

## Documentation

- [RPC Implementation Guide](docs/rpc.md) - Complete RPC documentation
- [Sensor Example README](examples/rpc/litepb_rpc/README.md) - Example walkthrough

## Protobuf Compatibility

LitePB achieves **100% wire format compatibility** with standard Protocol Buffers (protoc):

✅ **All scalar types**
   - int32, int64, uint32, uint64, sint32, sint64, fixed32, fixed64, sfixed32, sfixed64, float, double, bool, string, bytes

✅ **Zigzag encoding**
   - Correct sint32/sint64 encoding for negative numbers

✅ **Packed repeated fields**
   - Proto3 default packed encoding

✅ **Map fields**
   - Semantic interoperability (wire format ordering is undefined per spec)

✅ **Nested messages**
   - Length-delimited encoding

✅ **Oneof fields**
   - Union types with correct wire format

✅ **Enum types**
   - Varint encoding

Tested with bidirectional encode/decode against protoc C++ implementation. See `tests/interop/` for comprehensive interoperability tests.

## Key Implementation Details

### Proto3 Field Presence

- **Implicit fields** (no `optional`) → Direct types: `int32_t`, `std::string`
- **Explicit optional** (`optional int32`) → `std::optional<int32_t>`
- **Message/enum fields** → Direct types in proto3

### Maps

- Encoded as repeated message fields with key/value pairs
- Supports all scalar key types and message/enum values

### Oneofs

- Encoded as `std::variant` with type deduplication
- Multiple protobuf types mapping to same C++ type handled correctly

### Nested Messages

- Recursive serializer generation
- Forward declarations for sibling types
- Scoped blocks to avoid name collisions

## Contributing

Contributions welcome! Please ensure:

1. All tests pass: `pio test`
2. Code follows existing style
3. Add tests for new features
4. Update documentation as needed

## License

[Insert License Information]

## Acknowledgments

Built for embedded systems and IoT applications where standard Protocol Buffers libraries are too heavyweight.
