/**
 * @file well_known_types_serializers.h
 * @brief Serializer specializations for Google's well-known types
 *
 * This header provides serializer specializations for well-known types,
 * enabling them to be serialized and deserialized using the LitePB framework.
 *
 * @copyright Copyright (c) 2025
 * @license MIT License
 */

#pragma once

#include "litepb/litepb.h"
#include "litepb/well_known_types.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/proto_reader.h"

namespace litepb {

// Forward declarations
template<> class Serializer<::google::protobuf::Empty>;
template<> class Serializer<::google::protobuf::Timestamp>;
template<> class Serializer<::google::protobuf::Duration>;
template<> class Serializer<::google::protobuf::StringValue>;
template<> class Serializer<::google::protobuf::Int32Value>;
template<> class Serializer<::google::protobuf::Int64Value>;
template<> class Serializer<::google::protobuf::UInt32Value>;
template<> class Serializer<::google::protobuf::UInt64Value>;
template<> class Serializer<::google::protobuf::FloatValue>;
template<> class Serializer<::google::protobuf::DoubleValue>;
template<> class Serializer<::google::protobuf::BoolValue>;
template<> class Serializer<::google::protobuf::BytesValue>;
template<> class Serializer<::google::protobuf::Any>;

// Empty serializer
template<>
class Serializer<::google::protobuf::Empty> {
public:
    inline static bool serialize(const ::google::protobuf::Empty& value, litepb::OutputStream& stream) {
        // Write unknown fields
        return value.unknown_fields.serialize_to(stream);
    }
    
    inline static bool parse(::google::protobuf::Empty& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            // All fields are unknown for Empty
            if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Empty& value) {
        size_t size = 0;
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// Timestamp serializer
template<>
class Serializer<::google::protobuf::Timestamp> {
public:
    inline static bool serialize(const ::google::protobuf::Timestamp& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: seconds (int64)
        if (value.seconds != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.seconds))) return false;
        }
        
