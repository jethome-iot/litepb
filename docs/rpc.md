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

### Message Format

**Stream Transports** (UART, TCP):
```
[msg_id:varint][method_id:varint][payload_len:varint][protobuf_data]
```

**Packet Transports** (UDP, LoRa):
```
[msg_id:varint][method_id:varint][protobuf_data]
```

**Response Format**:
```
[msg_id:varint][method_id:varint][error_code:varint][app_code:varint][payload_len:varint][protobuf_data]
```

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

Two-layer error system:

1. **RPC Layer Errors** - Transport and protocol issues
   - `OK`, `TIMEOUT`, `PARSE_ERROR`, `HANDLER_NOT_FOUND`

2. **Application Errors** - Business logic errors
   - Custom error codes defined by application

```cpp
if (result.error.code != litepb::RpcError::OK) {
    // Handle RPC error
} else if (result.error.app_code != 0) {
    // Handle application error
}
```

## Examples

See `examples/rpc/litepb_rpc/` for complete working example with:
- Bidirectional sensor communication
- Multiple transport implementations
- Comprehensive test suite (38 tests)