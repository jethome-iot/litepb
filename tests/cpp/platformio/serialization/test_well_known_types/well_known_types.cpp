#include <unity.h>
#include <chrono>
#include <cstring>
#include "litepb/litepb.h"
#include "litepb/well_known_types.h"
#include "litepb/well_known_types_serializers.h"

void test_empty_serialization() {
    // Test Empty type serialization
    google::protobuf::Empty empty;
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(empty, output));
    
    // Empty should serialize to 0 bytes (no fields)
    TEST_ASSERT_EQUAL(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Empty decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
}

void test_timestamp_serialization() {
    // Test Timestamp type serialization
    google::protobuf::Timestamp timestamp;
    timestamp.seconds = 1234567890;
    timestamp.nanos = 123456789;
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(timestamp, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Timestamp decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL(timestamp.seconds, decoded.seconds);
    TEST_ASSERT_EQUAL(timestamp.nanos, decoded.nanos);
}

void test_timestamp_utilities() {
    // Test utility methods
    auto now = google::protobuf::Timestamp::now();
    TEST_ASSERT_GREATER_THAN(0, now.seconds);
    TEST_ASSERT_GREATER_OR_EQUAL(0, now.nanos);
    TEST_ASSERT_LESS_THAN(1000000000, now.nanos);
    
    // Test Unix timestamp conversion
    int64_t unix_time = 1234567890;
    auto ts = google::protobuf::Timestamp::from_unix_seconds(unix_time);
    TEST_ASSERT_EQUAL(unix_time, ts.seconds);
    TEST_ASSERT_EQUAL(0, ts.nanos);
    TEST_ASSERT_EQUAL(unix_time, ts.to_unix_seconds());
    
    // Test time_point conversion
    auto tp = ts.to_time_point();
    auto ts2 = google::protobuf::Timestamp::from_time_point(tp);
    TEST_ASSERT_EQUAL(ts.seconds, ts2.seconds);
    // Nanosecond precision might be lost in conversion, so check within reasonable range
    TEST_ASSERT_INT_WITHIN(1000, ts.nanos, ts2.nanos);
}

void test_duration_serialization() {
    // Test Duration type serialization
    google::protobuf::Duration duration;
    duration.seconds = 3600;  // 1 hour
    duration.nanos = 500000000;  // 0.5 seconds
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(duration, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Duration decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL(duration.seconds, decoded.seconds);
    TEST_ASSERT_EQUAL(duration.nanos, decoded.nanos);
}

void test_duration_utilities() {
    // Test milliseconds conversion
    int64_t millis = 1500;  // 1.5 seconds
    auto dur = google::protobuf::Duration::from_millis(millis);
    TEST_ASSERT_EQUAL(1, dur.seconds);
    TEST_ASSERT_EQUAL(500000000, dur.nanos);
    TEST_ASSERT_EQUAL(millis, dur.to_millis());
    
    // Test chrono duration conversion
    using namespace std::chrono;
    auto chrono_dur = seconds(10) + milliseconds(250);
    auto pb_dur = google::protobuf::Duration::from_chrono_duration(chrono_dur);
    TEST_ASSERT_EQUAL(10, pb_dur.seconds);
    TEST_ASSERT_EQUAL(250000000, pb_dur.nanos);
    
    auto chrono_dur2 = pb_dur.to_chrono_duration<int64_t, std::nano>();
    TEST_ASSERT_EQUAL(duration_cast<seconds>(chrono_dur).count(), 
                      duration_cast<seconds>(chrono_dur2).count());
}

void test_string_value_serialization() {
    // Test StringValue wrapper
    google::protobuf::StringValue str_val("Hello, World!");
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(str_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::StringValue decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL_STRING("Hello, World!", decoded.value.c_str());
    
    // Test conversion operator
    std::string s = decoded;
    TEST_ASSERT_EQUAL_STRING("Hello, World!", s.c_str());
}

void test_int32_value_serialization() {
    // Test Int32Value wrapper
    google::protobuf::Int32Value int_val(42);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(int_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Int32Value decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL(42, decoded.value);
    
    // Test conversion operator
    int32_t i = decoded;
    TEST_ASSERT_EQUAL(42, i);
}

void test_int64_value_serialization() {
    // Test Int64Value wrapper
    google::protobuf::Int64Value int_val(9223372036854775807LL);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(int_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Int64Value decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL(9223372036854775807LL, decoded.value);
}

void test_float_value_serialization() {
    // Test FloatValue wrapper
    google::protobuf::FloatValue float_val(3.14159f);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(float_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::FloatValue decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.14159f, decoded.value);
}

void test_double_value_serialization() {
    // Test DoubleValue wrapper
    google::protobuf::DoubleValue double_val(3.141592653589793);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(double_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::DoubleValue decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_DOUBLE_WITHIN(0.000000001, 3.141592653589793, decoded.value);
}

void test_bool_value_serialization() {
    // Test BoolValue wrapper
    google::protobuf::BoolValue bool_val(true);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(bool_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::BoolValue decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_TRUE(decoded.value);
    
    // Test false value
    google::protobuf::BoolValue bool_val2(false);
    output.clear();
    TEST_ASSERT_TRUE(litepb::serialize(bool_val2, output));
    
    // False value should be omitted in proto3 (default value)
    TEST_ASSERT_EQUAL(0, output.size());
}

void test_bytes_value_serialization() {
    // Test BytesValue wrapper
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    google::protobuf::BytesValue bytes_val(data);
    
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(bytes_val, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::BytesValue decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_EQUAL(data.size(), decoded.value.size());
    TEST_ASSERT_EQUAL_MEMORY(data.data(), decoded.value.data(), data.size());
}

void test_any_serialization() {
    // Test Any type with embedded Timestamp
    google::protobuf::Timestamp ts;
    ts.seconds = 1234567890;
    ts.nanos = 123456789;
    
    // Serialize the timestamp first
    litepb::BufferOutputStream ts_output;
    TEST_ASSERT_TRUE(litepb::serialize(ts, ts_output));
    
    // Create Any with the timestamp
    google::protobuf::Any any;
    any.set_type("google.protobuf.Timestamp");
    any.value.assign(ts_output.data(), ts_output.data() + ts_output.size());
    
    // Serialize the Any
    litepb::BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(any, output));
    TEST_ASSERT_GREATER_THAN(0, output.size());
    
    // Test deserialization
    litepb::BufferInputStream input(output.data(), output.size());
    google::protobuf::Any decoded;
    TEST_ASSERT_TRUE(litepb::parse(decoded, input));
    
    TEST_ASSERT_TRUE(decoded.is("google.protobuf.Timestamp"));
    TEST_ASSERT_EQUAL_STRING("type.googleapis.com/google.protobuf.Timestamp", 
                            decoded.type_url.c_str());
    
    // Deserialize the embedded timestamp
    litepb::BufferInputStream ts_input(decoded.value.data(), decoded.value.size());
    google::protobuf::Timestamp decoded_ts;
    TEST_ASSERT_TRUE(litepb::parse(decoded_ts, ts_input));
    
    TEST_ASSERT_EQUAL(ts.seconds, decoded_ts.seconds);
    TEST_ASSERT_EQUAL(ts.nanos, decoded_ts.nanos);
}

void test_wrapper_default_values() {
    // Test that default values are properly handled
    google::protobuf::StringValue empty_str;
    google::protobuf::Int32Value zero_int;
    google::protobuf::BoolValue false_bool;
    
    // These should all serialize to empty (proto3 default value optimization)
    litepb::BufferOutputStream output;
    
    TEST_ASSERT_TRUE(litepb::serialize(empty_str, output));
    TEST_ASSERT_EQUAL(0, output.size());
    
    output.clear();
    TEST_ASSERT_TRUE(litepb::serialize(zero_int, output));
    TEST_ASSERT_EQUAL(0, output.size());
    
    output.clear();
    TEST_ASSERT_TRUE(litepb::serialize(false_bool, output));
    TEST_ASSERT_EQUAL(0, output.size());
}

int runTests() {
    UNITY_BEGIN();
    
    RUN_TEST(test_empty_serialization);
    RUN_TEST(test_timestamp_serialization);
    RUN_TEST(test_timestamp_utilities);
    RUN_TEST(test_duration_serialization);
    RUN_TEST(test_duration_utilities);
    RUN_TEST(test_string_value_serialization);
    RUN_TEST(test_int32_value_serialization);
    RUN_TEST(test_int64_value_serialization);
    RUN_TEST(test_float_value_serialization);
    RUN_TEST(test_double_value_serialization);
    RUN_TEST(test_bool_value_serialization);
    RUN_TEST(test_bytes_value_serialization);
    RUN_TEST(test_any_serialization);
    RUN_TEST(test_wrapper_default_values);
    
    return UNITY_END();
}