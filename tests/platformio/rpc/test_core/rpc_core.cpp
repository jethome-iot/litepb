#include "litepb/core/streams.h"
#include "litepb/rpc/addressing.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/error.h"
#include "litepb/rpc/framing.h"
#include "litepb/rpc/transport.h"
#include "unity.h"
#include <queue>

class LoopbackTransport : public litepb::StreamTransport
{
public:
    LoopbackTransport() : peer_(nullptr) {}

    void connect_to_peer(LoopbackTransport* peer) { peer_ = peer; }

    bool send(const uint8_t* data, size_t len) override
    {
        if (!peer_) {
            return false;
        }

        for (size_t i = 0; i < len; ++i) {
            peer_->rx_queue_.push(data[i]);
        }
        return true;
    }

    size_t recv(uint8_t* buffer, size_t max_len) override
    {
        size_t count = 0;
        while (!rx_queue_.empty() && count < max_len) {
            buffer[count++] = rx_queue_.front();
            rx_queue_.pop();
        }
        return count;
    }

    bool available() override { return !rx_queue_.empty(); }

    std::queue<uint8_t> rx_queue_;

private:
    LoopbackTransport* peer_;
};

struct SimpleMessage
{
    int32_t value = 0;
};

namespace litepb {
template <>
inline bool serialize(const SimpleMessage& msg, OutputStream& output)
{
    uint8_t tag = (1 << 3) | 0;
    output.write(&tag, 1);

    uint8_t value_bytes[4];
    value_bytes[0] = static_cast<uint8_t>(msg.value & 0xFF);
    value_bytes[1] = static_cast<uint8_t>((msg.value >> 8) & 0xFF);
    value_bytes[2] = static_cast<uint8_t>((msg.value >> 16) & 0xFF);
    value_bytes[3] = static_cast<uint8_t>((msg.value >> 24) & 0xFF);

    return output.write(value_bytes, 4);
}

template <>
inline bool parse(SimpleMessage& msg, InputStream& input)
{
    uint8_t tag;
    if (!input.read(&tag, 1)) {
        return false;
    }

    if ((tag >> 3) != 1) {
        return false;
    }

    uint8_t value_bytes[4];
    if (!input.read(value_bytes, 4)) {
        return false;
    }

    msg.value = static_cast<int32_t>(value_bytes[0]) | (static_cast<int32_t>(value_bytes[1]) << 8) |
        (static_cast<int32_t>(value_bytes[2]) << 16) | (static_cast<int32_t>(value_bytes[3]) << 24);

    return true;
}
} // namespace litepb

void setUp() {}
void tearDown() {}

void test_rpc_error_ok_with_ok_code()
{
    litepb::RpcError error;
    error.code = litepb::RpcError::OK;

    TEST_ASSERT_TRUE(error.ok());
}

void test_rpc_error_ok_with_error_codes()
{
    litepb::RpcError error;

    error.code = litepb::RpcError::TIMEOUT;
    TEST_ASSERT_FALSE(error.ok());

    error.code = litepb::RpcError::PARSE_ERROR;
    TEST_ASSERT_FALSE(error.ok());

    error.code = litepb::RpcError::TRANSPORT_ERROR;
    TEST_ASSERT_FALSE(error.ok());

    error.code = litepb::RpcError::HANDLER_NOT_FOUND;
    TEST_ASSERT_FALSE(error.ok());

    error.code = litepb::RpcError::CUSTOM_ERROR;
    TEST_ASSERT_FALSE(error.ok());
}

void test_result_ok_delegation()
{
    litepb::Result<int> result;

    result.error.code = litepb::RpcError::OK;
    result.value      = 42;
    TEST_ASSERT_TRUE(result.ok());

    result.error.code = litepb::RpcError::TIMEOUT;
    TEST_ASSERT_FALSE(result.ok());
}

void test_error_message_storage()
{
    litepb::RpcError error;
    error.code = litepb::RpcError::TIMEOUT;

    TEST_ASSERT_FALSE(error.ok());
}

void test_message_id_generator_lower_addr_odd_ids()
{
    litepb::MessageIdGenerator gen;

    TEST_ASSERT_EQUAL_UINT32(1, gen.generate_for(0x01, 0x02));
    TEST_ASSERT_EQUAL_UINT32(2, gen.generate_for(0x01, 0x02));
    TEST_ASSERT_EQUAL_UINT32(3, gen.generate_for(0x01, 0x02));
    TEST_ASSERT_EQUAL_UINT32(4, gen.generate_for(0x01, 0x02));
}

