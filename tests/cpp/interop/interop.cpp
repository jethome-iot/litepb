#include "litepb/litepb.h"
#include "test_vectors.pb.h"
#include "protoc/test_vectors.pb.h"
#include "unity.h"
#ifdef HAVE_ABSL_LOG
#include <absl/log/initialize.h>
#endif
#include <google/protobuf/stubs/common.h>

void setUp() {}
void tearDown() {}

template<typename LitePBMsg, typename ProtocMsg>
struct RoundTripConfig {
    std::function<void(LitePBMsg&)> build_litepb;
    std::function<void(const ProtocMsg&)> verify_protoc;
    std::function<void(ProtocMsg&)> build_protoc;
    std::function<void(const LitePBMsg&)> verify_litepb;
    std::function<void(ProtocMsg&)> build_protoc_baseline = nullptr;
    std::function<void()> post_round_trip = nullptr;
    bool expect_binary_equivalence = true;
};

template<typename LitePBMsg, typename ProtocMsg>
void run_round_trip(const RoundTripConfig<LitePBMsg, ProtocMsg>& config) {
    LitePBMsg litepb_msg;
    config.build_litepb(litepb_msg);
    
    litepb::BufferOutputStream stream;
    TEST_ASSERT_TRUE(litepb::serialize(litepb_msg, stream));
    std::vector<uint8_t> litepb_encoded(stream.data(), stream.data() + stream.size());
    
    ProtocMsg protoc_msg;
    TEST_ASSERT_TRUE(protoc_msg.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
    config.verify_protoc(protoc_msg);
    
    ProtocMsg protoc_msg2;
    config.build_protoc(protoc_msg2);
    std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
    std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
    
    litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
    LitePBMsg litepb_decoded;
    TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));
    config.verify_litepb(litepb_decoded);
    
    if (config.expect_binary_equivalence) {
        ProtocMsg protoc_msg3;
        if (config.build_protoc_baseline) {
            config.build_protoc_baseline(protoc_msg3);
        } else {
            TEST_ASSERT_TRUE(protoc_msg3.ParseFromArray(litepb_encoded.data(), litepb_encoded.size()));
        }
        std::string protoc_bytes_str = protoc_msg3.SerializeAsString();
        std::vector<uint8_t> protoc_bytes(protoc_bytes_str.begin(), protoc_bytes_str.end());
        
        TEST_ASSERT_EQUAL_size_t(protoc_bytes.size(), litepb_encoded.size());
        if (protoc_bytes.size() > 0) {
            TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_bytes.data(), litepb_encoded.data(), protoc_bytes.size());
        }
    }
    
    if (config.post_round_trip) {
        config.post_round_trip();
    }
}

void test_basic_scalars()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::BasicScalars, litepb::test::interop::BasicScalars> config;
    
    config.build_litepb = [](auto& msg) {
        msg.int32_field = 42;
        msg.string_field = "test";
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(42, msg.int32_field());
        TEST_ASSERT_EQUAL_STRING("test", msg.string_field().c_str());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_int32_field(100);
        msg.set_string_field("hello");
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(100, msg.int32_field);
        TEST_ASSERT_EQUAL_STRING("hello", msg.string_field.c_str());
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_int32_field(42);
        msg.set_string_field("test");
    };
    
    run_round_trip(config);
}

