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

void test_rpc_error_propagation_basic()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 1000);

    bool response_received                     = false;
    litepb::RpcError::Code received_error_code = litepb::RpcError::OK;

    peer_b_channel.on<SimpleMessage, SimpleMessage>(
        1, 1, [](const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::TRANSPORT_ERROR;
            result.value.value = req.value;
            return result;
        });

    SimpleMessage request;
    request.value = 42;

    peer_a_channel.call<SimpleMessage, SimpleMessage>(1, 1, request, [&](const litepb::Result<SimpleMessage>& result) {
        response_received   = true;
        received_error_code = result.error.code;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::TRANSPORT_ERROR, static_cast<int32_t>(received_error_code));
}

void test_rpc_error_propagation_various_codes()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 1000);

    litepb::RpcError::Code test_error_codes[] = { litepb::RpcError::TIMEOUT, litepb::RpcError::PARSE_ERROR,
                                                  litepb::RpcError::TRANSPORT_ERROR, litepb::RpcError::HANDLER_NOT_FOUND };

    for (size_t i = 0; i < sizeof(test_error_codes) / sizeof(test_error_codes[0]); ++i) {
        litepb::RpcError::Code expected_error_code = test_error_codes[i];

        bool response_received                     = false;
        litepb::RpcError::Code received_error_code = litepb::RpcError::OK;

        peer_b_channel.on<SimpleMessage, SimpleMessage>(
            1, i + 1, [expected_error_code](const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
                litepb::Result<SimpleMessage> result;
                result.error.code  = expected_error_code;
                result.value.value = req.value;
                return result;
            });

        SimpleMessage request;
        request.value = static_cast<int32_t>(i);

        peer_a_channel.call<SimpleMessage, SimpleMessage>(1, i + 1, request,
                                                                   [&](const litepb::Result<SimpleMessage>& result) {
                                                                       response_received   = true;
                                                                       received_error_code = result.error.code;
                                                                   });

        for (int j = 0; j < 10 && !response_received; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }

        TEST_ASSERT_TRUE(response_received);
        TEST_ASSERT_EQUAL_INT32(expected_error_code, static_cast<int32_t>(received_error_code));
    }
}

// test_app_code_zero_vs_nonzero removed - app_code no longer part of RPC protocol

// test_app_code_negative_values removed - app_code no longer part of RPC protocol

// test_app_code_boundary_values removed - app_code no longer part of RPC protocol

// test_wire_format_structure removed - FramedMessage no longer part of protocol

// test_msg_id_varint_one_byte removed - FramedMessage no longer part of protocol

// test_msg_id_varint_two_bytes removed - FramedMessage no longer part of protocol

// test_msg_id_varint_max_uint16 removed - FramedMessage no longer part of protocol

void test_timeout_overrides_server_error()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 1000);

    peer_b_channel.on<SimpleMessage, SimpleMessage>(
        1, 1, [](const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::TRANSPORT_ERROR;
            result.value.value = req.value;
            return result;
        });

    bool timeout_received                      = false;
    litepb::RpcError::Code received_error_code = litepb::RpcError::OK;

    SimpleMessage request;
    request.value = 42;

    peer_a_channel.call<SimpleMessage, SimpleMessage>(
        1, 1, request,
        [&](const litepb::Result<SimpleMessage>& result) {
            timeout_received    = true;
            received_error_code = result.error.code;
        },
        50);

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

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 1000);

    peer_b_channel.on<SimpleMessage, SimpleMessage>(
        1, 1, [](const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::TRANSPORT_ERROR;
            result.value.value = req.value;
            return result;
        });

    peer_b_channel.on<SimpleMessage, SimpleMessage>(
        2, 1, [](const SimpleMessage& req) -> litepb::Result<SimpleMessage> {
            litepb::Result<SimpleMessage> result;
            result.error.code  = litepb::RpcError::OK;
            result.value.value = req.value * 2;
            return result;
        });

    bool service1_response_received            = false;
    litepb::RpcError::Code service1_error_code = litepb::RpcError::OK;

    SimpleMessage request1;
    request1.value = 10;

    peer_a_channel.call<SimpleMessage, SimpleMessage>(1, 1, request1, [&](const litepb::Result<SimpleMessage>& result) {
        service1_response_received = true;
        service1_error_code        = result.error.code;
    });

    for (int i = 0; i < 20 && !service1_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(service1_response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::TRANSPORT_ERROR, static_cast<int32_t>(service1_error_code));

    bool service2_response_received            = false;
    litepb::RpcError::Code service2_error_code = litepb::RpcError::OK;
    int32_t service2_value                     = 0;

    SimpleMessage request2;
    request2.value = 20;

    peer_a_channel.call<SimpleMessage, SimpleMessage>(2, 1, request2, [&](const litepb::Result<SimpleMessage>& result) {
        service2_response_received = true;
        service2_error_code        = result.error.code;
        service2_value             = result.value.value;
    });

    for (int i = 0; i < 20 && !service2_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(service2_response_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::OK, static_cast<int32_t>(service2_error_code));
    TEST_ASSERT_EQUAL_INT32(40, service2_value);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_rpc_error_propagation_basic);
    RUN_TEST(test_rpc_error_propagation_various_codes);
    RUN_TEST(test_timeout_overrides_server_error);
    RUN_TEST(test_multi_service_error_isolation);
    return UNITY_END();
}
