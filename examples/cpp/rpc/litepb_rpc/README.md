# LitePB RPC Sensor Service Example

## Overview

This example demonstrates LitePB's complete RPC (Remote Procedure Call) framework through a realistic sensor service implementation. It showcases bidirectional communication, async callbacks, multiple transport options, and comprehensive error handling in a production-ready pattern suitable for embedded systems.

## What This Example Demonstrates

- **Service Definition**: Protocol Buffers service with RPC methods
- **Bidirectional RPC**: Both peers can initiate calls (peer-to-peer architecture)
- **Async Callbacks**: Non-blocking, event-driven communication
- **Type-Safe Stubs**: Generated client and server interfaces
- **Multiple Transports**: LoopbackTransport, TCP, UART implementations
- **Error Handling**: Comprehensive error detection and recovery
- **Timeout Management**: Request deadlines and timeout handling
- **Event Notifications**: Fire-and-forget messages for alerts
- **Production Patterns**: Event loop, message correlation, resource management

## Prerequisites

- C++17 compatible compiler
- LitePB library with RPC support (`LITEPB_WITH_RPC` defined)
- PlatformIO for building and testing
- Protocol Buffers compiler (`protoc`)

## Building and Running

### Main Example

```bash
# From this directory
pio run -e litepb_rpc -t exec

# Or from project root
cd examples/cpp/rpc/litepb_rpc
pio run -e litepb_rpc -t exec

# For embedded platforms
pio run -e esp32_rpc -t upload    # ESP32 with RPC
```

### Running Tests

The example includes a comprehensive test suite with 38 tests:

```bash
# Run all tests
pio test

# Run specific test suites
pio test -e test_sensor_loopback      # Loopback transport tests
pio test -e test_sensor_service       # Service implementation tests
pio test -e test_sensor_simulator     # Sensor simulation tests
pio test -e test_sensor_integration   # Integration tests
```

## Architecture

### Service Definition (proto/examples/sensor.proto)

```protobuf
syntax = "proto3";
package examples;
import "litepb/rpc_options.proto";

// Request for sensor reading
message SensorRequest {
    int32 sensor_id = 1;
    bool include_metadata = 2;
}

// Response with sensor data
message SensorResponse {
    int32 sensor_id = 1;
    float temperature = 2;
    float humidity = 3;
    int64 timestamp = 4;
    string status = 5;
    
    message Metadata {
        string location = 1;
        string calibration_date = 2;
        float accuracy = 3;
    }
    Metadata metadata = 6;
}

// Alert notification (event)
message SensorAlert {
    int32 sensor_id = 1;
    enum AlertType {
        TEMPERATURE_HIGH = 0;
        TEMPERATURE_LOW = 1;
        HUMIDITY_HIGH = 2;
        HUMIDITY_LOW = 3;
        SENSOR_FAILURE = 4;
    }
    AlertType type = 2;
    float value = 3;
    string message = 4;
}

// Sensor service definition
service SensorService {
    // Request-response RPC
    rpc GetReading(SensorRequest) returns (SensorResponse) {
        option (litepb.rpc.method_id) = 1;
    }
    
    // Fire-and-forget event
    rpc NotifyAlert(SensorAlert) returns (google.protobuf.Empty) {
        option (litepb.rpc.method_id) = 2;
        option (litepb.rpc.fire_and_forget) = true;
    }
}
```

### Implementation Structure

```
src/
├── main.cpp                 # Main example application
├── sensor_service_impl.h    # Service implementation
├── sensor_simulator.h       # Simulated sensor data
├── transport_factory.h      # Transport creation utilities
└── event_loop.h            # Event processing loop

test/
├── test_sensor_loopback.cpp     # Transport tests
├── test_sensor_service.cpp      # Service tests
├── test_sensor_simulator.cpp    # Simulator tests
└── test_sensor_integration.cpp  # Integration tests
```

## Code Walkthrough

### Server Implementation

