#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/transport.h"
#include "loopback_transport.h"
#include "sensor.pb.h"
#include "unity.h"
#include <cstring>

using namespace examples::sensor;
using namespace litepb;

void setUp() {}
void tearDown() {}

void test_get_reading_method()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool handler_called  = false;
    bool callback_called = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&handler_called](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            (void) src_addr;
            TEST_ASSERT_EQUAL_INT32(42, req.sensor_id);
            handler_called = true;

            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 25.5f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    ReadingRequest request;
    request.sensor_id = 42;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&callback_called](const Result<ReadingResponse>& result) {
            TEST_ASSERT_TRUE(result.ok());
            TEST_ASSERT_EQUAL_INT32(42, result.value.sensor_id);
            TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.5f, result.value.temperature);
            TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(result.value.status));
            callback_called = true;
        });

    for (int i = 0; i < 20 && !callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(handler_called);
    TEST_ASSERT_TRUE(callback_called);
}

void test_notify_alert_method()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool handler_called  = false;
    bool callback_called = false;

    peer_a_channel.on_internal<AlertEvent, AlertAck>(
        1, 2, [&handler_called](uint64_t src_addr, const AlertEvent& req) -> Result<AlertAck> {
            (void) src_addr;
            TEST_ASSERT_EQUAL_INT32(99, req.sensor_id);
            TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.5f, req.temperature);
            TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(req.status));
            TEST_ASSERT_EQUAL_STRING("Temperature too high", req.message.c_str());
            handler_called = true;

            Result<AlertAck> result;
            result.value.received = true;
            result.error.code     = RpcError::OK;
            return result;
        });

    AlertEvent alert;
    alert.sensor_id   = 99;
    alert.temperature = 85.5f;
    alert.status      = SensorStatus::WARNING;
    alert.message     = "Temperature too high";

    peer_b_channel.call_internal<AlertEvent, AlertAck>(1, 2, alert, [&callback_called](const Result<AlertAck>& result) {
        TEST_ASSERT_TRUE(result.ok());
        TEST_ASSERT_TRUE(result.value.received);
        callback_called = true;
    });

    for (int i = 0; i < 20 && !callback_called; ++i) {
        peer_a_channel.process();
        peer_b_channel.process();
    }

    TEST_ASSERT_TRUE(handler_called);
    TEST_ASSERT_TRUE(callback_called);
}

void test_bidirectional_communication()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool get_reading_done  = false;
    bool notify_alert_done = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 30.0f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    peer_a_channel.on_internal<AlertEvent, AlertAck>(1, 2, [](uint64_t src_addr, const AlertEvent& req) -> Result<AlertAck> {
        (void) src_addr;
        Result<AlertAck> result;
        result.value.received = true;
        result.error.code     = RpcError::OK;
        return result;
    });

    ReadingRequest request;
    request.sensor_id = 10;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, request,
                                                                  [&get_reading_done](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      get_reading_done = true;
                                                                  });

    for (int i = 0; i < 30 && !get_reading_done; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(get_reading_done);

    AlertEvent alert;
    alert.sensor_id   = 10;
    alert.temperature = 75.0f;
    alert.status      = SensorStatus::WARNING;
    alert.message     = "Test alert";

    peer_b_channel.call_internal<AlertEvent, AlertAck>(1, 2, alert, [&notify_alert_done](const Result<AlertAck>& result) {
        TEST_ASSERT_TRUE(result.ok());
        notify_alert_done = true;
    });

    for (int i = 0; i < 30 && !notify_alert_done; ++i) {
        peer_a_channel.process();
        peer_b_channel.process();
    }

    TEST_ASSERT_TRUE(notify_alert_done);
}

void test_async_callback_execution()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool handler_called  = false;
    bool callback_called = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&handler_called](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            handler_called = true;

            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 20.0f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    ReadingRequest request;
    request.sensor_id = 1;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, request,
                                                                  [&callback_called](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      callback_called = true;
                                                                  });

    for (int i = 0; i < 50 && !callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(handler_called);
    TEST_ASSERT_TRUE(callback_called);
}

void test_invalid_sensor_ids()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool callback_called         = false;
    SensorStatus received_status = SensorStatus::OK;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;

            if (req.sensor_id < 0) {
                // Return application-specific error through the response data
                result.value.sensor_id   = req.sensor_id;
                result.value.temperature = 0.0f;
                result.value.status      = SensorStatus::ERROR; // Use application error status
                result.error.code        = RpcError::OK;        // RPC itself succeeded
            }
            else {
                result.value.sensor_id   = req.sensor_id;
                result.value.temperature = 25.0f;
                result.value.status      = SensorStatus::OK;
                result.error.code        = RpcError::OK;
            }

            return result;
        });

    ReadingRequest request;
    request.sensor_id = -1;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&callback_called, &received_status](const Result<ReadingResponse>& result) {
            callback_called = true;
            TEST_ASSERT_TRUE(result.ok()); // RPC should succeed
            received_status = result.value.status;
        });

    for (int i = 0; i < 20 && !callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(callback_called);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(received_status));
}

