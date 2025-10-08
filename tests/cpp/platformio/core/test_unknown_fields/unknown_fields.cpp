#include <unity.h>
#include "litepb/core/unknown_fields.h"
#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include <cstring>

using namespace litepb;

void setUp() {}
void tearDown() {}

// Test varint field preservation
void test_unknown_varint_field() {
    UnknownFieldSet unknown_fields;
    
    // Add a varint field
    unknown_fields.add_varint(10, 12345);
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Verify the serialized data
    // Should contain: tag (field 10, wire type 0) = 0x50, varint 12345
    const uint8_t* data = output.data();
    TEST_ASSERT_EQUAL_UINT8(0x50, data[0]);  // (10 << 3) | 0
    
    // Parse back and verify
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(10, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_VARINT, wire_type);
    
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(12345, value);
}

// Test fixed32 field preservation
void test_unknown_fixed32_field() {
    UnknownFieldSet unknown_fields;
    
    // Add a fixed32 field
    unknown_fields.add_fixed32(20, 0x12345678);
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Verify the serialized data
    // Should contain: tag (field 20, wire type 5) = 0xA5, fixed32 value
    const uint8_t* data = output.data();
    TEST_ASSERT_EQUAL_UINT8(0xA5, data[0]);  // (20 << 3) | 5
    
    // Parse back and verify
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(20, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_FIXED32, wire_type);
    
    uint32_t value;
    TEST_ASSERT_TRUE(reader.read_fixed32(value));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, value);
}

// Test fixed64 field preservation
void test_unknown_fixed64_field() {
    UnknownFieldSet unknown_fields;
    
    // Add a fixed64 field
    unknown_fields.add_fixed64(30, 0x123456789ABCDEF0ULL);
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Verify the serialized data
    // Should contain: tag (field 30, wire type 1) = 0xF1, fixed64 value
    const uint8_t* data = output.data();
    TEST_ASSERT_EQUAL_UINT8(0xF1, data[0]);  // (30 << 3) | 1
    
    // Parse back and verify
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(30, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_FIXED64, wire_type);
    
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_fixed64(value));
    TEST_ASSERT_EQUAL_HEX64(0x123456789ABCDEF0ULL, value);
}

// Test length-delimited field preservation
void test_unknown_length_delimited_field() {
    UnknownFieldSet unknown_fields;
    
    // Add a length-delimited field
    const char* test_data = "Hello, World!";
    unknown_fields.add_length_delimited(40, reinterpret_cast<const uint8_t*>(test_data), strlen(test_data));
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Verify the serialized data
    // Should contain: tag (field 40, wire type 2) = 0x142 = 0xC2 0x02 (varint), length, data
    const uint8_t* data = output.data();
    TEST_ASSERT_EQUAL_UINT8(0xC2, data[0]);  // (40 << 3) | 2 = 0x142, first byte 0xC2
    TEST_ASSERT_EQUAL_UINT8(0x02, data[1]);  // Continuation of tag, second byte 0x02
    
    // Parse back and verify
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(40, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_LENGTH_DELIMITED, wire_type);
    
    std::string value;
    TEST_ASSERT_TRUE(reader.read_string(value));
    TEST_ASSERT_EQUAL_STRING(test_data, value.c_str());
}

// Test multiple unknown fields
void test_multiple_unknown_fields() {
    UnknownFieldSet unknown_fields;
    
    // Add multiple fields of different types
    unknown_fields.add_varint(1, 100);
    unknown_fields.add_fixed32(2, 0xDEADBEEF);
    unknown_fields.add_length_delimited(3, reinterpret_cast<const uint8_t*>("test"), 4);
    unknown_fields.add_fixed64(4, 0x1122334455667788ULL);
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Parse back and verify all fields
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    // Field 1: varint
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(1, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_VARINT, wire_type);
    uint64_t varint_value;
    TEST_ASSERT_TRUE(reader.read_varint(varint_value));
    TEST_ASSERT_EQUAL_UINT64(100, varint_value);
    
    // Field 2: fixed32
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(2, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_FIXED32, wire_type);
    uint32_t fixed32_value;
    TEST_ASSERT_TRUE(reader.read_fixed32(fixed32_value));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, fixed32_value);
    
    // Field 3: length-delimited
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(3, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_LENGTH_DELIMITED, wire_type);
    std::string str_value;
    TEST_ASSERT_TRUE(reader.read_string(str_value));
    TEST_ASSERT_EQUAL_STRING("test", str_value.c_str());
    
    // Field 4: fixed64
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(4, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_FIXED64, wire_type);
    uint64_t fixed64_value;
    TEST_ASSERT_TRUE(reader.read_fixed64(fixed64_value));
    TEST_ASSERT_EQUAL_HEX64(0x1122334455667788ULL, fixed64_value);
}