```cpp
class SensorServiceImpl : public SensorServiceServer {
private:
    SensorSimulator simulator;
    
public:
    // RPC method implementation
    litepb::Result<SensorResponse> GetReading(
        uint64_t sender_address,
        const SensorRequest& request) override {
        
        litepb::Result<SensorResponse> result;
        
        // Validate request
        if (request.sensor_id < 0 || request.sensor_id >= MAX_SENSORS) {
            result.error.code = litepb::RpcError::OK;  // RPC succeeded
            result.value.status = "ERROR: Invalid sensor ID";
            return result;
        }
        
        // Get sensor data
        auto data = simulator.read_sensor(request.sensor_id);
        
        // Populate response
        result.value.sensor_id = request.sensor_id;
        result.value.temperature = data.temperature;
        result.value.humidity = data.humidity;
        result.value.timestamp = get_timestamp_ms();
        result.value.status = "OK";
        
        // Include metadata if requested
        if (request.include_metadata) {
            result.value.metadata.location = get_sensor_location(request.sensor_id);
            result.value.metadata.calibration_date = "2024-01-15";
            result.value.metadata.accuracy = 0.95f;
        }
        
        // Check for alerts
        if (data.temperature > HIGH_TEMP_THRESHOLD) {
            schedule_alert(request.sensor_id, 
                         SensorAlert::TEMPERATURE_HIGH, 
                         data.temperature);
        }
        
        result.error.code = litepb::RpcError::OK;
        return result;
    }
    
    // Event handler (no response)
    void NotifyAlert(uint64_t sender_address,
                    const SensorAlert& alert) override {
        
        std::cout << "[ALERT] Sensor " << alert.sensor_id 
                  << ": " << alert.message << std::endl;
        
        // Take action based on alert type
        switch (alert.type) {
            case SensorAlert::TEMPERATURE_HIGH:
                activate_cooling();
                break;
            case SensorAlert::TEMPERATURE_LOW:
                activate_heating();
                break;
            case SensorAlert::SENSOR_FAILURE:
                mark_sensor_offline(alert.sensor_id);
                schedule_maintenance();
                break;
        }
        
        // Log to persistent storage
        log_alert(alert);
    }
};
```

### Client Implementation

```cpp
class SensorClient {
private:
    litepb::RpcChannel& channel;
    SensorServiceClient client;
    
public:
    SensorClient(litepb::RpcChannel& ch) 
        : channel(ch), client(ch) {}
    
    void request_reading(int sensor_id, uint64_t server_addr) {
        // Prepare request
        SensorRequest request;
        request.sensor_id = sensor_id;
        request.include_metadata = true;
        
        // Make async RPC call
        client.GetReading(
            request,
            [this, sensor_id](const litepb::Result<SensorResponse>& result) {
                handle_response(sensor_id, result);
            },
            5000,        // 5 second timeout
            server_addr  // Destination address
        );
    }
    
    void send_alert(int sensor_id, float temperature, uint64_t dest) {
        // Create alert
        SensorAlert alert;
        alert.sensor_id = sensor_id;
        alert.type = SensorAlert::TEMPERATURE_HIGH;
        alert.value = temperature;
        alert.message = "Temperature exceeds threshold";
        
        // Send as fire-and-forget event
        client.NotifyAlert(alert, dest);
        // No callback, no timeout - it's an event
    }
    
private:
    void handle_response(int sensor_id, 
                        const litepb::Result<SensorResponse>& result) {
        if (!result.ok()) {
            // Handle RPC-level error
            std::cerr << "RPC failed for sensor " << sensor_id 
                     << ": " << rpc_error_to_string(result.error.code) 
                     << std::endl;
                     
            if (result.error.code == litepb::RpcError::TIMEOUT) {
                // Retry with longer timeout
                retry_with_backoff(sensor_id);
            }
            return;
        }
        
        // Check application-level status
        if (result.value.status != "OK") {
            std::cerr << "Sensor error: " << result.value.status << std::endl;
            return;
        }
        
        // Process successful response
        std::cout << "Sensor " << sensor_id << ": "
                  << result.value.temperature << "°C, "
                  << result.value.humidity << "% RH" << std::endl;
                  
        // Update local cache
        update_sensor_cache(sensor_id, result.value);
    }
};
```

### Transport Implementations

#### LoopbackTransport (In-Memory Testing)

