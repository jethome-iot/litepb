#include "enums.pb.h"
#include "litepb/litepb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_enum_serialization()
{
    using namespace test::enums;

    // Create a message with enum values
    EnumMessage msg;
    msg.color       = Color::COLOR_BLUE;
    msg.priority    = Priority::PRIORITY_HIGH;
    msg.temperature = Temperature::TEMP_FREEZING;
    msg.status      = AliasedStatus::STATUS_RUNNING;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Check that data was written
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    EnumMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 enum fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_BLUE), static_cast<int32_t>(deserialized.color));

    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Priority::PRIORITY_HIGH), static_cast<int32_t>(deserialized.priority));

    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Temperature::TEMP_FREEZING), static_cast<int32_t>(deserialized.temperature));

    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(AliasedStatus::STATUS_RUNNING), static_cast<int32_t>(deserialized.status));
}

void test_nested_enum_serialization()
{
    using namespace test::enums;

    // Create a message with nested enum
    MessageWithNestedEnum msg;
    msg.state          = MessageWithNestedEnum::InternalState::STATE_READY;
    msg.external_color = Color::COLOR_GREEN;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Check that data was written
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    MessageWithNestedEnum deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 enum fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(MessageWithNestedEnum::InternalState::STATE_READY),
                            static_cast<int32_t>(deserialized.state));

    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_GREEN), static_cast<int32_t>(deserialized.external_color));
}

void test_repeated_enums_serialization()
{
    using namespace test::enums;

    // Create a message with repeated enums
    RepeatedEnums msg;
    msg.colors.push_back(Color::COLOR_RED);
    msg.colors.push_back(Color::COLOR_GREEN);
    msg.colors.push_back(Color::COLOR_BLUE);

    msg.priorities.push_back(Priority::PRIORITY_LOW);
    msg.priorities.push_back(Priority::PRIORITY_MEDIUM);
    msg.priorities.push_back(Priority::PRIORITY_HIGH);

    msg.states.push_back(MessageWithNestedEnum::InternalState::STATE_INIT);
    msg.states.push_back(MessageWithNestedEnum::InternalState::STATE_READY);
    msg.states.push_back(MessageWithNestedEnum::InternalState::STATE_BUSY);

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Check that data was written
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    RepeatedEnums deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify colors
    TEST_ASSERT_EQUAL_UINT32(3, deserialized.colors.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_RED), static_cast<int32_t>(deserialized.colors[0]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_GREEN), static_cast<int32_t>(deserialized.colors[1]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_BLUE), static_cast<int32_t>(deserialized.colors[2]));

    // Verify priorities
    TEST_ASSERT_EQUAL_UINT32(3, deserialized.priorities.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Priority::PRIORITY_LOW), static_cast<int32_t>(deserialized.priorities[0]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Priority::PRIORITY_MEDIUM), static_cast<int32_t>(deserialized.priorities[1]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Priority::PRIORITY_HIGH), static_cast<int32_t>(deserialized.priorities[2]));

    // Verify states
    TEST_ASSERT_EQUAL_UINT32(3, deserialized.states.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(MessageWithNestedEnum::InternalState::STATE_INIT),
                            static_cast<int32_t>(deserialized.states[0]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(MessageWithNestedEnum::InternalState::STATE_READY),
                            static_cast<int32_t>(deserialized.states[1]));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(MessageWithNestedEnum::InternalState::STATE_BUSY),
                            static_cast<int32_t>(deserialized.states[2]));
}

void test_enum_edge_cases()
{
    using namespace test::enums;

    // Test with negative enum values
    EnumMessage msg;
    msg.temperature = Temperature::TEMP_FREEZING; // -10

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    EnumMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify negative value - proto3 enum fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(-10, static_cast<int32_t>(deserialized.temperature));
}

void test_enum_aliasing()
{
    using namespace test::enums;

    // Test aliased enum values - ensure both messages are initialized identically
    EnumMessage msg1;
    msg1.color       = Color::COLOR_UNSPECIFIED;       // Explicitly set to default
    msg1.priority    = Priority::PRIORITY_UNSPECIFIED; // Explicitly set to default
    msg1.temperature = Temperature::TEMP_UNKNOWN;      // Explicitly set to default
    msg1.status      = AliasedStatus::STATUS_STARTED;

    EnumMessage msg2;
    msg2.color       = Color::COLOR_UNSPECIFIED;       // Explicitly set to default
    msg2.priority    = Priority::PRIORITY_UNSPECIFIED; // Explicitly set to default
    msg2.temperature = Temperature::TEMP_UNKNOWN;      // Explicitly set to default
    msg2.status      = AliasedStatus::STATUS_RUNNING;  // Same value as STATUS_STARTED

    // Serialize both
    litepb::BufferOutputStream stream1;
    TEST_ASSERT_TRUE(litepb::serialize(msg1, stream1));

    litepb::BufferOutputStream stream2;
    TEST_ASSERT_TRUE(litepb::serialize(msg2, stream2));

    // The serialized data should be identical since both status values are aliases (same underlying value)
    TEST_ASSERT_EQUAL_UINT32(stream1.size(), stream2.size());
    TEST_ASSERT_EQUAL_MEMORY(stream1.data(), stream2.data(), stream1.size());
}

void test_calendar_enum()
{
    using namespace test::enums;

    // Create a calendar message
    Calendar msg;
    msg.day_of_week  = DayOfWeek::DAY_FRIDAY;
    msg.month        = Month::MONTH_OCTOBER;
    msg.day_of_month = 31;
    msg.year         = 2025;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Calendar deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_INT32(5, static_cast<int32_t>(deserialized.day_of_week));

    TEST_ASSERT_EQUAL_INT32(10, static_cast<int32_t>(deserialized.month));

    TEST_ASSERT_EQUAL_INT32(31, deserialized.day_of_month);

    TEST_ASSERT_EQUAL_INT32(2025, deserialized.year);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_enum_serialization);
    RUN_TEST(test_nested_enum_serialization);
    RUN_TEST(test_repeated_enums_serialization);
    RUN_TEST(test_enum_edge_cases);
    RUN_TEST(test_enum_aliasing);
    RUN_TEST(test_calendar_enum);
    return UNITY_END();
}
