#include "unity.h"

extern void test_basic_scalars();
extern void test_all_scalars();
extern void test_signed_ints();
extern void test_fixed_types();
extern void test_repeated_fields();
extern void test_map_field();
extern void test_outer_nested();
extern void test_oneof_int();
extern void test_oneof_string();
extern void test_enum();

int runTests()
{
    UNITY_BEGIN();

    RUN_TEST(test_basic_scalars);
    RUN_TEST(test_all_scalars);
    RUN_TEST(test_signed_ints);
    RUN_TEST(test_fixed_types);
    RUN_TEST(test_repeated_fields);
    RUN_TEST(test_map_field);
    RUN_TEST(test_outer_nested);
    RUN_TEST(test_oneof_int);
    RUN_TEST(test_oneof_string);
    RUN_TEST(test_enum);

    return UNITY_END();
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;
    return runTests();
}