```cpp
class LoopbackTransport : public litepb::Transport {
private:
    std::queue<Message> tx_queue;
    std::queue<Message> rx_queue;
    
public:
    bool send(const uint8_t* data, size_t len,
              uint64_t src_addr, uint64_t dst_addr) override {
        Message msg;
        msg.data.assign(data, data + len);
        msg.src_addr = src_addr;
        msg.dst_addr = dst_addr;
        
        // Simulate network by swapping src/dst for loopback
        tx_queue.push(msg);
        
        // Mirror to rx_queue for loopback
        std::swap(msg.src_addr, msg.dst_addr);
        rx_queue.push(msg);
        
        return true;
    }
    
    size_t recv(uint8_t* buffer, size_t max_len,
                uint64_t& src_addr, uint64_t& dst_addr) override {
        if (rx_queue.empty()) {
            return 0;
        }
        
        Message msg = rx_queue.front();
        rx_queue.pop();
        
        size_t copy_len = std::min(msg.data.size(), max_len);
        std::memcpy(buffer, msg.data.data(), copy_len);
        
        src_addr = msg.src_addr;
        dst_addr = msg.dst_addr;
        
        return copy_len;
    }
    
    bool available() override {
        return !rx_queue.empty();
    }
};
```

### Event Loop

```cpp
class EventLoop {
private:
    litepb::RpcChannel& channel;
    bool running = true;
    uint32_t last_sensor_poll = 0;
    
public:
    void run() {
        std::cout << "Starting event loop..." << std::endl;
        
        while (running) {
            // Process incoming RPC messages
            if (channel.process()) {
                // Had activity, process immediately
                continue;
            }
            
            // Periodic sensor polling
            uint32_t now = get_time_ms();
            if (now - last_sensor_poll > POLL_INTERVAL_MS) {
                last_sensor_poll = now;
                poll_all_sensors();
            }
            
            // Check for local events
            process_local_events();
            
            // Small delay to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    void stop() {
        running = false;
    }
};
```

## Expected Output

```
=== LitePB RPC Sensor Service Example ===

Initializing transport: LoopbackTransport
Setting up RPC channel (Node address: 0x1001)
Registering SensorService...

Starting bidirectional communication demo...

[CLIENT] Requesting reading from sensor 1...
[SERVER] Processing GetReading(sensor_id=1)
[CLIENT] Received: Sensor 1: 23.5°C, 45.2% RH
         Location: Room A, Calibration: 2024-01-15

[CLIENT] Requesting reading from sensor 2...
[SERVER] Processing GetReading(sensor_id=2)
[CLIENT] Received: Sensor 2: 24.1°C, 43.8% RH
         Location: Room B, Calibration: 2024-01-15

[SERVER] Temperature threshold exceeded on sensor 3!
[SERVER] Sending alert to monitoring station...
[CLIENT] [ALERT] Sensor 3: Temperature exceeds threshold (28.5°C)
[CLIENT] Activating cooling system...

Simulating timeout scenario...
[CLIENT] Requesting reading from offline sensor 99...
[CLIENT] RPC failed for sensor 99: RPC timeout
[CLIENT] Retrying with extended timeout...

Running for 10 seconds...
Processed 142 RPC calls
Average latency: 0.8ms
No errors detected

=== Example completed successfully ===
```

## Test Coverage

The example includes comprehensive tests across 4 suites:

### test_sensor_loopback (10 tests)
- Basic send/receive operations
- Bidirectional communication
- Message ordering
- Buffer overflow handling
- Address validation

### test_sensor_service (12 tests)
- GetReading with valid sensors
- Invalid sensor handling
- Metadata inclusion
- Alert generation
- Error responses
- Concurrent requests

### test_sensor_simulator (8 tests)
- Sensor data generation
- Temperature/humidity ranges
- Failure simulation
- Data consistency
- Thread safety

### test_sensor_integration (8 tests)
- End-to-end communication
- Timeout handling
- Retry logic
- Event notifications
- Performance benchmarks
- Resource cleanup

## Transport Options

### Available Transports

1. **LoopbackTransport**
   - In-memory for testing
   - Zero latency
   - Perfect for unit tests

