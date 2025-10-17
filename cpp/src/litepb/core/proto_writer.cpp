#include "litepb/core/proto_writer.h"
#include "litepb/core/unknown_fields.h"
#include <cstring>

namespace litepb {

bool ProtoWriter::write_varint(uint64_t value)
{
    uint8_t buffer[10];
    size_t size = 0;

    while (value >= 0x80) {
        buffer[size++] = static_cast<uint8_t>(value | 0x80);
        value >>= 7;
    }
    buffer[size++] = static_cast<uint8_t>(value);

    return stream_.write(buffer, size);
}

bool ProtoWriter::write_fixed32(uint32_t value)
{
    uint8_t buffer[4];
    buffer[0] = static_cast<uint8_t>(value);
    buffer[1] = static_cast<uint8_t>(value >> 8);
    buffer[2] = static_cast<uint8_t>(value >> 16);
    buffer[3] = static_cast<uint8_t>(value >> 24);
    return stream_.write(buffer, 4);
}

bool ProtoWriter::write_fixed64(uint64_t value)
{
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = static_cast<uint8_t>(value & 0xFF);
        value >>= 8;
    }
    return stream_.write(bytes, 8);
}

bool ProtoWriter::write_float(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    return write_fixed32(bits);
}

bool ProtoWriter::write_double(double value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    return write_fixed64(bits);
}

bool ProtoWriter::write_bytes(const uint8_t* data, size_t size)
{
    if (!write_varint(size))
        return false;
    if (size > 0 && data) {
        return stream_.write(data, size);
    }
    return true;
}

bool ProtoWriter::write_string(const std::string& str)
{
    return write_bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

bool ProtoWriter::write_tag(uint32_t field_number, WireType type)
{
    return write_varint((field_number << 3) | type);
}

bool ProtoWriter::write_sint32(int32_t value)
{
    return write_varint(zigzag_encode32(value));
}

bool ProtoWriter::write_sint64(int64_t value)
{
    return write_varint(zigzag_encode64(value));
}

bool ProtoWriter::write_sfixed32(int32_t value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(int32_t));
    return write_fixed32(bits);
}

bool ProtoWriter::write_sfixed64(int64_t value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(int64_t));
    return write_fixed64(bits);
}

size_t ProtoWriter::varint_size(uint64_t value)
{
    size_t size = 1;
    while (value >= 128) {
        value >>= 7;
        size++;
    }
    return size;
}

size_t ProtoWriter::string_size(uint32_t field_number, const std::string& value)
{
    size_t size = varint_size((field_number << 3) | WIRE_TYPE_LENGTH_DELIMITED); // tag
    size += varint_size(value.size()); // length prefix
    size += value.size(); // string data
    return size;
}

size_t ProtoWriter::bytes_size(uint32_t field_number, const std::vector<uint8_t>& value)
{
    size_t size = varint_size((field_number << 3) | WIRE_TYPE_LENGTH_DELIMITED); // tag
    size += varint_size(value.size()); // length prefix
    size += value.size(); // bytes data
    return size;
}

size_t ProtoWriter::unknown_fields_size(const UnknownFieldSet& unknown_fields)
{
    return unknown_fields.byte_size();
}

} // namespace litepb
