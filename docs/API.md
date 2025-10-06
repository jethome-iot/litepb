# LitePB API Reference

## Table of Contents

- [Core Serialization API](#core-serialization-api)
- [Stream Interfaces](#stream-interfaces)
- [Wire Format Types](#wire-format-types)
- [RPC API](#rpc-api)
- [Generated Code Interface](#generated-code-interface)
- [Error Handling](#error-handling)
- [Configuration Macros](#configuration-macros)

## Core Serialization API

The core serialization API provides the main entry points for encoding and decoding Protocol Buffers messages.

### Namespace: `litepb`

All LitePB functionality is contained within the `litepb` namespace.

### Primary Functions

#### `serialize`

```cpp
template<typename T>
bool serialize(const T& msg, OutputStream& stream);
```

Serializes a Protocol Buffers message to an output stream.

**Template Parameters:**
- `T` - The message type (automatically deduced)

**Parameters:**
- `msg` - The message to serialize
- `stream` - The output stream to write to

**Returns:**
- `true` if serialization succeeded, `false` on error

**Example:**
```cpp
Person person;
person.name = "Alice";
person.age = 30;

litepb::BufferOutputStream output;
if (litepb::serialize(person, output)) {
    // Success - use output.data() and output.size()
}
```

#### `parse`

```cpp
template<typename T>
bool parse(T& msg, InputStream& stream);
```

Parses a Protocol Buffers message from an input stream.

**Template Parameters:**
- `T` - The message type (automatically deduced)

**Parameters:**
- `msg` - The message to populate (output parameter)
- `stream` - The input stream to read from

**Returns:**
- `true` if parsing succeeded, `false` on error

**Example:**
```cpp
uint8_t data[] = {...};  // Serialized message
litepb::BufferInputStream input(data, sizeof(data));

Person person;
if (litepb::parse(person, input)) {
    // Success - person is populated
}
```

#### `byte_size`

```cpp
template<typename T>
size_t byte_size(const T& msg);
```

Calculates the serialized size of a message in bytes.

**Template Parameters:**
- `T` - The message type (automatically deduced)

**Parameters:**
- `msg` - The message to calculate size for

**Returns:**
- Number of bytes required to serialize the message

**Example:**
```cpp
MyMessage msg;
// ... populate message ...

size_t size = litepb::byte_size(msg);
std::cout << "Message will require " << size << " bytes" << std::endl;
```

### Serializer Template

```cpp
template<typename T>
class Serializer {
public:
    static bool serialize(const T& msg, OutputStream& stream);
    static bool parse(T& msg, InputStream& stream);
    static size_t byte_size(const T& msg);
};
```

The `Serializer` template is specialized by generated code for each message type. Users typically don't interact with this class directly, using the free functions instead.

## Stream Interfaces

LitePB uses abstract stream interfaces for I/O operations, allowing flexibility in data sources and destinations.

### OutputStream

```cpp
class OutputStream {
public:
    virtual ~OutputStream() = default;
    virtual bool write(const uint8_t* data, size_t size) = 0;
    virtual size_t position() const = 0;
};
```

Abstract interface for writing bytes.

**Methods:**
- `write(data, size)` - Write bytes to the stream. Returns `true` on success.
- `position()` - Get the current write position in bytes.

### InputStream

```cpp
class InputStream {
public:
    virtual ~InputStream() = default;
    virtual bool read(uint8_t* data, size_t size) = 0;
    virtual bool skip(size_t size) = 0;
    virtual size_t position() const = 0;
    virtual size_t available() const = 0;
};
```

Abstract interface for reading bytes.

**Methods:**
- `read(data, size)` - Read bytes from the stream. Returns `true` if all bytes were read.
- `skip(size)` - Skip bytes without reading. Returns `true` on success.
- `position()` - Get the current read position in bytes.
- `available()` - Get the number of bytes available for reading.

### BufferOutputStream

```cpp
class BufferOutputStream : public OutputStream {
public:
    BufferOutputStream();
    bool write(const uint8_t* data, size_t size) override;
    size_t position() const override;
    
    const uint8_t* data() const;
    size_t size() const;
    void clear();
};
```

Dynamic memory buffer output stream using `std::vector`.

**Methods:**
- `data()` - Get pointer to the buffer data
- `size()` - Get the size of the buffer
- `clear()` - Clear the buffer contents

**Example:**
```cpp
litepb::BufferOutputStream output;
litepb::serialize(msg, output);
send_bytes(output.data(), output.size());
```

### BufferInputStream

```cpp
class BufferInputStream : public InputStream {
public:
    BufferInputStream(const uint8_t* data, size_t size);
    bool read(uint8_t* data, size_t size) override;
    bool skip(size_t size) override;
    size_t position() const override;
    size_t available() const override;
};
```

Memory buffer input stream (does not own the buffer).

**Constructor:**
- `BufferInputStream(data, size)` - Create stream from existing buffer

**Example:**
```cpp
uint8_t buffer[256];
size_t received = receive_bytes(buffer, sizeof(buffer));
litepb::BufferInputStream input(buffer, received);
litepb::parse(msg, input);
```

### FixedOutputStream

```cpp
template<size_t SIZE>
class FixedOutputStream : public OutputStream {
public:
    FixedOutputStream();
    bool write(const uint8_t* data, size_t size) override;
    size_t position() const override;
    
    const uint8_t* data() const;
    size_t capacity() const;
    void clear();
};
```

Fixed-size stack-allocated output stream for embedded systems.

**Template Parameters:**
- `SIZE` - Buffer size in bytes

**Methods:**
- `data()` - Get pointer to the buffer
- `capacity()` - Get the total capacity
- `clear()` - Reset write position to 0

**Example:**
```cpp
litepb::FixedOutputStream<256> output;
if (litepb::serialize(msg, output)) {
    uart_send(output.data(), output.position());
}
```

### FixedInputStream

```cpp
template<size_t SIZE>
class FixedInputStream : public InputStream {
public:
    FixedInputStream(const uint8_t* data, size_t size);
    bool read(uint8_t* data, size_t size) override;
    bool skip(size_t size) override;
    size_t position() const override;
    size_t available() const override;
};
```

Fixed-size input stream with internal buffer copy.

**Template Parameters:**
- `SIZE` - Internal buffer size

**Constructor:**
- `FixedInputStream(data, size)` - Copy data into internal buffer

**Example:**
```cpp
uint8_t received[128];
size_t size = uart_receive(received, sizeof(received));

litepb::FixedInputStream<256> input(received, size);
litepb::parse(msg, input);
```

## Wire Format Types

### WireType Enum

```cpp
enum WireType {
    WIRE_TYPE_VARINT           = 0,
    WIRE_TYPE_FIXED64          = 1,
    WIRE_TYPE_LENGTH_DELIMITED = 2,
    WIRE_TYPE_START_GROUP      = 3,  // Deprecated
    WIRE_TYPE_END_GROUP        = 4,  // Deprecated
    WIRE_TYPE_FIXED32          = 5
};
```

Protocol Buffers wire type identifiers.

### ProtoWriter

```cpp
class ProtoWriter {
public:
    explicit ProtoWriter(OutputStream& stream);
    
    bool write_varint(uint64_t value);
    bool write_fixed32(uint32_t value);
    bool write_fixed64(uint64_t value);
    bool write_sfixed32(int32_t value);
    bool write_sfixed64(int64_t value);
    bool write_float(float value);
    bool write_double(double value);
    bool write_bytes(const uint8_t* data, size_t size);
    bool write_string(const std::string& str);
    bool write_tag(uint32_t field_number, WireType type);
    bool write_sint32(int32_t value);
    bool write_sint64(int64_t value);
    
    static size_t varint_size(uint64_t value);
    static size_t fixed32_size();  // Always 4
    static size_t fixed64_size();  // Always 8
    static size_t sint32_size(int32_t value);
    static size_t sint64_size(int64_t value);
};
```

Low-level Protocol Buffers wire format writer (used internally by generated code).

### ProtoReader

```cpp
class ProtoReader {
public:
    explicit ProtoReader(InputStream& stream);
    
    bool read_varint(uint64_t& value);
    bool read_fixed32(uint32_t& value);
    bool read_fixed64(uint64_t& value);
    bool read_sfixed32(int32_t& value);
    bool read_sfixed64(int64_t& value);
    bool read_float(float& value);
    bool read_double(double& value);
    bool read_bytes(std::vector<uint8_t>& data);
    bool read_string(std::string& str);
    bool read_tag(uint32_t& field_number, WireType& type);
    bool skip_field(WireType type);
    bool read_sint32(int32_t& value);
    bool read_sint64(int64_t& value);
    
    size_t position() const;
};
```

Low-level Protocol Buffers wire format reader (used internally by generated code).

## RPC API

The RPC API is available when `LITEPB_WITH_RPC` is defined.

### RpcChannel

```cpp
class RpcChannel {
public:
    RpcChannel(Transport& transport, uint64_t local_address, 
               uint32_t default_timeout_ms = 5000);
    
    void process();
    
    template<typename Req, typename Resp>
    bool call_internal(uint16_t service_id, uint32_t method_id, 
                      const Req& request,
                      std::function<void(const Result<Resp>&)> callback,
                      uint32_t timeout_ms = 0, 
                      uint64_t dst_addr = 0);
    
    template<typename Req>
    bool send_event(uint16_t service_id, uint32_t method_id, 
                   const Req& request, 
                   uint64_t dst_addr = 0);
    
    template<typename Req, typename Resp>
    void on_internal(uint16_t service_id, uint32_t method_id,
                    std::function<Result<Resp>(uint64_t, const Req&)> handler);
    
    template<typename Req>
    void on_event(uint16_t service_id, uint32_t method_id,
                 std::function<void(uint64_t, const Req&)> handler);
};
```

Central RPC channel for managing remote procedure calls.

**Constructor Parameters:**
- `transport` - Transport layer implementation
- `local_address` - This node's address
- `default_timeout_ms` - Default timeout for RPC calls

**Key Methods:**
- `process()` - Process incoming messages (call in event loop)
- `call_internal()` - Make an RPC call (used by generated code)
- `send_event()` - Send a fire-and-forget event
- `on_internal()` - Register an RPC handler (used by generated code)
- `on_event()` - Register an event handler

### Transport Interface

```cpp
class Transport {
public:
    virtual ~Transport() = default;
    
    virtual bool send(const uint8_t* data, size_t len,
                     uint64_t src_addr, uint64_t dst_addr) = 0;
    
    virtual size_t recv(uint8_t* buffer, size_t max_len,
                       uint64_t& src_addr, uint64_t& dst_addr) = 0;
    
    virtual bool available() = 0;
};
```

Abstract transport layer interface.

**Methods:**
- `send()` - Send data with addressing
- `recv()` - Receive data with addressing
- `available()` - Check if data is available

### StreamTransport

```cpp
class StreamTransport : public Transport {
    // Marker class for stream-based transports
};
```

Base class for stream-based transports (UART, TCP, etc.).

### PacketTransport

```cpp
class PacketTransport : public Transport {
public:
    virtual size_t recv_packet(uint8_t* buffer, size_t max_len) = 0;
    virtual bool send_packet(const uint8_t* data, size_t len) = 0;
};
```

Base class for packet-based transports (UDP, LoRa, etc.).

### RPC Addressing Constants

```cpp
namespace litepb {
    constexpr uint64_t RPC_ADDRESS_WILDCARD  = 0x0000000000000000ULL;
    constexpr uint64_t RPC_ADDRESS_BROADCAST = 0xFFFFFFFFFFFFFFFFULL;
}
```

Special addresses for RPC operations:
- `RPC_ADDRESS_WILDCARD` - Accept from any source
- `RPC_ADDRESS_BROADCAST` - Send to all nodes

## Generated Code Interface

The code generator creates specific patterns for each message and service.

### Generated Message Structure

For a proto message:
```protobuf
message Person {
    string name = 1;
    int32 age = 2;
}
```

Generated C++ structure:
```cpp
struct Person {
    std::string name;
    int32_t age;
    
    // Default constructor
    Person();
    
    // Equality operators
    bool operator==(const Person& other) const;
    bool operator!=(const Person& other) const;
};
```

### Generated Serializer Specialization

```cpp
template<>
class Serializer<Person> {
public:
    static bool serialize(const Person& msg, OutputStream& stream);
    static bool parse(Person& msg, InputStream& stream);
    static size_t byte_size(const Person& msg);
};
```

### Generated Service Interface

For a proto service:
```protobuf
service MyService {
    rpc GetData(Request) returns (Response);
}
```

Generated server base class:
```cpp
class MyServiceServer {
public:
    virtual ~MyServiceServer() = default;
    
    virtual litepb::Result<Response> GetData(
        uint64_t sender_address,
        const Request& request) = 0;
};
```

Generated client proxy:
```cpp
class MyServiceClient {
public:
    MyServiceClient(litepb::RpcChannel& channel);
    
    void GetData(const Request& request,
                 std::function<void(const litepb::Result<Response>&)> callback,
                 uint32_t timeout_ms = 0,
                 uint64_t dst_addr = 0);
};
```

Generated registration function:
```cpp
void register_my_service(litepb::RpcChannel& channel, 
                         MyServiceServer& service);
```

## Error Handling

### RpcError

```cpp
struct RpcError {
    enum Code {
        OK                = 0,
        TIMEOUT           = 1,
        PARSE_ERROR       = 2,
        TRANSPORT_ERROR   = 3,
        HANDLER_NOT_FOUND = 4,
        UNKNOWN           = 5
    };
    
    Code code;
    
    bool ok() const { return code == OK; }
};
```

RPC protocol-level error codes.

### Result Template

```cpp
template<typename T>
struct Result {
    T value;
    RpcError error;
    
    bool ok() const { return error.ok(); }
};
```

Combines a value with error status for RPC returns.

### Error Conversion

```cpp
inline const char* rpc_error_to_string(RpcError::Code code);
```

Convert error codes to human-readable strings.

**Example:**
```cpp
litepb::Result<Response> result = ...;
if (!result.ok()) {
    std::cerr << "Error: " << rpc_error_to_string(result.error.code) << std::endl;
}
```

## Configuration Macros

### Core Configuration

```cpp
// Define before including LitePB headers

// Enable RPC support (disabled by default)
#define LITEPB_WITH_RPC

// Set initial RPC buffer size (default: 1024)
#define LITEPB_RPC_INITIAL_BUFFER_SIZE 2048
```

### Platform-Specific Configuration

```cpp
// For ESP-IDF
#ifdef ESP_PLATFORM
    #define LITEPB_ESP32_COMPAT
#endif

// For Linux native builds
#ifdef __linux__
    #define LITEPB_LINUX_COMPAT
#endif
```

### Debug Configuration

```cpp
// Enable debug output (development only)
#define LITEPB_DEBUG

// Enable RPC trace logging
#define LITEPB_RPC_TRACE

// Enable serialization verification
#define LITEPB_VERIFY_SERIALIZATION
```

## Usage Examples

### Basic Message Serialization

```cpp
#include "generated/my_messages.pb.h"
#include "litepb/litepb.h"

void example_serialization() {
    // Create and populate message
    SensorData data;
    data.sensor_id = 42;
    data.temperature = 23.5;
    data.timestamp = get_timestamp();
    
    // Serialize to buffer
    litepb::BufferOutputStream output;
    if (!litepb::serialize(data, output)) {
        handle_error("Serialization failed");
        return;
    }
    
    // Use serialized data
    transmit(output.data(), output.size());
}
```

### Fixed-Size Buffer Usage

```cpp
void embedded_example() {
    // Stack-allocated buffer
    litepb::FixedOutputStream<512> output;
    
    MyMessage msg;
    msg.field1 = 100;
    msg.field2 = "test";
    
    if (!litepb::serialize(msg, output)) {
        // Handle buffer overflow
        return;
    }
    
    // Send exactly the bytes used
    uart_write(output.data(), output.position());
}
```

### RPC Client Usage

```cpp
void rpc_client_example(litepb::RpcChannel& channel) {
    // Create client stub
    MyServiceClient client(channel);
    
    // Prepare request
    GetDataRequest request;
    request.id = 123;
    
    // Make async RPC call
    client.GetData(
        request,
        [](const litepb::Result<GetDataResponse>& result) {
            if (result.ok()) {
                process_data(result.value);
            } else {
                handle_rpc_error(result.error);
            }
        },
        5000,      // 5 second timeout
        SERVER_ID  // Target server address
    );
}
```

### RPC Server Implementation

```cpp
class MyServiceImpl : public MyServiceServer {
public:
    litepb::Result<GetDataResponse> GetData(
        uint64_t sender,
        const GetDataRequest& request) override {
        
        litepb::Result<GetDataResponse> result;
        
        // Process request
        if (request.id < 0) {
            result.error.code = litepb::RpcError::OK;
            result.value.error = "Invalid ID";
            return result;
        }
        
        // Populate response
        result.value.data = fetch_data(request.id);
        result.value.timestamp = get_time();
        result.error.code = litepb::RpcError::OK;
        
        return result;
    }
};

void setup_server(Transport& transport) {
    litepb::RpcChannel channel(transport, MY_ADDRESS);
    MyServiceImpl service;
    register_my_service(channel, service);
    
    while (running) {
        channel.process();
        delay(1);
    }
}
```

## Thread Safety

LitePB is designed for single-threaded use:

- **Not Thread-Safe**: Core serialization functions are not thread-safe
- **RpcChannel**: Must be used from a single thread
- **Message Objects**: Should not be shared between threads without synchronization

For multi-threaded applications:
- Use separate message instances per thread
- Protect shared RpcChannel with mutex
- Consider thread-local storage for frequent operations

## Performance Tips

1. **Pre-calculate Sizes**: Use `byte_size()` to allocate exact buffer sizes
2. **Reuse Objects**: Clear and reuse messages/streams instead of creating new ones
3. **Batch Operations**: Process multiple messages per event loop iteration
4. **Optimize Field Order**: Place frequently accessed fields first
5. **Use Fixed-Width Types**: When size is predictable, use fixed32/fixed64
6. **Pack Repeated Fields**: Ensure repeated fields use packed encoding

## Compatibility

LitePB maintains 100% wire format compatibility with standard Protocol Buffers:

- **Proto2**: Full support including required fields and default values
- **Proto3**: Full support including implicit presence
- **Interoperability**: Can exchange messages with protoc-generated code
- **Version**: Compatible with Protocol Buffers 3.x wire format

## Limitations

- No support for self-referential/recursive message types
- No support for deprecated groups
- No support for extensions
- No support for reflection/introspection
- No support for JSON encoding/decoding
- No support for text format