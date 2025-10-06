#ifndef LITEPB_RPC_TRANSPORT_H
#define LITEPB_RPC_TRANSPORT_H

#include <cstddef>
#include <cstdint>

namespace litepb {

/**
 * @brief Abstract transport layer interface for RPC communication
 *
 * Transport provides a common interface for sending and receiving bytes
 * across different physical layers (UART, TCP, UDP, SPI, LoRa, CAN, etc.).
 *
 * Concrete implementations must provide methods for:
 * - Sending raw byte buffers
 * - Receiving data into buffers
 * - Checking if data is available
 *
 * The Transport abstraction allows the RPC layer to remain independent
 * of the underlying communication mechanism.
 *
 * @note Implementations must be non-blocking to support async operation
 * @note recv() may return partial data; framing layer handles buffering
 */
class Transport
{
public:
    virtual ~Transport() = default;

    /**
     * @brief Send raw bytes over the transport with addressing
     *
     * Attempts to transmit the provided data buffer with transport-level addressing.
     * For stream-based transports (UART, TCP), this may send partial data.
     * For packet-based transports (UDP, LoRa), this should send the complete buffer or fail.
     *
     * @param data Pointer to buffer containing bytes to send
     * @param len Number of bytes to send
     * @param src_addr Source address for the message
     * @param dst_addr Destination address for the message
     * @param msg_id Message identifier for correlating responses
     * @return true if send initiated successfully, false on error
     *
     * @note Non-blocking: may return before all data is transmitted
     * @note Stream transports: may send partial data (check return value)
     * @note Packet transports: should send complete buffer atomically
     * @note Addressing is handled at the transport layer
     */
    virtual bool send(const uint8_t* data, size_t len, uint64_t src_addr, uint64_t dst_addr, uint16_t msg_id) = 0;

    /**
     * @brief Receive bytes from the transport with addressing
     *
     * Reads available data into the provided buffer and extracts transport-level addressing.
     * The amount of data returned depends on the transport type:
     * - Stream transports: May return any amount up to max_len
     * - Packet transports: May return less than a full packet if buffered
     *
     * @param buffer Destination buffer for received data
     * @param max_len Maximum bytes to read (buffer capacity)
     * @param src_addr Output: Source address from received message
     * @param dst_addr Output: Destination address from received message
     * @param msg_id Output: Message identifier from received message
     * @return Number of bytes actually read (0 if none available)
     *
     * @note Non-blocking: returns immediately with available data
     * @note Return value of 0 indicates no data available (not an error)
     * @note Caller must check return value to determine bytes received
     * @note Addressing information is extracted from transport layer
     */
    virtual size_t recv(uint8_t* buffer, size_t max_len, uint64_t& src_addr, uint64_t& dst_addr, uint16_t& msg_id) = 0;

