#include "litepb/litepb.h"
#include "simple.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_simple_message_serialization()
{
    using namespace test::simple;

    // Create a SimpleMessage with all fields populated
    SimpleMessage msg;
    msg.name = "test_name";
    msg.id   = 42;
    msg.tags = { "tag1", "tag2", "tag3" };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    SimpleMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_STRING("test_name", deserialized.name.c_str());

    TEST_ASSERT_EQUAL_INT32(42, deserialized.id);

    TEST_ASSERT_EQUAL_size_t(3, deserialized.tags.size());
    TEST_ASSERT_EQUAL_STRING("tag1", deserialized.tags[0].c_str());
    TEST_ASSERT_EQUAL_STRING("tag2", deserialized.tags[1].c_str());
    TEST_ASSERT_EQUAL_STRING("tag3", deserialized.tags[2].c_str());
}

void test_simple_message_empty()
{
    using namespace test::simple;

    // Create empty message
    SimpleMessage msg;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    SimpleMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all fields have default values in proto3
    TEST_ASSERT_EQUAL_STRING("", deserialized.name.c_str()); // Empty string is default
    TEST_ASSERT_EQUAL_INT32(0, deserialized.id);             // 0 is default for int32
    TEST_ASSERT_EQUAL_size_t(0, deserialized.tags.size());
}

void test_request_serialization()
{
    using namespace test::simple;

    // Create TestRequest
    TestRequest req;
    req.client_info   = "TestClient/1.0";
    req.version_major = 2;
    req.version_minor = 5;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(req, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    TestRequest deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 fields are direct types, not optional
    TEST_ASSERT_EQUAL_STRING("TestClient/1.0", deserialized.client_info.c_str());

    TEST_ASSERT_EQUAL_UINT32(2, deserialized.version_major);

    TEST_ASSERT_EQUAL_UINT32(5, deserialized.version_minor);
}

void test_response_serialization()
{
    using namespace test::simple;

    // Create TestResponse with success
    TestResponse resp;
    resp.success = true;
    resp.message = "Operation completed successfully";
    resp.code    = 200;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(resp, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    TestResponse deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 fields are direct types, not optional
    TEST_ASSERT_TRUE(deserialized.success);

    TEST_ASSERT_EQUAL_STRING("Operation completed successfully", deserialized.message.c_str());

    TEST_ASSERT_EQUAL_INT32(200, deserialized.code);
}

void test_response_failure()
{
    using namespace test::simple;

    // Create TestResponse with failure
    TestResponse resp;
    resp.success = false;
    resp.message = "Error occurred";
    resp.code    = 500;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(resp, stream));

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    TestResponse deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values - proto3 fields are direct types, not optional
    TEST_ASSERT_FALSE(deserialized.success);

    TEST_ASSERT_EQUAL_INT32(500, deserialized.code);
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_simple_message_serialization);
    RUN_TEST(test_simple_message_empty);
    RUN_TEST(test_request_serialization);
    RUN_TEST(test_response_serialization);
    RUN_TEST(test_response_failure);
    return UNITY_END();
}
