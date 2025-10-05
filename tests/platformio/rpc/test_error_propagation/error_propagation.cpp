#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include "litepb/rpc/addressing.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/error.h"
#include "litepb/rpc/framing.h"
#include "litepb/rpc/transport.h"
#include "unity.h"
#include <climits>
#include <cstdint>
#include <queue>
#include <vector>

class LoopbackTransport : public litepb::StreamTransport
{
public:
    LoopbackTransport() : peer_(nullptr), capture_enabled_(false) {}

    void connect_to_peer(LoopbackTransport* peer) { peer_ = peer; }

    void enable_capture()
    {
        capture_enabled_ = true;
        captured_bytes_.clear();
    }

    void disable_capture() { capture_enabled_ = false; }

    const std::vector<uint8_t>& get_captured_bytes() const { return captured_bytes_; }

    bool send(const uint8_t* data, size_t len) override
    {
        if (capture_enabled_) {
            for (size_t i = 0; i < len; ++i) {
                captured_bytes_.push_back(data[i]);
            }
        }

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
    bool capture_enabled_;
    std::vector<uint8_t> captured_bytes_;
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

void test_app_code_propagation_basic()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool response_received                     = false;
    litepb::RpcError::Code received_error_code = litepb::RpcError::OK;
    int32_t received_app_code                  = 0;

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 404;
            result.value.value    = req.value;
            return result;
        });

    SimpleMessage request;
    request.value = 42;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        response_received   = true;
        received_error_code = result.error.code;
        received_app_code   = result.error.app_code;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code));
    TEST_ASSERT_EQUAL_INT32(404, received_app_code);
}

void test_app_code_propagation_various_codes()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    int32_t test_app_codes[] = { 100, 404, 500, 1000 };

    for (size_t i = 0; i < sizeof(test_app_codes) / sizeof(test_app_codes[0]); ++i) {
        int32_t expected_app_code = test_app_codes[i];

        bool response_received                     = false;
        litepb::RpcError::Code received_error_code = litepb::RpcError::OK;
        int32_t received_app_code                  = 0;

        peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
            1, i + 1, [expected_app_code](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
                (void) src_addr;
                litepb::Result<SimpleMessage> result;
                result.error.code     = litepb::RpcError::CUSTOM_ERROR;
                result.error.app_code = expected_app_code;
                result.value.value    = req.value;
                return result;
            });

        SimpleMessage request;
        request.value = static_cast<int32_t>(i);

        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, i + 1, request,
                                                                   [&](const litepb::Result<SimpleMessage>& result) {
                                                                       response_received   = true;
                                                                       received_error_code = result.error.code;
                                                                       received_app_code   = result.error.app_code;
                                                                   });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }

        TEST_ASSERT_TRUE(response_received);
        TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code));
        TEST_ASSERT_EQUAL_INT32(expected_app_code, received_app_code);
    }
}

void test_app_code_zero_vs_nonzero()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool response_zero_received                     = false;
    litepb::RpcError::Code received_error_code_zero = litepb::RpcError::OK;
    int32_t received_app_code_zero                  = -1;

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 0;
            result.value.value    = req.value;
            return result;
        });

    SimpleMessage request_zero;
    request_zero.value = 1;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request_zero,
                                                               [&](const litepb::Result<SimpleMessage>& result) {
                                                                   response_zero_received   = true;
                                                                   received_error_code_zero = result.error.code;
                                                                   received_app_code_zero   = result.error.app_code;
                                                               });

    for (int i = 0; i < 10 && !response_zero_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_zero_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code_zero));
    TEST_ASSERT_EQUAL_INT32(0, received_app_code_zero);

    bool response_nonzero_received                     = false;
    litepb::RpcError::Code received_error_code_nonzero = litepb::RpcError::OK;
    int32_t received_app_code_nonzero                  = 0;

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 2, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 404;
            result.value.value    = req.value;
            return result;
        });

    SimpleMessage request_nonzero;
    request_nonzero.value = 2;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 2, request_nonzero,
                                                               [&](const litepb::Result<SimpleMessage>& result) {
                                                                   response_nonzero_received   = true;
                                                                   received_error_code_nonzero = result.error.code;
                                                                   received_app_code_nonzero   = result.error.app_code;
                                                               });

    for (int i = 0; i < 10 && !response_nonzero_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_nonzero_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code_nonzero));
    TEST_ASSERT_EQUAL_INT32(404, received_app_code_nonzero);
}

