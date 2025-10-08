#include "litepb/core/proto_reader.h"
#include "litepb/core/unknown_fields.h"
#include <cstring>

namespace litepb {

bool ProtoReader::read_varint(uint64_t& value)
{
    value = 0;
    uint8_t byte;
    int shift = 0;

    for (int i = 0; i < 10; i++) {
        if (!stream_.read(&byte, 1))
            return false;

        if (i == 9 && byte > 1)
            return false;

        value |= static_cast<uint64_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            return true;
        }
        shift += 7;
    }

    return false;
}

bool ProtoReader::read_fixed32(uint32_t& value)
{
    uint8_t buffer[4];
    if (!stream_.read(buffer, 4))
        return false;

    value = static_cast<uint32_t>(buffer[0]) | (static_cast<uint32_t>(buffer[1]) << 8) | (static_cast<uint32_t>(buffer[2]) << 16) |
        (static_cast<uint32_t>(buffer[3]) << 24);
    return true;
}

bool ProtoReader::read_fixed64(uint64_t& value)
{
    uint8_t buffer[8];
    if (!stream_.read(buffer, 8))
        return false;

    value = static_cast<uint64_t>(buffer[0]) | (static_cast<uint64_t>(buffer[1]) << 8) | (static_cast<uint64_t>(buffer[2]) << 16) |
        (static_cast<uint64_t>(buffer[3]) << 24) | (static_cast<uint64_t>(buffer[4]) << 32) |
        (static_cast<uint64_t>(buffer[5]) << 40) | (static_cast<uint64_t>(buffer[6]) << 48) |
        (static_cast<uint64_t>(buffer[7]) << 56);
    return true;
}

bool ProtoReader::read_sfixed32(int32_t& value)
{
    uint32_t bits;
    if (!read_fixed32(bits))
        return false;
    std::memcpy(&value, &bits, sizeof(int32_t));
    return true;
}

bool ProtoReader::read_sfixed64(int64_t& value)
{
    uint64_t bits;
    if (!read_fixed64(bits))
        return false;
    std::memcpy(&value, &bits, sizeof(int64_t));
    return true;
}

bool ProtoReader::read_float(float& value)
{
    uint32_t bits;
    if (!read_fixed32(bits))
        return false;
    std::memcpy(&value, &bits, sizeof(float));
    return true;
}

bool ProtoReader::read_double(double& value)
{
    uint64_t bits;
    if (!read_fixed64(bits))
        return false;
    std::memcpy(&value, &bits, sizeof(double));
    return true;
}

bool ProtoReader::read_bytes(std::vector<uint8_t>& data)
{
    uint64_t size;
    if (!read_varint(size))
        return false;

    if (size > 0) {
        data.resize(size);
        return stream_.read(data.data(), size);
    }
    data.clear();
    return true;
}

bool ProtoReader::read_string(std::string& str)
{
    uint64_t size;
    if (!read_varint(size))
        return false;

    if (size > 0) {
        str.resize(size);
        return stream_.read(reinterpret_cast<uint8_t*>(&str[0]), size);
    }
    str.clear();
    return true;
}

bool ProtoReader::read_tag(uint32_t& field_number, WireType& type)
{
    uint64_t tag;
    if (!read_varint(tag))
        return false;

    field_number = static_cast<uint32_t>(tag >> 3);
    type         = static_cast<WireType>(tag & 7);
    return true;
}

bool ProtoReader::skip_field(WireType type)
{
    switch (type) {
    case WIRE_TYPE_VARINT: {
        uint64_t dummy;
        return read_varint(dummy);
    }
    case WIRE_TYPE_FIXED64: {
        return stream_.skip(8);
    }
    case WIRE_TYPE_LENGTH_DELIMITED: {
        uint64_t size;
        if (!read_varint(size))
            return false;
        return stream_.skip(size);
    }
    case WIRE_TYPE_START_GROUP:
    case WIRE_TYPE_END_GROUP:
        return false;
    case WIRE_TYPE_FIXED32: {
        return stream_.skip(4);
    }
    default:
        return false;
    }
}

bool ProtoReader::read_sint32(int32_t& value)
{
    uint64_t encoded;
    if (!read_varint(encoded))
        return false;
    value = zigzag_decode32(static_cast<uint32_t>(encoded));
    return true;
}

bool ProtoReader::read_sint64(int64_t& value)
{
    uint64_t encoded;
    if (!read_varint(encoded))
        return false;
    value = zigzag_decode64(encoded);
    return true;
}

