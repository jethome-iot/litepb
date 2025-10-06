#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include "unity.h"
#include <cstring>
#include <limits>

void setUp() {}
void tearDown() {}

void test_write_varint_zero()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_varint(0));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x00, stream.data()[0]);
}

void test_write_varint_127()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_varint(127));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x7F, stream.data()[0]);
}

void test_write_varint_128()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_varint(128));
    TEST_ASSERT_EQUAL_UINT32(2, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x80, stream.data()[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, stream.data()[1]);
}

void test_write_varint_255()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_varint(255));
    TEST_ASSERT_EQUAL_UINT32(2, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0xFF, stream.data()[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, stream.data()[1]);
}

void test_write_varint_large()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    uint64_t large_value = 0xFFFFFFFFFFFFFFFFULL;
    TEST_ASSERT_TRUE(writer.write_varint(large_value));
    TEST_ASSERT_EQUAL_UINT32(10, stream.size());
}

void test_write_fixed32()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_fixed32(0x12345678));
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x78, stream.data()[0]);
    TEST_ASSERT_EQUAL_UINT8(0x56, stream.data()[1]);
    TEST_ASSERT_EQUAL_UINT8(0x34, stream.data()[2]);
    TEST_ASSERT_EQUAL_UINT8(0x12, stream.data()[3]);
}

void test_write_fixed32_zero()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_fixed32(0));
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, stream.data()[i]);
    }
}

void test_write_fixed64()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_fixed64(0x123456789ABCDEF0ULL));
    TEST_ASSERT_EQUAL_UINT32(8, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0xF0, stream.data()[0]);
    TEST_ASSERT_EQUAL_UINT8(0xDE, stream.data()[1]);
    TEST_ASSERT_EQUAL_UINT8(0xBC, stream.data()[2]);
    TEST_ASSERT_EQUAL_UINT8(0x9A, stream.data()[3]);
}

void test_write_fixed64_zero()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_fixed64(0));
    TEST_ASSERT_EQUAL_UINT32(8, stream.size());
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, stream.data()[i]);
    }
}

void test_write_sfixed32_positive()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sfixed32(12345));
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());
}

void test_write_sfixed32_negative()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sfixed32(-12345));
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());
}

void test_write_sfixed64_positive()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sfixed64(123456789LL));
    TEST_ASSERT_EQUAL_UINT32(8, stream.size());
}

void test_write_sfixed64_negative()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sfixed64(-123456789LL));
    TEST_ASSERT_EQUAL_UINT32(8, stream.size());
}

void test_write_float()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_float(3.14159f));
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());
}

void test_write_double()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_double(3.14159265358979));
    TEST_ASSERT_EQUAL_UINT32(8, stream.size());
}

void test_write_empty_string()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    std::string empty_str = "";
    TEST_ASSERT_TRUE(writer.write_string(empty_str));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x00, stream.data()[0]);
}

void test_write_normal_string()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    std::string test_str = "Hello";
    TEST_ASSERT_TRUE(writer.write_string(test_str));
    TEST_ASSERT_EQUAL_UINT32(6, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x05, stream.data()[0]);
    TEST_ASSERT_EQUAL_MEMORY("Hello", &stream.data()[1], 5);
}

void test_write_large_string()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    std::string large_str(200, 'A');
    TEST_ASSERT_TRUE(writer.write_string(large_str));
    TEST_ASSERT_TRUE(stream.size() > 200);
}

void test_write_empty_bytes()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_bytes(nullptr, 0));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x00, stream.data()[0]);
}

void test_write_normal_bytes()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    TEST_ASSERT_TRUE(writer.write_bytes(data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT32(5, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x04, stream.data()[0]);
    TEST_ASSERT_EQUAL_MEMORY(data, &stream.data()[1], 4);
}

void test_write_tag()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_tag(1, litepb::WIRE_TYPE_VARINT));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x08, stream.data()[0]);
}

void test_write_tag_large_field_number()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_tag(300, litepb::WIRE_TYPE_LENGTH_DELIMITED));
    TEST_ASSERT_TRUE(stream.size() > 1);
}

void test_write_sint32_positive()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sint32(150));
    TEST_ASSERT_TRUE(stream.size() > 0);
}

void test_write_sint32_negative()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sint32(-150));
    TEST_ASSERT_TRUE(stream.size() > 0);
}

void test_write_sint32_min_max()
{
    litepb::BufferOutputStream stream1;
    litepb::ProtoWriter writer1(stream1);
    TEST_ASSERT_TRUE(writer1.write_sint32(std::numeric_limits<int32_t>::min()));

    litepb::BufferOutputStream stream2;
    litepb::ProtoWriter writer2(stream2);
    TEST_ASSERT_TRUE(writer2.write_sint32(std::numeric_limits<int32_t>::max()));
}

void test_write_sint64_positive()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sint64(150000LL));
    TEST_ASSERT_TRUE(stream.size() > 0);
}

void test_write_sint64_negative()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_sint64(-150000LL));
    TEST_ASSERT_TRUE(stream.size() > 0);
}

void test_write_sint64_min_max()
{
    litepb::BufferOutputStream stream1;
    litepb::ProtoWriter writer1(stream1);
    TEST_ASSERT_TRUE(writer1.write_sint64(std::numeric_limits<int64_t>::min()));

    litepb::BufferOutputStream stream2;
    litepb::ProtoWriter writer2(stream2);
    TEST_ASSERT_TRUE(writer2.write_sint64(std::numeric_limits<int64_t>::max()));
}