2. **TcpTransport** (example)
   ```cpp
   TcpTransport transport("192.168.1.100", 8080);
   ```
   - Network communication
   - Client-server or peer-to-peer
   - Supports multiple connections

3. **UartTransport** (example)
   ```cpp
   UartTransport transport("/dev/ttyUSB0", 115200);
   ```
   - Serial communication
   - Embedded systems
   - Point-to-point links

### Implementing Custom Transport

```cpp
class CustomTransport : public litepb::Transport {
public:
    bool send(const uint8_t* data, size_t len,
              uint64_t src, uint64_t dst) override {
        // Your implementation
        return true;
    }
    
    size_t recv(uint8_t* buffer, size_t max_len,
                uint64_t& src, uint64_t& dst) override {
        // Your implementation
        return 0;
    }
    
    bool available() override {
        // Check if data available
        return false;
    }
};
```

## Performance Characteristics

| Metric | LoopbackTransport | TCP (Local) | UART (115200) |
|--------|------------------|-------------|---------------|
| Latency | <0.1ms | 0.5-1ms | 2-5ms |
| Throughput | >10MB/s | 100MB/s | 11KB/s |
| CPU Usage | <1% | 2-5% | 1-2% |
| Memory | 10KB | 50KB | 5KB |

## Best Practices Demonstrated

1. **Error Handling**
   - Separate RPC and application errors
   - Timeout and retry strategies
   - Graceful degradation

2. **Resource Management**
   - Reuse message objects
   - Bounded queues
   - Memory pools for embedded

3. **Async Patterns**
   - Non-blocking operations
   - Event-driven architecture
   - Callback management

4. **Testing**
   - Unit tests for components
   - Integration tests
   - Performance benchmarks

## Customization Ideas

### 1. Add Authentication

```cpp
message AuthRequest {
    string token = 1;
}

service SecureSensorService {
    rpc Authenticate(AuthRequest) returns (AuthResponse);
    rpc GetReading(SensorRequest) returns (SensorResponse);
}
```

### 2. Implement Streaming

```cpp
// Continuous sensor updates
void stream_sensor_data(int sensor_id, uint64_t client) {
    while (streaming) {
        auto data = read_sensor(sensor_id);
        send_update(client, data);
        sleep(100);  // 10Hz updates
    }
}
```

### 3. Add Service Discovery

```cpp
message DiscoveryRequest {
    string service_name = 1;
}

message DiscoveryResponse {
    repeated uint64 addresses = 1;
}
```

## Troubleshooting

### Common Issues

1. **"RPC timeout" errors**
   - Increase timeout value
   - Check transport connectivity
   - Verify server is processing

2. **"Handler not found" errors**
   - Ensure service is registered
   - Check service/method IDs match
   - Verify proto definitions synchronized

3. **Message corruption**
   - Add transport-level checksums
   - Verify framing for stream transports
   - Check for buffer overflows

### Debug Techniques

```cpp
// Enable RPC tracing
#define LITEPB_RPC_TRACE

// Add logging to transport
class DebugTransport : public Transport {
    bool send(...) override {
        log("TX: %d bytes to 0x%llx", len, dst);
        return underlying.send(...);
    }
};

// Monitor channel state
channel.dump_pending_calls();
channel.get_stats();
```

## Next Steps

1. **Extend the Service**: Add more RPC methods and message types
2. **Implement Real Transport**: Replace loopback with TCP/UART
3. **Add Security**: Implement authentication and encryption
4. **Build Network**: Create multi-node sensor network
5. **Production Deployment**: Apply patterns to your embedded system

## Project Files

- `src/main.cpp` - Main example application
- `src/sensor_service_impl.h` - Service implementation
- `src/sensor_simulator.h` - Sensor data simulation
- `src/transport_factory.h` - Transport utilities
- `proto/examples/sensor.proto` - Service definition
- `test/*.cpp` - Comprehensive test suite
- `platformio.ini` - Build configuration
- `generated/` - Generated stubs (created by litepb_gen)

## License

This example is part of LitePB and is released under the MIT License.
Copyright (c) 2025 JetHome LLC