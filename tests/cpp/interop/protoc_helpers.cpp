#include "protoc_helpers.h"
#include "protoc/test_vectors.pb.h"
#include <memory>

std::vector<uint8_t> protoc_encode_basic_scalars(int32_t int32_field, const std::string& string_field)
{
    auto msg = std::make_unique<litepb::test::interop::BasicScalars>();
    msg->set_int32_field(int32_field);
    msg->set_string_field(string_field);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_basic_scalars(const std::vector<uint8_t>& buffer, int32_t& int32_field, std::string& string_field)
{
    auto msg = std::make_unique<litepb::test::interop::BasicScalars>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    int32_field  = msg->int32_field();
    string_field = msg->string_field();

    return true;
}

std::vector<uint8_t> protoc_encode_all_scalars(const AllScalarsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::AllScalars>();
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
    auto msg = std::make_unique<litepb::test::interop::AllScalars>();
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
    auto msg = std::make_unique<litepb::test::interop::SignedInts>();
    msg->set_v_sint32(data.v_sint32);
    msg->set_v_sint64(data.v_sint64);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_signed_ints(const std::vector<uint8_t>& buffer, SignedIntsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::SignedInts>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.v_sint32 = msg->v_sint32();
    data.v_sint64 = msg->v_sint64();

    return true;
}

std::vector<uint8_t> protoc_encode_fixed_types(const FixedTypesData& data)
{
    auto msg = std::make_unique<litepb::test::interop::FixedTypes>();
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
    auto msg = std::make_unique<litepb::test::interop::FixedTypes>();
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
    auto msg = std::make_unique<litepb::test::interop::RepeatedFields>();
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
    auto msg = std::make_unique<litepb::test::interop::RepeatedFields>();
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
    auto msg = std::make_unique<litepb::test::interop::MapField>();
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
    auto msg = std::make_unique<litepb::test::interop::MapField>();
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
    auto msg    = std::make_unique<litepb::test::interop::Outer>();
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
    auto msg = std::make_unique<litepb::test::interop::Outer>();
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
    auto msg = std::make_unique<litepb::test::interop::OneofTest>();
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
    auto msg = std::make_unique<litepb::test::interop::OneofTest>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    if (msg->choice_case() == litepb::test::interop::OneofTest::kIntValue) {
        data.is_int_value = true;
        data.int_value    = msg->int_value();
    }
    else if (msg->choice_case() == litepb::test::interop::OneofTest::kStringValue) {
        data.is_int_value = false;
        data.string_value = msg->string_value();
    }

    return true;
}

std::vector<uint8_t> protoc_encode_enum_test(const EnumTestData& data)
{
    auto msg = std::make_unique<litepb::test::interop::EnumTest>();
    msg->set_value(static_cast<litepb::test::interop::TestEnum>(data.value));

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_enum_test(const std::vector<uint8_t>& buffer, EnumTestData& data)
{
    auto msg = std::make_unique<litepb::test::interop::EnumTest>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.value = static_cast<int32_t>(msg->value());

    return true;
}

std::vector<uint8_t> protoc_encode_optional_fields(const OptionalFieldsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::OptionalFields>();
    if (data.has_opt_int32) {
        msg->set_opt_int32(data.opt_int32);
    }
    if (data.has_opt_string) {
        msg->set_opt_string(data.opt_string);
    }
    if (data.has_opt_bool) {
        msg->set_opt_bool(data.opt_bool);
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_optional_fields(const std::vector<uint8_t>& buffer, OptionalFieldsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::OptionalFields>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.has_opt_int32 = msg->has_opt_int32();
    if (data.has_opt_int32) {
        data.opt_int32 = msg->opt_int32();
    }
    data.has_opt_string = msg->has_opt_string();
    if (data.has_opt_string) {
        data.opt_string = msg->opt_string();
    }
    data.has_opt_bool = msg->has_opt_bool();
    if (data.has_opt_bool) {
        data.opt_bool = msg->opt_bool();
    }

    return true;
}

std::vector<uint8_t> protoc_encode_empty_message(const EmptyMessageData& data)
{
    (void) data;
    auto msg = std::make_unique<litepb::test::interop::EmptyMessage>();

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_empty_message(const std::vector<uint8_t>& buffer, EmptyMessageData& data)
{
    (void) data;
    auto msg = std::make_unique<litepb::test::interop::EmptyMessage>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    return true;
}

std::vector<uint8_t> protoc_encode_unpacked_repeated(const UnpackedRepeatedData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnpackedRepeated>();
    for (const auto& v : data.unpacked_ints) {
        msg->add_unpacked_ints(v);
    }
    for (const auto& v : data.unpacked_floats) {
        msg->add_unpacked_floats(v);
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_unpacked_repeated(const std::vector<uint8_t>& buffer, UnpackedRepeatedData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnpackedRepeated>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.unpacked_ints.clear();
    for (int i = 0; i < msg->unpacked_ints_size(); i++) {
        data.unpacked_ints.push_back(msg->unpacked_ints(i));
    }

    data.unpacked_floats.clear();
    for (int i = 0; i < msg->unpacked_floats_size(); i++) {
        data.unpacked_floats.push_back(msg->unpacked_floats(i));
    }

    return true;
}

std::vector<uint8_t> protoc_encode_different_maps(const DifferentMapsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::DifferentMaps>();
    for (const auto& kv : data.int_to_string) {
        (*msg->mutable_int_to_string())[kv.first] = kv.second;
    }
    for (const auto& kv : data.string_to_string) {
        (*msg->mutable_string_to_string())[kv.first] = kv.second;
    }
    for (const auto& kv : data.int_to_int) {
        (*msg->mutable_int_to_int())[kv.first] = kv.second;
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_different_maps(const std::vector<uint8_t>& buffer, DifferentMapsData& data)
{
    auto msg = std::make_unique<litepb::test::interop::DifferentMaps>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.int_to_string.clear();
    for (const auto& kv : msg->int_to_string()) {
        data.int_to_string[kv.first] = kv.second;
    }

    data.string_to_string.clear();
    for (const auto& kv : msg->string_to_string()) {
        data.string_to_string[kv.first] = kv.second;
    }

    data.int_to_int.clear();
    for (const auto& kv : msg->int_to_int()) {
        data.int_to_int[kv.first] = kv.second;
    }

    return true;
}

std::vector<uint8_t> protoc_encode_repeated_messages(const RepeatedMessagesData& data)
{
    auto msg = std::make_unique<litepb::test::interop::RepeatedMessages>();
    for (const auto& item : data.items) {
        auto* item_msg = msg->add_items();
        item_msg->set_id(item.id);
        item_msg->set_name(item.name);
    }

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_repeated_messages(const std::vector<uint8_t>& buffer, RepeatedMessagesData& data)
{
    auto msg = std::make_unique<litepb::test::interop::RepeatedMessages>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.items.clear();
    for (int i = 0; i < msg->items_size(); i++) {
        ItemData item;
        item.id   = msg->items(i).id();
        item.name = msg->items(i).name();
        data.items.push_back(item);
    }

    return true;
}

std::vector<uint8_t> protoc_encode_nested_enum_test(const NestedEnumTestData& data)
{
    auto msg = std::make_unique<litepb::test::interop::NestedEnumTest>();
    msg->set_status(static_cast<litepb::test::interop::NestedEnumTest::Status>(data.status));
    msg->set_code(data.code);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_nested_enum_test(const std::vector<uint8_t>& buffer, NestedEnumTestData& data)
{
    auto msg = std::make_unique<litepb::test::interop::NestedEnumTest>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.status = static_cast<int32_t>(msg->status());
    data.code   = msg->code();

    return true;
}

std::vector<uint8_t> protoc_encode_large_field_numbers(const LargeFieldNumbersData& data)
{
    auto msg = std::make_unique<litepb::test::interop::LargeFieldNumbers>();
    msg->set_field_1(data.field_1);
    msg->set_field_100(data.field_100);
    msg->set_field_1000(data.field_1000);
    msg->set_field_10000(data.field_10000);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_large_field_numbers(const std::vector<uint8_t>& buffer, LargeFieldNumbersData& data)
{
    auto msg = std::make_unique<litepb::test::interop::LargeFieldNumbers>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.field_1     = msg->field_1();
    data.field_100   = msg->field_100();
    data.field_1000  = msg->field_1000();
    data.field_10000 = msg->field_10000();

    return true;
}

std::vector<uint8_t> protoc_encode_unknown_fields_new(const UnknownFieldsNewData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnknownFieldsNew>();
    msg->set_known_field(data.known_field);
    msg->set_extra_field(data.extra_field);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_unknown_fields_new(const std::vector<uint8_t>& buffer, UnknownFieldsNewData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnknownFieldsNew>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.known_field = msg->known_field();
    data.extra_field = msg->extra_field();

    return true;
}

std::vector<uint8_t> protoc_encode_unknown_fields_old(const UnknownFieldsOldData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnknownFieldsOld>();
    msg->set_known_field(data.known_field);

    size_t size = msg->ByteSizeLong();
    std::vector<uint8_t> buffer(size);
    msg->SerializeToArray(buffer.data(), size);
    return buffer;
}

bool protoc_decode_unknown_fields_old(const std::vector<uint8_t>& buffer, UnknownFieldsOldData& data)
{
    auto msg = std::make_unique<litepb::test::interop::UnknownFieldsOld>();
    if (!msg->ParseFromArray(buffer.data(), buffer.size())) {
        return false;
    }

    data.known_field = msg->known_field();

    return true;
}