void test_all_scalars()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::AllScalars, litepb::test::interop::AllScalars> config;
    
    config.build_litepb = [](auto& msg) {
        msg.v_int32 = -123;
        msg.v_int64 = -9876543210LL;
        msg.v_uint32 = 456;
        msg.v_uint64 = 12345678901ULL;
        msg.v_bool = true;
        msg.v_string = "hello world";
        msg.v_bytes = { 0x01, 0x02, 0x03, 0xFF };
        msg.v_float = 3.14f;
        msg.v_double = 2.718281828;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(-123, msg.v_int32());
        TEST_ASSERT_EQUAL_INT64(-9876543210LL, msg.v_int64());
        TEST_ASSERT_EQUAL_UINT32(456, msg.v_uint32());
        TEST_ASSERT_EQUAL_UINT64(12345678901ULL, msg.v_uint64());
        TEST_ASSERT_TRUE(msg.v_bool());
        TEST_ASSERT_EQUAL_STRING("hello world", msg.v_string().c_str());
        std::vector<uint8_t> expected_bytes = { 0x01, 0x02, 0x03, 0xFF };
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_bytes.data(), reinterpret_cast<const uint8_t*>(msg.v_bytes().data()), 4);
        TEST_ASSERT_EQUAL_FLOAT(3.14f, msg.v_float());
        TEST_ASSERT_EQUAL_DOUBLE(2.718281828, msg.v_double());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_v_int32(789);
        msg.set_v_int64(1234567890123LL);
        msg.set_v_uint32(321);
        msg.set_v_uint64(9876543210ULL);
        msg.set_v_bool(false);
        msg.set_v_string("test");
        msg.set_v_bytes(std::string("\xAA\xBB", 2));
        msg.set_v_float(1.23f);
        msg.set_v_double(4.56);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(789, msg.v_int32);
        TEST_ASSERT_EQUAL_INT64(1234567890123LL, msg.v_int64);
        TEST_ASSERT_EQUAL_UINT32(321, msg.v_uint32);
        TEST_ASSERT_EQUAL_UINT64(9876543210ULL, msg.v_uint64);
        TEST_ASSERT_FALSE(msg.v_bool);
        TEST_ASSERT_EQUAL_STRING("test", msg.v_string.c_str());
        std::vector<uint8_t> expected_bytes = { 0xAA, 0xBB };
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_bytes.data(), msg.v_bytes.data(), 2);
        TEST_ASSERT_EQUAL_FLOAT(1.23f, msg.v_float);
        TEST_ASSERT_EQUAL_DOUBLE(4.56, msg.v_double);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_v_int32(-123);
        msg.set_v_int64(-9876543210LL);
        msg.set_v_uint32(456);
        msg.set_v_uint64(12345678901ULL);
        msg.set_v_bool(true);
        msg.set_v_string("hello world");
        msg.set_v_bytes(std::string("\x01\x02\x03\xFF", 4));
        msg.set_v_float(3.14f);
        msg.set_v_double(2.718281828);
    };
    
    run_round_trip(config);
}

void test_signed_ints()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::SignedInts, litepb::test::interop::SignedInts> config;
    
    config.build_litepb = [](auto& msg) {
        msg.v_sint32 = -42;
        msg.v_sint64 = -9876543210LL;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(-42, msg.v_sint32());
        TEST_ASSERT_EQUAL_INT64(-9876543210LL, msg.v_sint64());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_v_sint32(123);
        msg.set_v_sint64(1234567890123LL);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(123, msg.v_sint32);
        TEST_ASSERT_EQUAL_INT64(1234567890123LL, msg.v_sint64);
    };
    
    config.expect_binary_equivalence = false;
    
    run_round_trip(config);
}

void test_fixed_types()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::FixedTypes, litepb::test::interop::FixedTypes> config;
    
    config.build_litepb = [](auto& msg) {
        msg.v_fixed32 = 0x12345678;
        msg.v_fixed64 = 0x123456789ABCDEF0ULL;
        msg.v_sfixed32 = -123456;
        msg.v_sfixed64 = -9876543210LL;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_UINT32(0x12345678, msg.v_fixed32());
        TEST_ASSERT_EQUAL_UINT64(0x123456789ABCDEF0ULL, msg.v_fixed64());
        TEST_ASSERT_EQUAL_INT32(-123456, msg.v_sfixed32());
        TEST_ASSERT_EQUAL_INT64(-9876543210LL, msg.v_sfixed64());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_v_fixed32(0xABCDEF00);
        msg.set_v_fixed64(0xFEDCBA9876543210ULL);
        msg.set_v_sfixed32(654321);
        msg.set_v_sfixed64(1234567890LL);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_UINT32(0xABCDEF00, msg.v_fixed32);
        TEST_ASSERT_EQUAL_UINT64(0xFEDCBA9876543210ULL, msg.v_fixed64);
        TEST_ASSERT_EQUAL_INT32(654321, msg.v_sfixed32);
        TEST_ASSERT_EQUAL_INT64(1234567890LL, msg.v_sfixed64);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_v_fixed32(0x12345678);
        msg.set_v_fixed64(0x123456789ABCDEF0ULL);
        msg.set_v_sfixed32(-123456);
        msg.set_v_sfixed64(-9876543210LL);
    };
    
    run_round_trip(config);
}

