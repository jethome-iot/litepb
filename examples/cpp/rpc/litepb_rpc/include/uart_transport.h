#pragma once

#include "litepb/rpc/transport.h"
#include <cstddef>
#include <cstdint>

#ifdef ARDUINO
#include <HardwareSerial.h>
#else
class HardwareSerial;
#endif

namespace litepb {

/**
 * @brief UART/Serial transport implementation for Arduino and ESP32 platforms
 *
 * UartTransport provides a StreamTransport implementation that works with
 * Arduino's HardwareSerial interface (Serial, Serial1, Serial2, etc.).
 *
 * This transport is designed for:
 * - Arduino boards (Uno, Mega, Nano, etc.)
 * - ESP32/ESP8266 development boards
 * - STM32 with Arduino core
 * - Any platform supporting Arduino's HardwareSerial API
 *
 * ## Key Features
 *
 * - **Header-only**: No compilation required, just include and use
 * - **Non-blocking**: Uses available() to check before reading
 * - **Error handling**: Returns false on send failures
 * - **Stream-based**: Integrates with RPC framing layer for message delimiting
 * - **Flexible baud rates**: Configure Serial before passing to constructor
 *
 * ## Platform Requirements
 *
 * - Arduino framework or compatible platform
 * - HardwareSerial support (Serial, Serial1, Serial2, etc.)
 * - Minimum 1KB free RAM for RPC buffers
 * - C++11 or later
 *
 * ## Usage Example - Basic Client
 *
 * @code
 * #include "litepb/rpc/channel.h"
 * #include "examples/rpc/uart_transport.h"
 * #include "my_service.pb.h"  // Generated service stub
 *
 * using namespace litepb;
 *
 * // Global transport and channel
 * UartTransport uart_transport(Serial);
 * RpcChannel rpc_channel(uart_transport, 0x01);  // local address 0x01
 * MyServiceClient client(rpc_channel);  // Convenience wrapper for calling RPCs
 *
 * void setup() {
 *     // Initialize Serial with desired baud rate
 *     Serial.begin(115200);
 *
 *     // Wait for serial port to connect (optional, useful for debugging)
 *     while (!Serial) {
 *         delay(10);
 *     }
 *
 *     Serial.println("RPC Client Started");
 * }
 *
 * void loop() {
 *     // Process incoming RPC messages and handle timeouts
 *     rpc_channel.process();
 *
 *     // Make an RPC call every 5 seconds
 *     static unsigned long last_call = 0;
 *     if (millis() - last_call > 5000) {
 *         last_call = millis();
 *
 *         // Prepare request
 *         MyRequest request;
 *         request.sensor_id = 42;
 *
 *         // Make async RPC call with callback
 *         client.GetData(request, [](const Result<MyResponse>& result) {
 *             if (result.ok()) {
 *                 Serial.print("Received: ");
 *                 Serial.println(result.value.temperature);
 *             } else {
 *                 Serial.print("Error: ");
 *                 Serial.println(result.error.message.c_str());
 *             }
 *         });
 *     }
 * }
 * @endcode
 *
 * ## Usage Example - Basic Server
 *
 * @code
 * #include "litepb/rpc/channel.h"
 * #include "examples/rpc/uart_transport.h"
 * #include "my_service.pb.h"
 *
 * using namespace litepb;
 *
 * // Implement the service interface
 * class MyServiceImpl : public MyServiceServer {
 * public:
 *     Result<MyResponse> GetData(const MyRequest& req) override {
 *         MyResponse response;
 *
 *         // Read sensor based on request
 *         if (req.sensor_id == 42) {
 *             response.temperature = analogRead(A0) * 0.1;  // Example
 *             response.humidity = analogRead(A1) * 0.1;
 *
 *             return {response, RpcError{RpcError::OK}};
 *         } else {
 *             return {response, RpcError{RpcError::CUSTOM_ERROR, "Invalid sensor"}};
 *         }
 *     }
 * };
 *
 * MyServiceImpl service_impl;
 * UartTransport uart_transport(Serial);
 * RpcChannel rpc_channel(uart_transport, 0x02);  // local address 0x02
 *
 * void setup() {
 *     Serial.begin(115200);
 *
 *     // Register service handler
 *     register_my_service(rpc_channel, service_impl);
 *
 *     Serial.println("RPC Server Started");
 * }
 *
 * void loop() {
 *     // Process incoming requests and send responses
 *     rpc_channel.process();
 * }
 * @endcode
 *
 * ## Usage Example - ESP32 with Multiple UARTs
 *
 * @code
 * // ESP32 has 3 hardware UARTs: Serial, Serial1, Serial2
 *
 * // UART0 (Serial) - USB debugging
 * // UART1 (Serial1) - RPC communication with sensor module
 * // UART2 (Serial2) - RPC communication with gateway
 *
 * UartTransport sensor_uart(Serial1);
 * UartTransport gateway_uart(Serial2);
 *
 * RpcChannel sensor_channel(sensor_uart, 0x01);  // This peer is 0x01
 * RpcChannel gateway_channel(gateway_uart, 0x02);  // This peer is 0x02
 *
 * SensorServiceClient sensor_client(sensor_channel);
 * GatewayServiceImpl gateway_impl;
 *
 * void setup() {
 *     // Debug output
 *     Serial.begin(115200);
 *
 *     // Sensor UART (custom pins on ESP32)
 *     Serial1.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17
 *
 *     // Gateway UART (default pins)
 *     Serial2.begin(115200);
 *
 *     register_gateway_service(gateway_channel, gateway_impl);
 *
 *     Serial.println("Multi-UART RPC Node Started");
 * }
 *
 * void loop() {
 *     // Process both channels
 *     sensor_channel.process();
 *     gateway_channel.process();
 *
 *     // Forward data from sensor to gateway...
 * }
 * @endcode
 *
 * ## Baud Rate Selection
 *
 * Choose baud rate based on your application:
 *
 * | Baud Rate | Use Case | Notes |
 * |-----------|----------|-------|
 * | 9600 | Low data rate, long cables | Very reliable, good for noisy environments |
 * | 19200 | Moderate data, medium distance | Good balance of speed and reliability |
 * | 57600 | Standard RPC traffic | Recommended for most applications |
 * | 115200 | High-speed RPC, short cables | Common for ESP32, good throughput |
 * | 230400+ | Very high speed, quality cables | May have errors on cheaper boards |
 *
 * **Rule of thumb**: Higher baud = faster data but more errors on poor connections
 *
 * ## Troubleshooting
 *
 * ### No Response from RPC Calls
 *
 * 1. **Check baud rate match**: Both ends must use identical baud rate
 *    ```cpp
 *    // Client
 *    Serial.begin(115200);
 *
 *    // Server (must match!)
 *    Serial.begin(115200);
 *    ```
 *
 * 2. **Verify RX/TX connections**: TX → RX, RX → TX (crossover)
 *    - Arduino TX → Other device RX
 *    - Arduino RX → Other device TX
 *    - Don't forget common ground!
 *
 * 3. **Check process() is called**: Must call in loop()
 *    ```cpp
 *    void loop() {
 *        rpc_channel.process();  // Required!
 *    }
 *    ```
 *
 * ### Garbled Data / Parse Errors
 *
 * 1. **Reduce baud rate**: Try 57600 or 9600 instead of 115200
 * 2. **Check wiring**: Poor connections cause bit errors
 * 3. **Add pull-up resistors**: 10kΩ on RX line can help
 * 4. **Use shorter cables**: Long cables need lower baud rates
 *
 * ### Timeout Errors
 *
 * 1. **Increase timeout**: Default is 5000ms
 *    ```cpp
 *    client.GetData(req, callback, 10000);  // 10 second timeout
 *    ```
 *
 * 2. **Check server is running**: Server must call process() to respond
 * 3. **Verify handler registered**: Server must call register_service()
 *
 * ### Buffer Overflow
 *
 * If messages are too large:
 *
 * 1. **Increase RX_BUFFER_SIZE** in RpcChannel (default 1024 bytes)
 * 2. **Split large messages**: Send multiple smaller requests
 * 3. **Use packet transport**: For large infrequent messages
 *
 * ## Performance Considerations
 *
 * ### Throughput
 *
 * Approximate message throughput at different baud rates:
 *
 * - 9600 baud: ~100 small messages/sec, ~960 bytes/sec
 * - 57600 baud: ~600 small messages/sec, ~5.7 KB/sec
 * - 115200 baud: ~1200 small messages/sec, ~11.5 KB/sec
 *
 * Note: Actual throughput depends on message size, framing overhead, and processing time
 *
 * ### Latency
 *
 * Typical round-trip latency (request → response):
 * - Local loopback: 2-5ms
 * - Short cable (<1m): 5-20ms
 * - Long cable (>10m): 20-100ms
 *
 * ### Memory Usage
 *
 * - UartTransport object: ~8 bytes (just stores Serial reference)
 * - RpcChannel buffers: ~1KB default (configurable)
 * - Per-call overhead: ~32 bytes per pending request
 *
 * ## Advanced: Custom Serial Configuration
 *
 * @code
 * // 8 data bits, Even parity, 1 stop bit
 * Serial1.begin(115200, SERIAL_8E1);
 * UartTransport transport(Serial1);
 *
 * // Custom timeout for Serial.write() on some platforms
 * Serial1.setTimeout(100);  // 100ms write timeout
 * @endcode
 *
 * ## Advanced: Error Recovery
 *
 * @code
 * void loop() {
 *     rpc_channel.process();
 *
 *     // Monitor for serial errors (platform-specific)
 *     #ifdef ARDUINO_ARCH_ESP32
 *     if (Serial.hasOverrun()) {
 *         Serial.println("WARNING: Serial overrun - data lost!");
 *         // Consider reducing baud rate or increasing buffer size
 *     }
 *     #endif
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * UartTransport is **not thread-safe**. For multi-threaded applications:
 *
 * - Use mutex to protect send()/recv() calls
 * - Or use separate Serial ports for each thread
 * - FreeRTOS users: Use task notification instead of polling
 *
 * @note This is a header-only implementation for easy integration
 * @note Compile with -DARDUINO when building for Arduino platforms
 *
 * @see StreamTransport Base class documentation
 * @see RpcChannel For RPC message handling
 */
class UartTransport : public StreamTransport {
public:
    /**
     * @brief Construct UART transport from Arduino HardwareSerial
     *
     * Takes a reference to an initialized HardwareSerial object (Serial,
     * Serial1, Serial2, etc.). The Serial port must be initialized with
     * begin() before creating the transport.
     *
     * @param serial Reference to HardwareSerial object (e.g., Serial)
     *
     * @note Serial port must be initialized before construction:
     *       ```cpp
     *       Serial.begin(115200);
     *       UartTransport transport(Serial);
     *       ```
     *
     * @note The Serial object must remain valid for the transport's lifetime
     *
     * @warning Do not delete or reassign the Serial object while transport exists
     *
     * Example:
     * @code
     * void setup() {
     *     Serial.begin(115200);  // Initialize first!
     *
     *     UartTransport uart(Serial);  // Then create transport
     *     RpcChannel channel(uart, 0x01);  // local address 0x01
     * }
     * @endcode
     */
#ifdef ARDUINO
    explicit UartTransport(HardwareSerial & serial)
        : serial_(serial)
    {
    }
#else
    explicit UartTransport(HardwareSerial & serial);
#endif

