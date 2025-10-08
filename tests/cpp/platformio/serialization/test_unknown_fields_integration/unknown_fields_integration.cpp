#include <unity.h>
#include "litepb/litepb.h"
#include "litepb/core/unknown_fields.h"
#include <cstring>

using namespace litepb;

void setUp() {}
void tearDown() {}

// Simple test message structure
struct TestMessage {
    int32_t known_field = 0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
};

// Manual serializer for testing
template<>
class litepb::Serializer<TestMessage> {
public:
    static bool serialize(const TestMessage& value, litepb::OutputStream& stream) {
        litepb::ProtoWriter writer(stream);
        
        // Write known field
        if (value.known_field != 0) {
            writer.write_tag(1, litepb::WIRE_TYPE_VARINT);
            writer.write_varint(value.known_field);
        }
        
        // Serialize unknown fields for forward/backward compatibility
        if (!value.unknown_fields.empty()) {
            if (!value.unknown_fields.serialize_to(stream)) return false;
        }
        
        return true;
    }
    
    static bool parse(TestMessage& value, litepb::InputStream& stream) {
        litepb::ProtoReader reader(stream);
        uint32_t field_number;
        litepb::WireType wire_type;
        
        while (reader.read_tag(field_number, wire_type)) {
            switch (field_number) {
                case 1: {
                    uint64_t temp_varint;
                    if (!reader.read_varint(temp_varint)) return false;
                    value.known_field = static_cast<int32_t>(temp_varint);
                    break;
                }
                default: {
                    // Capture unknown field for forward/backward compatibility
                    std::vector<uint8_t> unknown_data;
                    if (!reader.capture_unknown_field(wire_type, unknown_data)) return false;
                    
                    // Store in UnknownFieldSet based on wire type
                    switch (wire_type) {
                        case litepb::WIRE_TYPE_VARINT: {
                            // Decode varint from captured data
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
                            // Extract actual data (skip length prefix)
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
                        case litepb::WIRE_TYPE_START_GROUP: {
                            value.unknown_fields.add_group(field_number, 
                                unknown_data.data(), unknown_data.size());
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

// Test round-trip with unknown fields
void test_round_trip_with_unknown_fields() {
    // Create a message with known and unknown fields
    TestMessage original;
    original.known_field = 42;
    original.unknown_fields.add_varint(100, 999);
    original.unknown_fields.add_fixed32(101, 0xDEADBEEF);
    original.unknown_fields.add_length_delimited(102, 
        reinterpret_cast<const uint8_t*>("unknown data"), 12);
    
    // Serialize
    BufferOutputStream output;
    TEST_ASSERT_TRUE(litepb::serialize(original, output));
    
    // Parse back
    BufferInputStream input(output.data(), output.size());
    TestMessage parsed;
    TEST_ASSERT_TRUE(litepb::parse(parsed, input));
    
    // Verify known field
    TEST_ASSERT_EQUAL_INT32(42, parsed.known_field);
    
    // Verify unknown fields were preserved
    TEST_ASSERT_FALSE(parsed.unknown_fields.empty());
    TEST_ASSERT_EQUAL(3, parsed.unknown_fields.fields().size());
    
    // Serialize again to verify round-trip
    BufferOutputStream output2;
    TEST_ASSERT_TRUE(litepb::serialize(parsed, output2));
    
    // Compare buffers - should be identical
    TEST_ASSERT_EQUAL(output.size(), output2.size());
    TEST_ASSERT_EQUAL_MEMORY(output.data(), output2.data(), output.size());
}

// Test parsing message with only unknown fields
void test_parse_only_unknown_fields() {
    // Create a buffer with only unknown fields
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    writer.write_tag(200, WIRE_TYPE_VARINT);
    writer.write_varint(12345);
    writer.write_tag(201, WIRE_TYPE_FIXED32);
    writer.write_fixed32(0xCAFEBABE);
    
    // Parse the message
    BufferInputStream input(output.data(), output.size());
    TestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify no known fields
    TEST_ASSERT_EQUAL_INT32(0, msg.known_field);
    
    // Verify unknown fields were captured
    TEST_ASSERT_FALSE(msg.unknown_fields.empty());
    TEST_ASSERT_EQUAL(2, msg.unknown_fields.fields().size());
    
    // Re-serialize and verify
    BufferOutputStream output2;
    TEST_ASSERT_TRUE(litepb::serialize(msg, output2));
    TEST_ASSERT_EQUAL(output.size(), output2.size());
    TEST_ASSERT_EQUAL_MEMORY(output.data(), output2.data(), output.size());
}

// Test forward compatibility - old code reading new message
void test_forward_compatibility() {
    // Simulate a "new" message with additional fields
    BufferOutputStream output;
    ProtoWriter writer(output);
    
    // Known field
    writer.write_tag(1, WIRE_TYPE_VARINT);
    writer.write_varint(100);
    
    // "Future" fields that current code doesn't know about
    writer.write_tag(2, WIRE_TYPE_VARINT);
    writer.write_varint(200);
    writer.write_tag(3, WIRE_TYPE_LENGTH_DELIMITED);
    writer.write_string("future feature");
    writer.write_tag(4, WIRE_TYPE_FIXED64);
    writer.write_fixed64(0x123456789ABCDEF0ULL);
    
    // Parse with "old" code
    BufferInputStream input(output.data(), output.size());
    TestMessage msg;
    TEST_ASSERT_TRUE(litepb::parse(msg, input));
    
    // Verify known field
    TEST_ASSERT_EQUAL_INT32(100, msg.known_field);
    
    // Verify future fields were preserved as unknown
    TEST_ASSERT_FALSE(msg.unknown_fields.empty());
    TEST_ASSERT_EQUAL(3, msg.unknown_fields.fields().size());
    
    // Re-serialize - should maintain all fields
    BufferOutputStream output2;
    TEST_ASSERT_TRUE(litepb::serialize(msg, output2));
    
    // Parse again to verify nothing was lost
    BufferInputStream input2(output2.data(), output2.size());
    TestMessage msg2;
    TEST_ASSERT_TRUE(litepb::parse(msg2, input2));
    
    TEST_ASSERT_EQUAL_INT32(100, msg2.known_field);
    TEST_ASSERT_EQUAL(3, msg2.unknown_fields.fields().size());
}

int runTests() {
    UNITY_BEGIN();
    
    RUN_TEST(test_round_trip_with_unknown_fields);
    RUN_TEST(test_parse_only_unknown_fields);
    RUN_TEST(test_forward_compatibility);
    
    return UNITY_END();
}