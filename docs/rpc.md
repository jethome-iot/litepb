# LitePB RPC Guide

## Overview

LitePB RPC is a lightweight, asynchronous RPC layer built for embedded systems and native platforms. It provides type-safe remote procedure calls with minimal overhead and zero external dependencies.

**Status:** ✅ Production Ready - 149/149 tests passing

### Key Features

- **Async & Non-blocking** - Callback-based API with single-threaded event loop
- **Peer-to-Peer** - Fully bidirectional, any node can initiate requests
- **Transport Agnostic** - Works with UART, TCP, UDP, LoRa, or custom transports
- **Type Safe** - Generated client/server stubs from .proto definitions
- **Fire-and-Forget Events** - One-way messages without response tracking
- **Zero Dependencies** - C++17 standard library only

## Architecture

### Layered Design

```
┌─────────────────────────────────┐
│  .proto Service Definitions     │
└─────────────────────────────────┘
              ↓
┌─────────────────────────────────┐
│   Generated C++ Stubs           │
└─────────────────────────────────┘
              ↓
┌─────────────────────────────────┐
│   RpcChannel                    │
└─────────────────────────────────┘
              ↓
┌─────────────────────────────────┐
│   Framing Layer                 │
└─────────────────────────────────┘
              ↓
┌─────────────────────────────────┐
│   Transport Layer               │
└─────────────────────────────────┘
```

## Wire Protocol

### Protocol Architecture

The RPC system uses a clean separation between transport and protocol layers:

1. **Transport Layer** handles addressing and routing:
   - Source address (src_addr: 8 bytes)
   - Destination address (dst_addr: 8 bytes)
   - Message ID (msg_id: 16-bit) for request/response correlation

2. **RPC Protocol Layer** uses protobuf serialization:
   - Protocol version for backward compatibility
   - Service and method dispatch
   - Explicit message types (REQUEST, RESPONSE, EVENT)
   - Clean error handling separation

### RPC Message Structure

All RPC messages are serialized using the `RpcMessage` protobuf:

```protobuf
message RpcMessage {
    uint32 version = 1;           // Protocol version (currently 1)
    uint32 service_id = 2;        // Service identifier
    uint32 method_id = 3;         // Method within the service
    
    enum MessageType {
        REQUEST = 0;              // Expects response
        RESPONSE = 1;             // Reply to request
        EVENT = 2;                // Fire-and-forget
    }
    MessageType message_type = 4;
    
    oneof payload {
        bytes request_data = 5;    // For REQUEST
        RpcResponse response = 6;  // For RESPONSE
        bytes event_data = 7;      // For EVENT
    }
}
```

### Transport Framing

The transport layer adds addressing and framing around the RPC message:

**Stream Transports** (UART, TCP):
```
[Frame Length][src_addr][dst_addr][msg_id][RpcMessage protobuf]
```

**Packet Transports** (UDP, LoRa):
```
[src_addr][dst_addr][msg_id][RpcMessage protobuf]
```

### Addressing System

The RPC system supports flexible addressing modes:

- **Direct Addressing**: Messages sent to specific node addresses (1-65535)
- **Broadcast** (`RPC_ADDRESS_BROADCAST` = 0xFFFFFFFFFFFFFFFF): Messages sent to all nodes
- **Wildcard** (`RPC_ADDRESS_WILDCARD` = 0): Accept messages from any source

### Response Handling

Responses use the same frame structure as requests. The correlation happens via:
- Matching `msg_id` between request and response
- Source/destination addresses are swapped in the response
- Error handling is done at the application level within the protobuf payload

## Quick Start

### 1. Define Service

```protobuf
syntax = "proto3";
package example;

message Request {
    int32 value = 1;
}

message Response {
    int32 result = 1;
}

service Calculator {
    rpc Add(Request) returns (Response);
}
```

### 2. Implement Server

```cpp
class MyCalculator : public CalculatorServer {
public:
    litepb::Result<Response> Add(const Request& req) override {
        litepb::Result<Response> result;
        result.value.result = req.value + 10;
        result.error.code = litepb::RpcError::OK;
        return result;
    }
};

// Setup channel and register service
YourTransport transport;
litepb::RpcChannel channel(transport, 1, 5000);
MyCalculator calc;
register_calculator(channel, calc);
```

### 3. Use Client

```cpp
CalculatorClient client(channel);
Request req;
req.value = 32;

client.Add(req, [](const litepb::Result<Response>& result) {
    if (result.ok()) {
        std::cout << "Result: " << result.value.result << std::endl;
    }
});
```

## Transport Implementation

```cpp
class YourTransport : public Transport {
public:
    bool send(const uint8_t* data, size_t len, 
              uint64_t src_addr, uint64_t dst_addr, uint16_t msg_id) override {
        // Transport handles addressing and sends bytes to hardware
        return true;
    }
    
    size_t recv(uint8_t* buffer, size_t max_len,
                uint64_t& src_addr, uint64_t& dst_addr, uint16_t& msg_id) override {
        // Read available bytes and extract addressing info
        return bytes_read;
    }
    
    bool available() override {
        // Check if data available
        return has_data;
    }
};
```

## Error Handling

The RPC system uses a clean separation between RPC-layer and application-layer errors:

### RPC Layer Errors

These are protocol and transport issues detected by the framework:
- `OK` (0): Successful operation
- `TIMEOUT` (1): Request exceeded deadline
- `PARSE_ERROR` (2): Failed to parse message
- `TRANSPORT_ERROR` (3): Transport layer failure
- `HANDLER_NOT_FOUND` (4): No handler registered for method

### Application Layer Errors

Application-specific errors are handled within the response payload:
- Define custom error codes in your protobuf messages
- Return `RpcError::OK` from the RPC layer
- Include error details in the response data

```cpp
// Example: Application error handling
message SensorResponse {
    enum Status {
        OK = 0;
        SENSOR_NOT_FOUND = 1;
        OUT_OF_RANGE = 2;
    }
    Status status = 1;
    float value = 2;
}

// Server returns application error
litepb::Result<SensorResponse> GetReading(const Request& req) {
    litepb::Result<SensorResponse> result;
    result.error.code = litepb::RpcError::OK;  // RPC succeeded
    result.value.status = SensorResponse::SENSOR_NOT_FOUND;  // App error
    return result;
}

// Client checks both layers
client.GetReading(req, [](const litepb::Result<SensorResponse>& result) {
    if (result.error.code != litepb::RpcError::OK) {
        // Handle RPC layer error (timeout, transport, etc.)
        return;
    }
    if (result.value.status != SensorResponse::OK) {
        // Handle application error
        return;
    }
    // Process successful response
});
```

## Examples

See `examples/rpc/litepb_rpc/` for complete working example with:
- Bidirectional sensor communication
- Multiple transport implementations
- Comprehensive test suite (38 tests)

## Future Enhancements (TODO)

### Metadata and Context Propagation
- Add optional metadata map for cross-cutting concerns (tracing, auth tokens)
- Implement deadline propagation to prevent cascading timeouts
- Support for distributed tracing headers

### Streaming Support  
- Add STREAM_REQUEST, STREAM_DATA, STREAM_END message types
- Enable server-side streaming for large data transfers
- Support bidirectional streaming for real-time communication

### Connection Management
- Implement PING/PONG messages for keepalive
- Add connection state tracking and automatic reconnection
- Support for connection multiplexing

### Performance Optimizations
- Batch multiple requests in single transport frame
- Implement request/response compression
- Add caching layer for idempotent requests

### Security Features
- Built-in authentication/authorization framework
- Message encryption at RPC layer
- Request signing and verification