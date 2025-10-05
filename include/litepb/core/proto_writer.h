#ifndef LITEPB_CORE_PROTO_WRITER_H
#define LITEPB_CORE_PROTO_WRITER_H

#include "litepb/core/streams.h"
#include <cstdint>
#include <string>

namespace litepb {

enum WireType
{
    WIRE_TYPE_VARINT           = 0,
    WIRE_TYPE_FIXED64          = 1,
    WIRE_TYPE_LENGTH_DELIMITED = 2,
    WIRE_TYPE_START_GROUP      = 3,
    WIRE_TYPE_END_GROUP        = 4,
    WIRE_TYPE_FIXED32          = 5
};

class ProtoWriter
{
    OutputStream& stream_;

public:
    explicit ProtoWriter(OutputStream& stream) : stream_(stream) {}

    bool write_varint(uint64_t value);
    bool write_fixed32(uint32_t value);
    bool write_fixed64(uint64_t value);
    bool write_sfixed32(int32_t value);
    bool write_sfixed64(int64_t value);
    bool write_float(float value);
    bool write_double(double value);

    bool write_bytes(const uint8_t* data, size_t size);
    bool write_string(const std::string& str);

    bool write_tag(uint32_t field_number, WireType type);

    bool write_sint32(int32_t value);
    bool write_sint64(int64_t value);

    static size_t varint_size(uint64_t value);
    static size_t fixed32_size() { return 4; }
    static size_t fixed64_size() { return 8; }
    static size_t sint32_size(int32_t value) { return varint_size(zigzag_encode32(value)); }
    static size_t sint64_size(int64_t value) { return varint_size(zigzag_encode64(value)); }

private:
    static uint64_t zigzag_encode32(int32_t value) { return (static_cast<uint32_t>(value) << 1) ^ (value >> 31); }

    static uint64_t zigzag_encode64(int64_t value) { return (static_cast<uint64_t>(value) << 1) ^ (value >> 63); }
};

} // namespace litepb

#endif
