#include "unity.h"

// Include just the necessary headers
#include "litepb/litepb.h"
#include "protoc_helpers.h"
#include "build/generated/test_vectors.pb.h"

void setUp() {}
void tearDown() {}

// Import just one test function
extern void test_basic_scalars();

int main() {
    UNITY_BEGIN();
    
    // Test just the basic scalars
    RUN_TEST(test_basic_scalars);
    
    return UNITY_END();
}