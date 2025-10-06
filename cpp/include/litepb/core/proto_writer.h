/**
 * @file proto_writer.h
 * @brief Protocol Buffers wire format writer
 * 
 * This header defines the ProtoWriter class which handles low-level Protocol Buffers
 * wire format writing. It provides methods for writing various wire types including
 * varints, fixed-width values, and length-delimited data to output streams.
 * 
 * The writer handles all Protocol Buffers encoding rules including zigzag encoding
 * for signed integers and proper field tag generation.
 * 
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#include "litepb/core/streams.h"
#include <cstdint>
#include <string>

namespace litepb {

/**
 * @brief Protocol Buffers wire type enumeration
 * 
 * Defines the wire types used in Protocol Buffers encoding. Each field
 * in a message is tagged with one of these wire types to indicate how
 * the field's value is encoded on the wire.
 */
enum WireType
{
    WIRE_TYPE_VARINT           = 0,  ///< Variable-length integer (int32, int64, uint32, uint64, sint32, sint64, bool, enum)
    WIRE_TYPE_FIXED64          = 1,  ///< 64-bit fixed-width (fixed64, sfixed64, double)
    WIRE_TYPE_LENGTH_DELIMITED = 2,  ///< Length-delimited (string, bytes, embedded messages, packed repeated fields)
    WIRE_TYPE_START_GROUP      = 3,  ///< Start group (deprecated, not supported)
    WIRE_TYPE_END_GROUP        = 4,  ///< End group (deprecated, not supported)
    WIRE_TYPE_FIXED32          = 5   ///< 32-bit fixed-width (fixed32, sfixed32, float)
};

/**
 * @brief Low-level Protocol Buffers wire format writer
 * 
 * ProtoWriter provides methods for writing Protocol Buffers wire format data
 * to an output stream. It handles all standard Protocol Buffers wire types
 * and encoding schemes including varints, fixed-width values, and length-delimited data.
 * 
 * This class is used internally by generated code and typically should not be
 * used directly by application code.
 * 
 * @example Writing wire format data
 * @code{.cpp}
 * litepb::BufferOutputStream output;
 * litepb::ProtoWriter writer(output);
 * 
 * // Write field 1 as varint
 * writer.write_tag(1, WIRE_TYPE_VARINT);
 * writer.write_varint(123);
 * 
 * // Write field 2 as string
 * writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
 * writer.write_string("hello");
 * 
 * // Write field 3 as float
 * writer.write_tag(3, WIRE_TYPE_FIXED32);
 * writer.write_float(3.14f);
 * @endcode
 */
class ProtoWriter
{
    OutputStream& stream_;

public:
    /**
     * @brief Construct a ProtoWriter for the given output stream
     * @param stream The output stream to write to
     */
    explicit ProtoWriter(OutputStream& stream) : stream_(stream) {}

    /**
     * @brief Write a variable-length integer (varint)
     * 
     * Writes a base-128 encoded variable-length integer to the stream.
     * Used for int32, int64, uint32, uint64, bool, and enum types.
     * 
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_varint(uint64_t value);
    
    /**
     * @brief Write a 32-bit fixed-width value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_fixed32(uint32_t value);
    
    /**
     * @brief Write a 64-bit fixed-width value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_fixed64(uint64_t value);
    
    /**
     * @brief Write a signed 32-bit fixed-width value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_sfixed32(int32_t value);
    
    /**
     * @brief Write a signed 64-bit fixed-width value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_sfixed64(int64_t value);
    
    /**
     * @brief Write a 32-bit floating point value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_float(float value);
    
    /**
     * @brief Write a 64-bit floating point value
     * @param value The value to write
     * @return true if write succeeded, false on error
     */
    bool write_double(double value);

    /**
     * @brief Write length-delimited bytes
     * 
     * Writes a varint length prefix followed by the raw bytes.
     * 
     * @param data Pointer to the data to write
     * @param size Number of bytes to write
     * @return true if write succeeded, false on error
     */
    bool write_bytes(const uint8_t* data, size_t size);
    
    /**
     * @brief Write a length-delimited string
     * 
     * Writes a varint length prefix followed by the UTF-8 string data.
     * 
     * @param str The string to write
     * @return true if write succeeded, false on error
     */
    bool write_string(const std::string& str);

    /**
     * @brief Write a field tag (field number and wire type)
     * 
     * Writes an encoded field tag containing the field number and wire type.
     * This must be written before each field value.
     * 
     * @param field_number The field number (from .proto definition)
     * @param type The wire type for the field
     * @return true if write succeeded, false on error
     */
    bool write_tag(uint32_t field_number, WireType type);

    /**
     * @brief Write a zigzag-encoded signed 32-bit integer
     * 
     * Encodes the value using zigzag encoding for efficient representation
     * of negative numbers, then writes it as a varint.
     * 
     * @param value The value to encode and write
     * @return true if write succeeded, false on error
     */
    bool write_sint32(int32_t value);
    
    /**
     * @brief Write a zigzag-encoded signed 64-bit integer
     * 
     * Encodes the value using zigzag encoding for efficient representation
     * of negative numbers, then writes it as a varint.
     * 
     * @param value The value to encode and write
     * @return true if write succeeded, false on error
     */
    bool write_sint64(int64_t value);

    /**
     * @brief Calculate the serialized size of a varint
     * @param value The value to calculate size for
     * @return Number of bytes required to encode the varint
     */
    static size_t varint_size(uint64_t value);
    
    /**
     * @brief Get the serialized size of a fixed32 value
     * @return Always returns 4
     */
    static size_t fixed32_size() { return 4; }
    
    /**
     * @brief Get the serialized size of a fixed64 value
     * @return Always returns 8
     */
    static size_t fixed64_size() { return 8; }
    
    /**
     * @brief Calculate the serialized size of a zigzag-encoded sint32
     * @param value The value to calculate size for
     * @return Number of bytes required to encode the value
     */
    static size_t sint32_size(int32_t value) { return varint_size(zigzag_encode32(value)); }
    
    /**
     * @brief Calculate the serialized size of a zigzag-encoded sint64
     * @param value The value to calculate size for
     * @return Number of bytes required to encode the value
     */
    static size_t sint64_size(int64_t value) { return varint_size(zigzag_encode64(value)); }

private:
    /**
     * @brief Encode a 32-bit signed value using zigzag encoding
     * @param value The value to encode
     * @return The zigzag-encoded value
     */
    static uint64_t zigzag_encode32(int32_t value) { return (static_cast<uint32_t>(value) << 1) ^ (value >> 31); }

    /**
     * @brief Encode a 64-bit signed value using zigzag encoding
     * @param value The value to encode
     * @return The zigzag-encoded value
     */
    static uint64_t zigzag_encode64(int64_t value) { return (static_cast<uint64_t>(value) << 1) ^ (value >> 63); }
};

} // namespace litepb
