#pragma once

#include "litepb/rpc/transport.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace litepb {

/**
 * @brief UDP socket transport implementation for native platforms
 *
 * UdpTransport provides a PacketTransport implementation using standard
 * UDP sockets (POSIX). Designed for desktop applications, servers,
 * and native embedded Linux systems requiring datagram-based communication.
 *
 * This transport is designed for:
 * - Linux systems (Ubuntu, Debian, Raspberry Pi OS, etc.)
 * - macOS (Darwin)
 * - Embedded Linux platforms (Raspberry Pi, BeagleBone, OpenWrt)
 *
 * ## Key Features
 *
 * - **Header-only**: No compilation required, just include and use
 * - **POSIX-compliant**: Works on Linux and macOS
 * - **Non-blocking I/O**: Uses select() for efficient data checking
 * - **Error handling**: Comprehensive errno checks
 * - **Packet-based**: Preserves message boundaries (no framing layer needed)
 * - **Bidirectional**: Stores remote address for client/server communication
 *
 * ## Packet vs Stream Transport
 *
 * Unlike TCP (StreamTransport), UDP (PacketTransport) preserves message boundaries:
 *
 * | Characteristic | TCP (Stream) | UDP (Packet) |
 * |----------------|--------------|--------------|
 * | Message boundaries | Lost (requires framing) | Preserved (inherent) |
 * | Delivery guarantee | Guaranteed, ordered | Best-effort, unordered |
 * | Overhead | Length prefix per message | None (size implicit) |
 * | Connection state | Stateful (connected) | Stateless (connectionless) |
 * | Max message size | Unlimited (stream) | ~1400 bytes (MTU limit) |
 * | Use case | Reliable, large data | Low latency, small messages |
 *
 * **When to use UDP**:
 * - Low latency is critical (gaming, real-time control)
 * - Small, periodic messages (sensor data, telemetry)
 * - Broadcast/multicast communication
 * - Network discovery protocols
 * - Lossy networks where retries are application-specific
 *
 * **When to use TCP**:
 * - Large data transfers (files, bulk data)
 * - Guaranteed delivery required
 * - Ordered message processing needed
 * - Internet communication (NAT traversal)
 *
 * ## Platform Requirements
 *
 * - POSIX-compliant system
 * - C++11 or later
 * - No external dependencies (uses standard sockets API)
 *
 * ## Usage Example - UDP Client
 *
 * @code
 * #include "litepb/rpc/channel.h"
 * #include "examples/rpc/udp_transport.h"
 * #include "my_service.pb.h"
 * #include <iostream>
 * #include <cstring>
 *
 * using namespace litepb;
 *
 * // Helper function to create UDP client socket
 * int create_udp_client(const char* server_host, int server_port) {
 *     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 *     if (sockfd < 0) {
 *         std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
 *         return -1;
 *     }
 *
 *     // Set non-blocking mode
 *     int flags = fcntl(sockfd, F_GETFL, 0);
 *     fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
 *
 *     // Prepare server address
 *     struct sockaddr_in server_addr;
 *     memset(&server_addr, 0, sizeof(server_addr));
 *     server_addr.sin_family = AF_INET;
 *     server_addr.sin_port = htons(server_port);
 *     inet_pton(AF_INET, server_host, &server_addr.sin_addr);
 *
 *     // "Connect" the UDP socket (sets default destination)
 *     // Note: This doesn't establish a connection, just sets the default peer
 *     connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
 *
 *     return sockfd;
 * }
 *
 * int main() {
 *     // Create UDP socket connected to peer (UDP client socket operation)
 *     int sockfd = create_udp_client("127.0.0.1", 8080);
 *     if (sockfd < 0) {
 *         std::cerr << "Failed to create UDP socket" << std::endl;
 *         return 1;
 *     }
 *
 *     std::cout << "UDP RPC Peer ready (remote peer: 127.0.0.1:8080)" << std::endl;
 *
 *     // Create transport and RPC channel
 *     UdpTransport udp_transport(sockfd);
 *     RpcChannel rpc_channel(udp_transport, 0x01);  // local address 0x01
 *     MyServiceClient client(rpc_channel);  // Convenience wrapper for calling RPCs
 *
 *     // Main event loop
 *     bool running = true;
 *     int attempts = 0;
 *     const int max_attempts = 3;
 *
 *     while (running && attempts < max_attempts) {
 *         // Process incoming RPC messages and handle timeouts
 *         rpc_channel.process();
 *
 *         // Make an RPC call (first attempt)
 *         if (attempts == 0) {
 *             MyRequest request;
 *             request.sensor_id = 42;
 *
 *             std::cout << "Sending RPC request (attempt " << (attempts + 1) << ")..." << std::endl;
 *
 *             client.GetData(request, [&](const Result<MyResponse>& result) {
 *                 if (result.ok()) {
 *                     std::cout << "Temperature: " << result.value.temperature << std::endl;
 *                     std::cout << "Humidity: " << result.value.humidity << std::endl;
 *                     running = false;  // Success, exit
 *                 } else {
 *                     std::cerr << "RPC Error: " << result.error.message << std::endl;
 *                     attempts++;
 *
 *                     // UDP packets can be lost, retry on timeout
 *                     if (attempts < max_attempts) {
 *                         std::cout << "Retrying..." << std::endl;
 *                     } else {
 *                         std::cerr << "Max retry attempts reached" << std::endl;
 *                         running = false;
 *                     }
 *                 }
 *             });
 *
 *             attempts++;  // Increment after sending first request
 *         }
 *
 *         // Small delay to prevent busy-waiting
 *         usleep(1000);
 *     }
 *
 *     // Cleanup
 *     close(sockfd);
 *
 *     return 0;
 * }
 * @endcode
 *
 * ## Usage Example - UDP Server
 *
 * @code
 * #include "litepb/rpc/channel.h"
 * #include "examples/rpc/udp_transport.h"
 * #include "my_service.pb.h"
 * #include <iostream>
 * #include <cstring>
 *
 * using namespace litepb;
 *
 * // Implement the service interface
 * class MyServiceImpl : public MyServiceServer {
 * public:
 *     Result<MyResponse> GetData(const MyRequest& req) override {
 *         MyResponse response;
 *
 *         std::cout << "Received request for sensor " << req.sensor_id << std::endl;
 *
 *         if (req.sensor_id == 42) {
 *             response.temperature = 23.5;
 *             response.humidity = 65.2;
 *
 *             return {response, RpcError{RpcError::OK}};
 *         } else {
 *             return {response, RpcError{RpcError::CUSTOM_ERROR, "Invalid sensor ID"}};
 *         }
 *     }
 * };
 *
 * // Helper function to create UDP server socket
 * int create_udp_server(int port) {
 *     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 *     if (sockfd < 0) {
 *         return -1;
 *     }
 *
 *     // Set non-blocking mode
 *     int flags = fcntl(sockfd, F_GETFL, 0);
 *     fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
 *
 *     // Allow address reuse
 *     int opt = 1;
 *     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
 *
 *     // Bind to port
 *     struct sockaddr_in server_addr;
 *     memset(&server_addr, 0, sizeof(server_addr));
 *     server_addr.sin_family = AF_INET;
 *     server_addr.sin_addr.s_addr = INADDR_ANY;
 *     server_addr.sin_port = htons(port);
 *
 *     if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
 *         close(sockfd);
 *         return -1;
 *     }
 *
 *     return sockfd;
 * }
 *
 * int main() {
 *     // Create UDP server socket (UDP server socket operation - bind to port)
 *     int sockfd = create_udp_server(8080);
 *     if (sockfd < 0) {
 *         std::cerr << "Failed to create UDP server socket" << std::endl;
 *         return 1;
 *     }
 *
 *     std::cout << "UDP RPC Peer listening on port 8080..." << std::endl;
 *
 *     // Create transport and RPC channel (this peer has address 0x02)
 *     MyServiceImpl service_impl;
 *     UdpTransport udp_transport(sockfd);
 *     RpcChannel rpc_channel(udp_transport, 0x02);  // local address 0x02
 *
 *     // Register service handler
 *     register_my_service(rpc_channel, service_impl);
 *
 *     // Main event loop
 *     std::cout << "Processing RPC requests..." << std::endl;
 *     while (true) {
 *         // Process incoming requests and send responses
 *         rpc_channel.process();
 *
 *         // Small delay to prevent busy-waiting
 *         usleep(1000);
 *     }
 *
 *     // Cleanup (unreachable in this example)
 *     close(sockfd);
 *
 *     return 0;
 * }
 * @endcode
 *
 * ## MTU and Datagram Size Limits
 *
 * UDP packets have size constraints based on the Maximum Transmission Unit (MTU):
 *
 * ### Typical MTU Values
 *
 * | Network Type | MTU | Safe UDP Payload |
 * |--------------|-----|------------------|
 * | Ethernet | 1500 bytes | 1472 bytes |
 * | WiFi | 1500 bytes | 1472 bytes |
 * | PPPoE (DSL) | 1492 bytes | 1464 bytes |
 * | VPN (typical) | 1400 bytes | 1372 bytes |
 * | Internet (safe) | 576 bytes | 548 bytes |
 * | LoRaWAN | 51-242 bytes | 40-230 bytes |
 *
 * **Calculation**: UDP Payload = MTU - IP Header (20) - UDP Header (8)
 *
 * ### Recommended Limits
 *
 * - **LAN only**: Use up to 1400 bytes (safe for most local networks)
 * - **Internet**: Use up to 512 bytes (avoids fragmentation)
 * - **Critical systems**: Use up to 256 bytes (maximizes reliability)
 * - **LoRa/IoT**: Use up to 200 bytes (respects device limits)
 *
 * ### Fragmentation Warning
 *
 * If your message exceeds the network's MTU:
 * - IP fragmentation occurs (kernel splits packet)
 * - If ANY fragment is lost, entire message is lost
 * - Significantly increases packet loss probability
 * - Some networks block fragmented packets (security)
 *
 * **Best Practice**: Keep RPC messages under 1400 bytes for LAN, 512 for Internet
 *
 * @code
 * // Check message size before sending
 * MyRequest request;
 * // ... populate request ...
 *
 * BufferOutputStream temp_stream;
 * if (litepb::serialize(request, temp_stream)) {
 *     if (temp_stream.size() > 1400) {
 *         std::cerr << "WARNING: Message too large for UDP ("
 *                   << temp_stream.size() << " bytes)" << std::endl;
 *         // Consider splitting into multiple messages or using TCP
 *     }
 * }
 * @endcode
 *
 * ## Packet Loss and Reliability
 *
 * UDP does not guarantee delivery. Handle packet loss in your application:
 *
 * ### Retry Strategy
 *
 * @code
 * const int MAX_RETRIES = 3;
 * const int RETRY_TIMEOUT_MS = 2000;
 *
 * void send_with_retry(RpcChannel& channel, MyServiceClient& client) {
 *     for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
 *         bool received_response = false;
 *
 *         MyRequest request;
 *         request.sensor_id = 42;
 *
 *         client.GetData(request, [&](const Result<MyResponse>& result) {
 *             if (result.ok()) {
 *                 std::cout << "Success: " << result.value.temperature << std::endl;
 *                 received_response = true;
 *             }
 *         }, RETRY_TIMEOUT_MS);
 *
 *         // Wait for response or timeout
 *         auto start = std::chrono::steady_clock::now();
 *         while (!received_response) {
 *             channel.process();
 *
 *             auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
 *                 std::chrono::steady_clock::now() - start
 *             ).count();
 *
 *             if (elapsed > RETRY_TIMEOUT_MS) {
 *                 std::cout << "Timeout on attempt " << (attempt + 1) << std::endl;
 *                 break;
 *             }
 *
 *             usleep(1000);
 *         }
 *
 *         if (received_response) {
 *             return;  // Success
 *         }
 *     }
 *
 *     std::cerr << "Failed after " << MAX_RETRIES << " attempts" << std::endl;
 * }
 * @endcode
 *
 * ### Expected Packet Loss Rates
 *
 * - **Localhost**: 0% (reliable, no network involved)
 * - **Wired LAN**: 0.01-0.1% (very reliable)
 * - **WiFi (good)**: 0.1-1% (generally reliable)
 * - **WiFi (crowded)**: 1-10% (noticeable loss)
 * - **Internet**: 1-5% (variable, depends on route)
 * - **Cellular**: 5-20% (high loss, especially during handoff)
 *
 * ## Socket Configuration Best Practices
 *
 * ### Non-Blocking I/O Setup
 *
 * Always configure sockets as non-blocking before passing to UdpTransport:
 *
 * @code
 * int flags = fcntl(sockfd, F_GETFL, 0);
 * fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
 * @endcode
 *
 * ### Receive Buffer Size
 *
 * Increase buffer to prevent packet drops during bursts:
 *
 * @code
 * int recvbuf = 256 * 1024;  // 256KB receive buffer
 * setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&recvbuf, sizeof(recvbuf));
 * @endcode
 *
 * ### Broadcast Support
 *
 * Enable broadcast for discovery protocols:
 *
 * @code
 * int broadcast = 1;
 * setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
 *
 * // Send to broadcast address
 * struct sockaddr_in broadcast_addr;
 * broadcast_addr.sin_family = AF_INET;
 * broadcast_addr.sin_port = htons(8080);
 * broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
 * @endcode
 *
 * ### Multicast Support
 *
 * Join multicast group for one-to-many communication:
 *
 * @code
 * struct ip_mreq mreq;
 * mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");  // Multicast group
 * mreq.imr_interface.s_addr = INADDR_ANY;
 * setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
 * @endcode
 *
 * ## Troubleshooting
 *
 * ### No Response from Server
 *
 * 1. **Check firewall**: UDP may be blocked
 *    ```bash
 *    # Linux (ufw)
 *    sudo ufw allow 8080/udp
 *
 *    # Check if port is open
 *    nc -u -z 127.0.0.1 8080
 *    ```
 *
 * 2. **Verify socket is bound**: Server must bind before receiving
 *    ```cpp
 *    if (bind(sockfd, ...) < 0) {
 *        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
 *    }
 *    ```
 *
 * 3. **Check process() is called**: Must call in loop (both client and server)
 *    ```cpp
 *    while (true) {
 *        rpc_channel.process();  // Required!
 *    }
 *    ```
 *
 * ### Packets Being Dropped
 *
 * 1. **Reduce message size**: Stay under MTU (1400 bytes for LAN)
 *    ```cpp
 *    // Split large messages
 *    if (payload_size > 1400) {
 *        // Use TCP or implement chunking
 *    }
 *    ```
 *
 * 2. **Increase receive buffer**: Prevent kernel drops
 *    ```cpp
 *    int recvbuf = 512 * 1024;  // 512KB
 *    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&recvbuf, sizeof(recvbuf));
 *    ```
 *
 * 3. **Check for buffer overrun**: Monitor dropped packets
 *    ```bash
 *    # Linux: Check UDP statistics
 *    netstat -su | grep -i drop
 *    cat /proc/net/snmp | grep Udp
 *    ```
 *
 * ### "Address Already in Use" Error
 *
 * 1. **Enable SO_REUSEADDR**: Allows immediate rebind
 *    ```cpp
 *    int opt = 1;
 *    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
 *    ```
 *
 * 2. **Check for existing process**:
 *    ```bash
 *    # Linux/macOS
 *    lsof -i UDP:8080
 *
 *    # Windows
 *    netstat -ano | findstr :8080
 *    ```
 *
 * ### Connection Refused (ICMP Port Unreachable)
 *
 * Unlike TCP, UDP doesn't have connections, but you may see errors:
 *
 * 1. **Server not running**: Start server before sending packets
 * 2. **Wrong port**: Verify client sends to correct port
 * 3. **ICMP errors**: Some systems return "port unreachable" on recv()
 *    ```cpp
 *    // This is normal for UDP, just means server wasn't listening
 *    // Retry or check server status
 *    ```
 *
 * ## Performance Considerations
 *
 * ### Throughput
 *
 * Typical message throughput over UDP (local network):
 *
 * - Localhost (127.0.0.1): ~200,000 small packets/sec, ~100 MB/sec
 * - LAN (1 Gbps): ~100,000 packets/sec, ~800 MB/sec (limited by CPU)
 * - WAN (100 Mbps): ~10,000 packets/sec, ~10 MB/sec
 *
 * Note: UDP throughput higher than TCP due to lower overhead
 *
 * ### Latency
 *
 * Typical one-way latency (lower than TCP due to no handshake):
 * - Localhost: 0.01-0.05ms
 * - LAN: 0.2-1ms
 * - WAN (nearby): 2-30ms
 * - WAN (intercontinental): 50-200ms
 *
 * ### Memory Usage
 *
 * - UdpTransport object: ~20 bytes (socket fd + remote address)
 * - RpcChannel buffers: ~1KB default
 * - OS socket buffers: ~200KB per socket (configurable)
 * - Per-packet overhead: 28 bytes (IP + UDP headers)
 *
 * ### CPU Usage
 *
 * - select() with 1ms timeout: ~0.1% CPU (idle)
 * - Busy processing: Lower than TCP (no connection state)
 * - High packet rate: Consider batch processing with recvmmsg() (Linux)
 *
 * ## Advanced: NAT Traversal and Hole Punching
 *
 * UDP can work through NAT with hole punching:
 *
 * @code
 * // Client behind NAT sends packet to server (creates hole in NAT)
 * struct sockaddr_in server_addr;
 * server_addr.sin_family = AF_INET;
 * server_addr.sin_port = htons(8080);
 * inet_pton(AF_INET, "public.server.ip", &server_addr.sin_addr);
 *
 * // Send keepalive to maintain NAT mapping
 * const char keepalive = 0;
 * sendto(sockfd, &keepalive, 1, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
 *
 * // Server can now send packets back through the NAT hole
 * // NAT hole typically lasts 30-60 seconds, so send periodic keepalives
 * @endcode
 *
 * ## Advanced: Error Detection
 *
 * @code
 * // Enable extended error messages
 * int on = 1;
 * setsockopt(sockfd, SOL_IP, IP_RECVERR, &on, sizeof(on));
 *
 * // Check for errors after send/recv
 * if (send_result < 0 && errno == EMSGSIZE) {
 *     std::cerr << "Message too large for UDP MTU" << std::endl;
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * UdpTransport is **not thread-safe**. For multi-threaded applications:
 *
 * - Use mutex to protect send()/recv() calls on same socket
 * - Or use separate sockets for each thread
 * - Or use a dedicated I/O thread with message queue
 *
 * UDP is stateless, so multiple threads can safely use different sockets
 * bound to different ports.
 */