void test_custom_timeouts()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool timeout_received     = false;
    RpcError::Code error_code = RpcError::OK;

    ReadingRequest request;
    request.sensor_id = 100;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request,
        [&timeout_received, &error_code](const Result<ReadingResponse>& result) {
            timeout_received = true;
            error_code       = result.error.code;
        },
        50, 0);

    uint32_t start_time = get_current_time_ms();
    while (!timeout_received && (get_current_time_ms() - start_time) < 200) {
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(timeout_received);
    TEST_ASSERT_EQUAL_INT32(RpcError::TIMEOUT, static_cast<int32_t>(error_code));
}

void test_error_code_propagation()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool callback_called               = false;
    RpcError::Code received_error_code = RpcError::OK;
    std::string received_error_message;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;
            // Use TRANSPORT_ERROR as an example of RPC-level error
            result.error.code = RpcError::TRANSPORT_ERROR;
            return result;
        });

    ReadingRequest request;
    request.sensor_id = 50;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&callback_called, &received_error_code](const Result<ReadingResponse>& result) {
            callback_called     = true;
            received_error_code = result.error.code;
        });

    for (int i = 0; i < 20 && !callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(callback_called);
    TEST_ASSERT_EQUAL_INT32(RpcError::TRANSPORT_ERROR, static_cast<int32_t>(received_error_code));
}

void test_response_serialization()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool callback_called = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;
            result.value.sensor_id   = 123;
            result.value.temperature = 45.678f;
            result.value.status      = SensorStatus::WARNING;
            result.error.code        = RpcError::OK;
            return result;
        });

    ReadingRequest request;
    request.sensor_id = 123;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&callback_called](const Result<ReadingResponse>& result) {
            TEST_ASSERT_TRUE(result.ok());
            TEST_ASSERT_EQUAL_INT32(123, result.value.sensor_id);
            TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.678f, result.value.temperature);
            TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(result.value.status));
            callback_called = true;
        });

    for (int i = 0; i < 20 && !callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(callback_called);
}

void test_fire_and_forget_event_basic()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool handler_called          = false;
    int32_t received_sensor_id   = 0;
    float received_temperature   = 0.0f;
    SensorStatus received_status = SensorStatus::OK;
    std::string received_message;

    peer_b_channel.on_event<AlertNotification>(1, 3,
                                               [&handler_called, &received_sensor_id, &received_temperature, &received_status,
                                                &received_message](uint64_t src_addr, const AlertNotification& event) {
                                                   (void) src_addr;
                                                   handler_called       = true;
                                                   received_sensor_id   = event.sensor_id;
                                                   received_temperature = event.temperature;
                                                   received_status      = event.status;
                                                   received_message     = event.message;
                                               });

    AlertNotification alert;
    alert.sensor_id   = 42;
    alert.temperature = 85.5f;
    alert.status      = SensorStatus::WARNING;
    alert.message     = "High temperature alert";

    bool sent = peer_a_channel.send_event<AlertNotification>(1, 3, alert);
    TEST_ASSERT_TRUE(sent);

    for (int i = 0; i < 20 && !handler_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(handler_called);
    TEST_ASSERT_EQUAL_INT32(42, received_sensor_id);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.5f, received_temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(received_status));
    TEST_ASSERT_EQUAL_STRING("High temperature alert", received_message.c_str());
}

