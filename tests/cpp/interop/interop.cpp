#include "litepb/litepb.h"
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

    litepb::test::interop::BasicScalars protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(42, protoc_msg.int32_field());
    TEST_ASSERT_EQUAL_STRING("test", protoc_msg.string_field().c_str());

    litepb::test::interop::BasicScalars protoc_msg2;
    protoc_msg2.set_int32_field(100);
    protoc_msg2.set_string_field("hello");
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::BasicScalars litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.int32_field);
    TEST_ASSERT_EQUAL_STRING("hello", litepb_decoded.string_field.c_str());

    litepb::test::interop::BasicScalars protoc_msg3;
    protoc_msg3.set_int32_field(42);
    protoc_msg3.set_string_field("test");
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::AllScalars protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(-123, protoc_msg.v_int32());
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_msg.v_int64());
    TEST_ASSERT_EQUAL_UINT32(456, protoc_msg.v_uint32());
    TEST_ASSERT_EQUAL_UINT64(12345678901ULL, protoc_msg.v_uint64());
    TEST_ASSERT_TRUE(protoc_msg.v_bool());
    TEST_ASSERT_EQUAL_STRING("hello world", protoc_msg.v_string().c_str());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(litepb_msg.v_bytes.data(), reinterpret_cast<const uint8_t*>(protoc_msg.v_bytes().data()), 4);
    TEST_ASSERT_EQUAL_FLOAT(3.14f, protoc_msg.v_float());
    TEST_ASSERT_EQUAL_DOUBLE(2.718281828, protoc_msg.v_double());

    litepb::test::interop::AllScalars protoc_msg2;
    protoc_msg2.set_v_int32(789);
    protoc_msg2.set_v_int64(1234567890123LL);
    protoc_msg2.set_v_uint32(321);
    protoc_msg2.set_v_uint64(9876543210ULL);
    protoc_msg2.set_v_bool(false);
    protoc_msg2.set_v_string("test");
    protoc_msg2.set_v_bytes(std::string("\xAA\xBB", 2));
    protoc_msg2.set_v_float(1.23f);
    protoc_msg2.set_v_double(4.56);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::AllScalars litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(789, litepb_decoded.v_int32);
    TEST_ASSERT_EQUAL_INT64(1234567890123LL, litepb_decoded.v_int64);
    TEST_ASSERT_EQUAL_UINT32(321, litepb_decoded.v_uint32);
    TEST_ASSERT_EQUAL_UINT64(9876543210ULL, litepb_decoded.v_uint64);
    TEST_ASSERT_FALSE(litepb_decoded.v_bool);
    TEST_ASSERT_EQUAL_STRING("test", litepb_decoded.v_string.c_str());
    std::vector<uint8_t> expected_bytes = { 0xAA, 0xBB };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_bytes.data(), litepb_decoded.v_bytes.data(), 2);
    TEST_ASSERT_EQUAL_FLOAT(1.23f, litepb_decoded.v_float);
    TEST_ASSERT_EQUAL_DOUBLE(4.56, litepb_decoded.v_double);

    litepb::test::interop::AllScalars protoc_msg3;
    protoc_msg3.set_v_int32(-123);
    protoc_msg3.set_v_int64(-9876543210LL);
    protoc_msg3.set_v_uint32(456);
    protoc_msg3.set_v_uint64(12345678901ULL);
    protoc_msg3.set_v_bool(true);
    protoc_msg3.set_v_string("hello world");
    protoc_msg3.set_v_bytes(std::string("\x01\x02\x03\xFF", 4));
    protoc_msg3.set_v_float(3.14f);
    protoc_msg3.set_v_double(2.718281828);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::SignedInts protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(-42, protoc_msg.v_sint32());
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_msg.v_sint64());

    litepb::test::interop::SignedInts protoc_msg2;
    protoc_msg2.set_v_sint32(123);
    protoc_msg2.set_v_sint64(1234567890123LL);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
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

    litepb::test::interop::FixedTypes protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_UINT32(0x12345678, protoc_msg.v_fixed32());
    TEST_ASSERT_EQUAL_UINT64(0x123456789ABCDEF0ULL, protoc_msg.v_fixed64());
    TEST_ASSERT_EQUAL_INT32(-123456, protoc_msg.v_sfixed32());
    TEST_ASSERT_EQUAL_INT64(-9876543210LL, protoc_msg.v_sfixed64());

    litepb::test::interop::FixedTypes protoc_msg2;
    protoc_msg2.set_v_fixed32(0xABCDEF00);
    protoc_msg2.set_v_fixed64(0xFEDCBA9876543210ULL);
    protoc_msg2.set_v_sfixed32(654321);
    protoc_msg2.set_v_sfixed64(1234567890LL);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::FixedTypes litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_UINT32(0xABCDEF00, litepb_decoded.v_fixed32);
    TEST_ASSERT_EQUAL_UINT64(0xFEDCBA9876543210ULL, litepb_decoded.v_fixed64);
    TEST_ASSERT_EQUAL_INT32(654321, litepb_decoded.v_sfixed32);
    TEST_ASSERT_EQUAL_INT64(1234567890LL, litepb_decoded.v_sfixed64);

    litepb::test::interop::FixedTypes protoc_msg3;
    protoc_msg3.set_v_fixed32(0x12345678);
    protoc_msg3.set_v_fixed64(0x123456789ABCDEF0ULL);
    protoc_msg3.set_v_sfixed32(-123456);
    protoc_msg3.set_v_sfixed64(-9876543210LL);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::RepeatedFields protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_size_t(5, protoc_msg.packed_ints_size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_msg.packed_ints(0));
    TEST_ASSERT_EQUAL_INT32(5, protoc_msg.packed_ints(4));
    TEST_ASSERT_EQUAL_size_t(3, protoc_msg.strings_size());
    TEST_ASSERT_EQUAL_STRING("hello", protoc_msg.strings(0).c_str());
    TEST_ASSERT_EQUAL_STRING("test", protoc_msg.strings(2).c_str());

    litepb::test::interop::RepeatedFields protoc_msg2;
    protoc_msg2.add_packed_ints(10);
    protoc_msg2.add_packed_ints(20);
    protoc_msg2.add_packed_ints(30);
    protoc_msg2.add_strings("foo");
    protoc_msg2.add_strings("bar");
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
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

    litepb::test::interop::MapField protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_size_t(3, protoc_msg.items().size());
    TEST_ASSERT_EQUAL_INT32(100, protoc_msg.items().at("key1"));
    TEST_ASSERT_EQUAL_INT32(200, protoc_msg.items().at("key2"));
    TEST_ASSERT_EQUAL_INT32(300, protoc_msg.items().at("key3"));

    litepb::test::interop::MapField protoc_msg2;
    (*protoc_msg2.mutable_items())["foo"] = 42;
    (*protoc_msg2.mutable_items())["bar"] = 84;
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
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

    litepb::test::interop::Outer protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(42, protoc_msg.inner().value());
    TEST_ASSERT_EQUAL_STRING("outer_name", protoc_msg.name().c_str());

    litepb::test::interop::Outer protoc_msg2;
    protoc_msg2.mutable_inner()->set_value(123);
    protoc_msg2.set_name("test_name");
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::Outer litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(123, litepb_decoded.inner.value);
    TEST_ASSERT_EQUAL_STRING("test_name", litepb_decoded.name.c_str());

    litepb::test::interop::Outer protoc_msg3;
    protoc_msg3.mutable_inner()->set_value(42);
    protoc_msg3.set_name("outer_name");
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::OneofTest protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL(litepb::test::interop::OneofTest::kIntValue, protoc_msg.choice_case());
    TEST_ASSERT_EQUAL_INT32(42, protoc_msg.int_value());

    litepb::test::interop::OneofTest protoc_msg2;
    protoc_msg2.set_int_value(100);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::OneofTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_TRUE(std::holds_alternative<int32_t>(litepb_decoded.choice));
    TEST_ASSERT_EQUAL_INT32(100, std::get<int32_t>(litepb_decoded.choice));

    litepb::test::interop::OneofTest protoc_msg3;
    protoc_msg3.set_int_value(42);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::OneofTest protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL(litepb::test::interop::OneofTest::kStringValue, protoc_msg.choice_case());
    TEST_ASSERT_EQUAL_STRING("test_string", protoc_msg.string_value().c_str());

    litepb::test::interop::OneofTest protoc_msg2;
    protoc_msg2.set_string_value("hello");
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::OneofTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_TRUE(std::holds_alternative<std::string>(litepb_decoded.choice));
    TEST_ASSERT_EQUAL_STRING("hello", std::get<std::string>(litepb_decoded.choice).c_str());

    litepb::test::interop::OneofTest protoc_msg3;
    protoc_msg3.set_string_value("test_string");
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::EnumTest protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(litepb::test::interop::FIRST, protoc_msg.value());

    litepb::test::interop::EnumTest protoc_msg2;
    protoc_msg2.set_value(litepb::test::interop::SECOND);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::EnumTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::TestEnum::SECOND, litepb_decoded.value);

    litepb::test::interop::EnumTest protoc_msg3;
    protoc_msg3.set_value(litepb::test::interop::FIRST);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::OptionalFields protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_TRUE(protoc_msg.has_opt_int32());
    TEST_ASSERT_EQUAL_INT32(42, protoc_msg.opt_int32());
    TEST_ASSERT_TRUE(protoc_msg.has_opt_string());
    TEST_ASSERT_EQUAL_STRING("test", protoc_msg.opt_string().c_str());
    TEST_ASSERT_TRUE(protoc_msg.has_opt_bool());
    TEST_ASSERT_TRUE(protoc_msg.opt_bool());

    litepb::test::interop::OptionalFields protoc_msg2;
    protoc_msg2.set_opt_int32(100);
    protoc_msg2.set_opt_bool(false);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
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

    litepb::test::interop::OptionalFields protoc_msg3;
    protoc_msg3.set_opt_int32(42);
    protoc_msg3.set_opt_string("test");
    protoc_msg3.set_opt_bool(true);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_empty_message()
{
    litepb_gen::litepb::test::interop::EmptyMessage litepb_msg;

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());

    litepb::test::interop::EmptyMessage protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));

    litepb::test::interop::EmptyMessage protoc_msg2;
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::EmptyMessage litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    litepb::test::interop::EmptyMessage protoc_msg3;
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::UnpackedRepeated protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_size_t(5, protoc_msg.unpacked_ints_size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_msg.unpacked_ints(0));
    TEST_ASSERT_EQUAL_INT32(5, protoc_msg.unpacked_ints(4));
    TEST_ASSERT_EQUAL_size_t(3, protoc_msg.unpacked_floats_size());
    TEST_ASSERT_EQUAL_FLOAT(1.1f, protoc_msg.unpacked_floats(0));
    TEST_ASSERT_EQUAL_FLOAT(3.3f, protoc_msg.unpacked_floats(2));

    litepb::test::interop::UnpackedRepeated protoc_msg2;
    protoc_msg2.add_unpacked_ints(10);
    protoc_msg2.add_unpacked_ints(20);
    protoc_msg2.add_unpacked_floats(5.5f);
    protoc_msg2.add_unpacked_floats(6.6f);
    protoc_msg2.add_unpacked_floats(7.7f);
    protoc_msg2.add_unpacked_floats(8.8f);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::UnpackedRepeated litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(2, litepb_decoded.unpacked_ints.size());
    TEST_ASSERT_EQUAL_INT32(10, litepb_decoded.unpacked_ints[0]);
    TEST_ASSERT_EQUAL_INT32(20, litepb_decoded.unpacked_ints[1]);
    TEST_ASSERT_EQUAL_size_t(4, litepb_decoded.unpacked_floats.size());
    TEST_ASSERT_EQUAL_FLOAT(5.5f, litepb_decoded.unpacked_floats[0]);
    TEST_ASSERT_EQUAL_FLOAT(8.8f, litepb_decoded.unpacked_floats[3]);

    litepb::test::interop::UnpackedRepeated protoc_msg3;
    protoc_msg3.add_unpacked_ints(1);
    protoc_msg3.add_unpacked_ints(2);
    protoc_msg3.add_unpacked_ints(3);
    protoc_msg3.add_unpacked_ints(4);
    protoc_msg3.add_unpacked_ints(5);
    protoc_msg3.add_unpacked_floats(1.1f);
    protoc_msg3.add_unpacked_floats(2.2f);
    protoc_msg3.add_unpacked_floats(3.3f);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::DifferentMaps protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_size_t(2, protoc_msg.int_to_string().size());
    TEST_ASSERT_EQUAL_STRING("one", protoc_msg.int_to_string().at(1).c_str());
    TEST_ASSERT_EQUAL_STRING("two", protoc_msg.int_to_string().at(2).c_str());
    TEST_ASSERT_EQUAL_size_t(2, protoc_msg.string_to_string().size());
    TEST_ASSERT_EQUAL_STRING("alpha", protoc_msg.string_to_string().at("a").c_str());
    TEST_ASSERT_EQUAL_STRING("beta", protoc_msg.string_to_string().at("b").c_str());
    TEST_ASSERT_EQUAL_size_t(2, protoc_msg.int_to_int().size());
    TEST_ASSERT_EQUAL_INT64(100, protoc_msg.int_to_int().at(10));
    TEST_ASSERT_EQUAL_INT64(200, protoc_msg.int_to_int().at(20));

    litepb::test::interop::DifferentMaps protoc_msg2;
    (*protoc_msg2.mutable_int_to_string())[3] = "three";
    (*protoc_msg2.mutable_string_to_string())["c"] = "gamma";
    (*protoc_msg2.mutable_int_to_int())[30] = 300;
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
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

    litepb::test::interop::RepeatedMessages protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_size_t(2, protoc_msg.items_size());
    TEST_ASSERT_EQUAL_INT32(1, protoc_msg.items(0).id());
    TEST_ASSERT_EQUAL_STRING("first", protoc_msg.items(0).name().c_str());
    TEST_ASSERT_EQUAL_INT32(2, protoc_msg.items(1).id());
    TEST_ASSERT_EQUAL_STRING("second", protoc_msg.items(1).name().c_str());

    litepb::test::interop::RepeatedMessages protoc_msg2;
    auto* item = protoc_msg2.add_items();
    item->set_id(10);
    item->set_name("tenth");
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::RepeatedMessages litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_size_t(1, litepb_decoded.items.size());
    TEST_ASSERT_EQUAL_INT32(10, litepb_decoded.items[0].id);
    TEST_ASSERT_EQUAL_STRING("tenth", litepb_decoded.items[0].name.c_str());

    litepb::test::interop::RepeatedMessages protoc_msg3;
    auto* item1_ptr = protoc_msg3.add_items();
    item1_ptr->set_id(1);
    item1_ptr->set_name("first");
    auto* item2_ptr = protoc_msg3.add_items();
    item2_ptr->set_id(2);
    item2_ptr->set_name("second");
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::NestedEnumTest protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(litepb::test::interop::NestedEnumTest_Status_ACTIVE, protoc_msg.status());
    TEST_ASSERT_EQUAL_INT32(42, protoc_msg.code());

    litepb::test::interop::NestedEnumTest protoc_msg2;
    protoc_msg2.set_status(litepb::test::interop::NestedEnumTest_Status_INACTIVE);
    protoc_msg2.set_code(100);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::NestedEnumTest litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::NestedEnumTest::Status::INACTIVE, litepb_decoded.status);
    TEST_ASSERT_EQUAL_INT32(100, litepb_decoded.code);

    litepb::test::interop::NestedEnumTest protoc_msg3;
    protoc_msg3.set_status(litepb::test::interop::NestedEnumTest_Status_ACTIVE);
    protoc_msg3.set_code(42);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
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

    litepb::test::interop::LargeFieldNumbers protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(1, protoc_msg.field_1());
    TEST_ASSERT_EQUAL_INT32(100, protoc_msg.field_100());
    TEST_ASSERT_EQUAL_INT32(1000, protoc_msg.field_1000());
    TEST_ASSERT_EQUAL_INT32(10000, protoc_msg.field_10000());

    litepb::test::interop::LargeFieldNumbers protoc_msg2;
    protoc_msg2.set_field_1(11);
    protoc_msg2.set_field_100(110);
    protoc_msg2.set_field_1000(1100);
    protoc_msg2.set_field_10000(11000);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    litepb_gen::litepb::test::interop::LargeFieldNumbers litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));

    TEST_ASSERT_EQUAL_INT32(11, litepb_decoded.field_1);
    TEST_ASSERT_EQUAL_INT32(110, litepb_decoded.field_100);
    TEST_ASSERT_EQUAL_INT32(1100, litepb_decoded.field_1000);
    TEST_ASSERT_EQUAL_INT32(11000, litepb_decoded.field_10000);

    litepb::test::interop::LargeFieldNumbers protoc_msg3;
    protoc_msg3.set_field_1(1);
    protoc_msg3.set_field_100(100);
    protoc_msg3.set_field_1000(1000);
    protoc_msg3.set_field_10000(10000);
    std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
    std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
    
    TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
}