        // Field 2: nanos (int32)
        if (value.nanos != 0) {
            if (!writer.write_tag(2, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.nanos))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::Timestamp& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            switch (field_number) {
                case 1: // seconds
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.seconds = static_cast<int64_t>(temp);
                    }
                    break;
                    
                case 2: // nanos
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.nanos = static_cast<int32_t>(temp);
                    }
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Timestamp& value) {
        size_t size = 0;
        
        if (value.seconds != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.seconds));
        }
        
        if (value.nanos != 0) {
            size += litepb::ProtoWriter::varint_size(((2) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.nanos));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// Duration serializer
template<>
class Serializer<::google::protobuf::Duration> {
public:
    inline static bool serialize(const ::google::protobuf::Duration& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: seconds (int64)
        if (value.seconds != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.seconds))) return false;
        }
        
        // Field 2: nanos (int32)
        if (value.nanos != 0) {
            if (!writer.write_tag(2, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.nanos))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::Duration& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // seconds
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.seconds = static_cast<int64_t>(temp);
                    }
                    break;
                    
                case 2: // nanos
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.nanos = static_cast<int32_t>(temp);
                    }
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Duration& value) {
        size_t size = 0;
        
        if (value.seconds != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.seconds));
        }
        
        if (value.nanos != 0) {
            size += litepb::ProtoWriter::varint_size(((2) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.nanos));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// StringValue serializer
template<>
class Serializer<::google::protobuf::StringValue> {
public:
    inline static bool serialize(const ::google::protobuf::StringValue& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (string)
        if (!value.value.empty()) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_LENGTH_DELIMITED)) return false;
            if (!writer.write_string(value.value)) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::StringValue& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_LENGTH_DELIMITED) return false;
                    if (!reader.read_string(value.value)) return false;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::StringValue& value) {
        size_t size = 0;
        
        if (!value.value.empty()) {
            size += litepb::ProtoWriter::string_size(1, value.value);
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// Int32Value serializer
template<>
class Serializer<::google::protobuf::Int32Value> {
public:
    inline static bool serialize(const ::google::protobuf::Int32Value& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (int32)
        if (value.value != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.value))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::Int32Value& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    uint64_t value_64;
                    if (!reader.read_varint(value_64)) return false;
                    value.value = static_cast<int32_t>(value_64);
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Int32Value& value) {
        size_t size = 0;
        
        if (value.value != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.value));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// Int64Value serializer
template<>
class Serializer<::google::protobuf::Int64Value> {
public:
    inline static bool serialize(const ::google::protobuf::Int64Value& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (int64)
        if (value.value != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.value))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::Int64Value& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.value = static_cast<int32_t>(temp);
                    }
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Int64Value& value) {
        size_t size = 0;
        
        if (value.value != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.value));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// UInt32Value serializer
template<>
class Serializer<::google::protobuf::UInt32Value> {
public:
    inline static bool serialize(const ::google::protobuf::UInt32Value& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (uint32)
        if (value.value != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(static_cast<uint32_t>(value.value)))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::UInt32Value& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    uuint64_t value_64;
                    if (!reader.read_varint(value_64)) return false;
                    value.value = static_cast<uint32_t>(value_64);
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::UInt32Value& value) {
        size_t size = 0;
        
        if (value.value != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.value));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// UInt64Value serializer
template<>
class Serializer<::google::protobuf::UInt64Value> {
public:
    inline static bool serialize(const ::google::protobuf::UInt64Value& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (uint64)
        if (value.value != 0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(static_cast<uint64_t>(value.value)))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::UInt64Value& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    {
                        uint64_t temp;
                        if (!reader.read_varint(temp)) return false;
                        value.value = static_cast<int32_t>(temp);
                    }
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::UInt64Value& value) {
        size_t size = 0;
        
        if (value.value != 0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(static_cast<uint64_t>(value.value));
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// FloatValue serializer
template<>
class Serializer<::google::protobuf::FloatValue> {
public:
    inline static bool serialize(const ::google::protobuf::FloatValue& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (float)
        if (value.value != 0.0f) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_FIXED32)) return false;
            if (!writer.write_float(value.value)) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::FloatValue& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_FIXED32) return false;
                    if (!reader.read_float(value.value)) return false;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::FloatValue& value) {
        size_t size = 0;
        
        if (value.value != 0.0f) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 5) + litepb::ProtoWriter::fixed32_size();
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// DoubleValue serializer
template<>
class Serializer<::google::protobuf::DoubleValue> {
public:
    inline static bool serialize(const ::google::protobuf::DoubleValue& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (double)
        if (value.value != 0.0) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_FIXED64)) return false;
            if (!writer.write_double(value.value)) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::DoubleValue& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_FIXED64) return false;
                    if (!reader.read_double(value.value)) return false;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::DoubleValue& value) {
        size_t size = 0;
        
        if (value.value != 0.0) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 1) + litepb::ProtoWriter::fixed64_size();
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// BoolValue serializer
template<>
class Serializer<::google::protobuf::BoolValue> {
public:
    inline static bool serialize(const ::google::protobuf::BoolValue& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (bool)
        if (value.value) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_VARINT)) return false;
            if (!writer.write_varint(static_cast<uint64_t>(value.value ? 1 : 0))) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::BoolValue& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_VARINT) return false;
                    uint64_t bool_val;
                    if (!reader.read_varint(bool_val)) return false;
                    value.value = bool_val != 0;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::BoolValue& value) {
        size_t size = 0;
        
        if (value.value) {
            size += litepb::ProtoWriter::varint_size(((1) << 3) | 0) + litepb::ProtoWriter::varint_size(1);
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// BytesValue serializer
template<>
class Serializer<::google::protobuf::BytesValue> {
public:
    inline static bool serialize(const ::google::protobuf::BytesValue& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: value (bytes)
        if (!value.value.empty()) {
            if (!writer.write_bytes(1, value.value)) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::BytesValue& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // value
                    if (wire_type != litepb::WIRE_TYPE_LENGTH_DELIMITED) return false;
                    if (!reader.read_bytes(value.value)) return false;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::BytesValue& value) {
        size_t size = 0;
        
        if (!value.value.empty()) {
            size += litepb::ProtoWriter::bytes_size(1, value.value);
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

// Any serializer
template<>
class Serializer<::google::protobuf::Any> {
public:
    inline static bool serialize(const ::google::protobuf::Any& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Field 1: type_url (string)
        if (!value.type_url.empty()) {
            if (!writer.write_tag(1, litepb::WIRE_TYPE_LENGTH_DELIMITED)) return false;
            if (!writer.write_string(value.type_url)) return false;
        }
        
        // Field 2: value (bytes)
        if (!value.value.empty()) {
            if (!writer.write_bytes(2, value.value)) return false;
        }
        
        // Write unknown fields
        if (!value.unknown_fields.serialize_to(stream)) return false;
        
        return true;
    }
    
    inline static bool parse(::google::protobuf::Any& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            
            switch (field_number) {
                case 1: // type_url
                    if (wire_type != litepb::WIRE_TYPE_LENGTH_DELIMITED) return false;
                    if (!reader.read_string(value.type_url)) return false;
                    break;
                    
                case 2: // value
                    if (wire_type != litepb::WIRE_TYPE_LENGTH_DELIMITED) return false;
                    if (!reader.read_bytes(value.value)) return false;
                    break;
                    
                default:
                    if (!reader.skip_and_save(field_number, wire_type, value.unknown_fields)) return false;
                    break;
            }
        }
        
        return true;
    }
    
    inline static size_t byte_size(const ::google::protobuf::Any& value) {
        size_t size = 0;
        
        if (!value.type_url.empty()) {
            size += litepb::ProtoWriter::string_size(1, value.type_url);
        }
        
        if (!value.value.empty()) {
            size += litepb::ProtoWriter::bytes_size(2, value.value);
        }
        
        // Unknown fields size
        size += litepb::ProtoWriter::unknown_fields_size(value.unknown_fields);
        
        return size;
    }
};

} // namespace litepb