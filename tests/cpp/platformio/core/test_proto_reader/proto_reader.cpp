#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include "unity.h"
#include <cstring>
#include <limits>

void setUp() {}
void tearDown() {}

void test_read_varint_zero()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(0);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(0, value);
}

void test_read_varint_127()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(127);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(127, value);
}

void test_read_varint_128()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(128);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(128, value);
}

void test_read_varint_255()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(255);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(255, value);
}

void test_read_varint_large()
{
    uint64_t large_value = 0xFFFFFFFFFFFFFFFFULL;
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(large_value);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(large_value, value);
}

void test_read_varint_overflow()
{
    const uint8_t invalid_data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03 };
    litepb::BufferInputStream in_stream(invalid_data, sizeof(invalid_data));
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_varint(value));
}

void test_read_varint_truncated()
{
    const uint8_t truncated_data[] = { 0xFF };
    litepb::BufferInputStream in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_varint(value));
}

void test_read_fixed32()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed32(0x12345678);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint32_t value;
    TEST_ASSERT_TRUE(reader.read_fixed32(value));
    TEST_ASSERT_EQUAL_UINT32(0x12345678, value);
}

void test_read_fixed32_zero()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed32(0);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint32_t value;
    TEST_ASSERT_TRUE(reader.read_fixed32(value));
    TEST_ASSERT_EQUAL_UINT32(0, value);
}

void test_read_fixed32_truncated()
{
    const uint8_t truncated_data[] = { 0x01, 0x02 };
    litepb::BufferInputStream in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    uint32_t value;
    TEST_ASSERT_FALSE(reader.read_fixed32(value));
}

void test_read_fixed64()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed64(0x123456789ABCDEF0ULL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_fixed64(value));
    TEST_ASSERT_EQUAL_UINT64(0x123456789ABCDEF0ULL, value);
}

void test_read_fixed64_zero()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed64(0);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_fixed64(value));
    TEST_ASSERT_EQUAL_UINT64(0, value);
}

void test_read_fixed64_truncated()
{
    const uint8_t truncated_data[] = { 0x01, 0x02, 0x03 };
    litepb::BufferInputStream in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_fixed64(value));
}

void test_read_sfixed32_positive()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sfixed32(12345);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int32_t value;
    TEST_ASSERT_TRUE(reader.read_sfixed32(value));
    TEST_ASSERT_EQUAL_INT32(12345, value);
}

void test_read_sfixed32_negative()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sfixed32(-12345);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int32_t value;
    TEST_ASSERT_TRUE(reader.read_sfixed32(value));
    TEST_ASSERT_EQUAL_INT32(-12345, value);
}

void test_read_sfixed64_positive()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sfixed64(123456789LL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int64_t value;
    TEST_ASSERT_TRUE(reader.read_sfixed64(value));
    TEST_ASSERT_EQUAL_INT64(123456789LL, value);
}

void test_read_sfixed64_negative()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sfixed64(-123456789LL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int64_t value;
    TEST_ASSERT_TRUE(reader.read_sfixed64(value));
    TEST_ASSERT_EQUAL_INT64(-123456789LL, value);
}

void test_read_float()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_float(3.14159f);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    float value;
    TEST_ASSERT_TRUE(reader.read_float(value));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.14159f, value);
}

void test_read_double()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_double(3.14159265358979);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    double value;
    TEST_ASSERT_TRUE(reader.read_double(value));
    TEST_ASSERT_DOUBLE_WITHIN(0.0000001, 3.14159265358979, value);
}

void test_read_empty_string()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_string("");

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    std::string value;
    TEST_ASSERT_TRUE(reader.read_string(value));
    TEST_ASSERT_EQUAL_STRING("", value.c_str());
}

void test_read_normal_string()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_string("Hello World");

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    std::string value;
    TEST_ASSERT_TRUE(reader.read_string(value));
    TEST_ASSERT_EQUAL_STRING("Hello World", value.c_str());
}

void test_read_string_truncated()
{
    const uint8_t truncated_data[] = { 0x05, 'H', 'i' };
    litepb::BufferInputStream in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    std::string value;
    TEST_ASSERT_FALSE(reader.read_string(value));
}

