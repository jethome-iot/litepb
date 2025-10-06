#include "litepb/litepb.h"
#include "proto3_features.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_implicit_presence()
{
    using namespace test::proto3;

    // Create message with implicit presence fields
    Proto3Message msg;
    msg.implicit_string = "test";
    msg.implicit_int32  = 42;
    msg.implicit_bool   = true;
    msg.implicit_double = 3.14;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto3Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 implicit fields are direct types, not optional
    TEST_ASSERT_EQUAL_STRING("test", deserialized.implicit_string.c_str());

    TEST_ASSERT_EQUAL_INT32(42, deserialized.implicit_int32);

    TEST_ASSERT_TRUE(deserialized.implicit_bool);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, 3.14, deserialized.implicit_double);
}

void test_explicit_optional()
{
    using namespace test::proto3;

    // Create message with explicit optional fields
    Proto3Message msg;
    msg.explicit_optional_string = "optional";
    msg.explicit_optional_int32  = 99;
    msg.explicit_optional_bool   = false;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto3Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify explicit optional fields - these should still be optional in proto3
    TEST_ASSERT_TRUE(deserialized.explicit_optional_string.has_value());
    TEST_ASSERT_EQUAL_STRING("optional", deserialized.explicit_optional_string.value().c_str());

    TEST_ASSERT_TRUE(deserialized.explicit_optional_int32.has_value());
    TEST_ASSERT_EQUAL_INT32(99, deserialized.explicit_optional_int32.value());

    TEST_ASSERT_TRUE(deserialized.explicit_optional_bool.has_value());
    TEST_ASSERT_FALSE(deserialized.explicit_optional_bool.value());
}

void test_nested_message()
{
    using namespace test::proto3;

    // Create nested message
    NestedProto3 nested;
    nested.name  = "nested";
    nested.value = 123;

    Proto3Message msg;
    msg.nested_message    = nested;
    msg.repeated_messages = { nested, nested };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Proto3Message deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify nested message (message field - direct type with implicit scalar fields)
    TEST_ASSERT_EQUAL_STRING("nested", deserialized.nested_message.name.c_str());
    TEST_ASSERT_EQUAL_INT32(123, deserialized.nested_message.value);

    // Verify repeated messages
    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_messages.size());
    TEST_ASSERT_EQUAL_INT32(123, deserialized.repeated_messages[0].value);
}

void test_field_presence()
{
    using namespace test::proto3;

    // Create message with different field presence patterns
    FieldPresenceTest msg;
    msg.primitive_field    = 42;
    msg.optional_primitive = 99;

    NestedProto3 nested;
    nested.name       = "test";
    msg.message_field = nested;

    msg.repeated_field = { 1, 2, 3 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    FieldPresenceTest deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify primitive field (implicit - direct type)
    TEST_ASSERT_EQUAL_INT32(42, deserialized.primitive_field);

    // Verify optional primitive (explicit optional - std::optional)
    TEST_ASSERT_TRUE(deserialized.optional_primitive.has_value());
    TEST_ASSERT_EQUAL_INT32(99, deserialized.optional_primitive.value());

    // Verify message field (message field - direct type with implicit scalar fields)
    TEST_ASSERT_EQUAL_STRING("test", deserialized.message_field.name.c_str());

    // Verify repeated field
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_field.size());
}

void test_proto3_enums()
{
    using namespace test::proto3;

    // Proto3 enums must start with 0
    TEST_ASSERT_EQUAL_INT32(0, static_cast<int32_t>(Proto3Enum::UNKNOWN));
    TEST_ASSERT_EQUAL_INT32(1, static_cast<int32_t>(Proto3Enum::STARTED));
    TEST_ASSERT_EQUAL_INT32(2, static_cast<int32_t>(Proto3Enum::RUNNING));
}

void test_alias_enum()
{
    using namespace test::proto3;

    // Test enum with allow_alias
    TEST_ASSERT_EQUAL_INT32(0, static_cast<int32_t>(AliasEnum::UNKNOWN_ALIAS));
    TEST_ASSERT_EQUAL_INT32(1, static_cast<int32_t>(AliasEnum::STARTED_ALIAS));
    TEST_ASSERT_EQUAL_INT32(2, static_cast<int32_t>(AliasEnum::RUNNING_ALIAS));
    TEST_ASSERT_EQUAL_INT32(2, static_cast<int32_t>(AliasEnum::ALSO_RUNNING));

    // Aliases should have the same value
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(AliasEnum::RUNNING_ALIAS), static_cast<int32_t>(AliasEnum::ALSO_RUNNING));
}

void test_empty_message()
{
    using namespace test::proto3;

    // Test empty message
    Empty msg;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Empty deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Empty message should serialize/deserialize successfully
}

void test_wrapper_types()
{
    using namespace test::proto3;

    // Create message with wrapper-like optional fields
    WrapperTypesTest msg;
    msg.double_value = 1.23;
    msg.float_value  = 4.56f;
    msg.int64_value  = 123456789LL;
    msg.uint64_value = 987654321ULL;
    msg.int32_value  = 12345;
    msg.uint32_value = 54321U;
    msg.bool_value   = true;
    msg.string_value = "wrapped";
    msg.bytes_value  = { 0x01, 0x02, 0x03 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    WrapperTypesTest deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all wrapper values
    TEST_ASSERT_TRUE(deserialized.double_value.has_value());
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.23, deserialized.double_value.value());

    TEST_ASSERT_TRUE(deserialized.int32_value.has_value());
    TEST_ASSERT_EQUAL_INT32(12345, deserialized.int32_value.value());

    TEST_ASSERT_TRUE(deserialized.bool_value.has_value());
    TEST_ASSERT_TRUE(deserialized.bool_value.value());

    TEST_ASSERT_TRUE(deserialized.string_value.has_value());
    TEST_ASSERT_EQUAL_STRING("wrapped", deserialized.string_value.value().c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_implicit_presence);
    RUN_TEST(test_explicit_optional);
    RUN_TEST(test_nested_message);
    RUN_TEST(test_field_presence);
    RUN_TEST(test_proto3_enums);
    RUN_TEST(test_alias_enum);
    RUN_TEST(test_empty_message);
    RUN_TEST(test_wrapper_types);
    return UNITY_END();
}