void test_repeated_fields()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::RepeatedFields, litepb::test::interop::RepeatedFields> config;
    
    config.build_litepb = [](auto& msg) {
        msg.packed_ints = { 1, 2, 3, 4, 5 };
        msg.strings = { "hello", "world", "test" };
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(5, msg.packed_ints_size());
        TEST_ASSERT_EQUAL_INT32(1, msg.packed_ints(0));
        TEST_ASSERT_EQUAL_INT32(5, msg.packed_ints(4));
        TEST_ASSERT_EQUAL_size_t(3, msg.strings_size());
        TEST_ASSERT_EQUAL_STRING("hello", msg.strings(0).c_str());
        TEST_ASSERT_EQUAL_STRING("test", msg.strings(2).c_str());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.add_packed_ints(10);
        msg.add_packed_ints(20);
        msg.add_packed_ints(30);
        msg.add_strings("foo");
        msg.add_strings("bar");
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(3, msg.packed_ints.size());
        TEST_ASSERT_EQUAL_INT32(10, msg.packed_ints[0]);
        TEST_ASSERT_EQUAL_INT32(30, msg.packed_ints[2]);
        TEST_ASSERT_EQUAL_size_t(2, msg.strings.size());
        TEST_ASSERT_EQUAL_STRING("foo", msg.strings[0].c_str());
        TEST_ASSERT_EQUAL_STRING("bar", msg.strings[1].c_str());
    };
    
    config.expect_binary_equivalence = false;
    
    run_round_trip(config);
}

void test_map_field()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::MapField, litepb::test::interop::MapField> config;
    
    config.build_litepb = [](auto& msg) {
        msg.items["key1"] = 100;
        msg.items["key2"] = 200;
        msg.items["key3"] = 300;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(3, msg.items().size());
        TEST_ASSERT_EQUAL_INT32(100, msg.items().at("key1"));
        TEST_ASSERT_EQUAL_INT32(200, msg.items().at("key2"));
        TEST_ASSERT_EQUAL_INT32(300, msg.items().at("key3"));
    };
    
    config.build_protoc = [](auto& msg) {
        (*msg.mutable_items())["foo"] = 42;
        (*msg.mutable_items())["bar"] = 84;
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(2, msg.items.size());
        TEST_ASSERT_EQUAL_INT32(42, msg.items.at("foo"));
        TEST_ASSERT_EQUAL_INT32(84, msg.items.at("bar"));
    };
    
    config.expect_binary_equivalence = false;
    
    run_round_trip(config);
}

void test_outer_nested()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::Outer, litepb::test::interop::Outer> config;
    
    config.build_litepb = [](auto& msg) {
        msg.inner.value = 42;
        msg.name = "outer_name";
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(42, msg.inner().value());
        TEST_ASSERT_EQUAL_STRING("outer_name", msg.name().c_str());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.mutable_inner()->set_value(123);
        msg.set_name("test_name");
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(123, msg.inner.value);
        TEST_ASSERT_EQUAL_STRING("test_name", msg.name.c_str());
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.mutable_inner()->set_value(42);
        msg.set_name("outer_name");
    };
    
    run_round_trip(config);
}

void test_oneof_int()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::OneofTest, litepb::test::interop::OneofTest> config;
    
    config.build_litepb = [](auto& msg) {
        msg.choice = int32_t(42);
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL(litepb::test::interop::OneofTest::kIntValue, msg.choice_case());
        TEST_ASSERT_EQUAL_INT32(42, msg.int_value());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_int_value(100);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_TRUE(std::holds_alternative<int32_t>(msg.choice));
        TEST_ASSERT_EQUAL_INT32(100, std::get<int32_t>(msg.choice));
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_int_value(42);
    };
    
    run_round_trip(config);
}