void test_read_empty_bytes()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_bytes(nullptr, 0);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    std::vector<uint8_t> value;
    TEST_ASSERT_TRUE(reader.read_bytes(value));
    TEST_ASSERT_EQUAL_UINT32(0, value.size());
}

void test_read_normal_bytes()
{
    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_bytes(data, sizeof(data));

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    std::vector<uint8_t> value;
    TEST_ASSERT_TRUE(reader.read_bytes(value));
    TEST_ASSERT_EQUAL_UINT32(4, value.size());
    TEST_ASSERT_EQUAL_MEMORY(data, value.data(), 4);
}

void test_read_bytes_truncated()
{
    const uint8_t truncated_data[] = { 0x05, 0x01, 0x02 };
    litepb::BufferInputStream in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    std::vector<uint8_t> value;
    TEST_ASSERT_FALSE(reader.read_bytes(value));
}

void test_read_tag()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_tag(1, litepb::WIRE_TYPE_VARINT);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint32_t field_number;
    litepb::WireType type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, type));
    TEST_ASSERT_EQUAL_UINT32(1, field_number);
    TEST_ASSERT_EQUAL(litepb::WIRE_TYPE_VARINT, type);
}

void test_read_tag_large_field_number()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_tag(300, litepb::WIRE_TYPE_LENGTH_DELIMITED);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint32_t field_number;
    litepb::WireType type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, type));
    TEST_ASSERT_EQUAL_UINT32(300, field_number);
    TEST_ASSERT_EQUAL(litepb::WIRE_TYPE_LENGTH_DELIMITED, type);
}

void test_read_sint32_positive()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint32(150);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int32_t value;
    TEST_ASSERT_TRUE(reader.read_sint32(value));
    TEST_ASSERT_EQUAL_INT32(150, value);
}

void test_read_sint32_negative()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint32(-150);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int32_t value;
    TEST_ASSERT_TRUE(reader.read_sint32(value));
    TEST_ASSERT_EQUAL_INT32(-150, value);
}

void test_read_sint32_min_max()
{
    litepb::BufferOutputStream out_stream1;
    litepb::ProtoWriter writer1(out_stream1);
    writer1.write_sint32(std::numeric_limits<int32_t>::min());

    litepb::BufferInputStream in_stream1(out_stream1.data(), out_stream1.size());
    litepb::ProtoReader reader1(in_stream1);
    int32_t value1;
    TEST_ASSERT_TRUE(reader1.read_sint32(value1));
    TEST_ASSERT_EQUAL_INT32(std::numeric_limits<int32_t>::min(), value1);

    litepb::BufferOutputStream out_stream2;
    litepb::ProtoWriter writer2(out_stream2);
    writer2.write_sint32(std::numeric_limits<int32_t>::max());

    litepb::BufferInputStream in_stream2(out_stream2.data(), out_stream2.size());
    litepb::ProtoReader reader2(in_stream2);
    int32_t value2;
    TEST_ASSERT_TRUE(reader2.read_sint32(value2));
    TEST_ASSERT_EQUAL_INT32(std::numeric_limits<int32_t>::max(), value2);
}

void test_read_sint64_positive()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint64(150000LL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int64_t value;
    TEST_ASSERT_TRUE(reader.read_sint64(value));
    TEST_ASSERT_EQUAL_INT64(150000LL, value);
}

void test_read_sint64_negative()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint64(-150000LL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int64_t value;
    TEST_ASSERT_TRUE(reader.read_sint64(value));
    TEST_ASSERT_EQUAL_INT64(-150000LL, value);
}

