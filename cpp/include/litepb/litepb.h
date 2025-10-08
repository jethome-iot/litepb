/**
 * @file litepb.h
 * @brief Main header for LitePB - Lightweight Protocol Buffers for C++
 *
 * LitePB provides zero-dependency Protocol Buffers serialization for embedded systems
 * and native platforms. This header defines the core serialization API that works
 * with generated code to provide type-safe, efficient message encoding and decoding.
 *
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 *
 * @example Basic Serialization
 * @code{.cpp}
 * // Serialize a message
 * MyMessage msg;
 * msg.id = 123;
 * msg.name = "example";
 *
 * litepb::BufferOutputStream output;
 * if (litepb::serialize(msg, output)) {
 *     // Use output.data() and output.size()
 * }
 *
 * // Deserialize a message
 * litepb::BufferInputStream input(data, size);
 * MyMessage decoded;
 * if (litepb::parse(decoded, input)) {
 *     // Message successfully parsed
 * }
 * @endcode
 */

#pragma once

#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace litepb {

/**
 * @brief Template class for Protocol Buffers serialization
 *
 * The Serializer template is specialized by the code generator for each
 * Protocol Buffers message type. It provides the core serialization logic
 * that converts between C++ structs and wire format bytes.
 *
 * @tparam T The message type to serialize (generated from .proto files)
 *
 * @note This template is specialized by generated code. Users should not
 *       specialize this template manually.
 */
template <typename T>
class Serializer {
public:
    /**
     * @brief Serialize a message to an output stream
     *
     * @param msg The message to serialize
     * @param stream The output stream to write to
     * @return true if serialization succeeded, false on error
     */
    static bool serialize(const T & msg, OutputStream & stream);

    /**
     * @brief Parse a message from an input stream
     *
     * @param msg The message to populate (output parameter)
     * @param stream The input stream to read from
     * @return true if parsing succeeded, false on error
     */
    static bool parse(T & msg, InputStream & stream);

    /**
     * @brief Calculate the serialized size of a message
     *
     * @param msg The message to calculate size for
     * @return Number of bytes required to serialize the message
     */
    static size_t byte_size(const T & msg);
};

/**
 * @brief Serialize a Protocol Buffers message to a stream
 *
 * This is the main entry point for message serialization. It converts
 * a C++ message struct into Protocol Buffers wire format bytes.
 *
 * @tparam T The message type (automatically deduced)
 * @param msg The message to serialize
 * @param stream The output stream to write to
 * @return true if serialization succeeded, false on error
 *
 * @example
 * @code{.cpp}
 * Person person;
 * person.name = "Alice";
 * person.age = 30;
 *
 * litepb::BufferOutputStream output;
 * if (litepb::serialize(person, output)) {
 *     // Send output.data() over network or save to file
 *     send_bytes(output.data(), output.size());
 * }
 * @endcode
 */
template <typename T>
inline bool serialize(const T & msg, OutputStream & stream)
{
    return Serializer<T>::serialize(msg, stream);
}

/**
 * @brief Parse a Protocol Buffers message from a stream
 *
 * This is the main entry point for message deserialization. It converts
 * Protocol Buffers wire format bytes into a C++ message struct.
 *
 * @tparam T The message type (automatically deduced)
 * @param msg The message to populate (output parameter)
 * @param stream The input stream to read from
 * @return true if parsing succeeded, false on error
 *
 * @example
 * @code{.cpp}
 * // Receive bytes from network or file
 * uint8_t buffer[256];
 * size_t size = receive_bytes(buffer, sizeof(buffer));
 *
 * litepb::BufferInputStream input(buffer, size);
 * Person person;
 * if (litepb::parse(person, input)) {
 *     // Use the parsed message
 *     std::cout << "Name: " << person.name << std::endl;
 * }
 * @endcode
 */
template <typename T>
inline bool parse(T & msg, InputStream & stream)
{
    return Serializer<T>::parse(msg, stream);
}

/**
 * @brief Calculate the serialized size of a message in bytes
 *
 * This function calculates how many bytes are required to serialize
 * a message without actually performing the serialization. Useful for
 * pre-allocating buffers or checking size constraints.
 *
 * @tparam T The message type (automatically deduced)
 * @param msg The message to calculate size for
 * @return Number of bytes required to serialize the message
 *
 * @example
 * @code{.cpp}
 * MyMessage msg;
 * // ... populate message ...
 *
 * size_t size = litepb::byte_size(msg);
 * if (size > MAX_PACKET_SIZE) {
 *     // Message too large for packet
 * }
 * @endcode
 */
template <typename T>
inline size_t byte_size(const T & msg)
{
    return Serializer<T>::byte_size(msg);
}

} // namespace litepb
