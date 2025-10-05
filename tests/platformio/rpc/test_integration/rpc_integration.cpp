#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/transport.h"
#include "rpc_test.pb.h"
#include "unity.h"
#include <cstring>
#include <memory>
#include <queue>
#include <vector>

using namespace test::rpc;

void setUp() {}
void tearDown() {}

class LoopbackTransport : public litepb::StreamTransport
{
public:
    LoopbackTransport() : peer_(nullptr) {}

    void connect_to_peer(LoopbackTransport* peer) { peer_ = peer; }

    bool send(const uint8_t* data, size_t len, uint64_t src_addr, uint64_t dst_addr, uint16_t msg_id) override
    {
        if (!peer_) {
            return false;
        }

        // Store metadata for recv
        peer_->last_src_addr_ = src_addr;
        peer_->last_dst_addr_ = dst_addr;
        peer_->last_msg_id_ = msg_id;

        for (size_t i = 0; i < len; ++i) {
            peer_->rx_queue_.push(data[i]);
        }
        return true;
    }

    size_t recv(uint8_t* buffer, size_t max_len, uint64_t& src_addr, uint64_t& dst_addr, uint16_t& msg_id) override
    {
        // Return stored metadata
        src_addr = last_src_addr_;
        dst_addr = last_dst_addr_;
        msg_id = last_msg_id_;

        size_t count = 0;
        while (!rx_queue_.empty() && count < max_len) {
            buffer[count++] = rx_queue_.front();
            rx_queue_.pop();
        }
        return count;
    }

    bool available() override { return !rx_queue_.empty(); }

private:
    LoopbackTransport* peer_;
    std::queue<uint8_t> rx_queue_;
    uint64_t last_src_addr_ = 0;
    uint64_t last_dst_addr_ = 0;
    uint16_t last_msg_id_ = 0;
};

void test_client_server_roundtrip()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool response_received = false;
    int32_t response_value = 0;
    std::string response_message;

    peer_b_channel.on_internal<EchoRequest, EchoResponse>(
        1, 1, [](uint64_t src_addr, const EchoRequest& req) -> litepb::Result<EchoResponse> {
            (void) src_addr;
            litepb::Result<EchoResponse> result;
            result.value.value   = req.value;
            result.value.message = req.message;
            result.error.code    = litepb::RpcError::OK;
            return result;
        });

    EchoRequest request;
    request.value   = 42;
    request.message = "Hello, Server!";

    peer_a_channel.call_internal<EchoRequest, EchoResponse>(1, 1, request, [&](const litepb::Result<EchoResponse>& result) {
        TEST_ASSERT_TRUE(result.ok());
        response_received = true;
        response_value    = result.value.value;
        response_message  = result.value.message;
    });

    for (int i = 0; i < 10 && !response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(response_received);
    TEST_ASSERT_EQUAL_INT32(42, response_value);
    TEST_ASSERT_EQUAL_STRING("Hello, Server!", response_message.c_str());
}

void test_bidirectional_rpc()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool client_response_received = false;
    bool server_response_received = false;
    int32_t client_response_value = 0;
    int32_t server_response_value = 0;

    peer_a_channel.on_internal<EchoRequest, EchoResponse>(
        1, 1, [](uint64_t src_addr, const EchoRequest& req) -> litepb::Result<EchoResponse> {
            (void) src_addr;
            litepb::Result<EchoResponse> result;
            result.value.value   = req.value * 2;
            result.value.message = "Client echo: " + req.message;
            result.error.code    = litepb::RpcError::OK;
            return result;
        });

    peer_b_channel.on_internal<EchoRequest, EchoResponse>(
        1, 1, [](uint64_t src_addr, const EchoRequest& req) -> litepb::Result<EchoResponse> {
            (void) src_addr;
            litepb::Result<EchoResponse> result;
            result.value.value   = req.value * 3;
            result.value.message = "Server echo: " + req.message;
            result.error.code    = litepb::RpcError::OK;
            return result;
        });

    EchoRequest client_request;
    client_request.value   = 10;
    client_request.message = "From client";

    peer_a_channel.call_internal<EchoRequest, EchoResponse>(1, 1, client_request, [&](const litepb::Result<EchoResponse>& result) {
        TEST_ASSERT_TRUE(result.ok());
        client_response_received = true;
        client_response_value    = result.value.value;
    });

    for (int i = 0; i < 20 && !client_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(client_response_received);
    TEST_ASSERT_EQUAL_INT32(30, client_response_value);

    EchoRequest server_request;
    server_request.value   = 20;
    server_request.message = "From server";

    peer_b_channel.call_internal<EchoRequest, EchoResponse>(1, 1, server_request, [&](const litepb::Result<EchoResponse>& result) {
        TEST_ASSERT_TRUE(result.ok());
        server_response_received = true;
        server_response_value    = result.value.value;
    });

    for (int i = 0; i < 20 && !server_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(server_response_received);
    TEST_ASSERT_EQUAL_INT32(40, server_response_value);
}

void test_timeout_scenario()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 100);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 100);

    bool timeout_received             = false;
    litepb::RpcError::Code error_code = litepb::RpcError::OK;

    EchoRequest request;
    request.value   = 100;
    request.message = "Will timeout";

    peer_a_channel.call_internal<EchoRequest, EchoResponse>(
        1, 1, request,
        [&](const litepb::Result<EchoResponse>& result) {
            timeout_received = true;
            error_code       = result.error.code;
        },
        50, 0);

    uint32_t start_time = litepb::get_current_time_ms();
    while (!timeout_received && (litepb::get_current_time_ms() - start_time) < 200) {
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(timeout_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::TIMEOUT, static_cast<int32_t>(error_code));
}

void test_error_propagation()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    litepb::RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    litepb::RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool error_received               = false;
    litepb::RpcError::Code error_code = litepb::RpcError::OK;
    std::string error_message;

    peer_b_channel.on_internal<EchoRequest, EchoResponse>(
        1, 1, [](uint64_t src_addr, const EchoRequest& req) -> litepb::Result<EchoResponse> {
            (void) src_addr;
            litepb::Result<EchoResponse> result;

            if (req.value < 0) {
                result.error.code = litepb::RpcError::TRANSPORT_ERROR;
            }
            else {
                result.value.value   = req.value;
                result.value.message = req.message;
                result.error.code    = litepb::RpcError::OK;
            }

            return result;
        });

    EchoRequest request;
    request.value   = -5;
    request.message = "Invalid request";

    peer_a_channel.call_internal<EchoRequest, EchoResponse>(1, 1, request, [&](const litepb::Result<EchoResponse>& result) {
        error_received = true;
        error_code     = result.error.code;
    });

    for (int i = 0; i < 10 && !error_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(error_received);
    TEST_ASSERT_EQUAL_INT32(litepb::RpcError::TRANSPORT_ERROR, static_cast<int32_t>(error_code));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_client_server_roundtrip);
    RUN_TEST(test_bidirectional_rpc);
    RUN_TEST(test_timeout_scenario);
    RUN_TEST(test_error_propagation);

    return UNITY_END();
}