void test_read_sint64_min_max()
{
    litepb::BufferOutputStream out_stream1;
    litepb::ProtoWriter writer1(out_stream1);
    writer1.write_sint64(std::numeric_limits<int64_t>::min());

    litepb::BufferInputStream in_stream1(out_stream1.data(), out_stream1.size());
    litepb::ProtoReader reader1(in_stream1);
    int64_t value1;
    TEST_ASSERT_TRUE(reader1.read_sint64(value1));
    TEST_ASSERT_EQUAL_INT64(std::numeric_limits<int64_t>::min(), value1);

    litepb::BufferOutputStream out_stream2;
    litepb::ProtoWriter writer2(out_stream2);
    writer2.write_sint64(std::numeric_limits<int64_t>::max());

    litepb::BufferInputStream in_stream2(out_stream2.data(), out_stream2.size());
    litepb::ProtoReader reader2(in_stream2);
    int64_t value2;
    TEST_ASSERT_TRUE(reader2.read_sint64(value2));
    TEST_ASSERT_EQUAL_INT64(std::numeric_limits<int64_t>::max(), value2);
}

void test_skip_field_varint()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(12345);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_TRUE(reader.skip_field(litepb::WIRE_TYPE_VARINT));
    TEST_ASSERT_EQUAL_UINT32(out_stream.size(), reader.position());
}

void test_skip_field_fixed32()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed32(0x12345678);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_TRUE(reader.skip_field(litepb::WIRE_TYPE_FIXED32));
    TEST_ASSERT_EQUAL_UINT32(4, reader.position());
}

void test_skip_field_fixed64()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_fixed64(0x123456789ABCDEF0ULL);

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_TRUE(reader.skip_field(litepb::WIRE_TYPE_FIXED64));
    TEST_ASSERT_EQUAL_UINT32(8, reader.position());
}

void test_skip_field_length_delimited()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_string("Hello World");

    litepb::BufferInputStream in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_TRUE(reader.skip_field(litepb::WIRE_TYPE_LENGTH_DELIMITED));
    TEST_ASSERT_EQUAL_UINT32(out_stream.size(), reader.position());
}

void test_skip_field_start_group()
{
    const uint8_t dummy_data[] = { 0x00 };
    litepb::BufferInputStream in_stream(dummy_data, sizeof(dummy_data));
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_FALSE(reader.skip_field(litepb::WIRE_TYPE_START_GROUP));
}

void test_skip_field_end_group()
{
    const uint8_t dummy_data[] = { 0x00 };
    litepb::BufferInputStream in_stream(dummy_data, sizeof(dummy_data));
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_FALSE(reader.skip_field(litepb::WIRE_TYPE_END_GROUP));
}

void test_read_varint_10byte_overflow()
{
    const uint8_t invalid_data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02 };
    litepb::BufferInputStream in_stream(invalid_data, sizeof(invalid_data));
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_varint(value));
}

void test_read_varint_stream_failure()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_varint(300);

    litepb::FixedInputStream<1> in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_varint(value));
}

void test_read_fixed32_stream_failure()
{
    litepb::FixedInputStream<2> in_stream((const uint8_t*) "\x01\x02", 2);
    litepb::ProtoReader reader(in_stream);
    uint32_t value;
    TEST_ASSERT_FALSE(reader.read_fixed32(value));
}

void test_read_fixed64_stream_failure()
{
    litepb::FixedInputStream<4> in_stream((const uint8_t*) "\x01\x02\x03\x04", 4);
    litepb::ProtoReader reader(in_stream);
    uint64_t value;
    TEST_ASSERT_FALSE(reader.read_fixed64(value));
}

void test_read_string_stream_failure()
{
    const uint8_t truncated_data[] = { 0x05, 'H', 'e' };
    litepb::FixedInputStream<3> in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    std::string value;
    TEST_ASSERT_FALSE(reader.read_string(value));
}

void test_read_bytes_stream_failure()
{
    const uint8_t truncated_data[] = { 0x05, 0x01, 0x02 };
    litepb::FixedInputStream<3> in_stream(truncated_data, sizeof(truncated_data));
    litepb::ProtoReader reader(in_stream);
    std::vector<uint8_t> value;
    TEST_ASSERT_FALSE(reader.read_bytes(value));
}

void test_read_sint32_stream_failure()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint32(-150);

    litepb::FixedInputStream<1> in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int32_t value;
    TEST_ASSERT_FALSE(reader.read_sint32(value));
}

