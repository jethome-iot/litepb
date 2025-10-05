#ifndef LITEPB_RPC_FRAMING_H
#define LITEPB_RPC_FRAMING_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace litepb {

class OutputStream;
class InputStream;

class MessageIdGenerator
{
public:
    MessageIdGenerator();

    uint16_t generate_for(uint64_t local_addr, uint64_t dst_addr);

private:
    uint16_t counter_;
};

size_t encode_varint(uint32_t value, uint8_t* buffer);

size_t decode_varint(const uint8_t* buffer, size_t max_len, uint32_t& out_value);

struct FramedMessage
{
    uint64_t src_addr;
    uint64_t dst_addr;
    uint16_t msg_id;
    uint16_t service_id;
    uint32_t method_id;
    std::vector<uint8_t> payload;
};

bool encode_message(const FramedMessage& msg, OutputStream& output, bool is_stream_transport);

bool decode_message(InputStream& input, FramedMessage& msg, bool is_stream_transport);

} // namespace litepb

#endif
