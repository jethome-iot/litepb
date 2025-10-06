#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/transport.h"
#include "loopback_transport.h"
#include "sensor.pb.h"
#include "sensor_simulator.h"
#include "unity.h"
#include <cstring>
#include <vector>

using namespace examples::sensor;
using namespace litepb;

void setUp() {}
void tearDown() {}

void test_full_client_server_roundtrip()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    bool server_handler_called  = false;
    bool client_callback_called = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(1, 1,
                                                                [&](uint64_t src_addr, const auto& req) -> Result<ReadingResponse> {
                                                                    (void) src_addr;
                                                                    server_handler_called   = true;
                                                                    ReadingResponse reading = sensor.read_sensor(req.sensor_id);

                                                                    Result<ReadingResponse> result;
                                                                    result.value      = reading;
                                                                    result.error.code = RpcError::OK;
                                                                    return result;
                                                                });

    ReadingRequest request;
    request.sensor_id = 42;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&client_callback_called](const Result<ReadingResponse>& result) {
            TEST_ASSERT_TRUE(result.ok());
            TEST_ASSERT_EQUAL_INT32(42, result.value.sensor_id);
            TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, result.value.temperature);
            TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(result.value.status));
            client_callback_called = true;
        });

    for (int i = 0; i < 20 && !client_callback_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(server_handler_called);
    TEST_ASSERT_TRUE(client_callback_called);
}

void test_concurrent_pending_rpcs()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            ReadingResponse reading = sensor.read_sensor(req.sensor_id);

            Result<ReadingResponse> result;
            result.value      = reading;
            result.error.code = RpcError::OK;
            return result;
        });

    bool callback1_called = false;
    bool callback2_called = false;

    ReadingRequest req1;
    req1.sensor_id = 1;
    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, req1,
                                                                  [&callback1_called](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      TEST_ASSERT_EQUAL_INT32(1, result.value.sensor_id);
                                                                      callback1_called = true;
                                                                  });

    for (int i = 0; i < 30 && !callback1_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(callback1_called);

    ReadingRequest req2;
    req2.sensor_id = 2;
    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, req2,
                                                                  [&callback2_called](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      TEST_ASSERT_EQUAL_INT32(2, result.value.sensor_id);
                                                                      callback2_called = true;
                                                                  });

    for (int i = 0; i < 30 && !callback2_called; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(callback2_called);
}

void test_bidirectional_alert_after_reading()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    SensorSimulator sensor;
    sensor.simulate_temperature_spike();
    sensor.set_deterministic_variation(0.0f);  // Ensure consistent temperature for reliable testing

    bool reading_received = false;
    bool alert_received   = false;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&sensor](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            ReadingResponse reading = sensor.read_sensor(req.sensor_id);

            Result<ReadingResponse> result;
            result.value      = reading;
            result.error.code = RpcError::OK;
            return result;
        });

    peer_a_channel.on_internal<AlertEvent, AlertAck>(
        1, 2, [&alert_received](uint64_t src_addr, const AlertEvent& req) -> Result<AlertAck> {
            (void) src_addr;
            alert_received = true;
            TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(req.status));
            TEST_ASSERT_GREATER_THAN(80.0f, req.temperature);

            Result<AlertAck> result;
            result.value.received = true;
            result.error.code     = RpcError::OK;
            return result;
        });

    ReadingRequest request;
    request.sensor_id = 99;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
        1, 1, request, [&reading_received](const Result<ReadingResponse>& result) {
            TEST_ASSERT_TRUE(result.ok());
            TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(result.value.status));
            reading_received = true;
        });

    for (int i = 0; i < 30 && !reading_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_TRUE(reading_received);

    AlertEvent alert;
    alert.sensor_id   = 99;
    alert.temperature = 85.0f;
    alert.status      = SensorStatus::ERROR;
    alert.message     = "Temperature threshold exceeded!";

    peer_b_channel.call_internal<AlertEvent, AlertAck>(1, 2, alert,
                                                       [](const Result<AlertAck>& result) { TEST_ASSERT_TRUE(result.ok()); });

    for (int i = 0; i < 30 && !alert_received; ++i) {
        peer_a_channel.process();
        peer_b_channel.process();
    }

    TEST_ASSERT_TRUE(alert_received);
}

void test_process_pump_draining_queues()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    int server_handler_count  = 0;
    int client_callback_count = 0;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&server_handler_count](uint64_t src_addr, const auto& req) -> Result<ReadingResponse> {
            (void) src_addr;
            server_handler_count++;

            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 25.0f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    ReadingRequest req;
    req.sensor_id = 1;

    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, req,
                                                                  [&client_callback_count](const Result<ReadingResponse>& result) {
                                                                      TEST_ASSERT_TRUE(result.ok());
                                                                      client_callback_count++;
                                                                  });

    TEST_ASSERT_EQUAL_INT32(0, server_handler_count);
    TEST_ASSERT_EQUAL_INT32(0, client_callback_count);

    for (int i = 0; i < 30; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_EQUAL_INT32(1, server_handler_count);
    TEST_ASSERT_EQUAL_INT32(1, client_callback_count);
}

void test_callback_order_validation()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    std::vector<int32_t> call_order;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            Result<ReadingResponse> result;
            result.value.sensor_id   = req.sensor_id;
            result.value.temperature = 25.0f;
            result.value.status      = SensorStatus::OK;
            result.error.code        = RpcError::OK;
            return result;
        });

    ReadingRequest req1;
    req1.sensor_id = 1;
    peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(1, 1, req1, [&call_order](const Result<ReadingResponse>& result) {
        TEST_ASSERT_TRUE(result.ok());
        call_order.push_back(result.value.sensor_id);
    });

    for (int i = 0; i < 30 && call_order.size() < 1; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    TEST_ASSERT_EQUAL_UINT32(1, call_order.size());
    TEST_ASSERT_EQUAL_INT32(1, call_order[0]);
}

void test_stress_loop_repeated_readings()
{
    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 1, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 2, 1000);

    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    const int NUM_ITERATIONS = 5;
    int completed_count      = 0;

    peer_b_channel.on_internal<ReadingRequest, ReadingResponse>(
        1, 1, [&sensor](uint64_t src_addr, const ReadingRequest& req) -> Result<ReadingResponse> {
            (void) src_addr;
            ReadingResponse reading = sensor.read_sensor(req.sensor_id);

            Result<ReadingResponse> result;
            result.value      = reading;
            result.error.code = RpcError::OK;
            return result;
        });

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        ReadingRequest req;
        req.sensor_id      = i;
        bool callback_done = false;

        peer_a_channel.call_internal<ReadingRequest, ReadingResponse>(
            1, 1, req, [&completed_count, &callback_done, i](const Result<ReadingResponse>& result) {
                TEST_ASSERT_TRUE(result.ok());
                TEST_ASSERT_EQUAL_INT32(i, result.value.sensor_id);
                TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, result.value.temperature);
                completed_count++;
                callback_done = true;
            });

        for (int j = 0; j < 30 && !callback_done; ++j) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }

    TEST_ASSERT_EQUAL_INT32(NUM_ITERATIONS, completed_count);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_full_client_server_roundtrip);
    RUN_TEST(test_concurrent_pending_rpcs);
    RUN_TEST(test_bidirectional_alert_after_reading);
    RUN_TEST(test_process_pump_draining_queues);
    RUN_TEST(test_callback_order_validation);
    RUN_TEST(test_stress_loop_repeated_readings);
    return UNITY_END();
}
