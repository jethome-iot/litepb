#include "loopback_transport.h"
#include "unity.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_loopback_send_recv_basic()
{
    LoopbackTransport transport1;
    LoopbackTransport transport2;

    transport1.connect_to_peer(&transport2);
    transport2.connect_to_peer(&transport1);

    const uint8_t send_data[] = { 0x01, 0x02, 0x03, 0x04 };
    bool send_result          = transport1.send(send_data, 4);

    TEST_ASSERT_TRUE(send_result);
    TEST_ASSERT_TRUE(transport2.available());

    uint8_t recv_buffer[10];
    size_t recv_len = transport2.recv(recv_buffer, 10);

    TEST_ASSERT_EQUAL_UINT32(4, recv_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(send_data, recv_buffer, 4);
}

void test_loopback_available_transitions()
{
    LoopbackTransport transport1;
    LoopbackTransport transport2;

    transport1.connect_to_peer(&transport2);

    TEST_ASSERT_FALSE(transport2.available());

    const uint8_t data[] = { 0xAA };
    transport1.send(data, 1);

    TEST_ASSERT_TRUE(transport2.available());

    uint8_t buffer[1];
    transport2.recv(buffer, 1);

    TEST_ASSERT_FALSE(transport2.available());
}

void test_loopback_send_fails_without_peer()
{
    LoopbackTransport transport;

    const uint8_t data[] = { 0x01, 0x02 };
    bool result          = transport.send(data, 2);

    TEST_ASSERT_FALSE(result);
}

void test_loopback_zero_length_send()
{
    LoopbackTransport transport1;
    LoopbackTransport transport2;

    transport1.connect_to_peer(&transport2);

    const uint8_t data[] = { 0x01 };
    bool result          = transport1.send(data, 0);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transport2.available());
}

void test_loopback_empty_queue_recv()
{
    LoopbackTransport transport;

    uint8_t buffer[10];
    size_t recv_len = transport.recv(buffer, 10);

    TEST_ASSERT_EQUAL_UINT32(0, recv_len);
}

void test_loopback_cross_peer_propagation()
{
    LoopbackTransport transport1;
    LoopbackTransport transport2;

    transport1.connect_to_peer(&transport2);
    transport2.connect_to_peer(&transport1);

    const uint8_t data1[] = { 0x11, 0x22 };
    const uint8_t data2[] = { 0x33, 0x44 };

    transport1.send(data1, 2);
    transport2.send(data2, 2);

    TEST_ASSERT_TRUE(transport2.available());
    TEST_ASSERT_TRUE(transport1.available());

    uint8_t buffer1[2];
    uint8_t buffer2[2];

    size_t len1 = transport2.recv(buffer1, 2);
    size_t len2 = transport1.recv(buffer2, 2);

    TEST_ASSERT_EQUAL_UINT32(2, len1);
    TEST_ASSERT_EQUAL_UINT32(2, len2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data1, buffer1, 2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data2, buffer2, 2);
}

void test_loopback_partial_recv()
{
    LoopbackTransport transport1;
    LoopbackTransport transport2;

    transport1.connect_to_peer(&transport2);

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    transport1.send(data, 5);

    uint8_t buffer[2];
    size_t len1 = transport2.recv(buffer, 2);

    TEST_ASSERT_EQUAL_UINT32(2, len1);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);

    TEST_ASSERT_TRUE(transport2.available());

    size_t len2 = transport2.recv(buffer, 2);

    TEST_ASSERT_EQUAL_UINT32(2, len2);
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[1]);

    TEST_ASSERT_TRUE(transport2.available());

    size_t len3 = transport2.recv(buffer, 2);

    TEST_ASSERT_EQUAL_UINT32(1, len3);
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[0]);

    TEST_ASSERT_FALSE(transport2.available());
}

int runTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_loopback_send_recv_basic);
    RUN_TEST(test_loopback_available_transitions);
    RUN_TEST(test_loopback_send_fails_without_peer);
    RUN_TEST(test_loopback_zero_length_send);
    RUN_TEST(test_loopback_empty_queue_recv);
    RUN_TEST(test_loopback_cross_peer_propagation);
    RUN_TEST(test_loopback_partial_recv);
    return UNITY_END();
}
