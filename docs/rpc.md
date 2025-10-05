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

### Frame Structure

All RPC messages follow a consistent framing structure with addressing and service multiplexing:

**Stream Transports** (UART, TCP):
```
[src_addr:8bytes][dst_addr:8bytes][msg_id:varint][service_id:varint][method_id:varint][payload_len:varint][payload]
```

**Packet Transports** (UDP, LoRa):
```
[src_addr:8bytes][dst_addr:8bytes][msg_id:varint][service_id:varint][method_id:varint][payload]
```

### Field Descriptions

- **src_addr** (8 bytes): Source node address, little-endian
- **dst_addr** (8 bytes): Destination node address, little-endian
- **msg_id** (varint): 16-bit message ID for correlating requests/responses (0 = fire-and-forget event)
- **service_id** (varint): 16-bit service identifier for multiplexing
- **method_id** (varint): 32-bit method identifier within the service
- **payload_len** (varint): Length of payload (stream transports only)
- **payload**: Protobuf-encoded request or response data

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
    bool send(const uint8_t* data, size_t len) override {
        // Send bytes to hardware
        return true;
    }
    
    size_t recv(uint8_t* buffer, size_t max_len) override {
        // Read available bytes
        return bytes_read;
    }
    
    bool available() override {
        // Check if data available
        return has_data;
    }
};
```

## Error Handling

The RPC system uses a two-layer error model:

1. **RPC Layer Errors** - Transport and protocol issues
   - `OK` (0): Successful operation
   - `TIMEOUT` (1): Request exceeded deadline
   - `PARSE_ERROR` (2): Failed to parse message
   - `TRANSPORT_ERROR` (3): Transport layer failure
   - `HANDLER_NOT_FOUND` (4): No handler registered for method
   - `CUSTOM_ERROR` (100+): Application-defined errors

2. **Application Errors** - Business logic errors
   - Custom error codes defined within protobuf response messages

```cpp
// RPC layer errors are handled in the callback
client.Add(req, [](const litepb::Result<Response>& result) {
    if (result.error.code != litepb::RpcError::OK) {
        // Handle RPC layer error (timeout, transport, etc.)
        return;
    }
    // Process successful response
    // Application errors would be in the response message itself
});
```

Note: Error codes are not part of the wire protocol framing. RPC layer errors are detected by the framework (timeouts, parsing failures), while application errors are encoded within the protobuf response payload.

## Examples

See `examples/rpc/litepb_rpc/` for complete working example with:
- Bidirectional sensor communication
- Multiple transport implementations
- Comprehensive test suite (38 tests)