class UdpTransport : public PacketTransport
{
public:
    /**
     * @brief Construct UDP transport from existing socket
     *
     * Takes ownership of socket communication (but not lifecycle).
     * Socket must be created, configured (non-blocking), and optionally
     * bound (server) or connected (client) before passing to this constructor.
     *
     * @param sockfd UDP socket file descriptor (SOCK_DGRAM)
     *
     * @note Socket must be non-blocking (set O_NONBLOCK)
     * @note Server sockets should be bound before construction
     * @note Client sockets may use connect() to set default peer
     * @note Caller is responsible for closing socket after transport destruction
     */
    explicit UdpTransport(int sockfd) : sockfd_(sockfd), has_remote_addr_(false) { memset(&remote_addr_, 0, sizeof(remote_addr_)); }

    /**
     * @brief Send bytes as a UDP datagram
     *
     * Sends the entire buffer as a single UDP packet. For connected sockets,
     * uses the default peer. For unconnected sockets, uses the last received
     * remote address (set by recv_packet).
     *
     * @param data Pointer to data to send
     * @param len Length of data (must be <= MTU, typically <= 1400 bytes)
     * @return true if packet sent successfully, false on error
     *
     * @note Sends entire buffer atomically or fails (no partial sends)
     * @note Exceeding MTU (~1400 bytes) causes fragmentation or failure
     * @note Does not guarantee delivery (UDP best-effort)
     * @note For unconnected sockets, requires prior recv() to learn peer address
     */
    bool send(const uint8_t* data, size_t len) override { return send_packet(data, len); }

