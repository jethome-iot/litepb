#include "sensor.pb.h"
#include "sensor_simulator.h"
#include "unity.h"

using namespace examples::sensor;

void setUp() {}
void tearDown() {}

void test_normal_readings_deterministic()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_EQUAL_INT32(1, response.sensor_id);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(response.status));
}

void test_temperature_spike_simulation()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    sensor.simulate_temperature_spike();
    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(response.status));
}

void test_status_threshold_ok()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(30.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(response.status));
}

void test_status_threshold_warning_lower_bound()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(35.5f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.5f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(response.status));
}

void test_status_threshold_warning_upper_bound()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(55.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(response.status));
}

void test_status_threshold_error()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(60.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(response.status));
}

void test_temperature_reset()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    sensor.simulate_temperature_spike();
    ReadingResponse response1 = sensor.read_sensor(1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.0f, response1.temperature);

    sensor.reset_temperature();
    ReadingResponse response2 = sensor.read_sensor(1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, response2.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(response2.status));
}

void test_multiple_sensor_ids()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(0.0f);

    ReadingResponse response1 = sensor.read_sensor(100);
    ReadingResponse response2 = sensor.read_sensor(200);
    ReadingResponse response3 = sensor.read_sensor(300);

    TEST_ASSERT_EQUAL_INT32(100, response1.sensor_id);
    TEST_ASSERT_EQUAL_INT32(200, response2.sensor_id);
    TEST_ASSERT_EQUAL_INT32(300, response3.sensor_id);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, response1.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, response2.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, response3.temperature);
}

void test_status_exact_boundary_60()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(35.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(response.status));
}

void test_status_exact_boundary_80()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(55.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::WARNING, static_cast<int32_t>(response.status));
}

void test_negative_variation()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(-5.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::OK, static_cast<int32_t>(response.status));
}

void test_large_positive_variation()
{
    SensorSimulator sensor;
    sensor.set_deterministic_variation(70.0f);

    ReadingResponse response = sensor.read_sensor(1);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 95.0f, response.temperature);
    TEST_ASSERT_EQUAL_INT32(SensorStatus::ERROR, static_cast<int32_t>(response.status));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_normal_readings_deterministic);
    RUN_TEST(test_temperature_spike_simulation);
    RUN_TEST(test_status_threshold_ok);
    RUN_TEST(test_status_threshold_warning_lower_bound);
    RUN_TEST(test_status_threshold_warning_upper_bound);
    RUN_TEST(test_status_threshold_error);
    RUN_TEST(test_temperature_reset);
    RUN_TEST(test_multiple_sensor_ids);
    RUN_TEST(test_status_exact_boundary_60);
    RUN_TEST(test_status_exact_boundary_80);
    RUN_TEST(test_negative_variation);
    RUN_TEST(test_large_positive_variation);
    return UNITY_END();
}