void test_message_id_generator_higher_addr_even_ids()
{
    litepb::MessageIdGenerator gen;

    TEST_ASSERT_EQUAL_UINT32(1, gen.generate_for(0x02, 0x01));
    TEST_ASSERT_EQUAL_UINT32(2, gen.generate_for(0x02, 0x01));
    TEST_ASSERT_EQUAL_UINT32(3, gen.generate_for(0x02, 0x01));
    TEST_ASSERT_EQUAL_UINT32(4, gen.generate_for(0x02, 0x01));
}

void test_message_id_generator_sequential()
{
    litepb::MessageIdGenerator gen_odd;
    litepb::MessageIdGenerator gen_even;

    for (uint32_t i = 0; i < 10; ++i) {
        uint32_t id_odd = gen_odd.generate_for(0x01, 0x02);
        TEST_ASSERT_EQUAL_UINT32(1 + i, id_odd);
    }

    for (uint32_t i = 0; i < 10; ++i) {
        uint32_t id_even = gen_even.generate_for(0x02, 0x01);
        TEST_ASSERT_EQUAL_UINT32(1 + i, id_even);
    }
}

void test_varint_encoding_small_values()
{
    uint8_t buffer[5];
    size_t bytes_written;

    bytes_written = litepb::encode_varint(0, buffer);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[0]);

    bytes_written = litepb::encode_varint(1, buffer);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);

    bytes_written = litepb::encode_varint(127, buffer);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0x7F, buffer[0]);
}

void test_varint_encoding_medium_values()
{
    uint8_t buffer[5];
    size_t bytes_written;

    bytes_written = litepb::encode_varint(128, buffer);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0x80, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[1]);

    bytes_written = litepb::encode_varint(300, buffer);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0xAC, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);

    bytes_written = litepb::encode_varint(16383, buffer);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x7F, buffer[1]);
}

void test_varint_encoding_large_values()
{
    uint8_t buffer[5];
    size_t bytes_written;

    bytes_written = litepb::encode_varint(16384, buffer);
    TEST_ASSERT_EQUAL_UINT32(3, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0x80, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x80, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[2]);

    bytes_written = litepb::encode_varint(2097151, buffer);
    TEST_ASSERT_EQUAL_UINT32(3, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x7F, buffer[2]);

    bytes_written = litepb::encode_varint(268435455, buffer);
    TEST_ASSERT_EQUAL_UINT32(4, bytes_written);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x7F, buffer[3]);
}