bool ProtoReader::capture_unknown_field(WireType type, std::vector<uint8_t>& data)
{
    data.clear();
    
    switch (type) {
    case WIRE_TYPE_VARINT: {
        uint64_t value;
        if (!read_varint(value))
            return false;
        
        // Encode the varint back to bytes
        uint8_t buffer[10];
        size_t size = 0;
        while (value >= 0x80) {
            buffer[size++] = static_cast<uint8_t>(value | 0x80);
            value >>= 7;
        }
        buffer[size++] = static_cast<uint8_t>(value);
        
        data.assign(buffer, buffer + size);
        return true;
    }
    case WIRE_TYPE_FIXED32: {
        data.resize(4);
        return stream_.read(data.data(), 4);
    }
    case WIRE_TYPE_FIXED64: {
        data.resize(8);
        return stream_.read(data.data(), 8);
    }
    case WIRE_TYPE_LENGTH_DELIMITED: {
        uint64_t size;
        if (!read_varint(size))
            return false;
        
        // Store the length prefix and data together
        // First encode the length as varint
        uint8_t length_buffer[10];
        size_t length_size = 0;
        uint64_t temp = size;
        while (temp >= 0x80) {
            length_buffer[length_size++] = static_cast<uint8_t>(temp | 0x80);
            temp >>= 7;
        }
        length_buffer[length_size++] = static_cast<uint8_t>(temp);
        
        // Allocate space for length + data
        data.resize(length_size + size);
        
        // Copy length prefix
        std::memcpy(data.data(), length_buffer, length_size);
        
        // Read the actual data
        if (size > 0) {
            if (!stream_.read(data.data() + length_size, size))
                return false;
        }
        return true;
    }
    case WIRE_TYPE_START_GROUP: {
        // Groups are deprecated but we need to handle them
        // Read until we find the matching END_GROUP tag
        std::vector<uint8_t> group_data;
        uint32_t nested_field;
        WireType nested_type;
        
        while (read_tag(nested_field, nested_type)) {
            // Check for END_GROUP with matching field number
            if (nested_type == WIRE_TYPE_END_GROUP) {
                // Don't include the END_GROUP tag in the data
                data = std::move(group_data);
                return true;
            }
            
            // Capture the tag
            uint32_t tag = (nested_field << 3) | nested_type;
            uint8_t tag_buffer[5];
            size_t tag_size = 0;
            while (tag >= 0x80) {
                tag_buffer[tag_size++] = static_cast<uint8_t>(tag | 0x80);
                tag >>= 7;
            }
            tag_buffer[tag_size++] = static_cast<uint8_t>(tag);
            
            // Append tag to group data
            group_data.insert(group_data.end(), tag_buffer, tag_buffer + tag_size);
            
            // Capture the field data
            std::vector<uint8_t> field_data;
            if (!capture_unknown_field(nested_type, field_data))
                return false;
            
            // Append field data to group data
            group_data.insert(group_data.end(), field_data.begin(), field_data.end());
        }
        return false; // Missing END_GROUP
    }
    case WIRE_TYPE_END_GROUP:
        // Shouldn't encounter standalone END_GROUP
        return false;
    default:
        return false;
    }
}

bool ProtoReader::skip_and_save(uint32_t field_number, WireType type, UnknownFieldSet& unknown_fields)
{
    switch (type) {
    case WIRE_TYPE_VARINT: {
        uint64_t value;
        if (!read_varint(value))
            return false;
        unknown_fields.add_varint(field_number, value);
        return true;
    }
    case WIRE_TYPE_FIXED32: {
        uint32_t value;
        if (!read_fixed32(value))
            return false;
        unknown_fields.add_fixed32(field_number, value);
        return true;
    }
    case WIRE_TYPE_FIXED64: {
        uint64_t value;
        if (!read_fixed64(value))
            return false;
        unknown_fields.add_fixed64(field_number, value);
        return true;
    }
    case WIRE_TYPE_LENGTH_DELIMITED: {
        std::vector<uint8_t> data;
        if (!read_bytes(data))
            return false;
        unknown_fields.add_length_delimited(field_number, data.data(), data.size());
        return true;
    }
    case WIRE_TYPE_START_GROUP: {
        std::vector<uint8_t> data;
        if (!capture_unknown_field(type, data))
            return false;
        unknown_fields.add_group(field_number, data.data(), data.size());
        return true;
    }
    case WIRE_TYPE_END_GROUP:
        // Shouldn't encounter standalone END_GROUP
        return false;
    default:
        return false;
    }
}

} // namespace litepb
