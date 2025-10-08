/**
 * @file framing.h
 * @brief RPC message framing and transport layer integration
 *
 * This header defines the framing layer that sits between the RPC protocol
 * and the transport layer. It handles message boundaries for stream transports,
 * message ID generation for request/response correlation, and frame encoding/decoding.
 *
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#ifdef LITEPB_WITH_RPC

#include "rpc_protocol.pb.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace litepb {

class OutputStream;
class InputStream;

/**
 * @brief Generates unique message IDs for RPC request/response correlation
 *
 * MessageIdGenerator creates unique 16-bit message IDs used to match
 * RPC responses with their corresponding requests. It uses a simple
 * incrementing counter that wraps around, avoiding ID 0 which is
 * reserved for fire-and-forget events.
 *
 * @note Thread safety is not provided - use one generator per thread
 */
class MessageIdGenerator {
public:
    /**
     * @brief Default constructor initializes the counter
     */
    MessageIdGenerator();

    /**
     * @brief Generate a unique message ID
     *
     * Generates a non-zero message ID for request/response correlation.
     * The ID is unique within the scope of this generator instance.
     *
     * @param local_addr Source address (for future use in ID generation)
     * @param dst_addr Destination address (for future use in ID generation)
     * @return Unique non-zero message ID
     */
    uint16_t generate_for(uint64_t local_addr, uint64_t dst_addr);

private:
    uint16_t counter_; ///< Internal counter for ID generation
};

/**
 * @brief Transport frame structure
 *
 * Represents a frame ready for transmission over the transport layer.
 * Contains only the serialized RpcMessage payload - addressing and
 * message ID are handled by the transport layer itself.
 */
struct TransportFrame {
    std::vector<uint8_t> payload; ///< Serialized RpcMessage protobuf data
};

/**
 * @brief Encode a transport frame for transmission
 *
 * Encodes a transport frame for transmission over the wire. For stream
 * transports, adds a varint length prefix. For packet transports, writes
 * the payload directly.
 *
 * @param frame The frame to encode
 * @param output Output stream to write encoded data to
 * @param is_stream_transport true for stream transports (needs framing), false for packet
 * @return true if encoding succeeded, false on error
 *
 * @example Stream transport encoding
 * @code{.cpp}
 * TransportFrame frame;
 * frame.payload = serialized_message;
 *
 * BufferOutputStream output;
 * if (encode_transport_frame(frame, output, true)) {
 *     // Output contains: [length_varint][payload]
 *     uart_send(output.data(), output.size());
 * }
 * @endcode
 */
bool encode_transport_frame(const TransportFrame & frame, OutputStream & output, bool is_stream_transport);

/**
 * @brief Decode a transport frame from received data
 *
 * Decodes a transport frame from wire format. For stream transports,
 * reads the varint length prefix and extracts the payload. For packet
 * transports, reads all available data as the payload.
 *
 * @param input Input stream containing encoded frame data
 * @param frame Output frame to populate
 * @param is_stream_transport true for stream transports (has framing), false for packet
 * @return true if a complete frame was decoded, false if more data needed or error
 *
 * @example Stream transport decoding
 * @code{.cpp}
 * BufferInputStream input(received_data, received_size);
 * TransportFrame frame;
 *
 * if (decode_transport_frame(input, frame, true)) {
 *     // frame.payload contains the complete message
 *     rpc::RpcMessage msg;
 *     deserialize_rpc_message(frame.payload, msg);
 * }
 * @endcode
 */
bool decode_transport_frame(InputStream & input, TransportFrame & frame, bool is_stream_transport);

/**
 * @brief Serialize an RpcMessage to bytes
 *
 * Helper function to serialize an RpcMessage protobuf to a byte vector.
 *
 * @param msg The RpcMessage to serialize
 * @param output Output vector to store serialized bytes
 * @return true if serialization succeeded, false on error
 */
bool serialize_rpc_message(const rpc::RpcMessage & msg, std::vector<uint8_t> & output);

/**
 * @brief Deserialize an RpcMessage from bytes
 *
 * Helper function to deserialize an RpcMessage protobuf from a byte vector.
 *
 * @param input Input vector containing serialized bytes
 * @param msg Output RpcMessage to populate
 * @return true if deserialization succeeded, false on error
 */
bool deserialize_rpc_message(const std::vector<uint8_t> & input, rpc::RpcMessage & msg);

} // namespace litepb

#endif // LITEPB_WITH_RPC
