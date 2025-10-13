#include <iostream>
#include "unity.h"

void setUp() {}
void tearDown() {}

void test_simple() {
    TEST_ASSERT_EQUAL(1, 1);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_simple);
    return UNITY_END();
}