    /**
     * @brief Check if data is available to receive
     *
     * Quick check to determine if recv() would return data without blocking.
     * Useful for event loop optimization to avoid unnecessary recv() calls.
     *
     * @return true if data is available, false otherwise
     *
     * @note This is a hint; available() == true doesn't guarantee recv() success
     * @note Some transports may always return true and rely on recv() == 0
     */
    virtual bool available() = 0;
};

/**
 * @brief Marker class for stream-based transports
 *
 * StreamTransport indicates that the underlying transport operates on
 * continuous byte streams without inherent message boundaries. Examples
 * include UART, TCP sockets, and Serial ports.
 *
 * Stream Characteristics:
 * - recv() may return partial data at any byte boundary
 * - Multiple send() calls may be coalesced by the transport
 * - No guarantee that send(n) followed by recv() returns exactly n bytes
 * - Requires length-delimited framing for message delimiting
 *
 * Buffering Requirements:
 * - The RPC framing layer will handle buffering incomplete messages
 * - recv() should return whatever data is immediately available
 * - No need to buffer until a complete message arrives
 * - The framing layer uses varint length prefixes to detect boundaries
 *
 * Implementation Example:
 * @code
 * class UartTransport : public StreamTransport {
 *     bool send(const uint8_t* data, size_t len) override {
 *         return uart_write(data, len) > 0;
 *     }
 *
 *     size_t recv(uint8_t* buffer, size_t max_len) override {
 *         return uart_read(buffer, max_len);  // Returns whatever is available
 *     }
 *
 *     bool available() override {
 *         return uart_available() > 0;
 *     }
 * };
 * @endcode
 *
 * @note Stream transports must NOT attempt to buffer complete messages
 * @note Leave message delimiting to the RPC framing layer
 */
class StreamTransport : public Transport
{
public:
    virtual ~StreamTransport() = default;
};

/**
 * @brief Abstract class for packet-based transports
 *
 * PacketTransport handles communication channels where data is transmitted
 * in discrete packets with inherent boundaries. Examples include UDP, LoRa,
 * CAN bus, and other datagram-based protocols.
 *
 * Packet Characteristics:
 * - Each recv_packet() call returns exactly one complete packet
 * - Each send_packet() call transmits exactly one atomic packet
 * - Packet boundaries are preserved by the transport layer
 * - No need for length-delimited framing (packet size is implicit)
 *
 * Message Delimiting:
 * - Unlike streams, packets are already delimited by the transport
 * - recv_packet() will never return partial packets
 * - The RPC layer can skip varint length encoding for packet payloads
 * - Message format: [msg_id:4][method_len:varint][method_name][protobuf_data]
 *   (no payload_len varint needed)
 *
 * Implementation Guidance:
 * - recv_packet() should block internally or return 0 if no complete packet
 * - send_packet() should send the entire buffer or fail atomically
 * - Max packet size is transport-dependent (UDP: ~1400, LoRa: ~255)
 * - Caller must ensure messages fit within transport's MTU
 *
 * Implementation Example:
 * @code
 * class UdpTransport : public PacketTransport {
 *     size_t recv_packet(uint8_t* buffer, size_t max_len) override {
 *         return udp_recvfrom(buffer, max_len);  // Returns full packet or 0
 *     }
 *
 *     bool send_packet(const uint8_t* data, size_t len) override {
 *         return udp_sendto(data, len) == len;  // All or nothing
 *     }
 *
 *     // Base Transport interface (often delegates to packet methods)
 *     bool send(const uint8_t* data, size_t len) override {
 *         return send_packet(data, len);
 *     }
 *
 *     size_t recv(uint8_t* buffer, size_t max_len) override {
 *         return recv_packet(buffer, max_len);
 *     }
 *
 *     bool available() override {
 *         return udp_pending() > 0;
 *     }
 * };
 * @endcode
 *
 * @note Each recv_packet() returns exactly one complete packet (never partial)
 * @note Each send_packet() transmits atomically (no fragmentation)
 * @note Implementations should respect transport MTU limits
 */
class PacketTransport : public Transport
{
public:
    virtual ~PacketTransport() = default;

    /**
     * @brief Receive one complete packet
     *
     * Reads exactly one complete packet from the transport into the buffer.
     * Unlike recv(), this method guarantees that the returned data represents
     * a complete, unfragmented packet as transmitted by the sender.
     *
     * @param buffer Destination buffer for packet data
     * @param max_len Maximum packet size (buffer capacity)
     * @return Number of bytes in the packet (0 if no packet available)
     *
     * @note Always returns a complete packet or 0 (never partial data)
     * @note Return value indicates actual packet size
     * @note If packet > max_len, behavior is implementation-defined (truncate/drop)
     */
    virtual size_t recv_packet(uint8_t* buffer, size_t max_len) = 0;

    /**
     * @brief Send one complete packet
     *
     * Transmits the entire buffer as a single atomic packet. The transport
     * ensures the receiver will get all bytes in one recv_packet() call,
     * preserving packet boundaries.
     *
     * @param data Pointer to packet data to send
     * @param len Size of packet in bytes
     * @return true if entire packet sent successfully, false on error
     *
     * @note Must send entire packet atomically or fail (no partial sends)
     * @note Caller must ensure len <= transport MTU
     * @note Exceeding MTU may cause silent drop or error (transport-dependent)
     */
    virtual bool send_packet(const uint8_t* data, size_t len) = 0;
};

} // namespace litepb

#endif
