#include "litepb/litepb.h"
#include "nested_messages.pb.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_simple_nesting()
{
    using namespace test::nested;

    // Create nested message
    OuterMessage::InnerMessage inner;
    inner.inner_field = "inner_value";
    inner.inner_value = 42;

    OuterMessage msg;
    msg.inner       = inner;
    msg.outer_field = "outer_value";

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    OuterMessage deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify outer field (implicit scalar - direct type)
    TEST_ASSERT_EQUAL_STRING("outer_value", deserialized.outer_field.c_str());

    // Verify inner message (message field - direct type)
    TEST_ASSERT_EQUAL_STRING("inner_value", deserialized.inner.inner_field.c_str());
    TEST_ASSERT_EQUAL_INT32(42, deserialized.inner.inner_value);
}

void test_deep_nesting()
{
    using namespace test::nested;

    // Create deeply nested structure
    Level1::Level2::Level3::Level4 level4;
    level4.deepest_field = "deepest";

    Level1::Level2::Level3 level3;
    level3.level4       = level4;
    level3.level3_field = "level3";

    Level1::Level2 level2;
    level2.level3       = level3;
    level2.level2_field = "level2";

    Level1 msg;
    msg.level2       = level2;
    msg.level1_field = "level1";

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Level1 deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all levels (all are implicit - direct types)
    TEST_ASSERT_EQUAL_STRING("level1", deserialized.level1_field.c_str());
    TEST_ASSERT_EQUAL_STRING("level2", deserialized.level2.level2_field.c_str());
    TEST_ASSERT_EQUAL_STRING("level3", deserialized.level2.level3.level3_field.c_str());
    TEST_ASSERT_EQUAL_STRING("deepest", deserialized.level2.level3.level4.deepest_field.c_str());
}

void test_multiple_nested_types()
{
    using namespace test::nested;

    // Create different nested types
    Container::TypeA typeA;
    typeA.a_field = "A";

    Container::TypeB typeB;
    typeB.b_field = 100;

    Container::TypeC typeC;
    typeC.c_field = true;

    Container msg;
    msg.type_a     = typeA;
    msg.type_b     = typeB;
    msg.type_c     = typeC;
    msg.repeated_a = { typeA, typeA };
    msg.map_b      = { { "key1", typeB } };

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    Container deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify all types (message fields - direct types)
    TEST_ASSERT_EQUAL_STRING("A", deserialized.type_a.a_field.c_str());
    TEST_ASSERT_EQUAL_INT32(100, deserialized.type_b.b_field);
    TEST_ASSERT_TRUE(deserialized.type_c.c_field);

    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_a.size());
    TEST_ASSERT_EQUAL_size_t(1, deserialized.map_b.size());
}

void test_complex_nested()
{
    using namespace test::nested;

    // Create complex nested message
    ComplexNested::Inner inner;
    inner.text   = "text";
    inner.number = 123;
    inner.tags   = { "tag1", "tag2" };
    inner.values = { { "k", 1 } };
    inner.data   = true;

    ComplexNested msg;
    msg.single_inner   = inner;
    msg.repeated_inner = { inner, inner };
    msg.mapped_inner   = { { "key", inner } };
    msg.optional_inner = inner;

    // Serialize
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(msg, stream));
    TEST_ASSERT_GREATER_THAN(0, stream.size());

    // Deserialize
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    ComplexNested deserialized;
    TEST_ASSERT_TRUE(litepb::parse(deserialized, input_stream));

    // Verify single_inner (message field - direct type)
    TEST_ASSERT_EQUAL_STRING("text", deserialized.single_inner.text.c_str());

    // Verify repeated_inner
    TEST_ASSERT_EQUAL_size_t(2, deserialized.repeated_inner.size());

    // Verify mapped_inner
    TEST_ASSERT_EQUAL_size_t(1, deserialized.mapped_inner.size());
    TEST_ASSERT_EQUAL_STRING("text", deserialized.mapped_inner["key"].text.c_str());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_simple_nesting);
    RUN_TEST(test_deep_nesting);
    RUN_TEST(test_multiple_nested_types);
    RUN_TEST(test_complex_nested);
    return UNITY_END();
}