void test_event_no_pending_call_slot()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    int event_count = 0;

    peer_b_channel.on_event<AlertNotification>(
        1, 3, [&event_count](uint64_t src_addr, const AlertNotification& event) { event_count++; });

    const int num_events = 20;
    for (int i = 0; i < num_events; ++i) {
        AlertNotification alert;
        alert.sensor_id   = i;
        alert.temperature = 25.0f + i;
        alert.status      = SensorStatus::OK;
        alert.message     = "Event " + std::to_string(i);

        bool sent = peer_a_channel.send_event<AlertNotification>(1, 3, alert);
        TEST_ASSERT_TRUE(sent);

        for (int j = 0; j < 5; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }

    for (int i = 0; i < 20; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_EQUAL_INT32(num_events, event_count);
}

void test_event_and_rpc_interleaving()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool first_rpc_done  = false;
    bool event_received  = false;
    bool second_rpc_done = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 30.0f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    peer_b_channel.on_event<AlertNotification>(1, 3, [&event_received](uint64_t src_addr, const AlertNotification& event) {
        (void) src_addr;
        event_received = true;
    });

    ReadingRequest request1;
    request1.sensor_id = 1;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, request1,
                                                                  [&first_rpc_done](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      first_rpc_done = true;
                                                                  });

    for (int i = 0; i < 20 && !first_rpc_done; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(first_rpc_done);

    AlertNotification alert;
    alert.sensor_id   = 99;
    alert.temperature = 75.0f;
    alert.status      = SensorStatus::WARNING;
    alert.message     = "Interleaved event";

    bool sent = peer_a_channel.send_event<AlertNotification>(1, 3, alert);
    TEST_ASSERT_TRUE(sent);

    for (int i = 0; i < 20 && !event_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(event_received);

    ReadingRequest request2;
    request2.sensor_id = 2;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, request2,
                                                                  [&second_rpc_done](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      second_rpc_done = true;
                                                                  });

    for (int i = 0; i < 20 && !second_rpc_done; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(second_rpc_done);
}

void test_bidirectional_events()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool client_event_received = false;
    bool server_event_received = false;
    int32_t client_received_id = 0;
    int32_t server_received_id = 0;

    peer_b_channel.on_event<AlertNotification>(
        1, 3, [&server_event_received, &server_received_id](uint64_t src_addr, const AlertNotification& event) {
            (void) src_addr;
            server_event_received = true;
            server_received_id    = event.sensor_id;
        });

    peer_a_channel.on_event<AlertNotification>(
        1, 3, [&client_event_received, &client_received_id](uint64_t src_addr, const AlertNotification& event) {
            (void) src_addr;
            client_event_received = true;
            client_received_id    = event.sensor_id;
        });

    AlertNotification client_to_server;
    client_to_server.sensor_id   = 100;
    client_to_server.temperature = 50.0f;
    client_to_server.status      = SensorStatus::OK;
    client_to_server.message     = "Client to server event";

    bool sent1 = peer_a_channel.send_event<AlertNotification>(1, 3, client_to_server);
    TEST_ASSERT_TRUE(sent1);

    for (int i = 0; i < 20 && !server_event_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(server_event_received);
    TEST_ASSERT_EQUAL_INT32(100, server_received_id);

    AlertNotification server_to_client;
    server_to_client.sensor_id   = 200;
    server_to_client.temperature = 60.0f;
    server_to_client.status      = SensorStatus::WARNING;
    server_to_client.message     = "Server to client event";

    bool sent2 = peer_b_channel.send_event<AlertNotification>(1, 3, server_to_client);
    TEST_ASSERT_TRUE(sent2);

    for (int i = 0; i < 20 && !client_event_received; ++i) {
        peer_a_channel.process();
        peer_b_channel.process();
    }

    TEST_ASSERT_TRUE(client_event_received);
    TEST_ASSERT_EQUAL_INT32(200, client_received_id);
}

void test_event_serialization_correctness()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    bool handler_called = false;
    AlertNotification received_event;

    peer_b_channel.on_event<AlertNotification>(
        1, 3, [&handler_called, &received_event](uint64_t src_addr, const AlertNotification& event) {
            (void) src_addr;
            handler_called = true;
            received_event = event;
        });

    AlertNotification alert;
    alert.sensor_id   = 12345;
    alert.temperature = 123.456f;
    alert.status      = SensorStatus::ERROR;
    alert.message     = "Complex serialization test with special chars: !@#$%^&*()";

    bool sent = peer_a_channel.send_event<AlertNotification>(1, 3, alert);
    TEST_ASSERT_TRUE(sent);

    for (int i = 0; i < 20 && !handler_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(handler_called);
    TEST_ASSERT_EQUAL_INT32(12345, received_event.sensor_id);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.456f, received_event.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(received_event.status));
    TEST_ASSERT_EQUAL_STRING("Complex serialization test with special chars: !@#$%^&*()", received_event.message.c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_get_reading_method);
    RUN_TEST(test_notify_alert_method);
    RUN_TEST(test_bidirectional_communication);
    RUN_TEST(test_async_callback_execution);
    RUN_TEST(test_invalid_sensor_ids);
    RUN_TEST(test_custom_timeouts);
    RUN_TEST(test_error_code_propagation);
    RUN_TEST(test_response_serialization);
    RUN_TEST(test_fire_and_forget_event_basic);
    RUN_TEST(test_event_no_pending_call_slot);
    RUN_TEST(test_event_and_rpc_interleaving);
    RUN_TEST(test_bidirectional_events);
    RUN_TEST(test_event_serialization_correctness);
    return UNITY_END();
}
