#include "build/generated/test_vectors.pb.h"
#include "build/generated/protoc/test_vectors.pb.h"
#include <iostream>
int main() {
    litepb::test::interop::BasicScalars litepb_msg;
    protoc_interop::BasicScalars protoc_msg;
    litepb_msg.int32_field = 42;
    protoc_msg.set_int32_field(42);
    std::cout << "Both messages created successfully" << std::endl;
    return 0;
}