void test_oneof_string()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::OneofTest, litepb::test::interop::OneofTest> config;
    
    config.build_litepb = [](auto& msg) {
        msg.choice = std::string("test_string");
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL(litepb::test::interop::OneofTest::kStringValue, msg.choice_case());
        TEST_ASSERT_EQUAL_STRING("test_string", msg.string_value().c_str());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_string_value("hello");
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_TRUE(std::holds_alternative<std::string>(msg.choice));
        TEST_ASSERT_EQUAL_STRING("hello", std::get<std::string>(msg.choice).c_str());
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_string_value("test_string");
    };
    
    run_round_trip(config);
}

void test_enum()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::EnumTest, litepb::test::interop::EnumTest> config;
    
    config.build_litepb = [](auto& msg) {
        msg.value = litepb_gen::litepb::test::interop::TestEnum::FIRST;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(litepb::test::interop::FIRST, msg.value());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_value(litepb::test::interop::SECOND);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::TestEnum::SECOND, msg.value);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_value(litepb::test::interop::FIRST);
    };
    
    run_round_trip(config);
}

void test_optional_fields()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::OptionalFields, litepb::test::interop::OptionalFields> config;
    
    config.build_litepb = [](auto& msg) {
        msg.opt_int32 = 42;
        msg.opt_string = "test";
        msg.opt_bool = true;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_TRUE(msg.has_opt_int32());
        TEST_ASSERT_EQUAL_INT32(42, msg.opt_int32());
        TEST_ASSERT_TRUE(msg.has_opt_string());
        TEST_ASSERT_EQUAL_STRING("test", msg.opt_string().c_str());
        TEST_ASSERT_TRUE(msg.has_opt_bool());
        TEST_ASSERT_TRUE(msg.opt_bool());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_opt_int32(100);
        msg.set_opt_bool(false);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_TRUE(msg.opt_int32.has_value());
        TEST_ASSERT_EQUAL_INT32(100, msg.opt_int32.value());
        TEST_ASSERT_FALSE(msg.opt_string.has_value());
        TEST_ASSERT_TRUE(msg.opt_bool.has_value());
        TEST_ASSERT_FALSE(msg.opt_bool.value());
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_opt_int32(42);
        msg.set_opt_string("test");
        msg.set_opt_bool(true);
    };
    
    config.post_round_trip = [&config]() {
        litepb_gen::litepb::test::interop::OptionalFields litepb_decoded;
        litepb::test::interop::OptionalFields protoc_msg2;
        config.build_protoc(protoc_msg2);
        std::string protoc_encoded_str = protoc_msg2.SerializeAsString();
        std::vector<uint8_t> protoc_encoded(protoc_encoded_str.begin(), protoc_encoded_str.end());
        
        litepb::BufferInputStream input_stream(protoc_encoded.data(), protoc_encoded.size());
        TEST_ASSERT_TRUE(litepb::parse(litepb_decoded, input_stream));
        
        litepb::BufferOutputStream reserialize_stream;
        TEST_ASSERT_TRUE(litepb::serialize(litepb_decoded, reserialize_stream));
        std::vector<uint8_t> litepb_reencoded(reserialize_stream.data(), reserialize_stream.data() + reserialize_stream.size());
        
        TEST_ASSERT_EQUAL_size_t(protoc_encoded.size(), litepb_reencoded.size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(protoc_encoded.data(), litepb_reencoded.data(), protoc_encoded.size());
    };
    
    run_round_trip(config);
}

void test_empty_message()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::EmptyMessage, litepb::test::interop::EmptyMessage> config;
    
    config.build_litepb = [](auto& msg) {
    };
    
    config.verify_protoc = [](const auto& msg) {
    };
    
    config.build_protoc = [](auto& msg) {
    };
    
    config.verify_litepb = [](const auto& msg) {
    };
    
    config.build_protoc_baseline = [](auto& msg) {
    };
    
    run_round_trip(config);
}

