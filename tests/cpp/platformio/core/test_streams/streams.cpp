#include "litepb/core/streams.h"
#include "unity.h"

void setUp() {}
void tearDown() {}

void test_buffer_output_stream()
{
    litepb::BufferOutputStream stream;

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    TEST_ASSERT_TRUE(stream.write(data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT32(4, stream.position());
    TEST_ASSERT_EQUAL_UINT32(4, stream.size());

    const uint8_t* output = stream.data();
    for (size_t i = 0; i < sizeof(data); ++i) {
        TEST_ASSERT_EQUAL_UINT8(data[i], output[i]);
    }
}

void test_buffer_input_stream()
{
    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    litepb::BufferInputStream stream(data, sizeof(data));

    TEST_ASSERT_EQUAL_UINT32(4, stream.available());
    TEST_ASSERT_EQUAL_UINT32(0, stream.position());

    uint8_t buffer[2];
    TEST_ASSERT_TRUE(stream.read(buffer, 2));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);
    TEST_ASSERT_EQUAL_UINT32(2, stream.position());
    TEST_ASSERT_EQUAL_UINT32(2, stream.available());

    TEST_ASSERT_TRUE(stream.skip(1));
    TEST_ASSERT_EQUAL_UINT32(3, stream.position());
    TEST_ASSERT_EQUAL_UINT32(1, stream.available());
}

void test_fixed_output_stream()
{
    litepb::FixedOutputStream<16> stream;

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    TEST_ASSERT_TRUE(stream.write(data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT32(4, stream.position());
    TEST_ASSERT_EQUAL_UINT32(16, stream.capacity());

    const uint8_t large_data[20] = { 0 };
    TEST_ASSERT_FALSE(stream.write(large_data, sizeof(large_data)));
}

void test_fixed_output_stream_data_integrity()
{
    litepb::FixedOutputStream<32> stream;

    const uint8_t data1[] = { 0xAA, 0xBB, 0xCC, 0xDD };
    const uint8_t data2[] = { 0x11, 0x22, 0x33 };
    
    TEST_ASSERT_TRUE(stream.write(data1, sizeof(data1)));
    TEST_ASSERT_TRUE(stream.write(data2, sizeof(data2)));
    TEST_ASSERT_EQUAL_UINT32(7, stream.position());

    const uint8_t* output = stream.data();
    TEST_ASSERT_EQUAL_UINT8(0xAA, output[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, output[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, output[2]);
    TEST_ASSERT_EQUAL_UINT8(0xDD, output[3]);
    TEST_ASSERT_EQUAL_UINT8(0x11, output[4]);
    TEST_ASSERT_EQUAL_UINT8(0x22, output[5]);
    TEST_ASSERT_EQUAL_UINT8(0x33, output[6]);
}

void test_fixed_output_stream_clear()
{
    litepb::FixedOutputStream<16> stream;

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    TEST_ASSERT_TRUE(stream.write(data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT32(4, stream.position());

    stream.clear();
    TEST_ASSERT_EQUAL_UINT32(0, stream.position());
    TEST_ASSERT_EQUAL_UINT32(16, stream.capacity());

    const uint8_t new_data[] = { 0xFF, 0xFE };
    TEST_ASSERT_TRUE(stream.write(new_data, sizeof(new_data)));
    TEST_ASSERT_EQUAL_UINT32(2, stream.position());
    
    const uint8_t* output = stream.data();
    TEST_ASSERT_EQUAL_UINT8(0xFF, output[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFE, output[1]);
}

void test_fixed_input_stream_basic()
{
    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    litepb::FixedInputStream<16> stream(data, sizeof(data));

    TEST_ASSERT_EQUAL_UINT32(5, stream.available());
    TEST_ASSERT_EQUAL_UINT32(0, stream.position());

    uint8_t buffer[3];
    TEST_ASSERT_TRUE(stream.read(buffer, 3));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[2]);
    TEST_ASSERT_EQUAL_UINT32(3, stream.position());
    TEST_ASSERT_EQUAL_UINT32(2, stream.available());
}

void test_fixed_input_stream_skip()
{
    const uint8_t data[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    litepb::FixedInputStream<16> stream(data, sizeof(data));

    TEST_ASSERT_TRUE(stream.skip(2));
    TEST_ASSERT_EQUAL_UINT32(2, stream.position());
    TEST_ASSERT_EQUAL_UINT32(4, stream.available());

    uint8_t buffer[2];
    TEST_ASSERT_TRUE(stream.read(buffer, 2));
    TEST_ASSERT_EQUAL_UINT8(0xCC, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xDD, buffer[1]);
    TEST_ASSERT_EQUAL_UINT32(4, stream.position());

    TEST_ASSERT_TRUE(stream.skip(2));
    TEST_ASSERT_EQUAL_UINT32(6, stream.position());
    TEST_ASSERT_EQUAL_UINT32(0, stream.available());

    TEST_ASSERT_FALSE(stream.skip(1));
}

void test_fixed_input_stream_truncation()
{
    const uint8_t large_data[32] = { 
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    
    litepb::FixedInputStream<16> stream(large_data, sizeof(large_data));

    TEST_ASSERT_EQUAL_UINT32(16, stream.available());

    uint8_t buffer[16];
    TEST_ASSERT_TRUE(stream.read(buffer, 16));
    
    for (size_t i = 0; i < 16; ++i) {
        TEST_ASSERT_EQUAL_UINT8(i, buffer[i]);
    }

    TEST_ASSERT_EQUAL_UINT32(0, stream.available());
    TEST_ASSERT_FALSE(stream.read(buffer, 1));
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_buffer_output_stream);
    RUN_TEST(test_buffer_input_stream);
    RUN_TEST(test_fixed_output_stream);
    RUN_TEST(test_fixed_output_stream_data_integrity);
    RUN_TEST(test_fixed_output_stream_clear);
    RUN_TEST(test_fixed_input_stream_basic);
    RUN_TEST(test_fixed_input_stream_skip);
    RUN_TEST(test_fixed_input_stream_truncation);
    return UNITY_END();
}
