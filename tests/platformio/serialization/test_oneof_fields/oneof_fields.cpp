#include "litepb/litepb.h"
#include "oneof_fields.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_basic_oneof()
{
    using namespace test::oneof;

    // Test with string value
    BasicOneof msg1;
    msg1.value = std::string("test_string");

    litepb::BufferOutputStream stream1;
    TEST_ASSERT_TRUE(litepb::serialize(msg1, stream1));

    litepb::BufferInputStream input_stream1(stream1.data(), stream1.size());
    BasicOneof deserialized1;
    TEST_ASSERT_TRUE(litepb::parse(deserialized1, input_stream1));

    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(deserialized1.value));
    TEST_ASSERT_EQUAL_STRING("test_string", std::get<std::string>(deserialized1.value).c_str());

    // Test with int value
    BasicOneof msg2;
    msg2.value = int32_t(42);

    litepb::BufferOutputStream stream2;
    TEST_ASSERT_TRUE(litepb::serialize(msg2, stream2));

    litepb::BufferInputStream input_stream2(stream2.data(), stream2.size());
    BasicOneof deserialized2;
    TEST_ASSERT_TRUE(litepb::parse(deserialized2, input_stream2));

    TEST_ASSERT_TRUE(std::holds_alternative<int32_t>(deserialized2.value));
    TEST_ASSERT_EQUAL_INT32(42, std::get<int32_t>(deserialized2.value));

    // Test with bool value
    BasicOneof msg3;
    msg3.value = true;

    litepb::BufferOutputStream stream3;
    TEST_ASSERT_TRUE(litepb::serialize(msg3, stream3));

    litepb::BufferInputStream input_stream3(stream3.data(), stream3.size());
    BasicOneof deserialized3;
    TEST_ASSERT_TRUE(litepb::parse(deserialized3, input_stream3));

    TEST_ASSERT_TRUE(std::holds_alternative<bool>(deserialized3.value));
    TEST_ASSERT_TRUE(std::get<bool>(deserialized3.value));
}

void test_oneof_with_messages()
{
    using namespace test::oneof;

    // Create simple message
    SimpleMessage simple;
    simple.text = "simple text";

    OneofWithMessages msg;
    msg.data = simple;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    OneofWithMessages deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify
    TEST_ASSERT_TRUE(std::holds_alternative<SimpleMessage>(deserialized.data));
    auto& deser_simple = std::get<SimpleMessage>(deserialized.data);
    TEST_ASSERT_EQUAL_STRING("simple text", deser_simple.text.c_str());
}

void test_multiple_oneofs()
{
    using namespace test::oneof;

    SimpleMessage msg_val;
    msg_val.text = "message";

    // Create message with multiple oneofs
    MultipleOneofs msg;
    msg.first          = std::string("first_string");
    msg.second         = false;
    msg.third          = msg_val;
    msg.regular_field  = "regular";
    msg.repeated_field = { 1, 2, 3 };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MultipleOneofs deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify first oneof
    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(deserialized.first));
    TEST_ASSERT_EQUAL_STRING("first_string", std::get<std::string>(deserialized.first).c_str());

    // Verify second oneof
    TEST_ASSERT_TRUE(std::holds_alternative<bool>(deserialized.second));
    TEST_ASSERT_FALSE(std::get<bool>(deserialized.second));

    // Verify third oneof
    TEST_ASSERT_TRUE(std::holds_alternative<SimpleMessage>(deserialized.third));

    // Verify regular fields
    TEST_ASSERT_EQUAL_STRING("regular", deserialized.regular_field.c_str());

    TEST_ASSERT_EQUAL_size_t(3, deserialized.repeated_field.size());
    TEST_ASSERT_EQUAL_INT32(2, deserialized.repeated_field[1]);
}

void test_oneof_all_types()
{
    using namespace test::oneof;

    // Test with different scalar types
    OneofAllTypes msg1;
    msg1.value = int32_t(12345);

    litepb::BufferOutputStream stream1;
    TEST_ASSERT_TRUE(litepb::serialize(msg1, stream1));

    litepb::BufferInputStream input_stream1(stream1.data(), stream1.size());
    OneofAllTypes deserialized1;
    TEST_ASSERT_TRUE(litepb::parse(deserialized1, input_stream1));

    TEST_ASSERT_TRUE(std::holds_alternative<int32_t>(deserialized1.value));
    TEST_ASSERT_EQUAL_INT32(12345, std::get<int32_t>(deserialized1.value));

    // Test with double
    OneofAllTypes msg2;
    msg2.value = 3.14159;

    litepb::BufferOutputStream stream2;
    TEST_ASSERT_TRUE(litepb::serialize(msg2, stream2));

    litepb::BufferInputStream input_stream2(stream2.data(), stream2.size());
    OneofAllTypes deserialized2;
    TEST_ASSERT_TRUE(litepb::parse(deserialized2, input_stream2));

    TEST_ASSERT_TRUE(std::holds_alternative<double>(deserialized2.value));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 3.14159, std::get<double>(deserialized2.value));

    // Test with string
    OneofAllTypes msg3;
    msg3.value = std::string("oneof_string");

    litepb::BufferOutputStream stream3;
    TEST_ASSERT_TRUE(litepb::serialize(msg3, stream3));

    litepb::BufferInputStream input_stream3(stream3.data(), stream3.size());
    OneofAllTypes deserialized3;
    TEST_ASSERT_TRUE(litepb::parse(deserialized3, input_stream3));

    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(deserialized3.value));
    TEST_ASSERT_EQUAL_STRING("oneof_string", std::get<std::string>(deserialized3.value).c_str());
}

void test_empty_oneof()
{
    using namespace test::oneof;

    // Create message with unset oneof
    EmptyOneof msg;
    msg.other_field = "other";

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    EmptyOneof deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify oneof is empty (holds monostate)
    TEST_ASSERT_TRUE(std::holds_alternative<std::monostate>(deserialized.maybe_value));

    // Verify other field
    TEST_ASSERT_EQUAL_STRING("other", deserialized.other_field.c_str());
}

void test_oneof_overwrite()
{
    using namespace test::oneof;

    // Set oneof to one value, then change it
    BasicOneof msg;
    msg.value = int32_t(100);            // Set to int
    msg.value = std::string("replaced"); // Replace with string

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    BasicOneof deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Should only have the last set value (string)
    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(deserialized.value));
    TEST_ASSERT_EQUAL_STRING("replaced", std::get<std::string>(deserialized.value).c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_oneof);
    RUN_TEST(test_oneof_with_messages);
    RUN_TEST(test_multiple_oneofs);
    RUN_TEST(test_oneof_all_types);
    RUN_TEST(test_empty_oneof);
    RUN_TEST(test_oneof_overwrite);
    return UNITY_END();
}