    /**
     * @brief Receive bytes from a UDP datagram
     *
     * Receives one complete UDP packet. Updates internal remote address
     * for bidirectional communication on unconnected sockets.
     *
     * @param buffer Destination buffer for received data
     * @param max_len Maximum bytes to receive (buffer capacity)
     * @return Number of bytes received (0 if no packet available)
     *
     * @note Always returns a complete packet or 0 (never partial data)
     * @note Packets larger than max_len are truncated (data lost)
     * @note Updates remote_addr_ for subsequent send() calls
     * @note Non-blocking: returns 0 immediately if no data available
     */
    size_t recv(uint8_t* buffer, size_t max_len) override { return recv_packet(buffer, max_len); }

    /**
     * @brief Check if data is available to receive
     *
     * Uses select() to check if a UDP packet is waiting in the socket buffer.
     * Non-blocking check suitable for event loops.
     *
     * @return true if packet available, false otherwise
     *
     * @note Does not guarantee recv() will succeed (race condition possible)
     * @note Zero timeout select() for minimal latency
     */
    bool available() override
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd_, &read_fds);

        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;

        int result = select(sockfd_ + 1, &read_fds, NULL, NULL, &timeout);
        return result > 0;
    }

    /**
     * @brief Receive one complete UDP datagram
     *
     * Receives exactly one UDP packet using recvfrom(). Stores the sender's
     * address for bidirectional communication.
     *
     * @param buffer Destination buffer for packet data
     * @param max_len Maximum packet size (buffer capacity)
     * @return Number of bytes in packet (0 if no packet available)
     *
     * @note Returns complete packet or 0 (never partial data)
     * @note Packets > max_len are truncated (MSG_TRUNC on some systems)
     * @note Updates remote_addr_ for send() calls
     */
    size_t recv_packet(uint8_t* buffer, size_t max_len) override
    {
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        ssize_t bytes = recvfrom(sockfd_, buffer, max_len, 0, (struct sockaddr*) &from_addr, &from_len);

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNREFUSED) {
                return 0;
            }
            return 0;
        }

        if (bytes > 0) {
            remote_addr_     = from_addr;
            has_remote_addr_ = true;
        }

        return static_cast<size_t>(bytes);
    }

    /**
     * @brief Send one complete UDP datagram
     *
     * Sends the entire buffer as a single atomic UDP packet. For connected
     * sockets, uses send(). For unconnected sockets, uses sendto() with
     * the remote address from the last received packet.
     *
     * @param data Pointer to packet data
     * @param len Size of packet (should be <= 1400 bytes for reliability)
     * @return true if entire packet sent, false on error
     *
     * @note Sends atomically or fails (no partial sends)
     * @note For unconnected sockets without prior recv(), send will fail
     * @note Exceeding MTU causes fragmentation (unreliable) or error
     */
    bool send_packet(const uint8_t* data, size_t len) override
    {
        if (!has_remote_addr_) {
            ssize_t bytes = ::send(sockfd_, data, len, 0);

            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return false;
                }
                return false;
            }

            return bytes == static_cast<ssize_t>(len);
        }
        else {
            ssize_t bytes = sendto(sockfd_, data, len, 0, (struct sockaddr*) &remote_addr_, sizeof(remote_addr_));

            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EMSGSIZE) {
                    return false;
                }
                return false;
            }

            return bytes == static_cast<ssize_t>(len);
        }
    }

    /**
     * @brief Get the remote peer address
     *
     * Returns the address of the last peer that sent a packet to this socket.
     * Useful for logging and connection tracking.
     *
     * @return sockaddr_in structure with peer address (invalid if no packets received)
     */
    const struct sockaddr_in& remote_address() const { return remote_addr_; }

    /**
     * @brief Check if remote address is known
     *
     * @return true if at least one packet has been received (remote address known)
     */
    bool has_remote_address() const { return has_remote_addr_; }

private:
    int sockfd_;
    struct sockaddr_in remote_addr_;
    bool has_remote_addr_;
};

} // namespace litepb
