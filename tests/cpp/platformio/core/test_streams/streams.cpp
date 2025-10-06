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

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_buffer_output_stream);
    RUN_TEST(test_buffer_input_stream);
    RUN_TEST(test_fixed_output_stream);
    return UNITY_END();
}