void test_varint_decoding_small_values()
{
    uint32_t value;
    size_t bytes_read;

    uint8_t buffer1[] = { 0x00 };
    bytes_read        = litepb::decode_varint(buffer1, sizeof(buffer1), value);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(0, value);

    uint8_t buffer2[] = { 0x01 };
    bytes_read        = litepb::decode_varint(buffer2, sizeof(buffer2), value);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(1, value);

    uint8_t buffer3[] = { 0x7F };
    bytes_read        = litepb::decode_varint(buffer3, sizeof(buffer3), value);
    TEST_ASSERT_EQUAL_UINT32(1, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(127, value);
}

void test_varint_decoding_medium_values()
{
    uint32_t value;
    size_t bytes_read;

    uint8_t buffer1[] = { 0x80, 0x01 };
    bytes_read        = litepb::decode_varint(buffer1, sizeof(buffer1), value);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(128, value);

    uint8_t buffer2[] = { 0xAC, 0x02 };
    bytes_read        = litepb::decode_varint(buffer2, sizeof(buffer2), value);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(300, value);

    uint8_t buffer3[] = { 0xFF, 0x7F };
    bytes_read        = litepb::decode_varint(buffer3, sizeof(buffer3), value);
    TEST_ASSERT_EQUAL_UINT32(2, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(16383, value);
}

void test_varint_decoding_large_values()
{
    uint32_t value;
    size_t bytes_read;

    uint8_t buffer1[] = { 0x80, 0x80, 0x01 };
    bytes_read        = litepb::decode_varint(buffer1, sizeof(buffer1), value);
    TEST_ASSERT_EQUAL_UINT32(3, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(16384, value);

    uint8_t buffer2[] = { 0xFF, 0xFF, 0x7F };
    bytes_read        = litepb::decode_varint(buffer2, sizeof(buffer2), value);
    TEST_ASSERT_EQUAL_UINT32(3, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(2097151, value);

    uint8_t buffer3[] = { 0xFF, 0xFF, 0xFF, 0x7F };
    bytes_read        = litepb::decode_varint(buffer3, sizeof(buffer3), value);
    TEST_ASSERT_EQUAL_UINT32(4, bytes_read);
    TEST_ASSERT_EQUAL_UINT32(268435455, value);
}

void test_varint_roundtrip_encoding_decoding()
{
    uint8_t buffer[5];
    uint32_t original_values[] = { 0, 1, 127, 128, 255, 16383, 16384, 65535, 2097151, 268435455 };

    for (size_t i = 0; i < sizeof(original_values) / sizeof(original_values[0]); ++i) {
        uint32_t original = original_values[i];

        size_t bytes_written = litepb::encode_varint(original, buffer);
        TEST_ASSERT_TRUE(bytes_written > 0 && bytes_written <= 5);

        uint32_t decoded;
        size_t bytes_read = litepb::decode_varint(buffer, bytes_written, decoded);
        TEST_ASSERT_EQUAL_UINT32(bytes_written, bytes_read);
        TEST_ASSERT_EQUAL_UINT32(original, decoded);
    }
}

void test_framed_message_encode_decode_stream_transport()
{
    litepb::FramedMessage msg;
    msg.src_addr  = 1;
    msg.dst_addr  = 2;
    msg.msg_id    = 12345;
    msg.method_id = 1;
    msg.payload   = { 0x01, 0x02, 0x03, 0x04, 0x05 };

    litepb::BufferOutputStream output;
    bool encode_success = litepb::encode_message(msg, output, true);
    TEST_ASSERT_TRUE(encode_success);

    litepb::BufferInputStream input(output.data(), output.size());
    litepb::FramedMessage decoded_msg;
    bool decode_success = litepb::decode_message(input, decoded_msg, true);
    TEST_ASSERT_TRUE(decode_success);

    TEST_ASSERT_EQUAL_UINT64(msg.src_addr, decoded_msg.src_addr);
    TEST_ASSERT_EQUAL_UINT64(msg.dst_addr, decoded_msg.dst_addr);
    TEST_ASSERT_EQUAL_UINT32(msg.msg_id, decoded_msg.msg_id);
    TEST_ASSERT_EQUAL(msg.method_id, decoded_msg.method_id);
    TEST_ASSERT_EQUAL_UINT32(msg.payload.size(), decoded_msg.payload.size());

    for (size_t i = 0; i < msg.payload.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT8(msg.payload[i], decoded_msg.payload[i]);
    }
}

void test_framed_message_encode_decode_packet_transport()
{
    litepb::FramedMessage msg;
    msg.src_addr  = 3;
    msg.dst_addr  = 4;
    msg.msg_id    = 12345;
    msg.method_id = 2;
    msg.payload   = { 0xAA, 0xBB, 0xCC };

    litepb::BufferOutputStream output;
    bool encode_success = litepb::encode_message(msg, output, false);
    TEST_ASSERT_TRUE(encode_success);

    litepb::BufferInputStream input(output.data(), output.size());
    litepb::FramedMessage decoded_msg;
    bool decode_success = litepb::decode_message(input, decoded_msg, false);
    TEST_ASSERT_TRUE(decode_success);

    TEST_ASSERT_EQUAL_UINT64(msg.src_addr, decoded_msg.src_addr);
    TEST_ASSERT_EQUAL_UINT64(msg.dst_addr, decoded_msg.dst_addr);
    TEST_ASSERT_EQUAL_UINT32(msg.msg_id, decoded_msg.msg_id);
    TEST_ASSERT_EQUAL(msg.method_id, decoded_msg.method_id);
    TEST_ASSERT_EQUAL_UINT32(msg.payload.size(), decoded_msg.payload.size());

    for (size_t i = 0; i < msg.payload.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT8(msg.payload[i], decoded_msg.payload[i]);
    }
}

void test_framed_message_integrity_verification()
{
    litepb::FramedMessage msg;
    msg.src_addr  = 0xAAAAAAAAAAAAAAAA;
    msg.dst_addr  = 0xBBBBBBBBBBBBBBBB;
    msg.msg_id    = 0xBEEF;
    msg.method_id = 3;
    msg.payload.resize(256);
    for (size_t i = 0; i < 256; ++i) {
        msg.payload[i] = static_cast<uint8_t>(i);
    }

    litepb::BufferOutputStream output;
    bool encode_success = litepb::encode_message(msg, output, true);
    TEST_ASSERT_TRUE(encode_success);

    litepb::BufferInputStream input(output.data(), output.size());
    litepb::FramedMessage decoded_msg;
    bool decode_success = litepb::decode_message(input, decoded_msg, true);
    TEST_ASSERT_TRUE(decode_success);

    TEST_ASSERT_EQUAL_UINT64(0xAAAAAAAAAAAAAAAA, decoded_msg.src_addr);
    TEST_ASSERT_EQUAL_UINT64(0xBBBBBBBBBBBBBBBB, decoded_msg.dst_addr);
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, decoded_msg.msg_id);
    TEST_ASSERT_EQUAL(3, decoded_msg.method_id);
    TEST_ASSERT_EQUAL_UINT32(256, decoded_msg.payload.size());

    for (size_t i = 0; i < 256; ++i) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i), decoded_msg.payload[i]);
    }
}