void test_unpacked_repeated()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::UnpackedRepeated, litepb::test::interop::UnpackedRepeated> config;
    
    config.build_litepb = [](auto& msg) {
        msg.unpacked_ints = { 1, 2, 3, 4, 5 };
        msg.unpacked_floats = { 1.1f, 2.2f, 3.3f };
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(5, msg.unpacked_ints_size());
        TEST_ASSERT_EQUAL_INT32(1, msg.unpacked_ints(0));
        TEST_ASSERT_EQUAL_INT32(5, msg.unpacked_ints(4));
        TEST_ASSERT_EQUAL_size_t(3, msg.unpacked_floats_size());
        TEST_ASSERT_EQUAL_FLOAT(1.1f, msg.unpacked_floats(0));
        TEST_ASSERT_EQUAL_FLOAT(3.3f, msg.unpacked_floats(2));
    };
    
    config.build_protoc = [](auto& msg) {
        msg.add_unpacked_ints(10);
        msg.add_unpacked_ints(20);
        msg.add_unpacked_floats(5.5f);
        msg.add_unpacked_floats(6.6f);
        msg.add_unpacked_floats(7.7f);
        msg.add_unpacked_floats(8.8f);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(2, msg.unpacked_ints.size());
        TEST_ASSERT_EQUAL_INT32(10, msg.unpacked_ints[0]);
        TEST_ASSERT_EQUAL_INT32(20, msg.unpacked_ints[1]);
        TEST_ASSERT_EQUAL_size_t(4, msg.unpacked_floats.size());
        TEST_ASSERT_EQUAL_FLOAT(5.5f, msg.unpacked_floats[0]);
        TEST_ASSERT_EQUAL_FLOAT(8.8f, msg.unpacked_floats[3]);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.add_unpacked_ints(1);
        msg.add_unpacked_ints(2);
        msg.add_unpacked_ints(3);
        msg.add_unpacked_ints(4);
        msg.add_unpacked_ints(5);
        msg.add_unpacked_floats(1.1f);
        msg.add_unpacked_floats(2.2f);
        msg.add_unpacked_floats(3.3f);
    };
    
    run_round_trip(config);
}

void test_different_maps()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::DifferentMaps, litepb::test::interop::DifferentMaps> config;
    
    config.build_litepb = [](auto& msg) {
        msg.int_to_string[1] = "one";
        msg.int_to_string[2] = "two";
        msg.string_to_string["a"] = "alpha";
        msg.string_to_string["b"] = "beta";
        msg.int_to_int[10] = 100;
        msg.int_to_int[20] = 200;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(2, msg.int_to_string().size());
        TEST_ASSERT_EQUAL_STRING("one", msg.int_to_string().at(1).c_str());
        TEST_ASSERT_EQUAL_STRING("two", msg.int_to_string().at(2).c_str());
        TEST_ASSERT_EQUAL_size_t(2, msg.string_to_string().size());
        TEST_ASSERT_EQUAL_STRING("alpha", msg.string_to_string().at("a").c_str());
        TEST_ASSERT_EQUAL_STRING("beta", msg.string_to_string().at("b").c_str());
        TEST_ASSERT_EQUAL_size_t(2, msg.int_to_int().size());
        TEST_ASSERT_EQUAL_INT64(100, msg.int_to_int().at(10));
        TEST_ASSERT_EQUAL_INT64(200, msg.int_to_int().at(20));
    };
    
    config.build_protoc = [](auto& msg) {
        (*msg.mutable_int_to_string())[3] = "three";
        (*msg.mutable_string_to_string())["c"] = "gamma";
        (*msg.mutable_int_to_int())[30] = 300;
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(1, msg.int_to_string.size());
        TEST_ASSERT_EQUAL_STRING("three", msg.int_to_string.at(3).c_str());
        TEST_ASSERT_EQUAL_size_t(1, msg.string_to_string.size());
        TEST_ASSERT_EQUAL_STRING("gamma", msg.string_to_string.at("c").c_str());
        TEST_ASSERT_EQUAL_size_t(1, msg.int_to_int.size());
        TEST_ASSERT_EQUAL_INT64(300, msg.int_to_int.at(30));
    };
    
    config.expect_binary_equivalence = false;
    
    run_round_trip(config);
}

