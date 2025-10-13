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
extern void test_optional_fields();
extern void test_empty_message();
extern void test_unpacked_repeated();
extern void test_different_maps();
extern void test_repeated_messages();
extern void test_nested_enum_test();
extern void test_large_field_numbers();
extern void test_unknown_fields();

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
    RUN_TEST(test_optional_fields);
    RUN_TEST(test_empty_message);
    RUN_TEST(test_unpacked_repeated);
    RUN_TEST(test_different_maps);
    RUN_TEST(test_repeated_messages);
    RUN_TEST(test_nested_enum_test);
    RUN_TEST(test_large_field_numbers);
    RUN_TEST(test_unknown_fields);

    return UNITY_END();
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;
    return runTests();
}