void test_varint_size()
{
    TEST_ASSERT_EQUAL_UINT32(1, litepb::ProtoWriter::varint_size(0));
    TEST_ASSERT_EQUAL_UINT32(1, litepb::ProtoWriter::varint_size(127));
    TEST_ASSERT_EQUAL_UINT32(2, litepb::ProtoWriter::varint_size(128));
    TEST_ASSERT_EQUAL_UINT32(2, litepb::ProtoWriter::varint_size(255));
    TEST_ASSERT_EQUAL_UINT32(10, litepb::ProtoWriter::varint_size(0xFFFFFFFFFFFFFFFFULL));
}

void test_fixed_sizes()
{
    TEST_ASSERT_EQUAL_UINT32(4, litepb::ProtoWriter::fixed32_size());
    TEST_ASSERT_EQUAL_UINT32(8, litepb::ProtoWriter::fixed64_size());
}

void test_sint_sizes()
{
    TEST_ASSERT_EQUAL_UINT32(1, litepb::ProtoWriter::sint32_size(0));
    TEST_ASSERT_TRUE(litepb::ProtoWriter::sint32_size(150) > 0);
    TEST_ASSERT_TRUE(litepb::ProtoWriter::sint64_size(150000LL) > 0);
}

void test_write_bytes_null_data_pointer()
{
    litepb::BufferOutputStream stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_TRUE(writer.write_bytes(nullptr, 5));
    TEST_ASSERT_EQUAL_UINT32(1, stream.size());
    TEST_ASSERT_EQUAL_UINT8(0x05, stream.data()[0]);
}

void test_write_varint_stream_failure()
{
    litepb::FixedOutputStream<1> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_varint(300));
}

void test_write_fixed32_stream_failure()
{
    litepb::FixedOutputStream<2> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_fixed32(0x12345678));
}

void test_write_fixed64_stream_failure()
{
    litepb::FixedOutputStream<4> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_fixed64(0x123456789ABCDEF0ULL));
}

void test_write_string_stream_failure()
{
    litepb::FixedOutputStream<3> stream;
    litepb::ProtoWriter writer(stream);

    std::string test_str = "Hello";
    TEST_ASSERT_FALSE(writer.write_string(test_str));
}

void test_write_bytes_stream_failure()
{
    litepb::FixedOutputStream<3> stream;
    litepb::ProtoWriter writer(stream);

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    TEST_ASSERT_FALSE(writer.write_bytes(data, sizeof(data)));
}

void test_write_tag_stream_failure()
{
    litepb::FixedOutputStream<0> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_tag(1, litepb::WIRE_TYPE_VARINT));
}

void test_write_sint32_stream_failure()
{
    litepb::FixedOutputStream<1> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_sint32(-150));
}

void test_write_sint64_stream_failure()
{
    litepb::FixedOutputStream<1> stream;
    litepb::ProtoWriter writer(stream);

    TEST_ASSERT_FALSE(writer.write_sint64(-150000LL));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_write_varint_zero);
    RUN_TEST(test_write_varint_127);
    RUN_TEST(test_write_varint_128);
    RUN_TEST(test_write_varint_255);
    RUN_TEST(test_write_varint_large);
    RUN_TEST(test_write_fixed32);
    RUN_TEST(test_write_fixed32_zero);
    RUN_TEST(test_write_fixed64);
    RUN_TEST(test_write_fixed64_zero);
    RUN_TEST(test_write_sfixed32_positive);
    RUN_TEST(test_write_sfixed32_negative);
    RUN_TEST(test_write_sfixed64_positive);
    RUN_TEST(test_write_sfixed64_negative);
    RUN_TEST(test_write_float);
    RUN_TEST(test_write_double);
    RUN_TEST(test_write_empty_string);
    RUN_TEST(test_write_normal_string);
    RUN_TEST(test_write_large_string);
    RUN_TEST(test_write_empty_bytes);
    RUN_TEST(test_write_normal_bytes);
    RUN_TEST(test_write_tag);
    RUN_TEST(test_write_tag_large_field_number);
    RUN_TEST(test_write_sint32_positive);
    RUN_TEST(test_write_sint32_negative);
    RUN_TEST(test_write_sint32_min_max);
    RUN_TEST(test_write_sint64_positive);
    RUN_TEST(test_write_sint64_negative);
    RUN_TEST(test_write_sint64_min_max);
    RUN_TEST(test_varint_size);
    RUN_TEST(test_fixed_sizes);
    RUN_TEST(test_sint_sizes);
    RUN_TEST(test_write_bytes_null_data_pointer);
    RUN_TEST(test_write_varint_stream_failure);
    RUN_TEST(test_write_fixed32_stream_failure);
    RUN_TEST(test_write_fixed64_stream_failure);
    RUN_TEST(test_write_string_stream_failure);
    RUN_TEST(test_write_bytes_stream_failure);
    RUN_TEST(test_write_tag_stream_failure);
    RUN_TEST(test_write_sint32_stream_failure);
    RUN_TEST(test_write_sint64_stream_failure);
    return UNITY_END();
}
