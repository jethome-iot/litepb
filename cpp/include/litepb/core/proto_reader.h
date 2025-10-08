/**
 * @file proto_reader.h
 * @brief Protocol Buffers wire format reader
 *
 * This header defines the ProtoReader class which handles low-level Protocol Buffers
 * wire format reading. It provides methods for reading various wire types including
 * varints, fixed-width values, and length-delimited data from input streams.
 *
 * The reader handles all Protocol Buffers encoding rules including zigzag encoding
 * for signed integers and proper field tag parsing.
 *
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include <cstdint>
#include <string>
#include <vector>

namespace litepb {

/**
 * @brief Low-level Protocol Buffers wire format reader
 *
 * ProtoReader provides methods for reading Protocol Buffers wire format data
 * from an input stream. It handles all standard Protocol Buffers wire types
 * and encoding schemes including varints, fixed-width values, and length-delimited data.
 *
 * This class is used internally by generated code and typically should not be
 * used directly by application code.
 *
 * @example Reading wire format data
 * @code{.cpp}
 * litepb::BufferInputStream input(data, size);
 * litepb::ProtoReader reader(input);
 *
 * uint32_t field_number;
 * WireType wire_type;
 * while (reader.read_tag(field_number, wire_type)) {
 *     switch (field_number) {
 *     case 1:
 *         uint64_t value;
 *         reader.read_varint(value);
 *         break;
 *     case 2:
 *         std::string str;
 *         reader.read_string(str);
 *         break;
 *     default:
 *         reader.skip_field(wire_type);
 *     }
 * }
 * @endcode
 */
class ProtoReader {
    InputStream & stream_;

public:
    /**
     * @brief Construct a ProtoReader for the given input stream
     * @param stream The input stream to read from
     */
    explicit ProtoReader(InputStream & stream)
        : stream_(stream)
    {
    }

    /**
     * @brief Read a variable-length integer (varint)
     *
     * Reads a base-128 encoded variable-length integer from the stream.
     * Used for int32, int64, uint32, uint64, bool, and enum types.
     *
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error or EOF
     */
    bool read_varint(uint64_t & value);

    /**
     * @brief Read a 32-bit fixed-width value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_fixed32(uint32_t & value);

    /**
     * @brief Read a 64-bit fixed-width value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_fixed64(uint64_t & value);

    /**
     * @brief Read a signed 32-bit fixed-width value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_sfixed32(int32_t & value);

    /**
     * @brief Read a signed 64-bit fixed-width value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_sfixed64(int64_t & value);

    /**
     * @brief Read a 32-bit floating point value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_float(float & value);

    /**
     * @brief Read a 64-bit floating point value
     * @param value Output parameter for the read value
     * @return true if read succeeded, false on error
     */
    bool read_double(double & value);

    /**
     * @brief Read length-delimited bytes
     *
     * Reads a varint length prefix followed by that many bytes of data.
     *
     * @param data Output vector to store the read bytes
     * @return true if read succeeded, false on error
     */
    bool read_bytes(std::vector<uint8_t> & data);

    /**
     * @brief Read a length-delimited string
     *
     * Reads a varint length prefix followed by UTF-8 string data.
     *
     * @param str Output string to store the read text
     * @return true if read succeeded, false on error
     */
    bool read_string(std::string & str);

    /**
     * @brief Read a field tag (field number and wire type)
     *
     * Reads and decodes a field tag which contains the field number
     * and wire type information.
     *
     * @param field_number Output parameter for the field number
     * @param type Output parameter for the wire type
     * @return true if a tag was read, false on EOF
     */
    bool read_tag(uint32_t & field_number, WireType & type);

    /**
     * @brief Skip a field with the given wire type
     *
     * Skips over the data for a field without parsing it. Useful for
     * ignoring unknown fields or optional fields that aren't needed.
     *
     * @param type The wire type of the field to skip
     * @return true if skip succeeded, false on error
     */
    bool skip_field(WireType type);
    
    /**
     * @brief Skip a field and capture it as unknown field data
     *
     * Reads the field data and returns it for storage in an UnknownFieldSet.
     * This is used when a field is not recognized but needs to be preserved
     * for round-trip compatibility.
     *
     * @param type The wire type of the field to capture
     * @param data Output vector to store the captured field data
     * @return true if capture succeeded, false on error
     */
    bool capture_unknown_field(WireType type, std::vector<uint8_t>& data);

    /**
     * @brief Skip a field and save it directly to an UnknownFieldSet
     *
     * Convenience method that captures an unknown field and adds it directly
     * to an UnknownFieldSet based on the field type.
     *
     * @param field_number The field number
     * @param type The wire type of the field
     * @param unknown_fields The UnknownFieldSet to add the field to
     * @return true if skip and save succeeded, false on error
     */
    bool skip_and_save(uint32_t field_number, WireType type, class UnknownFieldSet& unknown_fields);

    /**
     * @brief Read a zigzag-encoded signed 32-bit integer
     *
     * Reads a varint and decodes it using zigzag decoding for efficient
     * representation of negative numbers.
     *
     * @param value Output parameter for the decoded value
     * @return true if read succeeded, false on error
     */
    bool read_sint32(int32_t & value);

    /**
     * @brief Read a zigzag-encoded signed 64-bit integer
     *
     * Reads a varint and decodes it using zigzag decoding for efficient
     * representation of negative numbers.
     *
     * @param value Output parameter for the decoded value
     * @return true if read succeeded, false on error
     */
    bool read_sint64(int64_t & value);

    /**
     * @brief Get the current read position in the stream
     * @return Current position in bytes
     */
    size_t position() const { return stream_.position(); }

private:
    /**
     * @brief Decode a 32-bit zigzag-encoded value
     * @param value The encoded value
     * @return The decoded signed value
     */
    static int32_t zigzag_decode32(uint32_t value) { return static_cast<int32_t>((value >> 1) ^ -(value & 1)); }

    /**
     * @brief Decode a 64-bit zigzag-encoded value
     * @param value The encoded value
     * @return The decoded signed value
     */
    static int64_t zigzag_decode64(uint64_t value) { return static_cast<int64_t>((value >> 1) ^ -(value & 1)); }
};

} // namespace litepb
