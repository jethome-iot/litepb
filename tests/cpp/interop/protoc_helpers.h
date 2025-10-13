#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

std::vector<uint8_t> protoc_encode_basic_scalars(int32_t int32_field, const std::string & string_field);
bool protoc_decode_basic_scalars(const std::vector<uint8_t> & buffer, int32_t & int32_field, std::string & string_field);

struct AllScalarsData {
    int32_t v_int32;
    int64_t v_int64;
    uint32_t v_uint32;
    uint64_t v_uint64;
    bool v_bool;
    std::string v_string;
    std::vector<uint8_t> v_bytes;
    float v_float;
    double v_double;
};

struct SignedIntsData {
    int32_t v_sint32;
    int64_t v_sint64;
};

struct FixedTypesData {
    uint32_t v_fixed32;
    uint64_t v_fixed64;
    int32_t v_sfixed32;
    int64_t v_sfixed64;
};

struct RepeatedFieldsData {
    std::vector<int32_t> packed_ints;
    std::vector<std::string> strings;
};

struct MapFieldData {
    std::map<std::string, int32_t> items;
};

struct OuterData {
    int32_t inner_value;
    std::string name;
};

struct OneofTestData {
    bool is_int_value;
    int32_t int_value;
    std::string string_value;
};

struct EnumTestData {
    int32_t value;
};

std::vector<uint8_t> protoc_encode_all_scalars(const AllScalarsData & data);
bool protoc_decode_all_scalars(const std::vector<uint8_t> & buffer, AllScalarsData & data);

std::vector<uint8_t> protoc_encode_signed_ints(const SignedIntsData & data);
bool protoc_decode_signed_ints(const std::vector<uint8_t> & buffer, SignedIntsData & data);

std::vector<uint8_t> protoc_encode_fixed_types(const FixedTypesData & data);
bool protoc_decode_fixed_types(const std::vector<uint8_t> & buffer, FixedTypesData & data);

std::vector<uint8_t> protoc_encode_repeated_fields(const RepeatedFieldsData & data);
bool protoc_decode_repeated_fields(const std::vector<uint8_t> & buffer, RepeatedFieldsData & data);

std::vector<uint8_t> protoc_encode_map_field(const MapFieldData & data);
bool protoc_decode_map_field(const std::vector<uint8_t> & buffer, MapFieldData & data);

std::vector<uint8_t> protoc_encode_outer(const OuterData & data);
bool protoc_decode_outer(const std::vector<uint8_t> & buffer, OuterData & data);

std::vector<uint8_t> protoc_encode_oneof_test(const OneofTestData & data);
bool protoc_decode_oneof_test(const std::vector<uint8_t> & buffer, OneofTestData & data);

std::vector<uint8_t> protoc_encode_enum_test(const EnumTestData & data);
bool protoc_decode_enum_test(const std::vector<uint8_t> & buffer, EnumTestData & data);

struct OptionalFieldsData {
    bool has_opt_int32;
    int32_t opt_int32;
    bool has_opt_string;
    std::string opt_string;
    bool has_opt_bool;
    bool opt_bool;
};

struct EmptyMessageData {
};

struct UnpackedRepeatedData {
    std::vector<int32_t> unpacked_ints;
    std::vector<float> unpacked_floats;
};

struct DifferentMapsData {
    std::map<int32_t, std::string> int_to_string;
    std::map<std::string, std::string> string_to_string;
    std::map<int64_t, int64_t> int_to_int;
};

struct ItemData {
    int32_t id;
    std::string name;
};

struct RepeatedMessagesData {
    std::vector<ItemData> items;
};

struct NestedEnumTestData {
    int32_t status;
    int32_t code;
};

struct LargeFieldNumbersData {
    int32_t field_1;
    int32_t field_100;
    int32_t field_1000;
    int32_t field_10000;
};

struct UnknownFieldsNewData {
    int32_t known_field;
    std::string extra_field;
};

struct UnknownFieldsOldData {
    int32_t known_field;
};

std::vector<uint8_t> protoc_encode_optional_fields(const OptionalFieldsData & data);
bool protoc_decode_optional_fields(const std::vector<uint8_t> & buffer, OptionalFieldsData & data);

std::vector<uint8_t> protoc_encode_empty_message(const EmptyMessageData & data);
bool protoc_decode_empty_message(const std::vector<uint8_t> & buffer, EmptyMessageData & data);

std::vector<uint8_t> protoc_encode_unpacked_repeated(const UnpackedRepeatedData & data);
bool protoc_decode_unpacked_repeated(const std::vector<uint8_t> & buffer, UnpackedRepeatedData & data);

std::vector<uint8_t> protoc_encode_different_maps(const DifferentMapsData & data);
bool protoc_decode_different_maps(const std::vector<uint8_t> & buffer, DifferentMapsData & data);

std::vector<uint8_t> protoc_encode_repeated_messages(const RepeatedMessagesData & data);
bool protoc_decode_repeated_messages(const std::vector<uint8_t> & buffer, RepeatedMessagesData & data);

std::vector<uint8_t> protoc_encode_nested_enum_test(const NestedEnumTestData & data);
bool protoc_decode_nested_enum_test(const std::vector<uint8_t> & buffer, NestedEnumTestData & data);

std::vector<uint8_t> protoc_encode_large_field_numbers(const LargeFieldNumbersData & data);
bool protoc_decode_large_field_numbers(const std::vector<uint8_t> & buffer, LargeFieldNumbersData & data);

std::vector<uint8_t> protoc_encode_unknown_fields_new(const UnknownFieldsNewData & data);
bool protoc_decode_unknown_fields_new(const std::vector<uint8_t> & buffer, UnknownFieldsNewData & data);

std::vector<uint8_t> protoc_encode_unknown_fields_old(const UnknownFieldsOldData & data);
bool protoc_decode_unknown_fields_old(const std::vector<uint8_t> & buffer, UnknownFieldsOldData & data);