void test_app_code_negative_values()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    int32_t test_negative_codes[] = { -1, -100, INT32_MIN };

    for (size_t i = 0; i < sizeof(test_negative_codes) / sizeof(test_negative_codes[0]); ++i) {
        int32_t expected_app_code = test_negative_codes[i];

        bool response_received                     = false;
        litepb::RpcError::Code received_error_code = litepb::RpcError::OK;
        int32_t received_app_code                  = 0;

        peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
            1, i + 1, [expected_app_code](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
                (void) src_addr;
                litepb::Result<SimpleMessage> result;
                result.error.code     = litepb::RpcError::CUSTOM_ERROR;
                result.error.app_code = expected_app_code;
                result.value.value    = req.value;
                return result;
            });

        SimpleMessage request;
        request.value = static_cast<int32_t>(i);

        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, i + 1, request,
                                                                   [&](const litepb::Result<SimpleMessage>& result) {
                                                                       response_received   = true;
                                                                       received_error_code = result.error.code;
                                                                       received_app_code   = result.error.app_code;
                                                                   });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }

        TEST_ASSERT_TRUE(response_received);
        TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code));
        TEST_ASSERT_EQUAL_INT32(expected_app_code, received_app_code);
    }
}

void test_app_code_boundary_values()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    int32_t test_boundary_codes[] = { INT32_MAX, INT32_MIN };

    for (size_t i = 0; i < sizeof(test_boundary_codes) / sizeof(test_boundary_codes[0]); ++i) {
        int32_t expected_app_code = test_boundary_codes[i];

        bool response_received                     = false;
        litepb::RpcError::Code received_error_code = litepb::RpcError::OK;
        int32_t received_app_code                  = 0;

        peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
            1, i + 1, [expected_app_code](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
                (void) src_addr;
                litepb::Result<SimpleMessage> result;
                result.error.code     = litepb::RpcError::CUSTOM_ERROR;
                result.error.app_code = expected_app_code;
                result.value.value    = req.value;
                return result;
            });

        SimpleMessage request;
        request.value = static_cast<int32_t>(i);

        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, i + 1, request,
                                                                   [&](const litepb::Result<SimpleMessage>& result) {
                                                                       response_received   = true;
                                                                       received_error_code = result.error.code;
                                                                       received_app_code   = result.error.app_code;
                                                                   });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }

        TEST_ASSERT_TRUE(response_received);
        TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(received_error_code));
        TEST_ASSERT_EQUAL_INT32(expected_app_code, received_app_code);
    }
}

void test_wire_format_structure()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 999;
            result.value.value    = req.value * 2;
            return result;
        });

    peer_b_transport.enable_capture();

    bool response_received = false;

    SimpleMessage request;
    request.value = 100;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        (void) result;
        response_received = true;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);

    const std::vector<uint8_t>& captured_bytes = peer_b_transport.get_captured_bytes();
    TEST_ASSERT_TRUE(captured_bytes.size() > 0);

    litepb::BufferInputStream frame_stream(captured_bytes.data(), captured_bytes.size());
    litepb::FramedMessage frame;
    TEST_ASSERT_TRUE(litepb::decode_message(frame_stream, frame, true));

    TEST_ASSERT_TRUE(frame.payload.size() > 0);

    litepb::BufferInputStream payload_stream(frame.payload.data(), frame.payload.size());
    litepb::ProtoReader reader(payload_stream);

    uint64_t error_code_u64;
    TEST_ASSERT_TRUE(reader.read_varint(error_code_u64));
    TEST_ASSERT_EQUAL_UINT32(litepb::RpcError::CUSTOM_ERROR, static_cast<uint32_t>(error_code_u64));

    uint64_t app_code_u64;
    TEST_ASSERT_TRUE(reader.read_varint(app_code_u64));
    TEST_ASSERT_EQUAL_INT32(999, static_cast<int32_t>(static_cast<uint32_t>(app_code_u64)));

    SimpleMessage response;
    TEST_ASSERT_TRUE(litepb::parse(response, payload_stream));
    TEST_ASSERT_EQUAL_INT32(200, response.value);
}

