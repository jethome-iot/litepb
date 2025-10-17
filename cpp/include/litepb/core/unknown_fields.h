/**
 * @file unknown_fields.h
 * @brief Unknown field preservation for Protocol Buffers compatibility
 *
 * This header defines the UnknownFieldSet class which stores fields that were
 * not recognized during deserialization. This is critical for forward/backward
 * compatibility as it allows messages to preserve fields that the current
 * version doesn't understand, ensuring they can be re-serialized intact.
 *
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include <cstdint>
#include <vector>

namespace litepb {

/**
 * @brief Storage for a single unknown field
 *
 * Represents a field that was encountered during parsing but was not
 * recognized by the message definition. Stores the field number, wire type,
 * and raw data to enable exact re-serialization.
 */
struct UnknownField {
    uint32_t field_number;  ///< Field number from the wire format
    WireType wire_type;     ///< Wire type of the field
    std::vector<uint8_t> data;  ///< Raw field data (interpretation depends on wire_type)
    
    /**
     * @brief Construct an unknown field
     * @param num Field number
     * @param type Wire type
     */
    UnknownField(uint32_t num, WireType type) 
        : field_number(num), wire_type(type) {}
};

/**
 * @brief Collection of unknown fields for a message
 *
 * UnknownFieldSet stores fields that were not recognized during message parsing.
 * This enables forward/backward compatibility by preserving unrecognized fields
 * so they can be re-serialized exactly as received.
 *
 * The fields are stored in the order they were encountered to maintain
 * serialization order when round-tripping messages.
 *
 * @example
 * @code{.cpp}
 * struct MyMessage {
 *     int32_t known_field;
 *     litepb::UnknownFieldSet unknown_fields;  // Added by code generator
 * };
 *
 * // During parsing, unknown fields are automatically captured
 * // During serialization, they are automatically written back
 * @endcode
 */
class UnknownFieldSet {
    std::vector<UnknownField> fields_;
    
public:
    /**
     * @brief Add a varint field
     * @param field_number Field number from wire format
     * @param value The varint value
     */
    void add_varint(uint32_t field_number, uint64_t value);
    
    /**
     * @brief Add a fixed32 field
     * @param field_number Field number from wire format
     * @param value The 32-bit value
     */
    void add_fixed32(uint32_t field_number, uint32_t value);
    
    /**
     * @brief Add a fixed64 field
     * @param field_number Field number from wire format
     * @param value The 64-bit value
     */
    void add_fixed64(uint32_t field_number, uint64_t value);
    
    /**
     * @brief Add a length-delimited field
     * @param field_number Field number from wire format
     * @param data Pointer to the field data
     * @param size Size of the field data in bytes
     */
    void add_length_delimited(uint32_t field_number, const uint8_t* data, size_t size);
    
    /**
     * @brief Add a group start field (deprecated in proto3)
     * @param field_number Field number from wire format
     * @param data Group data (everything until END_GROUP)
     * @param size Size of group data
     */
    void add_group(uint32_t field_number, const uint8_t* data, size_t size);
    
    /**
     * @brief Serialize all unknown fields to a stream
     * @param stream Output stream to write to
     * @return true if serialization succeeded
     */
    bool serialize_to(OutputStream& stream) const;
    
    /**
     * @brief Calculate the total serialized size of all unknown fields
     * @return Total size in bytes
     */
    size_t byte_size() const;
    
    /**
     * @brief Check if there are any unknown fields
     * @return true if the set is empty
     */
    bool empty() const { return fields_.empty(); }
    
    /**
     * @brief Clear all unknown fields
     */
    void clear() { fields_.clear(); }
    
    /**
     * @brief Get read-only access to the fields
     * @return Const reference to the internal vector of fields
     */
    const std::vector<UnknownField>& fields() const { return fields_; }
};

} // namespace litepb