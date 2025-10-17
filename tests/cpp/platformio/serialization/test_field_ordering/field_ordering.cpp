#include "unity.h"
#include "litepb/litepb.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/proto_reader.h"
#include "litepb/core/streams.h"
#include <string>
#include <vector>

using namespace litepb;

void setUp() {}
void tearDown() {}

// Test message with various field types
struct FieldOrderingTestMessage {
    int32_t scalar_int32 = 0;
    std::string scalar_string = "";
    float scalar_float = 0.0f;
    bool scalar_bool = false;
    std::vector<int32_t> repeated_int32;
    std::vector<std::string> repeated_string;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
};

// Nested message for testing message merging
struct NestedMessage {
    int32_t nested_field1 = 0;
    int32_t nested_field2 = 0;
    
    litepb::UnknownFieldSet unknown_fields;
};

struct MessageWithNested {
    int32_t field1 = 0;
    NestedMessage nested;
    
    litepb::UnknownFieldSet unknown_fields;
};

// Manual serializers for testing
template<>
class litepb::Serializer<FieldOrderingTestMessage> {
public:
    static bool serialize(const FieldOrderingTestMessage& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        if (value.scalar_int32 != 0) {
            writer.write_tag(1, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.scalar_int32);
        }
        
        if (!value.scalar_string.empty()) {
            writer.write_tag(2, litepb::WIRE_TYPE_LENGTH_DELIMITED);
            writer.write_string(value.scalar_string);
        }
        
        if (value.scalar_float != 0.0f) {
            writer.write_tag(3, litepb::WIRE_TYPE_FIXED32);
            writer.write_float(value.scalar_float);
        }
        
        if (value.scalar_bool) {
            writer.write_tag(4, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.scalar_bool ? 1 : 0);
        }
        
        for (const auto& item : value.repeated_int32) {
            writer.write_tag(5, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(item);
        }
        
        for (const auto& item : value.repeated_string) {
            writer.write_tag(6, litepb::WIRE_TYPE_LENGTH_DELIMITED);
            writer.write_string(item);
        }
        
        if (!value.unknown_fields.empty()) {
            if (!value.unknown_fields.serialize_to(stream)) return false;
        }
        
        return true;
    }
    
    static bool parse(FieldOrderingTestMessage& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            switch (field_number) {
                case 1: {
                    // Scalar int32 - last value wins
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.scalar_int32 = static_cast<int32_t>(temp_varint);
                    break;
                }
                case 2: {
                    // Scalar string - last value wins
                    if (!reader.read_string(value.scalar_string)) return false;
                    break;
                }
                case 3: {
                    // Scalar float - last value wins
                    if (!reader.read_float(value.scalar_float)) return false;
                    break;
                }
                case 4: {
                    // Scalar bool - last value wins
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.scalar_bool = (temp_varint != 0);
                    break;
                }
                case 5: {
                    // Repeated int32 - append all values
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.repeated_int32.push_back(static_cast<int32_t>(temp_varint));
                    break;
                }
                case 6: {
                    // Repeated string - append all values
                    std::string temp;
                    if (!reader.read_string(temp)) return false;
                    value.repeated_string.push_back(std::move(temp));
                    break;
                }
                default: {
                    // Capture unknown field for forward/backward compatibility
                    std::vector<uint8_t> unknown_data;
                    if (!reader.capture_unknown_field(wire_type, unknown_data)) return false;
                    
                    // Store in UnknownFieldSet based on wire type
                    switch (wire_type) {
                        case litepb::WIRE_TYPE_VARINT: {
                            uint64_t varint_value = 0;
                            size_t shift = 0;
                            for (size_t i = 0; i < unknown_data.size() && i < 10; ++i) {
                                varint_value |= static_cast<uint64_t>(unknown_data[i] & 0x7F) << shift;
                                if ((unknown_data[i] & 0x80) == 0) break;
                                shift += 7;
                            }
                            value.unknown_fields.add_varint(field_number, varint_value);
                            break;
                        }
                        case litepb::WIRE_TYPE_FIXED32: {
                            if (unknown_data.size() >= 4) {
                                uint32_t fixed32_value = 
                                    static_cast<uint32_t>(unknown_data[0]) |
                                    (static_cast<uint32_t>(unknown_data[1]) << 8) |
                                    (static_cast<uint32_t>(unknown_data[2]) << 16) |
                                    (static_cast<uint32_t>(unknown_data[3]) << 24);
                                value.unknown_fields.add_fixed32(field_number, fixed32_value);
                            }
                            break;
                        }
                        case litepb::WIRE_TYPE_FIXED64: {
                            if (unknown_data.size() >= 8) {
                                uint64_t fixed64_value = 0;
                                for (int i = 0; i < 8; ++i) {
                                    fixed64_value |= static_cast<uint64_t>(unknown_data[i]) << (i * 8);
                                }
                                value.unknown_fields.add_fixed64(field_number, fixed64_value);
                            }
                            break;
                        }
                        case litepb::WIRE_TYPE_LENGTH_DELIMITED: {
                            size_t pos = 0;
                            uint64_t len = 0;
                            int shift = 0;
                            while (pos < unknown_data.size() && pos < 10) {
                                len |= static_cast<uint64_t>(unknown_data[pos] & 0x7F) << shift;
                                if ((unknown_data[pos] & 0x80) == 0) {
                                    pos++;
                                    break;
                                }
                                shift += 7;
                                pos++;
                            }
                            if (pos < unknown_data.size()) {
                                value.unknown_fields.add_length_delimited(field_number, 
                                    unknown_data.data() + pos, unknown_data.size() - pos);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
            }
        }
        return true;
    }
};

// Serializer for NestedMessage
template<>
class litepb::Serializer<NestedMessage> {
public:
    static bool serialize(const NestedMessage& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        if (value.nested_field1 != 0) {
            writer.write_tag(1, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.nested_field1);
        }
        
        if (value.nested_field2 != 0) {
            writer.write_tag(2, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.nested_field2);
        }
        
        if (!value.unknown_fields.empty()) {
            if (!value.unknown_fields.serialize_to(stream)) return false;
        }
        
        return true;
    }
    
    static bool parse(NestedMessage& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            switch (field_number) {
                case 1: {
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.nested_field1 = static_cast<int32_t>(temp_varint);
                    break;
                }
                case 2: {
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.nested_field2 = static_cast<int32_t>(temp_varint);
                    break;
                }
                default: {
                    reader.skip_field(wire_type);
                    break;
                }
            }
        }
        return true;
    }
};

// Serializer for MessageWithNested
template<>
class litepb::Serializer<MessageWithNested> {
public:
    static bool serialize(const MessageWithNested& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        if (value.field1 != 0) {
            writer.write_tag(1, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.field1);
        }
        
        // For message fields, always write them (even if empty)
        {
            writer.write_tag(2, litepb::WIRE_TYPE_LENGTH_DELIMITED);
            litepb::BufferOutputStream temp_stream;
            if (!litepb::Serializer<NestedMessage>::serialize(value.nested, temp_stream)) return false;
            writer.write_varint(temp_stream.size());
            stream.write(temp_stream.data(), temp_stream.size());
        }
        
        if (!value.unknown_fields.empty()) {
            if (!value.unknown_fields.serialize_to(stream)) return false;
        }
        
        return true;
    }
    
    static bool parse(MessageWithNested& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            switch (field_number) {
                case 1: {
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.field1 = static_cast<int32_t>(temp_varint);
                    break;
                }
                case 2: {
                    // Message field - merge semantics
                    uint64_t size;
                    if (!reader.read_varint(size)) return false;
                    
                    // Create a limited stream for parsing the nested message
                    std::vector<uint8_t> buffer(size);
                    if (!stream.read(buffer.data(), size)) return false;
                    
                    BufferInputStream nested_stream(buffer.data(), size);
                    
                    // Parse into a temporary message for merging
                    NestedMessage temp_nested;
                    if (!litepb::Serializer<NestedMessage>::parse(temp_nested, nested_stream)) return false;
                    
                    // Merge: for scalar fields in messages, last value wins
                    // If temp_nested has non-default values, they override existing values
                    if (temp_nested.nested_field1 != 0) {
                        value.nested.nested_field1 = temp_nested.nested_field1;
                    }
                    if (temp_nested.nested_field2 != 0) {
                        value.nested.nested_field2 = temp_nested.nested_field2;
                    }
                    
                    break;
                }
                default: {
                    reader.skip_field(wire_type);
                    break;
                }
            }
        }
        return true;
    }
};

// Test: Fields can arrive in any order
void test_fields_out_of_order() {
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Write fields in reverse order (6, 5, 4, 3, 2, 1)
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("repeated2");
    
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(100);
    
    writer.write_tag(4, WIRE_TYPE_VARINT);
    writer.write_varint(1);  // true
    
    writer.write_tag(3, WIRE_TYPE_FIXED32);
    writer.write_float(3.14f);
    
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("test");
    
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(42);
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("repeated1");
    
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(200);
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    FieldOrderingTestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify all fields were read correctly despite being out of order
    TEST_ASSERT_EQUAL_INT32(42, msg.scalar_int32);
    TEST_ASSERT_EQUAL_STRING("test", msg.scalar_string.c_str());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.14f, msg.scalar_float);
    TEST_ASSERT_TRUE(msg.scalar_bool);
    
    // Repeated fields should preserve order of appearance
    TEST_ASSERT_EQUAL(2, msg.repeated_int32.size());
    TEST_ASSERT_EQUAL_INT32(100, msg.repeated_int32[0]);
    TEST_ASSERT_EQUAL_INT32(200, msg.repeated_int32[1]);
    
    TEST_ASSERT_EQUAL(2, msg.repeated_string.size());
    TEST_ASSERT_EQUAL_STRING("repeated2", msg.repeated_string[0].c_str());
    TEST_ASSERT_EQUAL_STRING("repeated1", msg.repeated_string[1].c_str());
}

// Test: For scalar fields, last value wins
void test_scalar_field_last_value_wins() {
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Write the same scalar field multiple times
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(10);
    
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(20);
    
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(30);
    
    // Same for string field
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("first");
    
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("second");
    
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("last");
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    FieldOrderingTestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify last value wins for scalar fields
    TEST_ASSERT_EQUAL_INT32(30, msg.scalar_int32);
    TEST_ASSERT_EQUAL_STRING("last", msg.scalar_string.c_str());
}

// Test: For repeated fields, all values are appended
void test_repeated_field_append_all() {
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Write repeated field values intermixed with other fields
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(10);
    
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(999);
    
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(20);
    
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(30);
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("str1");
    
    writer.write_tag(5, WIRE_TYPE_VARINT);
    writer.write_varint(40);
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("str2");
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("str3");
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    FieldOrderingTestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify all repeated values were appended in order
    TEST_ASSERT_EQUAL(4, msg.repeated_int32.size());
    TEST_ASSERT_EQUAL_INT32(10, msg.repeated_int32[0]);
    TEST_ASSERT_EQUAL_INT32(20, msg.repeated_int32[1]);
    TEST_ASSERT_EQUAL_INT32(30, msg.repeated_int32[2]);
    TEST_ASSERT_EQUAL_INT32(40, msg.repeated_int32[3]);
    
    TEST_ASSERT_EQUAL(3, msg.repeated_string.size());
    TEST_ASSERT_EQUAL_STRING("str1", msg.repeated_string[0].c_str());
    TEST_ASSERT_EQUAL_STRING("str2", msg.repeated_string[1].c_str());
    TEST_ASSERT_EQUAL_STRING("str3", msg.repeated_string[2].c_str());
    
    // Scalar field should have its value
    TEST_ASSERT_EQUAL_INT32(999, msg.scalar_int32);
}

// Test: Message fields merge correctly
void test_message_field_merging() {
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Write field1
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(100);
    
    // Write first nested message with only nested_field1
    {
        BufferOutputStream nested_output;
        ProtoWriter nested_writer(nested_output);
        nested_writer.write_tag(1, WIRE_TYPE_VARINT);
        nested_writer.write_varint(10);
        
        writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
        writer.write_varint(nested_output.size());
        output.write(nested_output.data(), nested_output.size());
    }
    
    // Write second nested message with only nested_field2
    {
        BufferOutputStream nested_output;
        ProtoWriter nested_writer(nested_output);
        nested_writer.write_tag(2, WIRE_TYPE_VARINT);
        nested_writer.write_varint(20);
        
        writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
        writer.write_varint(nested_output.size());
        output.write(nested_output.data(), nested_output.size());
    }
    
    // Write third nested message overriding nested_field1
    {
        BufferOutputStream nested_output;
        ProtoWriter nested_writer(nested_output);
        nested_writer.write_tag(1, WIRE_TYPE_VARINT);
        nested_writer.write_varint(30);
        
        writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);
        writer.write_varint(nested_output.size());
        output.write(nested_output.data(), nested_output.size());
    }
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    MessageWithNested msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify message field merging:
    // - nested_field1 should be 30 (last value wins)
    // - nested_field2 should be 20 (set in second message)
    TEST_ASSERT_EQUAL_INT32(100, msg.field1);
    TEST_ASSERT_EQUAL_INT32(30, msg.nested.nested_field1);
    TEST_ASSERT_EQUAL_INT32(20, msg.nested.nested_field2);
}