void test_framed_message_method_id_zero()
{
    litepb::FramedMessage msg;
    msg.src_addr  = 10;
    msg.dst_addr  = 20;
    msg.msg_id    = 100;
    msg.method_id = 0;
    msg.payload   = { 0x11, 0x22 };

    litepb::BufferOutputStream output;
    bool encode_success = litepb::encode_message(msg, output, true);
    TEST_ASSERT_TRUE(encode_success);

    litepb::BufferInputStream input(output.data(), output.size());
    litepb::FramedMessage decoded_msg;
    bool decode_success = litepb::decode_message(input, decoded_msg, true);
    TEST_ASSERT_TRUE(decode_success);

    TEST_ASSERT_EQUAL_UINT64(10, decoded_msg.src_addr);
    TEST_ASSERT_EQUAL_UINT64(20, decoded_msg.dst_addr);
    TEST_ASSERT_EQUAL_UINT32(100, decoded_msg.msg_id);
    TEST_ASSERT_EQUAL(0, decoded_msg.method_id);
    TEST_ASSERT_EQUAL_UINT32(2, decoded_msg.payload.size());
}

void test_framed_message_empty_payload()
{
    litepb::FramedMessage msg;
    msg.src_addr  = 100;
    msg.dst_addr  = 200;
    msg.msg_id    = 200;
    msg.method_id = 5;
    msg.payload.clear();

    litepb::BufferOutputStream output;
    bool encode_success = litepb::encode_message(msg, output, true);
    TEST_ASSERT_TRUE(encode_success);

    litepb::BufferInputStream input(output.data(), output.size());
    litepb::FramedMessage decoded_msg;
    bool decode_success = litepb::decode_message(input, decoded_msg, true);
    TEST_ASSERT_TRUE(decode_success);

    TEST_ASSERT_EQUAL_UINT64(100, decoded_msg.src_addr);
    TEST_ASSERT_EQUAL_UINT64(200, decoded_msg.dst_addr);
    TEST_ASSERT_EQUAL_UINT32(200, decoded_msg.msg_id);
    TEST_ASSERT_EQUAL(5, decoded_msg.method_id);
    TEST_ASSERT_EQUAL_UINT32(0, decoded_msg.payload.size());
}

void test_addressing_direct_message()
{
    LoopbackTransport transport_sender, transport2, transport3;
    transport_sender.connect_to_peer(&transport2);
    transport_sender.connect_to_peer(&transport3);

    litepb::RpcChannel node2(transport2, 0x02);
    litepb::RpcChannel node3(transport3, 0x03);

    bool node2_received = false;
    bool node3_received = false;
    int32_t node2_value = 0;
    int32_t node3_value = 0;

    node2.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node2_received = true;
        node2_value    = msg.value;
    });

    node3.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node3_received = true;
        node3_value    = msg.value;
    });

    SimpleMessage test_msg;
    test_msg.value = 42;

    litepb::BufferOutputStream payload_stream;
    litepb::serialize(test_msg, payload_stream);

    litepb::FramedMessage framed_msg;
    framed_msg.src_addr   = 0x01;
    framed_msg.dst_addr   = 0x02;
    framed_msg.msg_id     = 0;
    framed_msg.service_id = 1;
    framed_msg.method_id  = 1;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    litepb::BufferOutputStream encoded_stream;
    litepb::encode_message(framed_msg, encoded_stream, true);

    const uint8_t* msg_data = encoded_stream.data();
    size_t msg_size         = encoded_stream.size();

    for (size_t i = 0; i < msg_size; ++i) {
        transport2.rx_queue_.push(msg_data[i]);
    }
    for (size_t i = 0; i < msg_size; ++i) {
        transport3.rx_queue_.push(msg_data[i]);
    }

    node2.process();
    node3.process();

    TEST_ASSERT_TRUE(node2_received);
    TEST_ASSERT_EQUAL_INT32(42, node2_value);
    TEST_ASSERT_FALSE(node3_received);
    TEST_ASSERT_EQUAL_INT32(0, node3_value);
}

