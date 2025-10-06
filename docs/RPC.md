# LitePB RPC Implementation Guide

## Table of Contents
- [Overview](#overview)
- [Architecture](#architecture)
- [Protocol Specification](#protocol-specification)
- [Service Definition](#service-definition)
- [Implementation Guide](#implementation-guide)
- [Transport Layer](#transport-layer)
- [Error Handling](#error-handling)
- [Advanced Features](#advanced-features)
- [Best Practices](#best-practices)
- [Performance Optimization](#performance-optimization)

## Overview

LitePB RPC is a production-ready, lightweight remote procedure call framework designed specifically for embedded systems and resource-constrained environments. It provides enterprise-grade RPC capabilities with minimal overhead, making it ideal for IoT devices, sensor networks, and distributed embedded systems.

### Key Characteristics

- **Zero External Dependencies** - Pure C++17 implementation using only standard library
- **Async & Non-blocking** - Event-driven architecture prevents blocking on I/O operations
- **Fully Bidirectional** - Every node can act as both client and server simultaneously
- **Transport Agnostic** - Works with any byte-oriented transport layer
- **Type-Safe** - Compile-time type checking through generated stubs
- **Memory Efficient** - Predictable memory usage, no hidden allocations

### Design Philosophy

The RPC layer is built on several core principles:

1. **Simplicity** - Clean, understandable API with minimal cognitive overhead
2. **Efficiency** - Optimized for embedded systems with limited resources
3. **Reliability** - Comprehensive error handling and timeout management
4. **Flexibility** - Adaptable to various network topologies and transports
5. **Determinism** - Predictable behavior suitable for real-time systems

## Architecture

### System Layers

The RPC system follows a clean layered architecture:

```
┌─────────────────────────────────────────┐
│     Application Layer                    │
│  (Your service implementation)           │
├─────────────────────────────────────────┤
│     Generated Code Layer                 │
│  (Client/Server stubs from .proto)       │
├─────────────────────────────────────────┤
│     RPC Core Layer                       │
│  (RpcChannel, message routing)           │
├─────────────────────────────────────────┤
│     Protocol Layer                       │
│  (RpcMessage serialization)              │
├─────────────────────────────────────────┤
│     Framing Layer                        │
│  (Message boundaries, correlation)       │
├─────────────────────────────────────────┤
│     Transport Layer                      │
│  (UART, TCP, UDP, SPI, etc.)            │
└─────────────────────────────────────────┘
```

### Core Components

#### RpcChannel
The central hub for RPC operations. Manages:
- Message routing and dispatch
- Request/response correlation
- Timeout tracking
- Handler registration

#### Transport
Abstract interface for byte transmission. Implementations handle:
- Physical layer communication
- Address-based routing
- Connection management (if applicable)

#### Framing
Handles message delimitation for stream transports:
- Length-prefixed framing for streams (UART, TCP)
- Direct transmission for packet transports (UDP, LoRa)

#### Generated Stubs
Type-safe interfaces generated from .proto files:
- Client proxy classes for making RPC calls
- Server base classes for implementing services
- Automatic serialization/deserialization

## Protocol Specification

### Wire Format

The RPC protocol uses a clean separation of concerns:

#### Transport Frame (Stream Transports)
```
┌──────────────┬──────────────┬──────────────┬──────────────┬────────────────┐
│ Frame Length │  Src Address │  Dst Address │  Message ID  │  RPC Payload   │
│   (varint)   │   (8 bytes)  │   (8 bytes)  │  (2 bytes)   │   (variable)   │
└──────────────┴──────────────┴──────────────┴──────────────┴────────────────┘
```

#### Transport Frame (Packet Transports)
```
┌──────────────┬──────────────┬──────────────┬────────────────┐
│  Src Address │  Dst Address │  Message ID  │  RPC Payload   │
│   (8 bytes)  │   (8 bytes)  │  (2 bytes)   │   (variable)   │
└──────────────┴──────────────┴──────────────┴────────────────┘
```

### RPC Message Structure

All RPC payloads are encoded as Protocol Buffers messages:

```protobuf
message RpcMessage {
    uint32 version = 1;          // Protocol version (currently 1)
    uint32 service_id = 2;       // Service identifier
    uint32 method_id = 3;        // Method within service
    
    enum MessageType {
        REQUEST = 0;              // Expects response
        RESPONSE = 1;             // Reply to request
        EVENT = 2;                // Fire-and-forget
    }
    MessageType message_type = 4;
    
    oneof payload {
        bytes request_data = 5;   // Serialized request message
        RpcResponse response = 6; // Response with error info
        bytes event_data = 7;     // Serialized event message
    }
}

message RpcResponse {
    enum RpcErrorCode {
        OK = 0;
        TIMEOUT = 1;
        PARSE_ERROR = 2;
        TRANSPORT_ERROR = 3;
        HANDLER_NOT_FOUND = 4;
    }
    RpcErrorCode error_code = 1;
    bytes response_data = 2;      // Serialized response message
}
```

### Message Flow

#### Request/Response Flow
```
Client                          Server
  │                               │
  ├──────── REQUEST ──────────>   │
  │         (msg_id=42)           │
  │                               │ Process request
  │                               │ Execute handler
  │                               │
  │   <────── RESPONSE ────────   │
  │         (msg_id=42)           │
  │                               │
```

#### Event Flow (Fire-and-Forget)
```
Sender                          Receiver
  │                               │
  ├──────── EVENT ─────────>      │
  │         (msg_id=0)            │
  │                               │ Process event
  │         (no response)         │ Execute handler
  │                               │
```

## Service Definition

### Proto Service Syntax

Define services using standard Protocol Buffers service syntax:

```protobuf
syntax = "proto3";
package myapp;
import "litepb/rpc_options.proto";

// Request and response messages
message TemperatureRequest {
    int32 sensor_id = 1;
    bool fahrenheit = 2;
}

message TemperatureResponse {
    float temperature = 1;
    string unit = 2;
    int64 timestamp_ms = 3;
}

message CalibrationData {
    float offset = 1;
    float scale = 2;
}

// Service definition
service TemperatureSensor {
    // Standard RPC method
    rpc GetTemperature(TemperatureRequest) returns (TemperatureResponse) {
        option (litepb.rpc.method_id) = 1;
    }
    
    // Another RPC method
    rpc Calibrate(CalibrationData) returns (TemperatureResponse) {
        option (litepb.rpc.method_id) = 2;
    }
    
    // Event method (no response expected)
    rpc TemperatureAlert(TemperatureResponse) returns (google.protobuf.Empty) {
        option (litepb.rpc.method_id) = 3;
        option (litepb.rpc.fire_and_forget) = true;
    }
}
```

### Code Generation

Generate C++ code:
```bash
./litepb_gen temperature_sensor.proto -o generated/
```

This creates:
- `temperature_sensor.pb.h` - Message structures and serialization
- `TemperatureSensorServer` - Base class for service implementation
- `TemperatureSensorClient` - Client proxy for making RPC calls
- `register_temperature_sensor()` - Registration helper function

## Implementation Guide

### Server Implementation

```cpp
#include "generated/temperature_sensor.pb.h"
#include "litepb/rpc/channel.h"
#include "your_transport.h"

// Implement the service
class TemperatureSensorImpl : public TemperatureSensorServer {
public:
    litepb::Result<TemperatureResponse> GetTemperature(
        uint64_t sender_address,  // Who sent the request
        const TemperatureRequest& request) override {
        
        litepb::Result<TemperatureResponse> result;
        
        // Validate request
        if (request.sensor_id < 0 || request.sensor_id >= MAX_SENSORS) {
            result.error.code = litepb::RpcError::OK;  // RPC succeeded
            result.value.temperature = 0;
            result.value.unit = "ERROR: Invalid sensor ID";
            return result;
        }
        
        // Read sensor
        float temp_celsius = read_sensor(request.sensor_id);
        
        // Convert units if needed
        if (request.fahrenheit) {
            result.value.temperature = temp_celsius * 9.0f / 5.0f + 32.0f;
            result.value.unit = "°F";
        } else {
            result.value.temperature = temp_celsius;
            result.value.unit = "°C";
        }
        
        result.value.timestamp_ms = get_timestamp_ms();
        result.error.code = litepb::RpcError::OK;
        
        return result;
    }
    
    litepb::Result<TemperatureResponse> Calibrate(
        uint64_t sender_address,
        const CalibrationData& data) override {
        
        litepb::Result<TemperatureResponse> result;
        
        // Apply calibration
        apply_calibration(data.offset, data.scale);
        
        // Return current reading after calibration
        TemperatureRequest req;
        req.sensor_id = 0;
        req.fahrenheit = false;
        
        return GetTemperature(sender_address, req);
    }
    
    void TemperatureAlert(
        uint64_t sender_address,
        const TemperatureResponse& alert) override {
        
        // Handle temperature alert (no response needed)
        log_alert(sender_address, alert.temperature, alert.unit);
        trigger_alarm_if_needed(alert.temperature);
    }
};

// Set up and run the server
int main() {
    // Create transport (example: UART)
    UartTransport transport("/dev/ttyS0", 115200);
    
    // Create RPC channel
    // Parameters: transport, local_address, default_timeout_ms
    litepb::RpcChannel channel(transport, 0x1001, 5000);
    
    // Create and register service
    TemperatureSensorImpl service;
    register_temperature_sensor(channel, service);
    
    // Main event loop
    while (true) {
        // Process incoming RPC messages
        channel.process();
        
        // Do other work
        update_display();
        check_buttons();
        
        // Small delay to prevent busy-waiting
        delay_ms(1);
    }
    
    return 0;
}
```

### Client Implementation

```cpp
#include "generated/temperature_sensor.pb.h"
#include "litepb/rpc/channel.h"
#include "your_transport.h"

int main() {
    // Create transport
    UartTransport transport("/dev/ttyS1", 115200);
    
    // Create RPC channel
    litepb::RpcChannel channel(transport, 0x2001, 5000);
    
    // Create client stub
    TemperatureSensorClient client(channel);
    
    // Make synchronous-style call with callback
    TemperatureRequest request;
    request.sensor_id = 1;
    request.fahrenheit = true;
    
    // Call with callback
    client.GetTemperature(
        request,
        [](const litepb::Result<TemperatureResponse>& result) {
            if (result.ok()) {
                printf("Temperature: %.1f %s\n", 
                    result.value.temperature,
                    result.value.unit.c_str());
            } else {
                printf("RPC failed: %s\n", 
                    rpc_error_to_string(result.error.code));
            }
        },
        3000,        // Timeout in milliseconds
        0x1001       // Destination address (server)
    );
    
    // Send fire-and-forget event
    TemperatureResponse alert;
    alert.temperature = 85.5;
    alert.unit = "°F";
    alert.timestamp_ms = get_timestamp_ms();
    
    client.TemperatureAlert(alert, 0x1001);  // No callback, no timeout
    
    // Process responses
    uint32_t last_time = millis();
    while (true) {
        channel.process();
        
        // Send periodic requests
        if (millis() - last_time > 1000) {
            last_time = millis();
            // Make another request...
        }
        
        delay_ms(1);
    }
    
    return 0;
}
```

## Transport Layer

### Transport Interface

All transports must implement this interface:

```cpp
class Transport {
public:
    virtual ~Transport() = default;
    
    // Send data with addressing
    virtual bool send(const uint8_t* data, size_t len, 
                     uint64_t src_addr, uint64_t dst_addr) = 0;
    
    // Receive data with addressing
    virtual size_t recv(uint8_t* buffer, size_t max_len,
                       uint64_t& src_addr, uint64_t& dst_addr) = 0;
    
    // Check if data is available
    virtual bool available() = 0;
};
```

### Stream Transport Example (UART)

```cpp
class UartTransport : public StreamTransport {
private:
    int fd;
    uint64_t local_addr;
    
public:
    UartTransport(const char* device, int baudrate, uint64_t addr) 
        : local_addr(addr) {
        fd = open_uart(device, baudrate);
    }
    
    bool send(const uint8_t* data, size_t len, 
              uint64_t src_addr, uint64_t dst_addr) override {
        // For UART, we need to add addressing in the frame
        uint8_t header[18];
        write_uint64(header, src_addr);
        write_uint64(header + 8, dst_addr);
        write_uint16(header + 16, generate_msg_id());
        
        // Send header then data
        write(fd, header, sizeof(header));
        return write(fd, data, len) == len;
    }
    
    size_t recv(uint8_t* buffer, size_t max_len,
                uint64_t& src_addr, uint64_t& dst_addr) override {
        // Read any available data
        uint8_t header[18];
        if (read_available(fd, header, sizeof(header)) < sizeof(header)) {
            return 0;  // Need more data
        }
        
        src_addr = read_uint64(header);
        dst_addr = read_uint64(header + 8);
        // msg_id = read_uint16(header + 16);  // Handled by framing
        
        return read_available(fd, buffer, max_len);
    }
    
    bool available() override {
        return uart_bytes_available(fd) > 0;
    }
};
```

### Packet Transport Example (UDP)

```cpp
class UdpTransport : public PacketTransport {
private:
    int sock;
    struct sockaddr_in local_addr;
    std::map<uint64_t, struct sockaddr_in> peer_addrs;
    
public:
    UdpTransport(uint16_t port) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(port);
        local_addr.sin_addr.s_addr = INADDR_ANY;
        
        bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));
        set_nonblocking(sock);
    }
    
    bool send_packet(const uint8_t* data, size_t len) override {
        // UDP handles framing automatically
        return sendto(sock, data, len, 0, 
                     (struct sockaddr*)&peer_addr, 
                     sizeof(peer_addr)) == len;
    }
    
    size_t recv_packet(uint8_t* buffer, size_t max_len) override {
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        
        ssize_t received = recvfrom(sock, buffer, max_len, MSG_DONTWAIT,
                                   (struct sockaddr*)&sender, &sender_len);
        
        if (received > 0) {
            // Extract addressing from packet header or use IP:port mapping
            return received;
        }
        return 0;
    }
    
    // Implement base Transport interface
    bool send(const uint8_t* data, size_t len,
              uint64_t src_addr, uint64_t dst_addr) override {
        // Add addressing to packet and send
        uint8_t packet[1500];
        write_uint64(packet, src_addr);
        write_uint64(packet + 8, dst_addr);
        write_uint16(packet + 16, 0);  // msg_id
        memcpy(packet + 18, data, len);
        
        return send_packet(packet, 18 + len);
    }
    
    size_t recv(uint8_t* buffer, size_t max_len,
                uint64_t& src_addr, uint64_t& dst_addr) override {
        uint8_t packet[1500];
        size_t received = recv_packet(packet, sizeof(packet));
        
        if (received >= 18) {
            src_addr = read_uint64(packet);
            dst_addr = read_uint64(packet + 8);
            // msg_id handled by upper layer
            
            size_t payload_len = received - 18;
            if (payload_len <= max_len) {
                memcpy(buffer, packet + 18, payload_len);
                return payload_len;
            }
        }
        return 0;
    }
    
    bool available() override {
        return socket_has_data(sock);
    }
};
```

## Error Handling

### Error Categories

LitePB RPC uses a two-tier error model:

#### Protocol-Level Errors (RPC Framework)
These indicate issues with the RPC mechanism itself:

- `OK` - Operation completed successfully
- `TIMEOUT` - Request exceeded deadline
- `PARSE_ERROR` - Malformed message or corrupted data
- `TRANSPORT_ERROR` - Transport layer failure
- `HANDLER_NOT_FOUND` - No handler registered for method

#### Application-Level Errors (Business Logic)
These are specific to your application and encoded in response messages:

```protobuf
message SensorResponse {
    enum Status {
        SUCCESS = 0;
        SENSOR_OFFLINE = 1;
        OUT_OF_RANGE = 2;
        CALIBRATION_REQUIRED = 3;
    }
    Status status = 1;
    float value = 2;
    string error_message = 3;
}
```

### Error Handling Best Practices

```cpp
// Server-side error handling
litepb::Result<SensorResponse> GetReading(const SensorRequest& req) {
    litepb::Result<SensorResponse> result;
    
    // Check RPC-level issues first
    if (!transport_healthy()) {
        result.error.code = litepb::RpcError::TRANSPORT_ERROR;
        return result;
    }
    
    // RPC succeeds, but application may have errors
    result.error.code = litepb::RpcError::OK;
    
    // Check application-level issues
    if (!sensor_online(req.sensor_id)) {
        result.value.status = SensorResponse::SENSOR_OFFLINE;
        result.value.error_message = "Sensor not responding";
        return result;
    }
    
    float reading = read_sensor(req.sensor_id);
    if (reading < MIN_VALID || reading > MAX_VALID) {
        result.value.status = SensorResponse::OUT_OF_RANGE;
        result.value.value = reading;
        result.value.error_message = "Reading outside valid range";
        return result;
    }
    
    // Success case
    result.value.status = SensorResponse::SUCCESS;
    result.value.value = reading;
    return result;
}

// Client-side error handling
client.GetReading(request, [](const litepb::Result<SensorResponse>& result) {
    // Check RPC-level errors
    if (!result.ok()) {
        switch (result.error.code) {
            case litepb::RpcError::TIMEOUT:
                handle_timeout();
                break;
            case litepb::RpcError::TRANSPORT_ERROR:
                reconnect_transport();
                break;
            default:
                log_error("RPC failed: %s", 
                         rpc_error_to_string(result.error.code));
        }
        return;
    }
    
    // Check application-level errors
    switch (result.value.status) {
        case SensorResponse::SUCCESS:
            process_reading(result.value.value);
            break;
            
        case SensorResponse::SENSOR_OFFLINE:
            schedule_sensor_restart();
            break;
            
        case SensorResponse::OUT_OF_RANGE:
            log_warning("Sensor reading out of range: %.2f", 
                       result.value.value);
            break;
            
        case SensorResponse::CALIBRATION_REQUIRED:
            initiate_calibration();
            break;
    }
});
```

## Advanced Features

### Fire-and-Forget Events

Events are one-way messages that don't expect responses:

```cpp
// Server sends event to all connected clients
void broadcast_alert(RpcChannel& channel, float temperature) {
    TemperatureAlert alert;
    alert.temperature = temperature;
    alert.severity = TemperatureAlert::CRITICAL;
    alert.timestamp = get_timestamp();
    
    // Send to broadcast address
    channel.send_event(
        TEMPERATURE_SERVICE_ID,
        ALERT_METHOD_ID,
        alert,
        RPC_ADDRESS_BROADCAST  // All nodes receive this
    );
}

// Client registers event handler
channel.on_event(
    TEMPERATURE_SERVICE_ID,
    ALERT_METHOD_ID,
    [](uint64_t sender, const TemperatureAlert& alert) {
        if (alert.severity == TemperatureAlert::CRITICAL) {
            activate_cooling_system();
            log_critical_event(sender, alert.temperature);
        }
    }
);
```

### Bidirectional Communication

Any node can be both client and server:

```cpp
class BidirectionalNode {
private:
    litepb::RpcChannel channel;
    SensorServiceImpl sensor_service;
    ControlServiceImpl control_service;
    
public:
    BidirectionalNode(Transport& transport, uint64_t address)
        : channel(transport, address, 5000) {
        
        // Register as server for multiple services
        register_sensor_service(channel, sensor_service);
        register_control_service(channel, control_service);
    }
    
    void run() {
        // Create client stubs for calling other nodes
        SensorServiceClient remote_sensor(channel);
        ControlServiceClient remote_control(channel);
        
        uint32_t last_poll = millis();
        
        while (true) {
            // Process incoming requests (server role)
            channel.process();
            
            // Make outgoing requests (client role)
            if (millis() - last_poll > 1000) {
                last_poll = millis();
                
                // Query peer sensor
                SensorRequest req;
                req.sensor_id = PEER_SENSOR_ID;
                
                remote_sensor.GetReading(req, 
                    [this](const Result<SensorResponse>& result) {
                        if (result.ok()) {
                            update_peer_data(result.value);
                        }
                    },
                    2000,      // 2 second timeout
                    PEER_NODE  // Peer address
                );
            }
        }
    }
};
```

### Custom Addressing Schemes

Implement application-specific addressing:

```cpp
// Define address structure for your network
namespace addressing {
    constexpr uint64_t make_address(uint8_t network, uint8_t subnet, 
                                   uint16_t device, uint32_t port) {
        return ((uint64_t)network << 56) |
               ((uint64_t)subnet << 48) |
               ((uint64_t)device << 32) |
               ((uint64_t)port);
    }
    
    constexpr uint8_t get_network(uint64_t addr) {
        return (addr >> 56) & 0xFF;
    }
    
    constexpr uint8_t get_subnet(uint64_t addr) {
        return (addr >> 48) & 0xFF;
    }
    
    constexpr uint16_t get_device(uint64_t addr) {
        return (addr >> 32) & 0xFFFF;
    }
    
    constexpr uint32_t get_port(uint64_t addr) {
        return addr & 0xFFFFFFFF;
    }
}

// Use in your application
constexpr uint64_t SENSOR_NODE_1 = addressing::make_address(1, 1, 100, 5000);
constexpr uint64_t SENSOR_NODE_2 = addressing::make_address(1, 1, 101, 5000);
constexpr uint64_t CONTROL_NODE = addressing::make_address(1, 2, 200, 6000);
```

## Best Practices

### Memory Management

1. **Use Fixed-Size Buffers in Embedded Systems**
```cpp
// Good for embedded
litepb::FixedOutputStream<256> output;
litepb::FixedInputStream<256> input(data, size);

// Good for systems with heap
litepb::BufferOutputStream output;
litepb::BufferInputStream input(data, size);
```

2. **Reuse Message Objects**
```cpp
class EfficientHandler {
private:
    // Reuse these objects to avoid allocations
    SensorRequest request;
    SensorResponse response;
    
public:
    void handle_request(const uint8_t* data, size_t len) {
        // Clear and reuse
        request = SensorRequest();  // Reset to defaults
        
        BufferInputStream input(data, len);
        if (litepb::parse(request, input)) {
            process_request(request, response);
        }
    }
};
```

3. **Configure Buffer Sizes**
```cpp
// In your project configuration
#define LITEPB_RPC_INITIAL_BUFFER_SIZE 512  // Adjust based on message sizes
```

### Timeout Management

```cpp
// Use appropriate timeouts for different transports
constexpr uint32_t UART_TIMEOUT_MS = 1000;    // Fast local connection
constexpr uint32_t TCP_TIMEOUT_MS = 5000;     // Network connection
constexpr uint32_t LORA_TIMEOUT_MS = 30000;   // Slow, long-range

// Implement retry logic
void reliable_call(Client& client, const Request& req) {
    const int MAX_RETRIES = 3;
    int retries = 0;
    
    auto retry_handler = [&](const Result<Response>& result) {
        if (!result.ok() && result.error.code == RpcError::TIMEOUT) {
            if (++retries < MAX_RETRIES) {
                // Retry with exponential backoff
                uint32_t timeout = BASE_TIMEOUT * (1 << retries);
                client.CallMethod(req, retry_handler, timeout, SERVER_ADDR);
            } else {
                handle_permanent_failure();
            }
        } else {
            handle_success(result);
        }
    };
    
    client.CallMethod(req, retry_handler, BASE_TIMEOUT, SERVER_ADDR);
}
```

### Service Versioning

```cpp
// Use service ID to version your API
constexpr uint32_t SENSOR_SERVICE_V1 = 0x1000;
constexpr uint32_t SENSOR_SERVICE_V2 = 0x1001;

// Support multiple versions simultaneously
register_sensor_v1(channel, sensor_v1_impl);
register_sensor_v2(channel, sensor_v2_impl);
```

### Security Considerations

While LitePB doesn't provide built-in encryption, you can:

1. **Add Authentication**
```cpp
message AuthenticatedRequest {
    bytes signature = 1;
    bytes payload = 2;
}

bool verify_request(const AuthenticatedRequest& req) {
    return verify_hmac(req.signature, req.payload, shared_secret);
}
```

2. **Use Encrypted Transport**
```cpp
class EncryptedTransport : public Transport {
    bool send(const uint8_t* data, size_t len, ...) override {
        uint8_t encrypted[len + 16];
        size_t encrypted_len = encrypt_aes(data, len, encrypted, key, iv);
        return underlying_transport.send(encrypted, encrypted_len, ...);
    }
};
```

## Performance Optimization

### Message Design

1. **Use Appropriate Field Types**
```protobuf
// Good: Compact encoding
message EfficientMessage {
    uint32 id = 1;           // Uses varint encoding
    sint32 temperature = 2;  // Efficient for negative numbers
    bool active = 3;         // Single byte
    float value = 4;         // Fixed 4 bytes
}

// Less efficient
message InefficientMessage {
    string id = 1;           // Variable length with overhead
    int32 temperature = 2;   // Inefficient for negative
    int32 active = 3;        // Uses varint for boolean
    double value = 4;        // 8 bytes when 4 might suffice
}
```

2. **Pack Repeated Fields**
```protobuf
// Proto3 packs by default
message Data {
    repeated int32 samples = 1;  // Packed in proto3
}

// Proto2 requires explicit packing
message Data {
    repeated int32 samples = 1 [packed=true];
}
```

### Processing Optimization

1. **Batch Operations**
```cpp
// Process multiple messages per iteration
void efficient_process(RpcChannel& channel) {
    const int MAX_MESSAGES = 10;
    int processed = 0;
    
    while (processed < MAX_MESSAGES && channel.has_pending()) {
        channel.process_one();
        processed++;
    }
}
```

2. **Use Non-Blocking I/O**
```cpp
// Configure transport for non-blocking operation
void configure_uart_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

3. **Optimize Event Loop**
```cpp
void optimized_event_loop() {
    const uint32_t IDLE_DELAY = 10;
    const uint32_t BUSY_DELAY = 0;
    
    while (running) {
        bool had_activity = false;
        
        // Process RPC
        if (channel.process()) {
            had_activity = true;
        }
        
        // Process other tasks
        if (process_sensors()) {
            had_activity = true;
        }
        
        // Adaptive delay
        if (!had_activity) {
            delay_ms(IDLE_DELAY);  // Sleep when idle
        } else {
            delay_ms(BUSY_DELAY);   // Minimal delay when busy
        }
    }
}
```

## Troubleshooting

### Common Issues and Solutions

#### Issue: RPC Timeouts
**Causes:**
- Network latency
- Slow handler processing
- Inadequate timeout values

**Solutions:**
```cpp
// Increase timeout for slow operations
client.CallMethod(req, callback, 10000, dest);  // 10 second timeout

// Optimize handler processing
Result<Response> Handler(const Request& req) {
    // Move heavy processing to background
    queue_background_task(req);
    
    // Return immediately with status
    Result<Response> result;
    result.value.status = Response::PROCESSING;
    result.error.code = RpcError::OK;
    return result;
}
```

#### Issue: Message Corruption
**Causes:**
- Noisy communication channel
- Buffer overflows
- Incorrect framing

**Solutions:**
```cpp
// Add checksums at transport layer
class ChecksumTransport : public Transport {
    bool send(const uint8_t* data, size_t len, ...) override {
        uint32_t crc = calculate_crc32(data, len);
        // Send data + CRC
    }
    
    size_t recv(uint8_t* buffer, size_t max_len, ...) override {
        // Receive and verify CRC
    }
};

// Increase buffer sizes
#define LITEPB_RPC_INITIAL_BUFFER_SIZE 2048
```

#### Issue: Handler Not Found
**Causes:**
- Service not registered
- Mismatched service/method IDs
- Version mismatch

**Solutions:**
```cpp
// Verify registration
void debug_handlers(RpcChannel& channel) {
    // Log all registered handlers
    channel.dump_registered_handlers();
}

// Check version compatibility
if (request.version != EXPECTED_VERSION) {
    return error_response(INCOMPATIBLE_VERSION);
}
```

## Conclusion

LitePB RPC provides a robust, efficient, and flexible framework for implementing remote procedure calls in embedded systems. By following the patterns and best practices outlined in this guide, you can build reliable distributed systems that work efficiently even on resource-constrained devices.

Key takeaways:
- Use appropriate transports for your communication medium
- Handle both protocol and application-level errors appropriately
- Optimize message design for your use case
- Implement proper timeout and retry logic
- Consider memory constraints in embedded environments

For additional examples and reference implementations, see the [examples directory](../examples/cpp/rpc/).