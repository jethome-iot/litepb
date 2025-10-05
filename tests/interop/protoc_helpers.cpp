#include "protoc_helpers.h"
#include "protoc/test_vectors_protoc.pb.h"
#include <memory>

std::vector<uint8_t> protoc_encode_basic_scalars(int32_t int32_field, const std::string& string_field)
{
    auto msg = std::make_unique<protoc_interop::BasicScalars>();
    msg->set_int32_field(int32_field);
    msg->set_string_field(string_field);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_basic_scalars(const std::vector<uint8_t>& buffer, int32_t& int32_field, std::string& string_field)
{
    auto msg = std::make_unique<protoc_interop::BasicScalars>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    int32_field  = msg->int32_field();
    string_field = msg->string_field();

    return true;
}

std::vector<uint8_t> protoc_encode_all_scalars(const AllScalarsData& data)
{
    auto msg = std::make_unique<protoc_interop::AllScalars>();
    msg->set_v_int32(data.v_int32);
    msg->set_v_int64(data.v_int64);
    msg->set_v_uint32(data.v_uint32);
    msg->set_v_uint64(data.v_uint64);
    msg->set_v_bool(data.v_bool);
    msg->set_v_string(data.v_string);
    msg->set_v_bytes(data.v_bytes.data(), data.v_bytes.size());
    msg->set_v_float(data.v_float);
    msg->set_v_double(data.v_double);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_all_scalars(const std::vector<uint8_t>& buffer, AllScalarsData& data)
{
    auto msg = std::make_unique<protoc_interop::AllScalars>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.v_int32  = msg->v_int32();
    data.v_int64  = msg->v_int64();
    data.v_uint32 = msg->v_uint32();
    data.v_uint64 = msg->v_uint64();
    data.v_bool   = msg->v_bool();
    data.v_string = msg->v_string();
    data.v_bytes.assign(msg->v_bytes().begin(), msg->v_bytes().end());
    data.v_float  = msg->v_float();
    data.v_double = msg->v_double();

    return true;
}

std::vector<uint8_t> protoc_encode_signed_ints(const SignedIntsData& data)
{
    auto msg = std::make_unique<protoc_interop::SignedInts>();
    msg->set_v_sint32(data.v_sint32);
    msg->set_v_sint64(data.v_sint64);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_signed_ints(const std::vector<uint8_t>& buffer, SignedIntsData& data)
{
    auto msg = std::make_unique<protoc_interop::SignedInts>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.v_sint32 = msg->v_sint32();
    data.v_sint64 = msg->v_sint64();

    return true;
}

std::vector<uint8_t> protoc_encode_fixed_types(const FixedTypesData& data)
{
    auto msg = std::make_unique<protoc_interop::FixedTypes>();
    msg->set_v_fixed32(data.v_fixed32);
    msg->set_v_fixed64(data.v_fixed64);
    msg->set_v_sfixed32(data.v_sfixed32);
    msg->set_v_sfixed64(data.v_sfixed64);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_fixed_types(const std::vector<uint8_t>& buffer, FixedTypesData& data)
{
    auto msg = std::make_unique<protoc_interop::FixedTypes>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.v_fixed32  = msg->v_fixed32();
    data.v_fixed64  = msg->v_fixed64();
    data.v_sfixed32 = msg->v_sfixed32();
    data.v_sfixed64 = msg->v_sfixed64();

    return true;
}

std::vector<uint8_t> protoc_encode_repeated_fields(const RepeatedFieldsData& data)
{
    auto msg = std::make_unique<protoc_interop::RepeatedFields>();
    for (const auto& v : data.packed_ints) {
        msg->add_packed_ints(v);
    }
    for (const auto& s : data.strings) {
        msg->add_strings(s);
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_repeated_fields(const std::vector<uint8_t>& buffer, RepeatedFieldsData& data)
{
    auto msg = std::make_unique<protoc_interop::RepeatedFields>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.packed_ints.clear();
    for (int i = 0; i < msg->packed_ints_size(); i++) {
        data.packed_ints.push_back(msg->packed_ints(i));
    }

    data.strings.clear();
    for (int i = 0; i < msg->strings_size(); i++) {
        data.strings.push_back(msg->strings(i));
    }

    return true;
}

std::vector<uint8_t> protoc_encode_map_field(const MapFieldData& data)
{
    auto msg = std::make_unique<protoc_interop::MapField>();
    for (const auto& kv : data.items) {
        (*msg->mutable_items())[kv.first] = kv.second;
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_map_field(const std::vector<uint8_t>& buffer, MapFieldData& data)
{
    auto msg = std::make_unique<protoc_interop::MapField>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.items.clear();
    for (const auto& kv : msg->items()) {
        data.items[kv.first] = kv.second;
    }

    return true;
}

std::vector<uint8_t> protoc_encode_outer(const OuterData& data)
{
    auto msg    = std::make_unique<protoc_interop::Outer>();
    auto* inner = msg->mutable_inner();
    inner->set_value(data.inner_value);
    msg->set_name(data.name);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_outer(const std::vector<uint8_t>& buffer, OuterData& data)
{
    auto msg = std::make_unique<protoc_interop::Outer>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    if (msg->has_inner()) {
        data.inner_value = msg->inner().value();
    }
    data.name = msg->name();

    return true;
}

std::vector<uint8_t> protoc_encode_oneof_test(const OneofTestData& data)
{
    auto msg = std::make_unique<protoc_interop::OneofTest>();
    if (data.is_int_value) {
        msg->set_int_value(data.int_value);
    }
    else {
        msg->set_string_value(data.string_value);
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_oneof_test(const std::vector<uint8_t>& buffer, OneofTestData& data)
{
    auto msg = std::make_unique<protoc_interop::OneofTest>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    if (msg->choice_case() == protoc_interop::OneofTest::kIntValue) {
        data.is_int_value = true;
        data.int_value    = msg->int_value();
    }
    else if (msg->choice_case() == protoc_interop::OneofTest::kStringValue) {
        data.is_int_value = false;
        data.string_value = msg->string_value();
    }

    return true;
}

std::vector<uint8_t> protoc_encode_enum_test(const EnumTestData& data)
{
    auto msg = std::make_unique<protoc_interop::EnumTest>();
    msg->set_value(static_cast<protoc_interop::TestEnum>(data.value));

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_enum_test(const std::vector<uint8_t>& buffer, EnumTestData& data)
{
    auto msg = std::make_unique<protoc_interop::EnumTest>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.value = static_cast<int32_t>(msg->value());

    return true;
}
