#include "litepb/litepb.h"
#include "rpc.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_packaged_message()
{
    using namespace test::packaging::v1;

    // Create packaged message with local Color enum
    PackagedMessage msg;
    msg.message_field = "test message";
    msg.version       = 1;
    msg.color         = Color::COLOR_RED;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    PackagedMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify values (all implicit - direct types)
    TEST_ASSERT_EQUAL_STRING("test message", deserialized.message_field.c_str());
    TEST_ASSERT_EQUAL_INT32(1, deserialized.version);
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_RED), static_cast<int32_t>(deserialized.color));
}

void test_api_versioning()
{
    using namespace test::packaging::v1;

    // Create V1 request and response
    ApiVersion::V1::Request req1;
    req1.query = "v1 query";

    ApiVersion::V1::Response resp1;
    resp1.result = "v1 result";

    // Serialize and deserialize V1 messages
    litepb::BufferOutputStream stream1;
    TEST_ASSERT_TRUE(litepb::serialize(req1, stream1));

    litepb::BufferInputStream input_stream1(stream1.data(), stream1.size());
    ApiVersion::V1::Request deser_req1;
    TEST_ASSERT_TRUE(litepb::parse(deser_req1, input_stream1));

    TEST_ASSERT_EQUAL_STRING("v1 query", deser_req1.query.c_str());

    // Create V2 request and response
    ApiVersion::V2::Request req2;
    req2.query      = "v2 query";
    req2.parameters = { { "key", "value" } };

    ApiVersion::V2::Response resp2;
    resp2.result      = "v2 result";
    resp2.status_code = 200;

    // Serialize and deserialize V2 messages
    litepb::BufferOutputStream stream2;
    TEST_ASSERT_TRUE(litepb::serialize(req2, stream2));

    litepb::BufferInputStream input_stream2(stream2.data(), stream2.size());
    ApiVersion::V2::Request deser_req2;
    TEST_ASSERT_TRUE(litepb::parse(deser_req2, input_stream2));

    TEST_ASSERT_EQUAL_STRING("v2 query", deser_req2.query.c_str());

    TEST_ASSERT_EQUAL_size_t(1, deser_req2.parameters.size());
    TEST_ASSERT_EQUAL_STRING("value", deser_req2.parameters["key"].c_str());
}

void test_packaged_enum()
{
    using namespace test::packaging::v1;

    // Test packaged enum
    PackagedStatus status = PackagedStatus::PACKAGED_STATUS_ACTIVE;
    TEST_ASSERT_EQUAL_INT32(1, static_cast<int32_t>(status));
}

void test_complex_packaged_message()
{
    using namespace test::packaging::v1;

    // Create complex message with local package references
    PackagedMessage local;
    local.message_field = "local";
    local.version       = 2;
    local.color         = Color::COLOR_BLUE;

    ApiVersion::V2::Request api_req;
    api_req.query = "api query";

    ApiVersion::V2::Response api_resp;
    api_resp.result      = "api result";
    api_resp.status_code = 200;

    ComplexPackagedMessage msg;
    msg.local_message   = local;
    msg.external_colors = { Color::COLOR_RED, Color::COLOR_GREEN };
    msg.status_map      = { { "active", PackagedStatus::PACKAGED_STATUS_ACTIVE } };
    msg.api_request     = api_req;
    msg.api_response    = api_resp;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    ComplexPackagedMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify local message (message field - direct type)
    TEST_ASSERT_EQUAL_STRING("local", deserialized.local_message.message_field.c_str());

    // Verify external colors
    TEST_ASSERT_EQUAL_size_t(2, deserialized.external_colors.size());
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_RED), static_cast<int32_t>(deserialized.external_colors[0]));

    // Verify status map
    TEST_ASSERT_EQUAL_size_t(1, deserialized.status_map.size());

    // Verify API request/response (message fields - direct types)
    TEST_ASSERT_EQUAL_STRING("api query", deserialized.api_request.query.c_str());
}

void test_color_namespace_disambiguation()
{
    using namespace test::packaging::v1;

    // Test RGBColor (message)
    test::packaging::v1::RGBColor rgb_color;
    rgb_color.red   = 255;
    rgb_color.green = 128;
    rgb_color.blue  = 64;
    rgb_color.alpha = 255;

    // Test ColorTest which uses both
    ColorTest msg;
    msg.rgb_color  = rgb_color;
    msg.enum_color = Color::COLOR_GREEN;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    ColorTest deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify RGB color (message field - direct type with implicit scalar fields)
    TEST_ASSERT_EQUAL_INT32(255, deserialized.rgb_color.red);
    TEST_ASSERT_EQUAL_INT32(128, deserialized.rgb_color.green);

    // Verify enum color (enum field - direct type)
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(Color::COLOR_GREEN), static_cast<int32_t>(deserialized.enum_color));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_packaged_message);
    RUN_TEST(test_api_versioning);
    RUN_TEST(test_packaged_enum);
    RUN_TEST(test_complex_packaged_message);
    RUN_TEST(test_color_namespace_disambiguation);
    return UNITY_END();
}
