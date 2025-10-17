#include "litepb/litepb.h"
#include "scalar_types.pb.h"
#include "unity.h"
#include <cmath>
#include <cstring>

void setUp() {}
void tearDown() {}

void test_all_scalar_types()
{
    using namespace test::scalar;

    // Create message with all scalar types
    ScalarTypes msg;
    msg.field_int32    = -123456;
    msg.field_int64    = -9876543210LL;
    msg.field_uint32   = 123456U;
    msg.field_uint64   = 9876543210ULL;
    msg.field_sint32   = -654321;
    msg.field_sint64   = -1234567890LL;
    msg.field_fixed32  = 0xDEADBEEF;
    msg.field_fixed64  = 0xCAFEBABEDEADBEEFULL;
    msg.field_sfixed32 = -987654;
    msg.field_sfixed64 = -123456789012LL;
    msg.field_float    = 3.14159f;
    msg.field_double   = 2.718281828459045;
    msg.field_bool     = true;
    msg.field_string   = "Hello, LitePB!";
    msg.field_bytes    = { 0x01, 0x02, 0x03, 0xFF, 0xFE };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    ScalarTypes deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all values - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(-123456, deserialized.field_int32);

    TEST_ASSERT_EQUAL_INT64(-9876543210LL, deserialized.field_int64);

    TEST_ASSERT_EQUAL_UINT32(123456U, deserialized.field_uint32);

    TEST_ASSERT_EQUAL_UINT64(9876543210ULL, deserialized.field_uint64);

    TEST_ASSERT_EQUAL_INT32(-654321, deserialized.field_sint32);

    TEST_ASSERT_EQUAL_INT64(-1234567890LL, deserialized.field_sint64);

    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, deserialized.field_fixed32);

    TEST_ASSERT_EQUAL_UINT64(0xCAFEBABEDEADBEEFULL, deserialized.field_fixed64);

    TEST_ASSERT_EQUAL_INT32(-987654, deserialized.field_sfixed32);

    TEST_ASSERT_EQUAL_INT64(-123456789012LL, deserialized.field_sfixed64);

    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.14159f, deserialized.field_float);

    TEST_ASSERT_DOUBLE_WITHIN(0.000000001, 2.718281828459045, deserialized.field_double);

    TEST_ASSERT_TRUE(deserialized.field_bool);

    TEST_ASSERT_EQUAL_STRING("Hello, LitePB!", deserialized.field_string.c_str());

    TEST_ASSERT_EQUAL_size_t(5, deserialized.field_bytes.size());
    TEST_ASSERT_EQUAL_UINT8(0x01, deserialized.field_bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, deserialized.field_bytes[3]);
}

void test_default_values()
{
    using namespace test::scalar;

    // Create empty message (should have proto3 defaults)
    DefaultValues msg;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    DefaultValues deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Proto3: implicit fields have default values (direct types)
    TEST_ASSERT_EQUAL_INT32(0, deserialized.int_field);
    TEST_ASSERT_EQUAL_STRING("", deserialized.string_field.c_str());
    TEST_ASSERT_FALSE(deserialized.bool_field);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, deserialized.double_field);
}

void test_repeated_scalar_types()
{
    using namespace test::scalar;

    // Create message with repeated scalar types
    RepeatedScalarTypes msg;
    msg.repeated_int32  = { 1, -2, 3, -4, 5 };
    msg.repeated_uint32 = { 10, 20, 30, 40, 50 };
    msg.repeated_string = { "alpha", "beta", "gamma" };
    msg.repeated_bool   = { true, false, true, true };
    msg.repeated_double = { 1.1, 2.2, 3.3 };
    msg.repeated_bytes  = { { 0x01, 0x02 }, { 0xAA, 0xBB, 0xCC } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    RepeatedScalarTypes deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify repeated int32
    TEST_ASSERT_EQUAL_size_t(5, deserialized.repeated_int32.size());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.repeated_int32[0]);
    TEST_ASSERT_EQUAL_INT32(-4, deserialized.repeated_int32[3]);

    // Verify repeated uint32
    TEST_ASSERT_EQUAL_size_t(5, deserialized.repeated_uint32.size());
    TEST_ASSERT_EQUAL_UINT32(10, deserialized.repeated_uint32[0]);
    TEST_ASSERT_EQUAL_UINT32(50, deserialized.repeated_uint32[4]);

    // Verify repeated string
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_string.size());
    TEST_ASSERT_EQUAL_STRING("alpha", deserialized.repeated_string[0].c_str());
    TEST_ASSERT_EQUAL_STRING("gamma", deserialized.repeated_string[2].c_str());

    // Verify repeated bool
    TEST_ASSERT_EQUAL_size_t(4, deserialized.repeated_bool.size());
    TEST_ASSERT_TRUE(deserialized.repeated_bool[0]);
    TEST_ASSERT_FALSE(deserialized.repeated_bool[1]);

    // Verify repeated double
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_double.size());
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.2, deserialized.repeated_double[1]);

    // Verify repeated bytes
    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_bytes.size());
    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_bytes[0].size());
    TEST_ASSERT_EQUAL_UINT8(0x01, deserialized.repeated_bytes[0][0]);
}

void test_packed_types()
{
    using namespace test::scalar;

    // Create message with packed repeated fields
    PackedTypes msg;
    msg.packed_int32   = { 100, 200, 300, 400, 500 };
    msg.packed_double  = { 1.5, 2.5, 3.5 };
    msg.packed_bool    = { true, false, true, false };
    msg.packed_fixed32 = { 0x1111, 0x2222, 0x3333 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    PackedTypes deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify packed fields
    TEST_ASSERT_EQUAL_size_t(5, deserialized.packed_int32.size());
    TEST_ASSERT_EQUAL_INT32(300, deserialized.packed_int32[2]);

    TEST_ASSERT_EQUAL_size_t(3, deserialized.packed_double.size());
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.5, deserialized.packed_double[1]);

    TEST_ASSERT_EQUAL_size_t(4, deserialized.packed_bool.size());
    TEST_ASSERT_TRUE(deserialized.packed_bool[0]);
    TEST_ASSERT_FALSE(deserialized.packed_bool[3]);
}

void test_extreme_values()
{
    using namespace test::scalar;

    // Test extreme values for integer types
    ScalarTypes msg;
    msg.field_int32  = INT32_MIN;
    msg.field_int64  = INT64_MIN;
    msg.field_uint32 = UINT32_MAX;
    msg.field_uint64 = UINT64_MAX;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    ScalarTypes deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify extreme values - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, deserialized.field_int32);
    TEST_ASSERT_EQUAL_INT64(INT64_MIN, deserialized.field_int64);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, deserialized.field_uint32);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, deserialized.field_uint64);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_all_scalar_types);
    RUN_TEST(test_default_values);
    RUN_TEST(test_repeated_scalar_types);
    RUN_TEST(test_packed_types);
    RUN_TEST(test_extreme_values);
    return UNITY_END();
}