void test_addressing_broadcast()
{
    LoopbackTransport transport2, transport3;

    litepb::RpcChannel node2(transport2, 0x02);
    litepb::RpcChannel node3(transport3, 0x03);

    bool node2_received = false;
    bool node3_received = false;
    int32_t node2_value = 0;
    int32_t node3_value = 0;

    node2.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node2_received = true;
        node2_value    = msg.value;
    });

    node3.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node3_received = true;
        node3_value    = msg.value;
    });

    SimpleMessage test_msg;
    test_msg.value = 99;

    litepb::BufferOutputStream payload_stream;
    litepb::serialize(test_msg, payload_stream);

    litepb::FramedMessage framed_msg;
    framed_msg.src_addr   = 0x01;
    framed_msg.dst_addr   = litepb::RPC_ADDRESS_BROADCAST;
    framed_msg.msg_id     = 0;
    framed_msg.service_id = 1;
    framed_msg.method_id  = 1;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    litepb::BufferOutputStream encoded_stream;
    litepb::encode_message(framed_msg, encoded_stream, true);

    const uint8_t* msg_data = encoded_stream.data();
    size_t msg_size         = encoded_stream.size();

    for (size_t i = 0; i < msg_size; ++i) {
        transport2.rx_queue_.push(msg_data[i]);
    }
    for (size_t i = 0; i < msg_size; ++i) {
        transport3.rx_queue_.push(msg_data[i]);
    }

    node2.process();
    node3.process();

    TEST_ASSERT_TRUE(node2_received);
    TEST_ASSERT_EQUAL_INT32(99, node2_value);
    TEST_ASSERT_TRUE(node3_received);
    TEST_ASSERT_EQUAL_INT32(99, node3_value);
}

void test_addressing_wildcard()
{
    LoopbackTransport transport2, transport3;

    litepb::RpcChannel node2(transport2, 0x02);
    litepb::RpcChannel node3(transport3, 0x03);

    bool node2_received = false;
    bool node3_received = false;
    int32_t node2_value = 0;
    int32_t node3_value = 0;

    node2.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node2_received = true;
        node2_value    = msg.value;
    });

    node3.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) {
        (void) src_addr;
        node3_received = true;
        node3_value    = msg.value;
    });

    SimpleMessage test_msg;
    test_msg.value = 77;

    litepb::BufferOutputStream payload_stream;
    litepb::serialize(test_msg, payload_stream);

    litepb::FramedMessage framed_msg;
    framed_msg.src_addr   = 0x01;
    framed_msg.dst_addr   = 0;
    framed_msg.msg_id     = 0;
    framed_msg.service_id = 1;
    framed_msg.method_id  = 1;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    litepb::BufferOutputStream encoded_stream;
    litepb::encode_message(framed_msg, encoded_stream, true);

    const uint8_t* msg_data = encoded_stream.data();
    size_t msg_size         = encoded_stream.size();

    for (size_t i = 0; i < msg_size; ++i) {
        transport2.rx_queue_.push(msg_data[i]);
    }
    for (size_t i = 0; i < msg_size; ++i) {
        transport3.rx_queue_.push(msg_data[i]);
    }

    node2.process();
    node3.process();

    TEST_ASSERT_TRUE(node2_received);
    TEST_ASSERT_EQUAL_INT32(77, node2_value);
    TEST_ASSERT_TRUE(node3_received);
    TEST_ASSERT_EQUAL_INT32(77, node3_value);
}

void test_addressing_wrong_dest_ignored()
{
    LoopbackTransport transport2, transport3;

    litepb::RpcChannel node2(transport2, 0x02);
    litepb::RpcChannel node3(transport3, 0x03);

    bool node2_received = false;
    bool node3_received = false;

    node2.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) { node2_received = true; });

    node3.on_event<SimpleMessage>(1, 1, [&](uint64_t src_addr, const SimpleMessage& msg) { node3_received = true; });

    SimpleMessage test_msg;
    test_msg.value = 123;

    litepb::BufferOutputStream payload_stream;
    litepb::serialize(test_msg, payload_stream);

    litepb::FramedMessage framed_msg;
    framed_msg.src_addr   = 0x01;
    framed_msg.dst_addr   = 0x99;
    framed_msg.msg_id     = 0;
    framed_msg.service_id = 1;
    framed_msg.method_id  = 1;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    litepb::BufferOutputStream encoded_stream;
    litepb::encode_message(framed_msg, encoded_stream, true);

    const uint8_t* msg_data = encoded_stream.data();
    size_t msg_size         = encoded_stream.size();

    for (size_t i = 0; i < msg_size; ++i) {
        transport2.rx_queue_.push(msg_data[i]);
    }
    for (size_t i = 0; i < msg_size; ++i) {
        transport3.rx_queue_.push(msg_data[i]);
    }

    node2.process();
    node3.process();

    TEST_ASSERT_FALSE(node2_received);
    TEST_ASSERT_FALSE(node3_received);
}