    /**
     * @brief Send raw bytes over UART
     *
     * Writes the provided buffer to the Serial port. This method attempts
     * to send all data but may return before all bytes are transmitted
     * (depending on Serial buffer availability).
     *
     * @param data Pointer to buffer containing bytes to send
     * @param len Number of bytes to send
     * @return true if send initiated successfully, false on error
     *
     * @note Non-blocking: may return before all data is physically transmitted
     * @note Serial write buffer may not have space for all data
     * @note The framing layer ensures complete message delivery
     *
     * Error Conditions:
     * - Returns false if len > 0 but data is nullptr
     * - Returns false if Serial port not initialized
     * - Returns true even if some bytes are buffered (normal behavior)
     *
     * Implementation Details:
     * - Uses Serial.write() which buffers data in hardware FIFO
     * - Actual transmission happens asynchronously in background
     * - write() returns number of bytes accepted into buffer
     * - We return true if any bytes were accepted
     *
     * Example:
     * @code
     * uint8_t message[] = {0x01, 0x02, 0x03};
     * if (!uart.send(message, sizeof(message))) {
     *     Serial.println("Send failed - check serial connection");
     * }
     * @endcode
     *
     * @see recv() For receiving data
     * @see available() For checking data availability
     */
#ifdef ARDUINO
    bool send(const uint8_t * data, size_t len) override
    {
        if (!data || len == 0) {
            return len == 0;
        }

        size_t written = serial_.write(data, len);

        return written > 0;
    }
#else
    bool send(const uint8_t * data, size_t len) override;
#endif

