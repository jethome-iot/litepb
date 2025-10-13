#include "litepb/litepb.h"
#include "protoc_helpers.h"
#include "test_vectors.pb.h"
#include "unity.h"
#ifdef HAVE_ABSL_LOG
#include <absl/log/initialize.h>
#endif
#include <google/protobuf/stubs/common.h>

// Commenting out constructor/destructor to debug segfault
// __attribute__((constructor)) void interop_init()
// {
// #ifdef HAVE_ABSL_LOG
//     absl::InitializeLog();
// #endif
//     GOOGLE_PROTOBUF_VERIFY_VERSION;
// }

// __attribute__((destructor)) void interop_cleanup()
// {
//     google::protobuf::ShutdownProtobufLibrary();
// }

void setUp() {}
void tearDown() {}

void test_basic_scalars()
{
    litepb::test::interop::BasicScalars litepb_msg;
    litepb_msg.int32_field  = 42;
    litepb_msg.string_field = "test";

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());
    TEST_ASSERT_GREATER_THAN(0, litepb_encoded.size());

    int32_t int32_f;
    std::string string_f;

    TEST_ASSERT_TRUE(protoc_decode_basic_scalars(litepb_encoded, int32_f, string_f));
    TEST_ASSERT_EQUAL_INT32(42, int32_f);
    TEST_ASSERT_EQUAL_STRING("test", string_f.c_str());

    auto protoc_encoded = protoc_encode_basic_scalars(100, "hello");
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::BasicScalars litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.int32_field);
    TEST_ASSERT_EQUAL_STRING("hello", litepb_decoded.string_field.c_str());

    auto protoc_bytes = protoc_encode_basic_scalars(42, "test");
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_all_scalars()
{
    litepb::test::interop::AllScalars litepb_msg;
    litepb_msg.v_int32  = -123;
    litepb_msg.v_int64  = -9876543210LL;
    litepb_msg.v_uint32 = 456;
    litepb_msg.v_uint64 = 12345678901ULL;
    litepb_msg.v_bool   = true;
    litepb_msg.v_string = "hello world";
    litepb_msg.v_bytes  = { 0x01, 0x02, 0x03, 0xFF };
    litepb_msg.v_float  = 3.14f;
    litepb_msg.v_double = 2.718281828;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    AllScalarsData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_all_scalars(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(-123, protoc_data.v_int32);
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_data.v_int64);
    TEST_ASSERT_EQUAL_UINT32(456, protoc_data.v_uint32);
    TEST_ASSERT_EQUAL_UINT64(12345678901ULL, protoc_data.v_uint64);
    TEST_ASSERT_TRUE(protoc_data.v_bool);
    TEST_ASSERT_EQUAL_STRING("hello world", protoc_data.v_string.c_str());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(litepb_msg.v_bytes.data(), protoc_data.v_bytes.data(), 4);
    TEST_ASSERT_EQUAL_FLOAT(3.14f, protoc_data.v_float);
    TEST_ASSERT_EQUAL_DOUBLE(2.718281828, protoc_data.v_double);

    AllScalarsData encode_data;
    encode_data.v_int32  = 789;
    encode_data.v_int64  = 1234567890123LL;
    encode_data.v_uint32 = 321;
    encode_data.v_uint64 = 9876543210ULL;
    encode_data.v_bool   = false;
    encode_data.v_string = "test";
    encode_data.v_bytes  = { 0xAA, 0xBB };
    encode_data.v_float  = 1.23f;
    encode_data.v_double = 4.56;

    auto protoc_encoded = protoc_encode_all_scalars(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::AllScalars litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(789, litepb_decoded.v_int32);
    TEST_ASSERT_EQUAL_INT64(1234567890123LL, litepb_decoded.v_int64);
    TEST_ASSERT_EQUAL_UINT32(321, litepb_decoded.v_uint32);
    TEST_ASSERT_EQUAL_UINT64(9876543210ULL, litepb_decoded.v_uint64);
    TEST_ASSERT_FALSE(litepb_decoded.v_bool);
    TEST_ASSERT_EQUAL_STRING("test", litepb_decoded.v_string.c_str());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(encode_data.v_bytes.data(), litepb_decoded.v_bytes.data(), 2);
    TEST_ASSERT_EQUAL_FLOAT(1.23f, litepb_decoded.v_float);
    TEST_ASSERT_EQUAL_DOUBLE(4.56, litepb_decoded.v_double);

    AllScalarsData binary_check_data;
    binary_check_data.v_int32  = -123;
    binary_check_data.v_int64  = -9876543210LL;
    binary_check_data.v_uint32 = 456;
    binary_check_data.v_uint64 = 12345678901ULL;
    binary_check_data.v_bool   = true;
    binary_check_data.v_string = "hello world";
    binary_check_data.v_bytes  = { 0x01, 0x02, 0x03, 0xFF };
    binary_check_data.v_float  = 3.14f;
    binary_check_data.v_double = 2.718281828;

    auto protoc_bytes = protoc_encode_all_scalars(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_signed_ints()
{
    litepb::test::interop::SignedInts litepb_msg;
    litepb_msg.v_sint32 = -42;
    litepb_msg.v_sint64 = -9876543210LL;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    SignedIntsData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_signed_ints(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(-42, protoc_data.v_sint32);
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_data.v_sint64);

    SignedIntsData encode_data;
    encode_data.v_sint32 = 123;
    encode_data.v_sint64 = 1234567890123LL;

    auto protoc_encoded = protoc_encode_signed_ints(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::SignedInts litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(123, litepb_decoded.v_sint32);
    TEST_ASSERT_EQUAL_INT64(1234567890123LL, litepb_decoded.v_sint64);
}

void test_fixed_types()
{
    litepb::test::interop::FixedTypes litepb_msg;
    litepb_msg.v_fixed32  = 0x12345678;
    litepb_msg.v_fixed64  = 0x123456789ABCDEF0ULL;
    litepb_msg.v_sfixed32 = -123456;
    litepb_msg.v_sfixed64 = -9876543210LL;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    FixedTypesData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_fixed_types(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_UINT32(0x12345678, protoc_data.v_fixed32);
    TEST_ASSERT_EQUAL_UINT64(0x123456789ABCDEF0ULL, protoc_data.v_fixed64);
    TEST_ASSERT_EQUAL_INT32(-123456, protoc_data.v_sfixed32);
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_data.v_sfixed64);

    FixedTypesData encode_data;
    encode_data.v_fixed32  = 0xABCDEF00;
    encode_data.v_fixed64  = 0xFEDCBA9876543210ULL;
    encode_data.v_sfixed32 = 654321;
    encode_data.v_sfixed64 = 1234567890LL;

    auto protoc_encoded = protoc_encode_fixed_types(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::FixedTypes litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_UINT32(0xABCDEF00, litepb_decoded.v_fixed32);
    TEST_ASSERT_EQUAL_UINT64(0xFEDCBA9876543210ULL, litepb_decoded.v_fixed64);
    TEST_ASSERT_EQUAL_INT32(654321, litepb_decoded.v_sfixed32);
    TEST_ASSERT_EQUAL_INT64(1234567890LL, litepb_decoded.v_sfixed64);

    FixedTypesData binary_check_data;
    binary_check_data.v_fixed32  = 0x12345678;
    binary_check_data.v_fixed64  = 0x123456789ABCDEF0ULL;
    binary_check_data.v_sfixed32 = -123456;
    binary_check_data.v_sfixed64 = -9876543210LL;

    auto protoc_bytes = protoc_encode_fixed_types(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_repeated_fields()
{
    litepb::test::interop::RepeatedFields litepb_msg;
    litepb_msg.packed_ints = { 1, 2, 3, 4, 5 };
    litepb_msg.strings     = { "hello", "world", "test" };

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    RepeatedFieldsData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_repeated_fields(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_size_t(5, protoc_data.packed_ints.size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.packed_ints[0]);
    TEST_ASSERT_EQUAL_INT32(5, protoc_data.packed_ints[4]);
    TEST_ASSERT_EQUAL_size_t(3, protoc_data.strings.size());
    TEST_ASSERT_EQUAL_STRING("hello", protoc_data.strings[0].c_str());
    TEST_ASSERT_EQUAL_STRING("test", protoc_data.strings[2].c_str());

    RepeatedFieldsData encode_data;
    encode_data.packed_ints = { 10, 20, 30 };
    encode_data.strings     = { "foo", "bar" };

    auto protoc_encoded = protoc_encode_repeated_fields(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::RepeatedFields litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(3, litepb_decoded.packed_ints.size());
    TEST_ASSERT_EQUAL_INT32(10, litepb_decoded.packed_ints[0]);
    TEST_ASSERT_EQUAL_INT32(30, litepb_decoded.packed_ints[2]);
    TEST_ASSERT_EQUAL_size_t(2, litepb_decoded.strings.size());
    TEST_ASSERT_EQUAL_STRING("foo", litepb_decoded.strings[0].c_str());
    TEST_ASSERT_EQUAL_STRING("bar", litepb_decoded.strings[1].c_str());
}

void test_map_field()
{
    litepb::test::interop::MapField litepb_msg;
    litepb_msg.items["key1"] = 100;
    litepb_msg.items["key2"] = 200;
    litepb_msg.items["key3"] = 300;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    MapFieldData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_map_field(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_size_t(3, protoc_data.items.size());
    TEST_ASSERT_EQUAL_INT32(100, protoc_data.items["key1"]);
    TEST_ASSERT_EQUAL_INT32(200, protoc_data.items["key2"]);
    TEST_ASSERT_EQUAL_INT32(300, protoc_data.items["key3"]);

    MapFieldData encode_data;
    encode_data.items["foo"] = 42;
    encode_data.items["bar"] = 84;

    auto protoc_encoded = protoc_encode_map_field(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::MapField litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(2, litepb_decoded.items.size());
    TEST_ASSERT_EQUAL_INT32(42, litepb_decoded.items["foo"]);
    TEST_ASSERT_EQUAL_INT32(84, litepb_decoded.items["bar"]);

    // Note: We do NOT perform byte-for-byte comparison for map fields.
    // According to the official Protocol Buffers specification (protobuf.dev):
    // "Wire format ordering and map iteration ordering of map values is undefined,
    // so you cannot rely on your map items being in a particular order."
    // Source: https://protobuf.dev/programming-guides/proto3/#maps
    // The bidirectional encode/decode tests above verify full semantic interoperability,
    // which is the correct standard for map field compatibility.
}

void test_outer_nested()
{
    litepb::test::interop::Outer litepb_msg;
    litepb_msg.inner.value = 42;
    litepb_msg.name        = "outer_name";

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    OuterData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_outer(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(42, protoc_data.inner_value);
    TEST_ASSERT_EQUAL_STRING("outer_name", protoc_data.name.c_str());

    OuterData encode_data;
    encode_data.inner_value = 123;
    encode_data.name        = "test_name";

    auto protoc_encoded = protoc_encode_outer(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::Outer litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(123, litepb_decoded.inner.value);
    TEST_ASSERT_EQUAL_STRING("test_name", litepb_decoded.name.c_str());

    OuterData binary_check_data;
    binary_check_data.inner_value = 42;
    binary_check_data.name        = "outer_name";

    auto protoc_bytes = protoc_encode_outer(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_oneof_int()
{
    litepb::test::interop::OneofTest litepb_msg;
    litepb_msg.choice = int32_t(42);

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    OneofTestData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_oneof_test(litepb_encoded, protoc_data));
    TEST_ASSERT_TRUE(protoc_data.is_int_value);
    TEST_ASSERT_EQUAL_INT32(42, protoc_data.int_value);

    OneofTestData encode_data;
    encode_data.is_int_value = true;
    encode_data.int_value    = 100;

    auto protoc_encoded = protoc_encode_oneof_test(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::OneofTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_TRUE(std::holds_alternative<int32_t>(litepb_decoded.choice));
    TEST_ASSERT_EQUAL_INT32(100, std::get<int32_t>(litepb_decoded.choice));

    OneofTestData binary_check_data;
    binary_check_data.is_int_value = true;
    binary_check_data.int_value    = 42;

    auto protoc_bytes = protoc_encode_oneof_test(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_oneof_string()
{
    litepb::test::interop::OneofTest litepb_msg;
    litepb_msg.choice = std::string("test_string");

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    OneofTestData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_oneof_test(litepb_encoded, protoc_data));
    TEST_ASSERT_FALSE(protoc_data.is_int_value);
    TEST_ASSERT_EQUAL_STRING("test_string", protoc_data.string_value.c_str());

    OneofTestData encode_data;
    encode_data.is_int_value = false;
    encode_data.string_value = "hello";

    auto protoc_encoded = protoc_encode_oneof_test(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::OneofTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(litepb_decoded.choice));
    TEST_ASSERT_EQUAL_STRING("hello", std::get<std::string>(litepb_decoded.choice).c_str());

    OneofTestData binary_check_data;
    binary_check_data.is_int_value = false;
    binary_check_data.string_value = "test_string";

    auto protoc_bytes = protoc_encode_oneof_test(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_enum()
{
    litepb::test::interop::EnumTest litepb_msg;
    litepb_msg.value = litepb::test::interop::TestEnum::FIRST;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    EnumTestData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_enum_test(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.value);

    EnumTestData encode_data;
    encode_data.value = 2;

    auto protoc_encoded = protoc_encode_enum_test(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb::test::interop::EnumTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(litepb::test::interop::TestEnum::SECOND, litepb_decoded.value);

    EnumTestData binary_check_data;
    binary_check_data.value = 1;

    auto protoc_bytes = protoc_encode_enum_test(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}
