#include <iostream>
#include "litepb/litepb.h"
#include "build/generated/test_vectors.pb.h"

int main() {
    std::cout << "Testing namespace structure...\n";
    
    // Test basic creation
    litepb::test::interop::BasicScalars msg;
    msg.int32_field = 42;
    msg.string_field = "test";
    
    std::cout << "Message created successfully\n";
    
    // Test serialization
    litepb::BufferOutputStream stream;
    bool result = litepb::serialize(msg, stream);
    
    if (result) {
        std::cout << "Serialization successful! Size: " << stream.size() << " bytes\n";
    } else {
        std::cout << "Serialization failed!\n";
        return 1;
    }
    
    // Test deserialization
    litepb::BufferInputStream input_stream(stream.data(), stream.size());
    litepb::test::interop::BasicScalars decoded_msg;
    result = litepb::parse(decoded_msg, input_stream);
    
    if (result) {
        std::cout << "Deserialization successful!\n";
        std::cout << "int32_field: " << decoded_msg.int32_field << "\n";
        std::cout << "string_field: " << decoded_msg.string_field << "\n";
    } else {
        std::cout << "Deserialization failed!\n";
        return 1;
    }
    
    return 0;
}