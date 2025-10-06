#include "litepb/litepb.h"
#include "map_types.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_basic_maps()
{
    using namespace test::maps;

    // Create message with basic map types
    BasicMaps msg;
    msg.string_to_string = { { "key1", "value1" }, { "key2", "value2" } };
    msg.int_to_int       = { { 1, 10 }, { 2, 20 }, { 3, 30 } };
    msg.string_to_int    = { { "one", 1 }, { "two", 2 } };
    msg.int_to_string    = { { 100, "hundred" }, { 200, "two hundred" } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    BasicMaps deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify string_to_string map
    TEST_ASSERT_EQUAL_size_t(2, deserialized.string_to_string.size());
    TEST_ASSERT_EQUAL_STRING("value1", deserialized.string_to_string["key1"].c_str());
    TEST_ASSERT_EQUAL_STRING("value2", deserialized.string_to_string["key2"].c_str());

    // Verify int_to_int map
    TEST_ASSERT_EQUAL_size_t(3, deserialized.int_to_int.size());
    TEST_ASSERT_EQUAL_INT32(10, deserialized.int_to_int[1]);
    TEST_ASSERT_EQUAL_INT32(30, deserialized.int_to_int[3]);

    // Verify string_to_int map
    TEST_ASSERT_EQUAL_size_t(2, deserialized.string_to_int.size());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.string_to_int["one"]);

    // Verify int_to_string map
    TEST_ASSERT_EQUAL_size_t(2, deserialized.int_to_string.size());
    TEST_ASSERT_EQUAL_STRING("hundred", deserialized.int_to_string[100].c_str());
}

void test_map_with_messages()
{
    using namespace test::maps;

    // Create value messages
    ValueMessage val1;
    val1.name  = "first";
    val1.count = 10;
    val1.tags  = { "tag1", "tag2" };

    ValueMessage val2;
    val2.name  = "second";
    val2.count = 20;

    // Create message with map of messages
    MapWithMessages msg;
    msg.string_to_message = { { "a", val1 }, { "b", val2 } };
    msg.int_to_message    = { { 1, val1 }, { 2, val2 } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MapWithMessages deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify string_to_message map - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_size_t(2, deserialized.string_to_message.size());
    TEST_ASSERT_EQUAL_STRING("first", deserialized.string_to_message["a"].name.c_str());
    TEST_ASSERT_EQUAL_INT32(10, deserialized.string_to_message["a"].count);

    // Verify int_to_message map - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_size_t(2, deserialized.int_to_message.size());
    TEST_ASSERT_EQUAL_STRING("second", deserialized.int_to_message[2].name.c_str());
}

void test_map_with_enums()
{
    using namespace test::maps;

    // Create message with enum maps
    MapWithEnums msg;
    msg.string_to_enum = { { "red", Color::COLOR_RED }, { "blue", Color::COLOR_BLUE } };
    msg.int_to_status  = { { 1, Status::STATUS_ACTIVE }, { 2, Status::STATUS_INACTIVE } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MapWithEnums deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify string_to_enum map
    TEST_ASSERT_EQUAL_size_t(2, deserialized.string_to_enum.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_RED), static_cast<int32_t>(deserialized.string_to_enum["red"]));

    // Verify int_to_status map
    TEST_ASSERT_EQUAL_size_t(2, deserialized.int_to_status.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Status::STATUS_ACTIVE), static_cast<int32_t>(deserialized.int_to_status[1]));
}

void test_empty_maps()
{
    using namespace test::maps;

    // Create message with empty maps
    EmptyMaps msg;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    EmptyMaps deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify maps are empty
    TEST_ASSERT_EQUAL_size_t(0, deserialized.empty_string_map.size());
    TEST_ASSERT_EQUAL_size_t(0, deserialized.empty_message_map.size());
}

void test_mixed_with_maps()
{
    using namespace test::maps;

    ValueMessage val;
    val.name  = "test";
    val.count = 42;

    // Create message mixing maps with other fields
    MixedWithMaps msg;
    msg.regular_field  = "regular";
    msg.map_field      = { { "k1", 1 }, { "k2", 2 } };
    msg.repeated_field = { "r1", "r2" };
    msg.optional_field = 999;
    msg.message_field  = val;
    msg.message_map    = { { 1, val } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MixedWithMaps deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all field types - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_STRING("regular", deserialized.regular_field.c_str());

    TEST_ASSERT_EQUAL_size_t(2, deserialized.map_field.size());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.map_field["k1"]);

    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_field.size());
    TEST_ASSERT_EQUAL_STRING("r2", deserialized.repeated_field[1].c_str());

    // Verify optional field (explicit optional - std::optional)
    TEST_ASSERT_TRUE(deserialized.optional_field.has_value());
    TEST_ASSERT_EQUAL_INT32(999, deserialized.optional_field.value());

    TEST_ASSERT_EQUAL_size_t(1, deserialized.message_map.size());
    TEST_ASSERT_EQUAL_STRING("test", deserialized.message_map[1].name.c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_maps);
    RUN_TEST(test_map_with_messages);
    RUN_TEST(test_map_with_enums);
    RUN_TEST(test_empty_maps);
    RUN_TEST(test_mixed_with_maps);
    return UNITY_END();
}
