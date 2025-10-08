#include "litepb/core/unknown_fields.h"
#include "litepb/core/proto_writer.h"
#include <cstring>

namespace litepb {

void UnknownFieldSet::add_varint(uint32_t field_number, uint64_t value) {
    UnknownField field(field_number, WIRE_TYPE_VARINT);
    
    // Store the varint value in the data vector
    // We need to encode it as it would be on the wire
    uint8_t buffer[10];
    size_t size = 0;
    uint64_t temp = value;
    
    while (temp >= 0x80) {
        buffer[size++] = static_cast<uint8_t>(temp | 0x80);
        temp >>= 7;
    }
    buffer[size++] = static_cast<uint8_t>(temp);
    
    field.data.assign(buffer, buffer + size);
    fields_.push_back(std::move(field));
}

void UnknownFieldSet::add_fixed32(uint32_t field_number, uint32_t value) {
    UnknownField field(field_number, WIRE_TYPE_FIXED32);
    field.data.resize(4);
    
    // Store in little-endian format
    field.data[0] = static_cast<uint8_t>(value);
    field.data[1] = static_cast<uint8_t>(value >> 8);
    field.data[2] = static_cast<uint8_t>(value >> 16);
    field.data[3] = static_cast<uint8_t>(value >> 24);
    
    fields_.push_back(std::move(field));
}

void UnknownFieldSet::add_fixed64(uint32_t field_number, uint64_t value) {
    UnknownField field(field_number, WIRE_TYPE_FIXED64);
    field.data.resize(8);
    
    // Store in little-endian format
    for (int i = 0; i < 8; i++) {
        field.data[i] = static_cast<uint8_t>(value & 0xFF);
        value >>= 8;
    }
    
    fields_.push_back(std::move(field));
}

void UnknownFieldSet::add_length_delimited(uint32_t field_number, const uint8_t* data, size_t size) {
    UnknownField field(field_number, WIRE_TYPE_LENGTH_DELIMITED);
    
    // For length-delimited fields, we store the length prefix followed by the data
    // Calculate varint size for the length
    size_t length_size = ProtoWriter::varint_size(size);
    field.data.resize(length_size + size);
    
    // Encode the length as varint
    size_t pos = 0;
    uint64_t temp = size;
    while (temp >= 0x80) {
        field.data[pos++] = static_cast<uint8_t>(temp | 0x80);
        temp >>= 7;
    }
    field.data[pos++] = static_cast<uint8_t>(temp);
    
    // Copy the actual data
    if (data && size > 0) {
        std::memcpy(field.data.data() + pos, data, size);
    }
    
    fields_.push_back(std::move(field));
}

void UnknownFieldSet::add_group(uint32_t field_number, const uint8_t* data, size_t size) {
    // Groups are deprecated but we need to support them for compatibility
    // A group consists of START_GROUP tag, group data, and END_GROUP tag
    UnknownField field(field_number, WIRE_TYPE_START_GROUP);
    
    // Store the group data (everything between START_GROUP and END_GROUP)
    if (data && size > 0) {
        field.data.assign(data, data + size);
    }
    
    fields_.push_back(std::move(field));
}

bool UnknownFieldSet::serialize_to(OutputStream& stream) const {
    ProtoWriter writer(stream);
    
    for (const auto& field : fields_) {
        // Write the tag
        if (!writer.write_tag(field.field_number, field.wire_type)) {
            return false;
        }
        
        // Write the field data based on wire type
        switch (field.wire_type) {
            case WIRE_TYPE_VARINT:
            case WIRE_TYPE_FIXED32:
            case WIRE_TYPE_FIXED64:
            case WIRE_TYPE_LENGTH_DELIMITED:
                // For these types, the data is already properly encoded
                if (!stream.write(field.data.data(), field.data.size())) {
                    return false;
                }
                break;
                
            case WIRE_TYPE_START_GROUP:
                // Write the group data
                if (!stream.write(field.data.data(), field.data.size())) {
                    return false;
                }
                // Write the END_GROUP tag
                if (!writer.write_tag(field.field_number, WIRE_TYPE_END_GROUP)) {
                    return false;
                }
                break;
                
            case WIRE_TYPE_END_GROUP:
                // Should not have standalone END_GROUP in unknown fields
                break;
                
            default:
                // Unknown wire type, skip
                break;
        }
    }
    
    return true;
}

size_t UnknownFieldSet::byte_size() const {
    size_t total = 0;
    
    for (const auto& field : fields_) {
        // Add tag size
        uint32_t tag = (field.field_number << 3) | field.wire_type;
        total += ProtoWriter::varint_size(tag);
        
        // Add data size
        total += field.data.size();
        
        // For groups, add END_GROUP tag size
        if (field.wire_type == WIRE_TYPE_START_GROUP) {
            uint32_t end_tag = (field.field_number << 3) | WIRE_TYPE_END_GROUP;
            total += ProtoWriter::varint_size(end_tag);
        }
    }
    
    return total;
}

} // namespace litepb