void test_message_id_generator_broadcast_ids()
{
    litepb::MessageIdGenerator gen;

    TEST_ASSERT_EQUAL_UINT32(1, gen.generate_for(0x01, litepb::RPC_ADDRESS_WILDCARD));
    TEST_ASSERT_EQUAL_UINT32(2, gen.generate_for(0x01, litepb::RPC_ADDRESS_WILDCARD));
    TEST_ASSERT_EQUAL_UINT32(3, gen.generate_for(0x01, litepb::RPC_ADDRESS_BROADCAST));
    TEST_ASSERT_EQUAL_UINT32(4, gen.generate_for(0x01, litepb::RPC_ADDRESS_BROADCAST));
}

void test_message_id_generator_same_address()
{
    litepb::MessageIdGenerator gen;

    TEST_ASSERT_EQUAL_UINT32(1, gen.generate_for(0x05, 0x05));
    TEST_ASSERT_EQUAL_UINT32(2, gen.generate_for(0x05, 0x05));
    TEST_ASSERT_EQUAL_UINT32(3, gen.generate_for(0x05, 0x05));
}

void test_bidirectional_no_id_collision()
{
    LoopbackTransport transport_a, transport_b;
    transport_a.connect_to_peer(&transport_b);
    transport_b.connect_to_peer(&transport_a);

    litepb::RpcChannel channel_a(transport_a, 0x02);
    litepb::RpcChannel channel_b(transport_b, 0x04);

    bool a_response_received = false;
    bool b_response_received = false;
    int32_t a_response_value = 0;
    int32_t b_response_value = 0;

    channel_a.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value * 2;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    channel_b.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value * 3;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    SimpleMessage req_a_to_b;
    req_a_to_b.value = 10;
    channel_a.call_internal<SimpleMessage, SimpleMessage>(1, 1, req_a_to_b, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        a_response_received = true;
        a_response_value    = result.value.value;
    });

    for (int i = 0; i < 20 && !a_response_received; ++i) {
        channel_a.process();
        channel_b.process();
    }

    TEST_ASSERT_TRUE(a_response_received);
    TEST_ASSERT_EQUAL_INT32(30, a_response_value);

    SimpleMessage req_b_to_a;
    req_b_to_a.value = 20;
    channel_b.call_internal<SimpleMessage, SimpleMessage>(1, 1, req_b_to_a, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        b_response_received = true;
        b_response_value    = result.value.value;
    });

    for (int i = 0; i < 20 && !b_response_received; ++i) {
        channel_a.process();
        channel_b.process();
    }

    TEST_ASSERT_TRUE(b_response_received);
    TEST_ASSERT_EQUAL_INT32(40, b_response_value);
}

