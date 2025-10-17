#include "litepb/litepb.h"
#include "proto2_features.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_required_fields()
{
    using namespace test::proto2;

    // Create message with required fields
    NestedProto2 nested;
    nested.name  = "nested";
    nested.value = 100;

    Proto2Message msg;
    msg.required_string = "required";
    msg.required_int32  = 42;
    msg.required_nested = nested;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto2Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify required fields
    TEST_ASSERT_TRUE(deserialized.required_string.has_value());
    TEST_ASSERT_EQUAL_STRING("required", deserialized.required_string.value().c_str());

    TEST_ASSERT_TRUE(deserialized.required_int32.has_value());
    TEST_ASSERT_EQUAL_INT32(42, deserialized.required_int32.value());

    TEST_ASSERT_TRUE(deserialized.required_nested.has_value());
}

void test_optional_fields()
{
    using namespace test::proto2;

    // Create message with optional fields set
    Proto2Message msg;
    msg.required_string = "req";
    msg.required_int32  = 1;

    NestedProto2 nested;
    nested.name         = "nest";
    msg.required_nested = nested;

    msg.optional_string = "optional";
    msg.optional_int32  = 99;
    msg.optional_bool   = true;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto2Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify optional fields
    TEST_ASSERT_TRUE(deserialized.optional_string.has_value());
    TEST_ASSERT_EQUAL_STRING("optional", deserialized.optional_string.value().c_str());

    TEST_ASSERT_TRUE(deserialized.optional_int32.has_value());
    TEST_ASSERT_EQUAL_INT32(99, deserialized.optional_int32.value());

    TEST_ASSERT_TRUE(deserialized.optional_bool.has_value());
    TEST_ASSERT_TRUE(deserialized.optional_bool.value());
}

void test_default_values()
{
    using namespace test::proto2;

    // Create message with fields that have default values
    DefaultValueMessage msg;
    // Don't set any fields - they should get defaults

    // For proto2, if we don't set optional fields with defaults,
    // they won't be serialized but should use defaults when accessed
    // This test just verifies serialization works

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    DefaultValueMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // In LitePB, unset optional fields don't have values
    // The default values would be handled by the application layer
    TEST_ASSERT_FALSE(deserialized.int_value.has_value());
}

void test_default_values_set()
{
    using namespace test::proto2;

    // Create message and explicitly set values (including defaults)
    DefaultValueMessage msg;
    msg.int_value    = -42;
    msg.uint_value   = 42;
    msg.bool_value   = true;
    msg.string_value = "default text";
    msg.enum_value   = BytesEnum::VALUE_B;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    DefaultValueMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all values
    TEST_ASSERT_TRUE(deserialized.int_value.has_value());
    TEST_ASSERT_EQUAL_INT32(-42, deserialized.int_value.value());

    TEST_ASSERT_TRUE(deserialized.uint_value.has_value());
    TEST_ASSERT_EQUAL_UINT32(42, deserialized.uint_value.value());

    TEST_ASSERT_TRUE(deserialized.bool_value.has_value());
    TEST_ASSERT_TRUE(deserialized.bool_value.value());

    TEST_ASSERT_TRUE(deserialized.string_value.has_value());
    TEST_ASSERT_EQUAL_STRING("default text", deserialized.string_value.value().c_str());
}

void test_repeated_string()
{
    using namespace test::proto2;

    NestedProto2 nested;
    nested.name = "test";

    // Create message with repeated field
    Proto2Message msg;
    msg.required_string = "req";
    msg.required_int32  = 1;
    msg.required_nested = nested;
    msg.repeated_string = { "one", "two", "three" };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto2Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify repeated field
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_string.size());
    TEST_ASSERT_EQUAL_STRING("one", deserialized.repeated_string[0].c_str());
    TEST_ASSERT_EQUAL_STRING("three", deserialized.repeated_string[2].c_str());
}

void test_field_options()
{
    using namespace test::proto2;

    // Test packed and unpacked repeated fields
    FieldOptionsMessage msg;
    msg.packed_field   = { 1, 2, 3, 4, 5 };
    msg.unpacked_field = { 10, 20, 30 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    FieldOptionsMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify packed field
    TEST_ASSERT_EQUAL_size_t(5, deserialized.packed_field.size());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.packed_field[0]);
    TEST_ASSERT_EQUAL_INT32(5, deserialized.packed_field[4]);

    // Verify unpacked field
    TEST_ASSERT_EQUAL_size_t(3, deserialized.unpacked_field.size());
    TEST_ASSERT_EQUAL_INT32(20, deserialized.unpacked_field[1]);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_required_fields);
    RUN_TEST(test_optional_fields);
    RUN_TEST(test_default_values);
    RUN_TEST(test_default_values_set);
    RUN_TEST(test_repeated_string);
    RUN_TEST(test_field_options);
    return UNITY_END();
}
