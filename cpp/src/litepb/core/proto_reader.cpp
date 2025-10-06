#include "litepb/core/proto_reader.h"
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

} // namespace litepb