void test_msg_id_varint_one_byte()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::OK;
            result.value.value = req.value;
            return result;
        });

    SimpleMessage request;
    request.value = 42;

    for (int i = 1; i < 127; ++i) {
        bool response_received = false;
        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
            (void) result;
            response_received = true;
        });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }

    peer_a_transport.enable_capture();

    bool response_received = false;
    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        (void) result;
        response_received = true;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);

    const std::vector<uint8_t>& captured_bytes = peer_a_transport.get_captured_bytes();
    TEST_ASSERT_TRUE(captured_bytes.size() > 0);

    litepb::BufferInputStream frame_stream(captured_bytes.data(), captured_bytes.size());
    litepb::FramedMessage frame;
    TEST_ASSERT_TRUE(litepb::decode_message(frame_stream, frame, true));

    TEST_ASSERT_EQUAL_UINT16(127, frame.msg_id);

    size_t msg_id_offset = 16;
    uint8_t first_byte   = captured_bytes[msg_id_offset];
    TEST_ASSERT_EQUAL_UINT8(0x7F, first_byte);
    TEST_ASSERT_TRUE((first_byte & 0x80) == 0);
}

void test_msg_id_varint_two_bytes()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::OK;
            result.value.value = req.value;
            return result;
        });

    SimpleMessage request;
    request.value = 42;

    for (int i = 1; i < 128; ++i) {
        bool response_received = false;
        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
            (void) result;
            response_received = true;
        });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }

    peer_a_transport.enable_capture();

    bool response_received = false;
    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        (void) result;
        response_received = true;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);

    const std::vector<uint8_t>& captured_bytes = peer_a_transport.get_captured_bytes();
    TEST_ASSERT_TRUE(captured_bytes.size() > 0);

    litepb::BufferInputStream frame_stream(captured_bytes.data(), captured_bytes.size());
    litepb::FramedMessage frame;
    TEST_ASSERT_TRUE(litepb::decode_message(frame_stream, frame, true));

    TEST_ASSERT_EQUAL_UINT16(128, frame.msg_id);

    size_t msg_id_offset = 16;
    uint8_t first_byte   = captured_bytes[msg_id_offset];
    uint8_t second_byte  = captured_bytes[msg_id_offset + 1];
    TEST_ASSERT_EQUAL_UINT8(0x80, first_byte);
    TEST_ASSERT_EQUAL_UINT8(0x01, second_byte);
    TEST_ASSERT_TRUE((first_byte & 0x80) != 0);
    TEST_ASSERT_TRUE((second_byte & 0x80) == 0);
}

void test_msg_id_varint_max_uint16()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::OK;
            result.value.value = req.value;
            return result;
        });

    SimpleMessage request;
    request.value = 42;

    for (int i = 1; i < 65535; ++i) {
        bool response_received = false;
        peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
            (void) result;
            response_received = true;
        });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }

    peer_a_transport.enable_capture();

    bool response_received = false;
    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        (void) result;
        response_received = true;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);

    const std::vector<uint8_t>& captured_bytes = peer_a_transport.get_captured_bytes();
    TEST_ASSERT_TRUE(captured_bytes.size() > 0);

    litepb::BufferInputStream frame_stream(captured_bytes.data(), captured_bytes.size());
    litepb::FramedMessage frame;
    TEST_ASSERT_TRUE(litepb::decode_message(frame_stream, frame, true));

    TEST_ASSERT_EQUAL_UINT16(65535, frame.msg_id);

    size_t msg_id_offset = 16;
    uint8_t first_byte   = captured_bytes[msg_id_offset];
    uint8_t second_byte  = captured_bytes[msg_id_offset + 1];
    uint8_t third_byte   = captured_bytes[msg_id_offset + 2];
    TEST_ASSERT_EQUAL_UINT8(0xFF, first_byte);
    TEST_ASSERT_EQUAL_UINT8(0xFF, second_byte);
    TEST_ASSERT_EQUAL_UINT8(0x03, third_byte);
    TEST_ASSERT_TRUE((first_byte & 0x80) != 0);
    TEST_ASSERT_TRUE((second_byte & 0x80) != 0);
    TEST_ASSERT_TRUE((third_byte & 0x80) == 0);
}

