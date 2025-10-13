// Simple test to verify the namespace fix
#include <iostream>
#include <vector>
#include <cstdint>
#include "litepb/litepb.h"
#include "test_vectors.pb.h"

int main() {
    std::cout << "Testing namespace fix for litepb_gen..." << std::endl;
    
    // Test that we can create objects with the new namespace
    litepb_gen::litepb::test::interop::BasicScalars basic_msg;
    basic_msg.int32_field = 42;
    basic_msg.string_field = "test";
    
    // Test serialization
    litepb::BufferOutputStream stream;
    if (litepb::serialize(basic_msg, stream)) {
        std::cout << "✓ BasicScalars serialization successful" << std::endl;
    } else {
        std::cout << "✗ BasicScalars serialization failed" << std::endl;
        return 1;
    }
    
    // Test deserialization
    std::vector<uint8_t> data(stream.data(), stream.data() + stream.size());
    litepb::BufferInputStream input_stream(data.data(), data.size());
    litepb_gen::litepb::test::interop::BasicScalars decoded_msg;
    
    if (litepb::parse(decoded_msg, input_stream)) {
        std::cout << "✓ BasicScalars deserialization successful" << std::endl;
        
        if (decoded_msg.int32_field == 42 && decoded_msg.string_field == "test") {
            std::cout << "✓ Data integrity verified" << std::endl;
        } else {
            std::cout << "✗ Data integrity failed" << std::endl;
            return 1;
        }
    } else {
        std::cout << "✗ BasicScalars deserialization failed" << std::endl;
        return 1;
    }
    
    // Test other message types
    litepb_gen::litepb::test::interop::AllScalars all_scalars_msg;
    all_scalars_msg.v_int32 = -123;
    all_scalars_msg.v_bool = true;
    std::cout << "✓ AllScalars object created successfully" << std::endl;
    
    litepb_gen::litepb::test::interop::RepeatedFields repeated_msg;
    repeated_msg.packed_ints = {1, 2, 3};
    std::cout << "✓ RepeatedFields object created successfully" << std::endl;
    
    litepb_gen::litepb::test::interop::MapField map_msg;
    map_msg.items["key1"] = 100;
    std::cout << "✓ MapField object created successfully" << std::endl;
    
    litepb_gen::litepb::test::interop::Outer outer_msg;
    outer_msg.inner.value = 42;
    outer_msg.name = "test";
    std::cout << "✓ Outer (with nested Inner) object created successfully" << std::endl;
    
    litepb_gen::litepb::test::interop::OneofTest oneof_msg;
    oneof_msg.choice = int32_t(42);
    std::cout << "✓ OneofTest object created successfully" << std::endl;
    
    litepb_gen::litepb::test::interop::EnumTest enum_msg;
    enum_msg.value = litepb_gen::litepb::test::interop::TestEnum::FIRST;
    std::cout << "✓ EnumTest object created successfully" << std::endl;
    
    std::cout << "\n✅ All namespace tests passed! The namespace fix is working correctly." << std::endl;
    std::cout << "The namespace prefix 'litepb_gen' has been successfully applied." << std::endl;
    
    return 0;
}