// Test: Mixed order with duplicate fields
void test_mixed_order_and_duplicates() {
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Write a complex mix of fields in random order with duplicates
    writer.write_tag(5, WIRE_TYPE_VARINT);  // repeated_int32
    writer.write_varint(1);
    
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);  // scalar_string (first)
    writer.write_string("initial");
    
    writer.write_tag(5, WIRE_TYPE_VARINT);  // repeated_int32
    writer.write_varint(2);
    
    writer.write_tag(1, WIRE_TYPE_VARINT);  // scalar_int32 (first)
    writer.write_varint(100);
    
    writer.write_tag(3, WIRE_TYPE_FIXED32);  // scalar_float (first)
    writer.write_float(1.0f);
    
    writer.write_tag(5, WIRE_TYPE_VARINT);  // repeated_int32
    writer.write_varint(3);
    
    writer.write_tag(1, WIRE_TYPE_VARINT);  // scalar_int32 (second - should win)
    writer.write_varint(200);
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);  // repeated_string
    writer.write_string("a");
    
    writer.write_tag(3, WIRE_TYPE_FIXED32);  // scalar_float (second - should win)
    writer.write_float(2.0f);
    
    writer.write_tag(6, WIRE_TYPE_LENGTH_DELIMITED);  // repeated_string
    writer.write_string("b");
    
    writer.write_tag(2, WIRE_TYPE_LENGTH_DELIMITED);  // scalar_string (second - should win)
    writer.write_string("final");
    
    writer.write_tag(4, WIRE_TYPE_VARINT);  // scalar_bool
    writer.write_varint(1);
    
    writer.write_tag(5, WIRE_TYPE_VARINT);  // repeated_int32
    writer.write_varint(4);
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    FieldOrderingTestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify scalar fields have last values
    TEST_ASSERT_EQUAL_INT32(200, msg.scalar_int32);
    TEST_ASSERT_EQUAL_STRING("final", msg.scalar_string.c_str());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, msg.scalar_float);
    TEST_ASSERT_TRUE(msg.scalar_bool);
    
    // Verify repeated fields have all values in order
    TEST_ASSERT_EQUAL(4, msg.repeated_int32.size());
    TEST_ASSERT_EQUAL_INT32(1, msg.repeated_int32[0]);
    TEST_ASSERT_EQUAL_INT32(2, msg.repeated_int32[1]);
    TEST_ASSERT_EQUAL_INT32(3, msg.repeated_int32[2]);
    TEST_ASSERT_EQUAL_INT32(4, msg.repeated_int32[3]);
    
    TEST_ASSERT_EQUAL(2, msg.repeated_string.size());
    TEST_ASSERT_EQUAL_STRING("a", msg.repeated_string[0].c_str());
    TEST_ASSERT_EQUAL_STRING("b", msg.repeated_string[1].c_str());
}

// Register tests for Unity framework
int runTests() {
    UNITY_BEGIN();
    RUN_TEST(test_fields_out_of_order);
    RUN_TEST(test_scalar_field_last_value_wins);
    RUN_TEST(test_repeated_field_append_all);
    RUN_TEST(test_message_field_merging);
    RUN_TEST(test_mixed_order_and_duplicates);
    return UNITY_END();
}