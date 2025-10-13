#include "build/generated/test_vectors.pb.h"
#include <iostream>
int main() {
    litepb::test::interop::BasicScalars msg;
    msg.int32_field = 42;
    std::cout << "BasicScalars test passed" << std::endl;
    return 0;
}