void test_unknown_fields()
{
    litepb::test::interop::UnknownFieldsNew protoc_new;
    protoc_new.set_known_field(42);
    protoc_new.set_extra_field("extra");
    std::string new_encoded_str = protoc_new.SerializeAsString();
    std::vector<uint8_t> new_encoded(new_encoded_str.begin(), new_encoded_str.end());

    litepb::test::interop::UnknownFieldsOld protoc_old;
    TEST_ASSERT_TRUE(protoc_old.ParseFromArray(new_encoded.data(), new_encoded.size()));
    TEST_ASSERT_EQUAL_INT32(42, protoc_old.known_field());

    litepb::BufferInputStream input_stream(new_encoded.data(), new_encoded.size());
    litepb_gen::litepb::test::interop::UnknownFieldsOld litepb_old;
    TEST_ASSERT_TRUE(litepb::parse(litepb_old, input_stream));
    TEST_ASSERT_EQUAL_INT32(42, litepb_old.known_field);

    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_old, stream));
    std::vector<uint8_t> old_reencoded(stream.data(), stream.data() + stream.size());

    litepb::test::interop::UnknownFieldsNew protoc_new_decoded;
    TEST_ASSERT_TRUE(protoc_new_decoded.ParseFromArray(old_reencoded.data(), old_reencoded.size()));
    TEST_ASSERT_EQUAL_INT32(42, protoc_new_decoded.known_field());
    TEST_ASSERT_EQUAL_STRING("extra", protoc_new_decoded.extra_field().c_str());
}