void test_repeated_messages()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::RepeatedMessages, litepb::test::interop::RepeatedMessages> config;
    
    config.build_litepb = [](auto& msg) {
        litepb_gen::litepb::test::interop::Item item1;
        item1.id = 1;
        item1.name = "first";
        msg.items.push_back(item1);
        litepb_gen::litepb::test::interop::Item item2;
        item2.id = 2;
        item2.name = "second";
        msg.items.push_back(item2);
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(2, msg.items_size());
        TEST_ASSERT_EQUAL_INT32(1, msg.items(0).id());
        TEST_ASSERT_EQUAL_STRING("first", msg.items(0).name().c_str());
        TEST_ASSERT_EQUAL_INT32(2, msg.items(1).id());
        TEST_ASSERT_EQUAL_STRING("second", msg.items(1).name().c_str());
    };
    
    config.build_protoc = [](auto& msg) {
        auto* item = msg.add_items();
        item->set_id(10);
        item->set_name("tenth");
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_size_t(1, msg.items.size());
        TEST_ASSERT_EQUAL_INT32(10, msg.items[0].id);
        TEST_ASSERT_EQUAL_STRING("tenth", msg.items[0].name.c_str());
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        auto* item1_ptr = msg.add_items();
        item1_ptr->set_id(1);
        item1_ptr->set_name("first");
        auto* item2_ptr = msg.add_items();
        item2_ptr->set_id(2);
        item2_ptr->set_name("second");
    };
    
    run_round_trip(config);
}

void test_nested_enum_test()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::NestedEnumTest, litepb::test::interop::NestedEnumTest> config;
    
    config.build_litepb = [](auto& msg) {
        msg.status = litepb_gen::litepb::test::interop::NestedEnumTest::Status::ACTIVE;
        msg.code = 42;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(litepb::test::interop::NestedEnumTest_Status_ACTIVE, msg.status());
        TEST_ASSERT_EQUAL_INT32(42, msg.code());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_status(litepb::test::interop::NestedEnumTest_Status_INACTIVE);
        msg.set_code(100);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(litepb_gen::litepb::test::interop::NestedEnumTest::Status::INACTIVE, msg.status);
        TEST_ASSERT_EQUAL_INT32(100, msg.code);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_status(litepb::test::interop::NestedEnumTest_Status_ACTIVE);
        msg.set_code(42);
    };
    
    run_round_trip(config);
}

void test_large_field_numbers()
{
    RoundTripConfig<litepb_gen::litepb::test::interop::LargeFieldNumbers, litepb::test::interop::LargeFieldNumbers> config;
    
    config.build_litepb = [](auto& msg) {
        msg.field_1 = 1;
        msg.field_100 = 100;
        msg.field_1000 = 1000;
        msg.field_10000 = 10000;
    };
    
    config.verify_protoc = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(1, msg.field_1());
        TEST_ASSERT_EQUAL_INT32(100, msg.field_100());
        TEST_ASSERT_EQUAL_INT32(1000, msg.field_1000());
        TEST_ASSERT_EQUAL_INT32(10000, msg.field_10000());
    };
    
    config.build_protoc = [](auto& msg) {
        msg.set_field_1(11);
        msg.set_field_100(110);
        msg.set_field_1000(1100);
        msg.set_field_10000(11000);
    };
    
    config.verify_litepb = [](const auto& msg) {
        TEST_ASSERT_EQUAL_INT32(11, msg.field_1);
        TEST_ASSERT_EQUAL_INT32(110, msg.field_100);
        TEST_ASSERT_EQUAL_INT32(1100, msg.field_1000);
        TEST_ASSERT_EQUAL_INT32(11000, msg.field_10000);
    };
    
    config.build_protoc_baseline = [](auto& msg) {
        msg.set_field_1(1);
        msg.set_field_100(100);
        msg.set_field_1000(1000);
        msg.set_field_10000(10000);
    };
    
    run_round_trip(config);
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