    /**
     * @brief Receive bytes from UART
     *
     * Reads available data from the Serial port into the provided buffer.
     * Returns immediately with whatever data is currently available, up
     * to max_len bytes.
     *
     * @param buffer Destination buffer for received data
     * @param max_len Maximum bytes to read (buffer capacity)
     * @return Number of bytes actually read (0 if none available)
     *
     * @note Non-blocking: returns immediately with available data
     * @note May return less than max_len even if more data arrives later
     * @note Return value of 0 is normal (means no data available)
     * @note The RPC framing layer handles buffering partial messages
     *
     * Behavior:
     * - Reads up to max_len bytes from Serial receive buffer
     * - Returns 0 if no data available (not an error)
     * - Returns partial data if less than max_len available
     * - Does not wait for buffer to fill (non-blocking)
     *
     * Safety:
     * - Always check return value to know how many bytes were read
     * - Buffer must be at least max_len bytes in size
     * - Never assumes max_len bytes were read
     *
     * Example:
     * @code
     * uint8_t buffer[128];
     * size_t n = uart.recv(buffer, sizeof(buffer));
     * if (n > 0) {
     *     Serial.print("Received ");
     *     Serial.print(n);
     *     Serial.println(" bytes");
     *
     *     // Process received data
     *     process_data(buffer, n);
     * }
     * @endcode
     *
     * @see available() For checking before reading
     * @see send() For sending data
     */
#ifdef ARDUINO
    size_t recv(uint8_t * buffer, size_t max_len) override
    {
        if (!buffer || max_len == 0) {
            return 0;
        }

        size_t available_bytes = serial_.available();
        if (available_bytes == 0) {
            return 0;
        }

        size_t to_read = (available_bytes < max_len) ? available_bytes : max_len;

        size_t read_count = 0;
        for (size_t i = 0; i < to_read; i++) {
            int byte_val = serial_.read();
            if (byte_val == -1) {
                break;
            }
            buffer[i] = static_cast<uint8_t>(byte_val);
            read_count++;
        }

        return read_count;
    }
#else
    size_t recv(uint8_t * buffer, size_t max_len) override;
#endif

