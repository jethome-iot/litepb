# Sensor Service RPC Example

Complete demonstration of LitePB's RPC capabilities with bidirectional sensor communication.

## Overview

Implements a `SensorService` with:
- **GetReading**: Request sensor readings from another peer
- **NotifyAlert**: Send alerts when thresholds are exceeded

Features demonstrated:
- Async callbacks with type-safe generated stubs
- Bidirectional RPC (both peers can initiate calls)
- Multiple transport options (LoopbackTransport, TCP, UART)
- Error handling with `Result<T>` types
- Non-blocking event loop

## Building and Running

```bash
# From this directory
pio run -e litepb_rpc -t exec

# Run tests
pio test -e test_sensor_loopback
pio test -e test_sensor_service
pio test -e test_sensor_integration
```

## Files

- `proto/examples/sensor.proto` - Service definition
- `src/main.cpp` - Main example implementation
- `test/` - Comprehensive test suite (38 tests)
- `platformio.ini` - Build configuration

## Transport Options

Three transport implementations available:
1. **LoopbackTransport** - In-memory testing
2. **TcpTransport** - Network communication
3. **UartTransport** - Serial communication

## Test Coverage

38 tests across 4 test suites:
- `test_sensor_loopback` - Channel loopback functionality
- `test_sensor_service` - Service request/response
- `test_sensor_simulator` - Simulated sensor data
- `test_sensor_integration` - Integration with timeouts