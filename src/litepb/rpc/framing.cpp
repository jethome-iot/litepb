#include "litepb/rpc/framing.h"
#include "litepb/core/streams.h"
#include "litepb/rpc/addressing.h"
#include <cstring>

namespace litepb {

MessageIdGenerator::MessageIdGenerator() : counter_(1) {}

uint16_t MessageIdGenerator::generate_for(uint64_t local_addr, uint64_t dst_addr)
{
    uint16_t id = counter_;
    counter_++;
    return id;
}

size_t encode_varint(uint32_t value, uint8_t* buffer)
{
    size_t bytes_written = 0;

    while (value >= 0x80) {
        buffer[bytes_written++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
        value >>= 7;
    }

    buffer[bytes_written++] = static_cast<uint8_t>(value & 0x7F);

    return bytes_written;
}

size_t decode_varint(const uint8_t* buffer, size_t max_len, uint32_t& out_value)
{
    out_value         = 0;
    size_t bytes_read = 0;
    uint32_t shift    = 0;

    while (bytes_read < max_len && bytes_read < 5) {
        uint8_t byte = buffer[bytes_read++];
        out_value |= static_cast<uint32_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            return bytes_read;
        }

        shift += 7;
    }

    return 0;
}

bool encode_message(const FramedMessage& msg, OutputStream& output, bool is_stream_transport)
{
    uint8_t src_addr_bytes[8];
    src_addr_bytes[0] = static_cast<uint8_t>(msg.src_addr & 0xFF);
    src_addr_bytes[1] = static_cast<uint8_t>((msg.src_addr >> 8) & 0xFF);
    src_addr_bytes[2] = static_cast<uint8_t>((msg.src_addr >> 16) & 0xFF);
    src_addr_bytes[3] = static_cast<uint8_t>((msg.src_addr >> 24) & 0xFF);
    src_addr_bytes[4] = static_cast<uint8_t>((msg.src_addr >> 32) & 0xFF);
    src_addr_bytes[5] = static_cast<uint8_t>((msg.src_addr >> 40) & 0xFF);
    src_addr_bytes[6] = static_cast<uint8_t>((msg.src_addr >> 48) & 0xFF);
    src_addr_bytes[7] = static_cast<uint8_t>((msg.src_addr >> 56) & 0xFF);

    if (!output.write(src_addr_bytes, 8)) {
        return false;
    }

    uint8_t dst_addr_bytes[8];
    dst_addr_bytes[0] = static_cast<uint8_t>(msg.dst_addr & 0xFF);
    dst_addr_bytes[1] = static_cast<uint8_t>((msg.dst_addr >> 8) & 0xFF);
    dst_addr_bytes[2] = static_cast<uint8_t>((msg.dst_addr >> 16) & 0xFF);
    dst_addr_bytes[3] = static_cast<uint8_t>((msg.dst_addr >> 24) & 0xFF);
    dst_addr_bytes[4] = static_cast<uint8_t>((msg.dst_addr >> 32) & 0xFF);
    dst_addr_bytes[5] = static_cast<uint8_t>((msg.dst_addr >> 40) & 0xFF);
    dst_addr_bytes[6] = static_cast<uint8_t>((msg.dst_addr >> 48) & 0xFF);
    dst_addr_bytes[7] = static_cast<uint8_t>((msg.dst_addr >> 56) & 0xFF);

    if (!output.write(dst_addr_bytes, 8)) {
        return false;
    }

    uint8_t msg_id_varint[5];
    size_t msg_id_bytes = encode_varint(msg.msg_id, msg_id_varint);

    if (!output.write(msg_id_varint, msg_id_bytes)) {
        return false;
    }

    uint8_t service_id_varint[5];
    size_t service_id_bytes = encode_varint(msg.service_id, service_id_varint);

    if (!output.write(service_id_varint, service_id_bytes)) {
        return false;
    }

    uint8_t method_id_varint[5];
    size_t method_id_bytes = encode_varint(msg.method_id, method_id_varint);

    if (!output.write(method_id_varint, method_id_bytes)) {
        return false;
    }

    if (is_stream_transport) {
        uint8_t payload_len_varint[5];
        size_t payload_len_bytes = encode_varint(static_cast<uint32_t>(msg.payload.size()), payload_len_varint);

        if (!output.write(payload_len_varint, payload_len_bytes)) {
            return false;
        }
    }

    if (!msg.payload.empty()) {
        if (!output.write(msg.payload.data(), msg.payload.size())) {
            return false;
        }
    }

    return true;
}

bool decode_message(InputStream& input, FramedMessage& msg, bool is_stream_transport)
{
    uint8_t src_addr_bytes[8];
    if (!input.read(src_addr_bytes, 8)) {
        return false;
    }

    msg.src_addr = static_cast<uint64_t>(src_addr_bytes[0]) | (static_cast<uint64_t>(src_addr_bytes[1]) << 8) |
        (static_cast<uint64_t>(src_addr_bytes[2]) << 16) | (static_cast<uint64_t>(src_addr_bytes[3]) << 24) |
        (static_cast<uint64_t>(src_addr_bytes[4]) << 32) | (static_cast<uint64_t>(src_addr_bytes[5]) << 40) |
        (static_cast<uint64_t>(src_addr_bytes[6]) << 48) | (static_cast<uint64_t>(src_addr_bytes[7]) << 56);

    uint8_t dst_addr_bytes[8];
    if (!input.read(dst_addr_bytes, 8)) {
        return false;
    }

    msg.dst_addr = static_cast<uint64_t>(dst_addr_bytes[0]) | (static_cast<uint64_t>(dst_addr_bytes[1]) << 8) |
        (static_cast<uint64_t>(dst_addr_bytes[2]) << 16) | (static_cast<uint64_t>(dst_addr_bytes[3]) << 24) |
        (static_cast<uint64_t>(dst_addr_bytes[4]) << 32) | (static_cast<uint64_t>(dst_addr_bytes[5]) << 40) |
        (static_cast<uint64_t>(dst_addr_bytes[6]) << 48) | (static_cast<uint64_t>(dst_addr_bytes[7]) << 56);

    uint8_t msg_id_varint[5];
    size_t msg_id_bytes  = 0;
    uint32_t msg_id_temp = 0;

    for (size_t i = 0; i < 5; ++i) {
        if (!input.read(&msg_id_varint[i], 1)) {
            return false;
        }

        msg_id_bytes = decode_varint(msg_id_varint, i + 1, msg_id_temp);
        if (msg_id_bytes > 0) {
            break;
        }
    }

    if (msg_id_bytes == 0) {
        return false;
    }

    msg.msg_id = static_cast<uint16_t>(msg_id_temp);

    uint8_t service_id_varint[5];
    size_t service_id_bytes  = 0;
    uint32_t service_id_temp = 0;

    for (size_t i = 0; i < 5; ++i) {
        if (!input.read(&service_id_varint[i], 1)) {
            return false;
        }

        service_id_bytes = decode_varint(service_id_varint, i + 1, service_id_temp);
        if (service_id_bytes > 0) {
            break;
        }
    }

    if (service_id_bytes == 0) {
        return false;
    }

    msg.service_id = static_cast<uint16_t>(service_id_temp);

    uint8_t method_id_varint[5];
    size_t method_id_bytes = 0;

    for (size_t i = 0; i < 5; ++i) {
        if (!input.read(&method_id_varint[i], 1)) {
            return false;
        }

        method_id_bytes = decode_varint(method_id_varint, i + 1, msg.method_id);
        if (method_id_bytes > 0) {
            break;
        }
    }

    if (method_id_bytes == 0) {
        return false;
    }

    uint32_t payload_len = 0;

    if (is_stream_transport) {
        uint8_t payload_len_varint[5];
        size_t payload_len_bytes = 0;

        for (size_t i = 0; i < 5; ++i) {
            if (!input.read(&payload_len_varint[i], 1)) {
                return false;
            }

            payload_len_bytes = decode_varint(payload_len_varint, i + 1, payload_len);
            if (payload_len_bytes > 0) {
                break;
            }
        }

        if (payload_len_bytes == 0) {
            return false;
        }
    }
    else {
        payload_len = static_cast<uint32_t>(input.available());
    }

    if (payload_len > 0) {
        msg.payload.resize(payload_len);
        if (!input.read(msg.payload.data(), payload_len)) {
            return false;
        }
    }
    else {
        msg.payload.clear();
    }

    return true;
}

} // namespace litepb