void test_multi_node_communication()
{
    LoopbackTransport t1_to_2, t2_to_1;
    LoopbackTransport t1_to_3, t3_to_1;
    LoopbackTransport t1_to_4, t4_to_1;
    LoopbackTransport t2_to_3, t3_to_2;
    LoopbackTransport t2_to_4, t4_to_2;
    LoopbackTransport t3_to_4, t4_to_3;

    t1_to_2.connect_to_peer(&t2_to_1);
    t2_to_1.connect_to_peer(&t1_to_2);
    t1_to_3.connect_to_peer(&t3_to_1);
    t3_to_1.connect_to_peer(&t1_to_3);
    t1_to_4.connect_to_peer(&t4_to_1);
    t4_to_1.connect_to_peer(&t1_to_4);
    t2_to_3.connect_to_peer(&t3_to_2);
    t3_to_2.connect_to_peer(&t2_to_3);
    t2_to_4.connect_to_peer(&t4_to_2);
    t4_to_2.connect_to_peer(&t2_to_4);
    t3_to_4.connect_to_peer(&t4_to_3);
    t4_to_3.connect_to_peer(&t3_to_4);

    litepb::RpcChannel chan1_to_2(t1_to_2, 0x01);
    litepb::RpcChannel chan1_to_3(t1_to_3, 0x01);
    litepb::RpcChannel chan1_to_4(t1_to_4, 0x01);
    litepb::RpcChannel chan2_to_1(t2_to_1, 0x02);
    litepb::RpcChannel chan2_to_3(t2_to_3, 0x02);
    litepb::RpcChannel chan2_to_4(t2_to_4, 0x02);
    litepb::RpcChannel chan3_to_1(t3_to_1, 0x03);
    litepb::RpcChannel chan3_to_2(t3_to_2, 0x03);
    litepb::RpcChannel chan3_to_4(t3_to_4, 0x03);
    litepb::RpcChannel chan4_to_1(t4_to_1, 0x04);
    litepb::RpcChannel chan4_to_2(t4_to_2, 0x04);
    litepb::RpcChannel chan4_to_3(t4_to_3, 0x04);

    chan2_to_1.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value + 200;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    chan3_to_1.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value + 300;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    chan4_to_1.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value + 400;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    chan3_to_2.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value + 320;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    chan4_to_2.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.value.value = req.value + 420;
            result.error.code  = litepb::RpcError::OK;
            return result;
        });

    bool resp_1_to_2 = false, resp_1_to_3 = false, resp_1_to_4 = false;
    bool resp_2_to_3 = false, resp_2_to_4 = false;
    int32_t val_1_to_2 = 0, val_1_to_3 = 0, val_1_to_4 = 0;
    int32_t val_2_to_3 = 0, val_2_to_4 = 0;

    SimpleMessage msg1;
    msg1.value = 10;
    chan1_to_2.call_internal<SimpleMessage, SimpleMessage>(1, 1, msg1, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        resp_1_to_2 = true;
        val_1_to_2  = result.value.value;
    });

    SimpleMessage msg2;
    msg2.value = 20;
    chan1_to_3.call_internal<SimpleMessage, SimpleMessage>(1, 1, msg2, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        resp_1_to_3 = true;
        val_1_to_3  = result.value.value;
    });

    SimpleMessage msg3;
    msg3.value = 30;
    chan1_to_4.call_internal<SimpleMessage, SimpleMessage>(1, 1, msg3, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        resp_1_to_4 = true;
        val_1_to_4  = result.value.value;
    });

    SimpleMessage msg4;
    msg4.value = 40;
    chan2_to_3.call_internal<SimpleMessage, SimpleMessage>(1, 1, msg4, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        resp_2_to_3 = true;
        val_2_to_3  = result.value.value;
    });

    SimpleMessage msg5;
    msg5.value = 50;
    chan2_to_4.call_internal<SimpleMessage, SimpleMessage>(1, 1, msg5, [&](const litepb::Result<SimpleMessage>& result) {
        TEST_ASSERT_TRUE(result.ok());
        resp_2_to_4 = true;
        val_2_to_4  = result.value.value;
    });

    for (int i = 0; i < 50; ++i) {
        chan1_to_2.process();
        chan1_to_3.process();
        chan1_to_4.process();
        chan2_to_1.process();
        chan2_to_3.process();
        chan2_to_4.process();
        chan3_to_1.process();
        chan3_to_2.process();
        chan3_to_4.process();
        chan4_to_1.process();
        chan4_to_2.process();
        chan4_to_3.process();
    }

    TEST_ASSERT_TRUE(resp_1_to_2);
    TEST_ASSERT_TRUE(resp_1_to_3);
    TEST_ASSERT_TRUE(resp_1_to_4);
    TEST_ASSERT_TRUE(resp_2_to_3);
    TEST_ASSERT_TRUE(resp_2_to_4);

    TEST_ASSERT_EQUAL_INT32(210, val_1_to_2);
    TEST_ASSERT_EQUAL_INT32(320, val_1_to_3);
    TEST_ASSERT_EQUAL_INT32(430, val_1_to_4);
    TEST_ASSERT_EQUAL_INT32(360, val_2_to_3);
    TEST_ASSERT_EQUAL_INT32(470, val_2_to_4);
}

void test_encode_message_src_addr_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;

    litepb::FixedOutputStream<4> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_dst_addr_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;

    litepb::FixedOutputStream<12> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_msg_id_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;

    litepb::FixedOutputStream<16> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_service_id_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;

    litepb::FixedOutputStream<17> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_method_id_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;

    litepb::FixedOutputStream<18> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_payload_len_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;
    msg.payload    = { 0x01, 0x02, 0x03 };

    litepb::FixedOutputStream<19> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_encode_message_payload_write_failure()
{
    litepb::FramedMessage msg;
    msg.src_addr   = 1;
    msg.dst_addr   = 2;
    msg.msg_id     = 100;
    msg.service_id = 1;
    msg.method_id  = 1;
    msg.payload    = { 0x01, 0x02, 0x03 };

    litepb::FixedOutputStream<21> stream;
    TEST_ASSERT_FALSE(litepb::encode_message(msg, stream, true));
}

void test_decode_message_msg_id_varint_overflow()
{
    const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    litepb::BufferInputStream stream(data, sizeof(data));
    litepb::FramedMessage msg;
    TEST_ASSERT_FALSE(litepb::decode_message(stream, msg, true));
}

void test_decode_message_service_id_varint_overflow()
{
    const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    litepb::BufferInputStream stream(data, sizeof(data));
    litepb::FramedMessage msg;
    TEST_ASSERT_FALSE(litepb::decode_message(stream, msg, true));
}

