#ifndef LITEPB_CORE_PROTO_READER_H
#define LITEPB_CORE_PROTO_READER_H

#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include <cstdint>
#include <string>
#include <vector>

namespace litepb {

class ProtoReader
{
    InputStream& stream_;

public:
    explicit ProtoReader(InputStream& stream) : stream_(stream) {}

    bool read_varint(uint64_t& value);
    bool read_fixed32(uint32_t& value);
    bool read_fixed64(uint64_t& value);
    bool read_sfixed32(int32_t& value);
    bool read_sfixed64(int64_t& value);
    bool read_float(float& value);
    bool read_double(double& value);

    bool read_bytes(std::vector<uint8_t>& data);
    bool read_string(std::string& str);

    bool read_tag(uint32_t& field_number, WireType& type);

    bool skip_field(WireType type);

    bool read_sint32(int32_t& value);
    bool read_sint64(int64_t& value);

    size_t position() const { return stream_.position(); }

private:
    static int32_t zigzag_decode32(uint32_t value) { return static_cast<int32_t>((value >> 1) ^ -(value & 1)); }

    static int64_t zigzag_decode64(uint64_t value) { return static_cast<int64_t>((value >> 1) ^ -(value & 1)); }
};

} // namespace litepb

#endif