void test_read_sint64_stream_failure()
{
    litepb::BufferOutputStream out_stream;
    litepb::ProtoWriter writer(out_stream);
    writer.write_sint64(-150000LL);

    litepb::FixedInputStream<1> in_stream(out_stream.data(), out_stream.size());
    litepb::ProtoReader reader(in_stream);
    int64_t value;
    TEST_ASSERT_FALSE(reader.read_sint64(value));
}

void test_skip_field_unknown_wire_type()
{
    const uint8_t dummy_data[] = { 0x00 };
    litepb::BufferInputStream in_stream(dummy_data, sizeof(dummy_data));
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_FALSE(reader.skip_field(static_cast<litepb::WireType>(99)));
}

void test_skip_field_length_delimited_size_read_failure()
{
    litepb::FixedInputStream<0> in_stream(nullptr, 0);
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_FALSE(reader.skip_field(litepb::WIRE_TYPE_LENGTH_DELIMITED));
}

void test_skip_field_length_delimited_skip_failure()
{
    const uint8_t data[] = { 0x05 };
    litepb::BufferInputStream in_stream(data, sizeof(data));
    litepb::ProtoReader reader(in_stream);
    TEST_ASSERT_FALSE(reader.skip_field(litepb::WIRE_TYPE_LENGTH_DELIMITED));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_read_varint_zero);
    RUN_TEST(test_read_varint_127);
    RUN_TEST(test_read_varint_128);
    RUN_TEST(test_read_varint_255);
    RUN_TEST(test_read_varint_large);
    RUN_TEST(test_read_varint_overflow);
    RUN_TEST(test_read_varint_truncated);
    RUN_TEST(test_read_fixed32);
    RUN_TEST(test_read_fixed32_zero);
    RUN_TEST(test_read_fixed32_truncated);
    RUN_TEST(test_read_fixed64);
    RUN_TEST(test_read_fixed64_zero);
    RUN_TEST(test_read_fixed64_truncated);
    RUN_TEST(test_read_sfixed32_positive);
    RUN_TEST(test_read_sfixed32_negative);
    RUN_TEST(test_read_sfixed64_positive);
    RUN_TEST(test_read_sfixed64_negative);
    RUN_TEST(test_read_float);
    RUN_TEST(test_read_double);
    RUN_TEST(test_read_empty_string);
    RUN_TEST(test_read_normal_string);
    RUN_TEST(test_read_string_truncated);
    RUN_TEST(test_read_empty_bytes);
    RUN_TEST(test_read_normal_bytes);
    RUN_TEST(test_read_bytes_truncated);
    RUN_TEST(test_read_tag);
    RUN_TEST(test_read_tag_large_field_number);
    RUN_TEST(test_read_sint32_positive);
    RUN_TEST(test_read_sint32_negative);
    RUN_TEST(test_read_sint32_min_max);
    RUN_TEST(test_read_sint64_positive);
    RUN_TEST(test_read_sint64_negative);
    RUN_TEST(test_read_sint64_min_max);
    RUN_TEST(test_skip_field_varint);
    RUN_TEST(test_skip_field_fixed32);
    RUN_TEST(test_skip_field_fixed64);
    RUN_TEST(test_skip_field_length_delimited);
    RUN_TEST(test_skip_field_start_group);
    RUN_TEST(test_skip_field_end_group);
    RUN_TEST(test_read_varint_10byte_overflow);
    RUN_TEST(test_read_varint_stream_failure);
    RUN_TEST(test_read_fixed32_stream_failure);
    RUN_TEST(test_read_fixed64_stream_failure);
    RUN_TEST(test_read_string_stream_failure);
    RUN_TEST(test_read_bytes_stream_failure);
    RUN_TEST(test_read_sint32_stream_failure);
    RUN_TEST(test_read_sint64_stream_failure);
    RUN_TEST(test_skip_field_unknown_wire_type);
    RUN_TEST(test_skip_field_length_delimited_size_read_failure);
    RUN_TEST(test_skip_field_length_delimited_skip_failure);
    return UNITY_END();
}