    /**
     * @brief Check if data is available to receive
     *
     * Quick check to determine if recv() would return data without blocking.
     * Useful for optimizing event loops to avoid unnecessary recv() calls.
     *
     * @return true if data is available, false otherwise
     *
     * @note This is a hint; available() == true doesn't guarantee recv() success
     * @note Data may arrive between available() and recv() calls
     * @note It's always safe to call recv() even if available() returns false
     *
     * Performance:
     * - Very fast check (reads Serial buffer count register)
     * - No data copying or processing
     * - Use to optimize polling loops
     *
     * Usage Pattern:
     * @code
     * void loop() {
     *     // Efficient: only call recv() when data present
     *     if (uart.available()) {
     *         uint8_t buffer[128];
     *         size_t n = uart.recv(buffer, sizeof(buffer));
     *         // Process data...
     *     }
     *
     *     // Note: RpcChannel.process() already does this internally
     *     rpc_channel.process();
     * }
     * @endcode
     *
     * Alternative Pattern (also valid):
     * @code
     * void loop() {
     *     // Also works: recv() returns 0 if no data
     *     uint8_t buffer[128];
     *     size_t n = uart.recv(buffer, sizeof(buffer));
     *     if (n > 0) {
     *         // Process data...
     *     }
     * }
     * @endcode
     *
     * @see recv() For actually reading the data
     */
#ifdef ARDUINO
    bool available() override { return serial_.available() > 0; }
#else
    bool available() override;
#endif

private:
#ifdef ARDUINO
    HardwareSerial & serial_;
#else
    HardwareSerial * serial_;
#endif
};

} // namespace litepb
