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
    litepb_gen::litepb::test::interop::BasicScalars litepb_msg;
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
    litepb_gen::litepb::test::interop::BasicScalars litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.int32_field);
    TEST_ASSERT_EQUAL_STRING("hello", litepb_decoded.string_field.c_str());

    auto protoc_bytes = protoc_encode_basic_scalars(42, "test");
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_all_scalars()
{
    litepb_gen::litepb::test::interop::AllScalars litepb_msg;
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
    litepb_gen::litepb::test::interop::AllScalars litepb_decoded;
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
    litepb_gen::litepb::test::interop::SignedInts litepb_msg;
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
    litepb_gen::litepb::test::interop::SignedInts litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(123, litepb_decoded.v_sint32);
    TEST_ASSERT_EQUAL_INT64(1234567890123LL, litepb_decoded.v_sint64);
}

void test_fixed_types()
{
    litepb_gen::litepb::test::interop::FixedTypes litepb_msg;
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
    litepb_gen::litepb::test::interop::FixedTypes litepb_decoded;
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
    litepb_gen::litepb::test::interop::RepeatedFields litepb_msg;
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
    litepb_gen::litepb::test::interop::RepeatedFields litepb_decoded;
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
    litepb_gen::litepb::test::interop::MapField litepb_msg;
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
    litepb_gen::litepb::test::interop::MapField litepb_decoded;
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
    litepb_gen::litepb::test::interop::Outer litepb_msg;
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
    litepb_gen::litepb::test::interop::Outer litepb_decoded;
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
    litepb_gen::litepb::test::interop::OneofTest litepb_msg;
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
    litepb_gen::litepb::test::interop::OneofTest litepb_decoded;
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
    litepb_gen::litepb::test::interop::OneofTest litepb_msg;
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
    litepb_gen::litepb::test::interop::OneofTest litepb_decoded;
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
    litepb_gen::litepb::test::interop::EnumTest litepb_msg;
    litepb_msg.value = litepb_gen::litepb::test::interop::TestEnum::FIRST;

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
    litepb_gen::litepb::test::interop::EnumTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::TestEnum::SECOND, litepb_decoded.value);

    EnumTestData binary_check_data;
    binary_check_data.value = 1;

    auto protoc_bytes = protoc_encode_enum_test(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_optional_fields()
{
    litepb_gen::litepb::test::interop::OptionalFields litepb_msg;
    litepb_msg.opt_int32  = 42;
    litepb_msg.opt_string = "test";
    litepb_msg.opt_bool   = true;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    OptionalFieldsData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_optional_fields(litepb_encoded, protoc_data));
    TEST_ASSERT_TRUE(protoc_data.has_opt_int32);
    TEST_ASSERT_EQUAL_INT32(42, protoc_data.opt_int32);
    TEST_ASSERT_TRUE(protoc_data.has_opt_string);
    TEST_ASSERT_EQUAL_STRING("test", protoc_data.opt_string.c_str());
    TEST_ASSERT_TRUE(protoc_data.has_opt_bool);
    TEST_ASSERT_TRUE(protoc_data.opt_bool);

    OptionalFieldsData encode_data;
    encode_data.has_opt_int32  = true;
    encode_data.opt_int32      = 100;
    encode_data.has_opt_string = false;
    encode_data.has_opt_bool   = true;
    encode_data.opt_bool       = false;

    auto protoc_encoded = protoc_encode_optional_fields(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::OptionalFields litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_TRUE(litepb_decoded.opt_int32.has_value());
    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.opt_int32.value());
    TEST_ASSERT_FALSE(litepb_decoded.opt_string.has_value());
    TEST_ASSERT_TRUE(litepb_decoded.opt_bool.has_value());
    TEST_ASSERT_FALSE(litepb_decoded.opt_bool.value());

    litepb::BufferOutputStream reserialize_stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_decoded, reserialize_stream));
    std::vector<uint8_t> litepb_reencoded(reserialize_stream.data(), reserialize_stream.data() + reserialize_stream.size());

    TEST_ASSERT_EQUAL_size_t(protoc_encoded.size(), litepb_reencoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_encoded.data(), litepb_reencoded.data(), protoc_encoded.size());

    OptionalFieldsData binary_check_data;
    binary_check_data.has_opt_int32  = true;
    binary_check_data.opt_int32      = 42;
    binary_check_data.has_opt_string = true;
    binary_check_data.opt_string     = "test";
    binary_check_data.has_opt_bool   = true;
    binary_check_data.opt_bool       = true;

    auto protoc_bytes = protoc_encode_optional_fields(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_empty_message()
{
    litepb_gen::litepb::test::interop::EmptyMessage litepb_msg;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    EmptyMessageData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_empty_message(litepb_encoded, protoc_data));

    EmptyMessageData encode_data;

    auto protoc_encoded = protoc_encode_empty_message(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::EmptyMessage litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    EmptyMessageData binary_check_data;

    auto protoc_bytes = protoc_encode_empty_message(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    if (protoc_bytes.size() > 0) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
    }
}

void test_unpacked_repeated()
{
    litepb_gen::litepb::test::interop::UnpackedRepeated litepb_msg;
    litepb_msg.unpacked_ints   = { 1, 2, 3, 4, 5 };
    litepb_msg.unpacked_floats = { 1.1f, 2.2f, 3.3f };

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    UnpackedRepeatedData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_unpacked_repeated(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_size_t(5, protoc_data.unpacked_ints.size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.unpacked_ints[0]);
    TEST_ASSERT_EQUAL_INT32(5, protoc_data.unpacked_ints[4]);
    TEST_ASSERT_EQUAL_size_t(3, protoc_data.unpacked_floats.size());
    TEST_ASSERT_EQUAL_FLOAT(1.1f, protoc_data.unpacked_floats[0]);
    TEST_ASSERT_EQUAL_FLOAT(3.3f, protoc_data.unpacked_floats[2]);

    UnpackedRepeatedData encode_data;
    encode_data.unpacked_ints   = { 10, 20 };
    encode_data.unpacked_floats = { 5.5f, 6.6f, 7.7f, 8.8f };

    auto protoc_encoded = protoc_encode_unpacked_repeated(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::UnpackedRepeated litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(2, litepb_decoded.unpacked_ints.size());
    TEST_ASSERT_EQUAL_INT32(10, litepb_decoded.unpacked_ints[0]);
    TEST_ASSERT_EQUAL_INT32(20, litepb_decoded.unpacked_ints[1]);
    TEST_ASSERT_EQUAL_size_t(4, litepb_decoded.unpacked_floats.size());
    TEST_ASSERT_EQUAL_FLOAT(5.5f, litepb_decoded.unpacked_floats[0]);
    TEST_ASSERT_EQUAL_FLOAT(8.8f, litepb_decoded.unpacked_floats[3]);

    UnpackedRepeatedData binary_check_data;
    binary_check_data.unpacked_ints   = { 1, 2, 3, 4, 5 };
    binary_check_data.unpacked_floats = { 1.1f, 2.2f, 3.3f };

    auto protoc_bytes = protoc_encode_unpacked_repeated(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_different_maps()
{
    litepb_gen::litepb::test::interop::DifferentMaps litepb_msg;
    litepb_msg.int_to_string[1]     = "one";
    litepb_msg.int_to_string[2]     = "two";
    litepb_msg.string_to_string["a"] = "alpha";
    litepb_msg.string_to_string["b"] = "beta";
    litepb_msg.int_to_int[10]        = 100;
    litepb_msg.int_to_int[20]        = 200;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    DifferentMapsData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_different_maps(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_size_t(2, protoc_data.int_to_string.size());
    TEST_ASSERT_EQUAL_STRING("one", protoc_data.int_to_string[1].c_str());
    TEST_ASSERT_EQUAL_STRING("two", protoc_data.int_to_string[2].c_str());
    TEST_ASSERT_EQUAL_size_t(2, protoc_data.string_to_string.size());
    TEST_ASSERT_EQUAL_STRING("alpha", protoc_data.string_to_string["a"].c_str());
    TEST_ASSERT_EQUAL_STRING("beta", protoc_data.string_to_string["b"].c_str());
    TEST_ASSERT_EQUAL_size_t(2, protoc_data.int_to_int.size());
    TEST_ASSERT_EQUAL_INT64(100, protoc_data.int_to_int[10]);
    TEST_ASSERT_EQUAL_INT64(200, protoc_data.int_to_int[20]);

    DifferentMapsData encode_data;
    encode_data.int_to_string[3]     = "three";
    encode_data.string_to_string["c"] = "gamma";
    encode_data.int_to_int[30]        = 300;

    auto protoc_encoded = protoc_encode_different_maps(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::DifferentMaps litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(1, litepb_decoded.int_to_string.size());
    TEST_ASSERT_EQUAL_STRING("three", litepb_decoded.int_to_string[3].c_str());
    TEST_ASSERT_EQUAL_size_t(1, litepb_decoded.string_to_string.size());
    TEST_ASSERT_EQUAL_STRING("gamma", litepb_decoded.string_to_string["c"].c_str());
    TEST_ASSERT_EQUAL_size_t(1, litepb_decoded.int_to_int.size());
    TEST_ASSERT_EQUAL_INT64(300, litepb_decoded.int_to_int[30]);
}

void test_repeated_messages()
{
    litepb_gen::litepb::test::interop::RepeatedMessages litepb_msg;
    litepb_gen::litepb::test::interop::Item item1;
    item1.id   = 1;
    item1.name = "first";
    litepb_msg.items.push_back(item1);
    litepb_gen::litepb::test::interop::Item item2;
    item2.id   = 2;
    item2.name = "second";
    litepb_msg.items.push_back(item2);

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    RepeatedMessagesData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_repeated_messages(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_size_t(2, protoc_data.items.size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.items[0].id);
    TEST_ASSERT_EQUAL_STRING("first", protoc_data.items[0].name.c_str());
    TEST_ASSERT_EQUAL_INT32(2, protoc_data.items[1].id);
    TEST_ASSERT_EQUAL_STRING("second", protoc_data.items[1].name.c_str());

    RepeatedMessagesData encode_data;
    ItemData encode_item1;
    encode_item1.id   = 10;
    encode_item1.name = "tenth";
    encode_data.items.push_back(encode_item1);

    auto protoc_encoded = protoc_encode_repeated_messages(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::RepeatedMessages litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(1, litepb_decoded.items.size());
    TEST_ASSERT_EQUAL_INT32(10, litepb_decoded.items[0].id);
    TEST_ASSERT_EQUAL_STRING("tenth", litepb_decoded.items[0].name.c_str());

    RepeatedMessagesData binary_check_data;
    ItemData binary_item1;
    binary_item1.id   = 1;
    binary_item1.name = "first";
    binary_check_data.items.push_back(binary_item1);
    ItemData binary_item2;
    binary_item2.id   = 2;
    binary_item2.name = "second";
    binary_check_data.items.push_back(binary_item2);

    auto protoc_bytes = protoc_encode_repeated_messages(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_nested_enum_test()
{
    litepb_gen::litepb::test::interop::NestedEnumTest litepb_msg;
    litepb_msg.status = litepb_gen::litepb::test::interop::NestedEnumTest::Status::ACTIVE;
    litepb_msg.code   = 42;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    NestedEnumTestData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_nested_enum_test(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.status);
    TEST_ASSERT_EQUAL_INT32(42, protoc_data.code);

    NestedEnumTestData encode_data;
    encode_data.status = 2;
    encode_data.code   = 100;

    auto protoc_encoded = protoc_encode_nested_enum_test(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::NestedEnumTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::NestedEnumTest::Status::INACTIVE, litepb_decoded.status);
    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.code);

    NestedEnumTestData binary_check_data;
    binary_check_data.status = 1;
    binary_check_data.code   = 42;

    auto protoc_bytes = protoc_encode_nested_enum_test(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_large_field_numbers()
{
    litepb_gen::litepb::test::interop::LargeFieldNumbers litepb_msg;
    litepb_msg.field_1     = 1;
    litepb_msg.field_100   = 100;
    litepb_msg.field_1000  = 1000;
    litepb_msg.field_10000 = 10000;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    LargeFieldNumbersData protoc_data;
    TEST_ASSERT_TRUE(protoc_decode_large_field_numbers(litepb_encoded, protoc_data));
    TEST_ASSERT_EQUAL_INT32(1, protoc_data.field_1);
    TEST_ASSERT_EQUAL_INT32(100, protoc_data.field_100);
    TEST_ASSERT_EQUAL_INT32(1000, protoc_data.field_1000);
    TEST_ASSERT_EQUAL_INT32(10000, protoc_data.field_10000);

    LargeFieldNumbersData encode_data;
    encode_data.field_1     = 11;
    encode_data.field_100   = 110;
    encode_data.field_1000  = 1100;
    encode_data.field_10000 = 11000;

    auto protoc_encoded = protoc_encode_large_field_numbers(encode_data);
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::LargeFieldNumbers litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(11, litepb_decoded.field_1);
    TEST_ASSERT_EQUAL_INT32(110, litepb_decoded.field_100);
    TEST_ASSERT_EQUAL_INT32(1100, litepb_decoded.field_1000);
    TEST_ASSERT_EQUAL_INT32(11000, litepb_decoded.field_10000);

    LargeFieldNumbersData binary_check_data;
    binary_check_data.field_1     = 1;
    binary_check_data.field_100   = 100;
    binary_check_data.field_1000  = 1000;
    binary_check_data.field_10000 = 10000;

    auto protoc_bytes = protoc_encode_large_field_numbers(binary_check_data);
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_unknown_fields()
{
    UnknownFieldsNewData new_data;
    new_data.known_field = 42;
    new_data.extra_field = "extra";

    auto new_encoded = protoc_encode_unknown_fields_new(new_data);

    UnknownFieldsOldData old_data;
    TEST_ASSERT_TRUE(protoc_decode_unknown_fields_old(new_encoded, old_data));
    TEST_ASSERT_EQUAL_INT32(42, old_data.known_field);

    litepb::BufferInputStream input_stream(new_encoded.data(), new_encoded.size());
    litepb_gen::litepb::test::interop::UnknownFieldsOld litepb_old;
    TEST_ASSERT_TRUE(litepb::parse(litepb_old, input_stream));
    TEST_ASSERT_EQUAL_INT32(42, litepb_old.known_field);

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_old, stream));
    std::vector<uint8_t> old_reencoded(stream.data(), stream.data() + stream.size());

    UnknownFieldsNewData new_data_decoded;
    TEST_ASSERT_TRUE(protoc_decode_unknown_fields_new(old_reencoded, new_data_decoded));
    TEST_ASSERT_EQUAL_INT32(42, new_data_decoded.known_field);
    TEST_ASSERT_EQUAL_STRING("extra", new_data_decoded.extra_field.c_str());
}