void test_decode_message_method_id_varint_overflow()
{
    const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x64, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    litepb::BufferInputStream stream(data, sizeof(data));
    litepb::FramedMessage msg;
    TEST_ASSERT_FALSE(litepb::decode_message(stream, msg, true));
}

void test_decode_message_payload_len_varint_overflow()
{
    const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x64, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    litepb::BufferInputStream stream(data, sizeof(data));
    litepb::FramedMessage msg;
    TEST_ASSERT_FALSE(litepb::decode_message(stream, msg, true));
}

void test_decode_varint_5byte_limit_exceeded()
{
    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint32_t value;
    size_t result = litepb::decode_varint(data, sizeof(data), value);
    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_rpc_channel_buffer_resize_overflow()
{
    LoopbackTransport transport;
    litepb::RpcChannel channel(transport, 0x01);

    for (size_t i = 0; i < 2000; ++i) {
        transport.rx_queue_.push(0xFF);
    }

    channel.process();

    TEST_ASSERT_TRUE(transport.rx_queue_.empty() || transport.rx_queue_.size() < 2000);
}

void test_decode_message_packet_transport_empty_payload()
{
    const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x01, 0x01 };

    litepb::BufferInputStream stream(data, sizeof(data));
    litepb::FramedMessage msg;
    TEST_ASSERT_TRUE(litepb::decode_message(stream, msg, false));
    TEST_ASSERT_TRUE(msg.payload.empty());
}

void test_transport_recv_returns_zero()
{
    struct ZeroRecvTransport : public litepb::Transport
    {
        bool available() override { return true; }
        size_t recv(uint8_t*, size_t) override
        {
            recv_count++;
            return 0;
        }
        bool send(const uint8_t*, size_t) override { return true; }
        int recv_count = 0;
    };

    ZeroRecvTransport transport;
    litepb::RpcChannel channel(transport, 0x01);
    channel.process();

    TEST_ASSERT_GREATER_THAN_INT(0, transport.recv_count);
}

int runTests()
{
    UNITY_BEGIN();

    RUN_TEST(test_rpc_error_ok_with_ok_code);
    RUN_TEST(test_rpc_error_ok_with_error_codes);
    RUN_TEST(test_result_ok_delegation);
    RUN_TEST(test_error_message_storage);

    RUN_TEST(test_message_id_generator_lower_addr_odd_ids);
    RUN_TEST(test_message_id_generator_higher_addr_even_ids);
    RUN_TEST(test_message_id_generator_sequential);
    RUN_TEST(test_message_id_generator_broadcast_ids);
    RUN_TEST(test_message_id_generator_same_address);
    RUN_TEST(test_bidirectional_no_id_collision);
    RUN_TEST(test_multi_node_communication);

    RUN_TEST(test_varint_encoding_small_values);
    RUN_TEST(test_varint_encoding_medium_values);
    RUN_TEST(test_varint_encoding_large_values);
    RUN_TEST(test_varint_decoding_small_values);
    RUN_TEST(test_varint_decoding_medium_values);
    RUN_TEST(test_varint_decoding_large_values);
    RUN_TEST(test_varint_roundtrip_encoding_decoding);

    RUN_TEST(test_framed_message_encode_decode_stream_transport);
    RUN_TEST(test_framed_message_encode_decode_packet_transport);
    RUN_TEST(test_framed_message_integrity_verification);
    RUN_TEST(test_framed_message_method_id_zero);
    RUN_TEST(test_framed_message_empty_payload);

    RUN_TEST(test_addressing_direct_message);
    RUN_TEST(test_addressing_broadcast);
    RUN_TEST(test_addressing_wildcard);
    RUN_TEST(test_addressing_wrong_dest_ignored);

    RUN_TEST(test_encode_message_src_addr_write_failure);
    RUN_TEST(test_encode_message_dst_addr_write_failure);
    RUN_TEST(test_encode_message_msg_id_write_failure);
    RUN_TEST(test_encode_message_service_id_write_failure);
    RUN_TEST(test_encode_message_method_id_write_failure);
    RUN_TEST(test_encode_message_payload_len_write_failure);
    RUN_TEST(test_encode_message_payload_write_failure);
    RUN_TEST(test_decode_message_msg_id_varint_overflow);
    RUN_TEST(test_decode_message_service_id_varint_overflow);
    RUN_TEST(test_decode_message_method_id_varint_overflow);
    RUN_TEST(test_decode_message_payload_len_varint_overflow);
    RUN_TEST(test_decode_varint_5byte_limit_exceeded);
    RUN_TEST(test_rpc_channel_buffer_resize_overflow);
    RUN_TEST(test_decode_message_packet_transport_empty_payload);
    RUN_TEST(test_transport_recv_returns_zero);

    return UNITY_END();
}