// Test ProtoReader capture_unknown_field for varint
void test_capture_unknown_varint() {
    // Create a buffer with a varint field
    BufferOutputStream output;
    ProtoWriter writer(output);
    writer.write_tag(100, WIRE_TYPE_VARINT);
    writer.write_varint(0xDEADBEEF);
    
    // Read and capture the field
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(100, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_VARINT, wire_type);
    
    std::vector<uint8_t> captured;
    TEST_ASSERT_TRUE(reader.capture_unknown_field(wire_type, captured));
    TEST_ASSERT_TRUE(captured.size() > 0);
    
    // Decode the captured varint
    uint64_t value = 0;
    size_t shift = 0;
    for (size_t i = 0; i < captured.size() && i < 10; ++i) {
        value |= static_cast<uint64_t>(captured[i] & 0x7F) << shift;
        if ((captured[i] & 0x80) == 0) break;
        shift += 7;
    }
    TEST_ASSERT_EQUAL_UINT64(0xDEADBEEF, value);
}

// Test ProtoReader capture_unknown_field for length-delimited
void test_capture_unknown_length_delimited() {
    // Create a buffer with a length-delimited field
    BufferOutputStream output;
    ProtoWriter writer(output);
    writer.write_tag(200, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("Hello Protocol Buffers!");
    
    // Read and capture the field
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(200, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_LENGTH_DELIMITED, wire_type);
    
    std::vector<uint8_t> captured;
    TEST_ASSERT_TRUE(reader.capture_unknown_field(wire_type, captured));
    TEST_ASSERT_TRUE(captured.size() > 0);
    
    // Decode the captured length-delimited field
    size_t pos = 0;
    uint64_t len = 0;
    int shift = 0;
    while (pos < captured.size() && pos < 10) {
        len |= static_cast<uint64_t>(captured[pos] & 0x7F) << shift;
        if ((captured[pos] & 0x80) == 0) {
            pos++;
            break;
        }
        shift += 7;
        pos++;
    }
    
    std::string decoded_str(reinterpret_cast<const char*>(captured.data() + pos), len);
    TEST_ASSERT_EQUAL_STRING("Hello Protocol Buffers!", decoded_str.c_str());
}

// Test byte_size calculation
void test_unknown_fields_byte_size() {
    UnknownFieldSet unknown_fields;
    
    // Add fields and calculate expected size
    unknown_fields.add_varint(1, 1);  // Tag: 1 byte, Value: 1 byte = 2 bytes
    unknown_fields.add_fixed32(2, 0);  // Tag: 1 byte, Value: 4 bytes = 5 bytes
    unknown_fields.add_fixed64(3, 0);  // Tag: 1 byte, Value: 8 bytes = 9 bytes
    
    size_t expected_size = 2 + 5 + 9;
    TEST_ASSERT_EQUAL(expected_size, unknown_fields.byte_size());
    
    // Verify by serializing
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    TEST_ASSERT_EQUAL(expected_size, output.size());
}

// Test empty unknown field set
void test_empty_unknown_fields() {
    UnknownFieldSet unknown_fields;
    
    TEST_ASSERT_TRUE(unknown_fields.empty());
    TEST_ASSERT_EQUAL(0, unknown_fields.byte_size());
    
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    TEST_ASSERT_EQUAL(0, output.size());
}

// Test clear functionality
void test_unknown_fields_clear() {
    UnknownFieldSet unknown_fields;
    
    unknown_fields.add_varint(1, 100);
    unknown_fields.add_fixed32(2, 200);
    
    TEST_ASSERT_FALSE(unknown_fields.empty());
    
    unknown_fields.clear();
    
    TEST_ASSERT_TRUE(unknown_fields.empty());
    TEST_ASSERT_EQUAL(0, unknown_fields.byte_size());
}

// Test large field numbers
void test_unknown_large_field_numbers() {
    UnknownFieldSet unknown_fields;
    
    // Add a field with a large field number
    unknown_fields.add_varint(536870911, 42);  // Max field number (2^29 - 1)
    
    // Serialize to buffer
    BufferOutputStream output;
    TEST_ASSERT_TRUE(unknown_fields.serialize_to(output));
    
    // Parse back and verify
    BufferInputStream input(output.data(), output.size());
    ProtoReader reader(input);
    
    uint32_t field_number;
    WireType wire_type;
    TEST_ASSERT_TRUE(reader.read_tag(field_number, wire_type));
    TEST_ASSERT_EQUAL_UINT32(536870911, field_number);
    TEST_ASSERT_EQUAL(WIRE_TYPE_VARINT, wire_type);
    
    uint64_t value;
    TEST_ASSERT_TRUE(reader.read_varint(value));
    TEST_ASSERT_EQUAL_UINT64(42, value);
}

// Register tests for Unity framework
int runTests() {
    UNITY_BEGIN();
    
    // Basic field type tests
    RUN_TEST(test_unknown_varint_field);
    RUN_TEST(test_unknown_fixed32_field);
    RUN_TEST(test_unknown_fixed64_field);
    RUN_TEST(test_unknown_length_delimited_field);
    
    // Combined tests
    RUN_TEST(test_multiple_unknown_fields);
    
    // ProtoReader capture tests
    RUN_TEST(test_capture_unknown_varint);
    RUN_TEST(test_capture_unknown_length_delimited);
    
    // Utility tests
    RUN_TEST(test_unknown_fields_byte_size);
    RUN_TEST(test_empty_unknown_fields);
    RUN_TEST(test_unknown_fields_clear);
    RUN_TEST(test_unknown_large_field_numbers);
    
    return UNITY_END();
}