void test_timeout_overrides_server_error()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 500;
            result.value.value    = req.value;
            return result;
        });

    bool timeout_received                      = false;
    litepb::RpcError::Code received_error_code = litepb::RpcError::OK;

    SimpleMessage request;
    request.value = 42;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(
        1, 1, request,
        [&](const litepb::Result<SimpleMessage>& result) {
            timeout_received    = true;
            received_error_code = result.error.code;
        },
        50, 0);

    uint32_t start_time = litepb::get_current_time_ms();
    while (!timeout_received && (litepb::get_current_time_ms() - start_time) < 200) {
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(timeout_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::TIMEOUT, static_cast<int32_t>(received_error_code));
}

void test_multi_service_error_isolation()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        1, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::CUSTOM_ERROR;
            result.error.app_code = 100;
            result.value.value    = req.value;
            return result;
        });

    peer_b_channel.on_internal<SimpleMessage, SimpleMessage>(
        2, 1, [](uint64_t src_addr, const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            (void) src_addr;
            litepb::Result<SimpleMessage> result;
            result.error.code     = litepb::RpcError::OK;
            result.error.app_code = 0;
            result.value.value    = req.value * 2;
            return result;
        });

    bool service1_response_received            = false;
    litepb::RpcError::Code service1_error_code = litepb::RpcError::OK;
    int32_t service1_app_code                  = 0;

    SimpleMessage request1;
    request1.value = 10;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(1, 1, request1, [&](const litepb::Result<SimpleMessage>& result) {
        service1_response_received = true;
        service1_error_code        = result.error.code;
        service1_app_code          = result.error.app_code;
    });

    for (int i = 0; i < 20 && !service1_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(service1_response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::CUSTOM_ERROR, static_cast<int32_t>(service1_error_code));
    TEST_ASSERT_EQUAL_INT32(100, service1_app_code);

    bool service2_response_received            = false;
    litepb::RpcError::Code service2_error_code = litepb::RpcError::OK;
    int32_t service2_app_code                  = 0;
    int32_t service2_value                     = 0;

    SimpleMessage request2;
    request2.value = 20;

    peer_a_channel.call_internal<SimpleMessage, SimpleMessage>(2, 1, request2, [&](const litepb::Result<SimpleMessage>& result) {
        service2_response_received = true;
        service2_error_code        = result.error.code;
        service2_app_code          = result.error.app_code;
        service2_value             = result.value.value;
    });

    for (int i = 0; i < 20 && !service2_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(service2_response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::OK, static_cast<int32_t>(service2_error_code));
    TEST_ASSERT_EQUAL_INT32(0, service2_app_code);
    TEST_ASSERT_EQUAL_INT32(40, service2_value);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_app_code_propagation_basic);
    RUN_TEST(test_app_code_propagation_various_codes);
    RUN_TEST(test_app_code_zero_vs_nonzero);
    RUN_TEST(test_app_code_negative_values);
    RUN_TEST(test_app_code_boundary_values);
    RUN_TEST(test_wire_format_structure);
    RUN_TEST(test_msg_id_varint_one_byte);
    RUN_TEST(test_msg_id_varint_two_bytes);
    RUN_TEST(test_msg_id_varint_max_uint16);
    RUN_TEST(test_timeout_overrides_server_error);
    RUN_TEST(test_multi_service_error_isolation);
    return UNITY_END();
}
