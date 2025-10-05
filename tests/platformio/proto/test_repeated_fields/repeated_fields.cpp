#include "litepb/litepb.h"
#include "repeated_fields.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_repeated_basic()
{
    using namespace test::repeated;

    // Create message with basic repeated fields
    RepeatedBasic msg;
    msg.int_values    = { 1, 2, 3, 4, 5 };
    msg.string_values = { "hello", "world", "test" };
    msg.bool_values   = { true, false, true };
    msg.double_values = { 1.1, 2.2, 3.3, 4.4 };
    msg.bytes_values  = { { 0x01, 0x02 }, { 0xAA, 0xBB }, { 0xFF } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    RepeatedBasic deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify int values
    TEST_ASSERT_EQUAL_size_t(5, deserialized.int_values.size());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.int_values[0]);
    TEST_ASSERT_EQUAL_INT32(5, deserialized.int_values[4]);

    // Verify string values
    TEST_ASSERT_EQUAL_size_t(3, deserialized.string_values.size());
    TEST_ASSERT_EQUAL_STRING("hello", deserialized.string_values[0].c_str());
    TEST_ASSERT_EQUAL_STRING("test", deserialized.string_values[2].c_str());

    // Verify bool values
    TEST_ASSERT_EQUAL_size_t(3, deserialized.bool_values.size());
    TEST_ASSERT_TRUE(deserialized.bool_values[0]);
    TEST_ASSERT_FALSE(deserialized.bool_values[1]);

    // Verify double values
    TEST_ASSERT_EQUAL_size_t(4, deserialized.double_values.size());
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.2, deserialized.double_values[1]);

    // Verify bytes values
    TEST_ASSERT_EQUAL_size_t(3, deserialized.bytes_values.size());
    TEST_ASSERT_EQUAL_size_t(2, deserialized.bytes_values[0].size());
    TEST_ASSERT_EQUAL_UINT8(0xAA, deserialized.bytes_values[1][0]);
}

void test_repeated_messages()
{
    using namespace test::repeated;

    // Create submessages
    SubMessage sub1;
    sub1.name  = "first";
    sub1.value = 100;

    SubMessage sub2;
    sub2.name  = "second";
    sub2.value = 200;

    // Create message with repeated messages
    RepeatedMessages msg;
    msg.messages = { sub1, sub2 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    RepeatedMessages deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify repeated messages - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_size_t(2, deserialized.messages.size());

    TEST_ASSERT_EQUAL_STRING("first", deserialized.messages[0].name.c_str());
    TEST_ASSERT_EQUAL_INT32(100, deserialized.messages[0].value);

    TEST_ASSERT_EQUAL_STRING("second", deserialized.messages[1].name.c_str());
    TEST_ASSERT_EQUAL_INT32(200, deserialized.messages[1].value);
}

void test_repeated_enums()
{
    using namespace test::repeated;
    using test::enums::Color;

    // Create message with repeated enums
    RepeatedEnums msg;
    msg.colors   = { Color::COLOR_RED, Color::COLOR_GREEN, Color::COLOR_BLUE };
    msg.statuses = { Status::STATUS_OK, Status::STATUS_ERROR, Status::STATUS_PENDING };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    RepeatedEnums deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify repeated enums
    TEST_ASSERT_EQUAL_size_t(3, deserialized.colors.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_RED), static_cast<int32_t>(deserialized.colors[0]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_BLUE), static_cast<int32_t>(deserialized.colors[2]));

    TEST_ASSERT_EQUAL_size_t(3, deserialized.statuses.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Status::STATUS_OK), static_cast<int32_t>(deserialized.statuses[0]));
}

void test_packed_vs_unpacked()
{
    using namespace test::repeated;

    // Create message with packed and unpacked fields
    PackedTest msg;
    msg.packed_int32    = { 1, 2, 3, 4, 5 };
    msg.packed_uint32   = { 10, 20, 30 };
    msg.packed_bool     = { true, false, true, false };
    msg.packed_enum     = { Status::STATUS_OK, Status::STATUS_ERROR };
    msg.unpacked_int32  = { 100, 200, 300 };
    msg.unpacked_double = { 1.5, 2.5 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    PackedTest deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify packed fields
    TEST_ASSERT_EQUAL_size_t(5, deserialized.packed_int32.size());
    TEST_ASSERT_EQUAL_INT32(3, deserialized.packed_int32[2]);

    // Verify unpacked fields
    TEST_ASSERT_EQUAL_size_t(3, deserialized.unpacked_int32.size());
    TEST_ASSERT_EQUAL_INT32(200, deserialized.unpacked_int32[1]);
}

void test_empty_repeated_fields()
{
    using namespace test::repeated;

    // Create message with empty repeated fields
    EmptyRepeated msg;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    EmptyRepeated deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all repeated fields are empty
    TEST_ASSERT_EQUAL_size_t(0, deserialized.empty_ints.size());
    TEST_ASSERT_EQUAL_size_t(0, deserialized.empty_strings.size());
    TEST_ASSERT_EQUAL_size_t(0, deserialized.empty_messages.size());
}

void test_mixed_fields()
{
    using namespace test::repeated;

    SubMessage sub;
    sub.name  = "sub";
    sub.value = 42;

    // Create message with mixed singular and repeated fields
    MixedFields msg;
    msg.single_string     = "single";
    msg.repeated_ints     = { 1, 2, 3 };
    msg.single_message    = sub;
    msg.repeated_messages = { sub, sub };
    msg.optional_string   = "optional";
    msg.repeated_strings  = { "a", "b", "c" };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MixedFields deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify singular fields - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_STRING("single", deserialized.single_string.c_str());

    // Verify repeated fields
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_ints.size());
    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_messages.size());
    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_strings.size());

    // Verify optional field (explicit optional - std::optional)
    TEST_ASSERT_TRUE(deserialized.optional_string.has_value());
    TEST_ASSERT_EQUAL_STRING("optional", deserialized.optional_string.value().c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_repeated_basic);
    RUN_TEST(test_repeated_messages);
    RUN_TEST(test_repeated_enums);
    RUN_TEST(test_packed_vs_unpacked);
    RUN_TEST(test_empty_repeated_fields);
    RUN_TEST(test_mixed_fields);
    return UNITY_END();
}
