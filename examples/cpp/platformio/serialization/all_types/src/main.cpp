#include "litepb/litepb.h"
#include "showcase.pb.h"
#include <iomanip>
#include <iostream>
#include <variant>

using namespace alltypes;
using namespace litepb;

void print_bytes(const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0)
            std::cout << std::endl << "    ";
    }
    std::cout << std::endl;
}

bool verify_scalar_types(const TypeShowcase& original, const TypeShowcase& decoded)
{
    bool match = true;

    if (original.field_int32 != decoded.field_int32) {
        std::cout << "  ✗ int32 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_int64 != decoded.field_int64) {
        std::cout << "  ✗ int64 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_uint32 != decoded.field_uint32) {
        std::cout << "  ✗ uint32 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_uint64 != decoded.field_uint64) {
        std::cout << "  ✗ uint64 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_sint32 != decoded.field_sint32) {
        std::cout << "  ✗ sint32 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_sint64 != decoded.field_sint64) {
        std::cout << "  ✗ sint64 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_bool != decoded.field_bool) {
        std::cout << "  ✗ bool mismatch!" << std::endl;
        match = false;
    }
    if (original.field_string != decoded.field_string) {
        std::cout << "  ✗ string mismatch!" << std::endl;
        match = false;
    }
    if (original.field_bytes != decoded.field_bytes) {
        std::cout << "  ✗ bytes mismatch!" << std::endl;
        match = false;
    }

    return match;
}

bool verify_fixed_types(const TypeShowcase& original, const TypeShowcase& decoded)
{
    bool match = true;

    if (original.field_fixed32 != decoded.field_fixed32) {
        std::cout << "  ✗ fixed32 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_fixed64 != decoded.field_fixed64) {
        std::cout << "  ✗ fixed64 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_sfixed32 != decoded.field_sfixed32) {
        std::cout << "  ✗ sfixed32 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_sfixed64 != decoded.field_sfixed64) {
        std::cout << "  ✗ sfixed64 mismatch!" << std::endl;
        match = false;
    }
    if (original.field_float != decoded.field_float) {
        std::cout << "  ✗ float mismatch!" << std::endl;
        match = false;
    }
    if (original.field_double != decoded.field_double) {
        std::cout << "  ✗ double mismatch!" << std::endl;
        match = false;
    }

    return match;
}

bool verify_complex_types(const TypeShowcase& original, const TypeShowcase& decoded)
{
    bool match = true;

    if (original.field_enum != decoded.field_enum) {
        std::cout << "  ✗ enum mismatch!" << std::endl;
        match = false;
    }

    if (original.field_address.street != decoded.field_address.street ||
        original.field_address.city != decoded.field_address.city ||
        original.field_address.zip_code != decoded.field_address.zip_code) {
        std::cout << "  ✗ nested message mismatch!" << std::endl;
        match = false;
    }

    if (original.repeated_int32.size() != decoded.repeated_int32.size()) {
        std::cout << "  ✗ repeated_int32 size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (size_t i = 0; i < original.repeated_int32.size(); ++i) {
            if (original.repeated_int32[i] != decoded.repeated_int32[i]) {
                std::cout << "  ✗ repeated_int32[" << i << "] mismatch!" << std::endl;
                match = false;
            }
        }
    }

    if (original.repeated_string.size() != decoded.repeated_string.size()) {
        std::cout << "  ✗ repeated_string size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (size_t i = 0; i < original.repeated_string.size(); ++i) {
            if (original.repeated_string[i] != decoded.repeated_string[i]) {
                std::cout << "  ✗ repeated_string[" << i << "] mismatch: expected '" << original.repeated_string[i] << "', got '"
                          << decoded.repeated_string[i] << "'" << std::endl;
                match = false;
            }
        }
    }

    if (original.repeated_unpacked.size() != decoded.repeated_unpacked.size()) {
        std::cout << "  ✗ repeated_unpacked size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (size_t i = 0; i < original.repeated_unpacked.size(); ++i) {
            if (original.repeated_unpacked[i] != decoded.repeated_unpacked[i]) {
                std::cout << "  ✗ repeated_unpacked[" << i << "] mismatch: expected " << original.repeated_unpacked[i] << ", got "
                          << decoded.repeated_unpacked[i] << std::endl;
                match = false;
            }
        }
    }

    if (original.map_string_int.size() != decoded.map_string_int.size()) {
        std::cout << "  ✗ map_string_int size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (const auto& pair : original.map_string_int) {
            auto it = decoded.map_string_int.find(pair.first);
            if (it == decoded.map_string_int.end()) {
                std::cout << "  ✗ map_string_int missing key: '" << pair.first << "'" << std::endl;
                match = false;
            }
            else if (it->second != pair.second) {
                std::cout << "  ✗ map_string_int['" << pair.first << "'] mismatch: expected " << pair.second << ", got "
                          << it->second << std::endl;
                match = false;
            }
        }
    }

    if (original.map_string_message.size() != decoded.map_string_message.size()) {
        std::cout << "  ✗ map_string_message size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (const auto& pair : original.map_string_message) {
            auto it = decoded.map_string_message.find(pair.first);
            if (it == decoded.map_string_message.end()) {
                std::cout << "  ✗ map_string_message missing key: '" << pair.first << "'" << std::endl;
                match = false;
            }
            else {
                const Address& orig_addr = pair.second;
                const Address& dec_addr  = it->second;
                if (orig_addr.street != dec_addr.street || orig_addr.city != dec_addr.city ||
                    orig_addr.zip_code != dec_addr.zip_code) {
                    std::cout << "  ✗ map_string_message['" << pair.first << "'] address mismatch!" << std::endl;
                    match = false;
                }
            }
        }
    }

    if (std::holds_alternative<int32_t>(original.test_oneof) && std::holds_alternative<int32_t>(decoded.test_oneof)) {
        if (std::get<int32_t>(original.test_oneof) != std::get<int32_t>(decoded.test_oneof)) {
            std::cout << "  ✗ oneof test_oneof mismatch!" << std::endl;
            match = false;
        }
    }
    else if (original.test_oneof.index() != decoded.test_oneof.index()) {
        std::cout << "  ✗ oneof test_oneof type mismatch!" << std::endl;
        match = false;
    }

    if (original.repeated_addresses.size() != decoded.repeated_addresses.size()) {
        std::cout << "  ✗ repeated_addresses size mismatch!" << std::endl;
        match = false;
    }
    else {
        for (size_t i = 0; i < original.repeated_addresses.size(); ++i) {
            const Address& orig_addr = original.repeated_addresses[i];
            const Address& dec_addr  = decoded.repeated_addresses[i];
            if (orig_addr.street != dec_addr.street || orig_addr.city != dec_addr.city || orig_addr.zip_code != dec_addr.zip_code) {
                std::cout << "  ✗ repeated_addresses[" << i << "] mismatch!" << std::endl;
                match = false;
            }
        }
    }

    return match;
}

int main()
{
    std::cout << "=== LitePB All Types Showcase ===" << std::endl;
    std::cout << std::endl;

    // Create and populate a message with ALL protobuf types
    std::cout << "Step 1: Creating TypeShowcase message with all field types..." << std::endl;
    TypeShowcase showcase;

    // Scalar types
    showcase.field_int32  = -42;
    showcase.field_int64  = -9223372036854775807LL;
    showcase.field_uint32 = 4294967295U;
    showcase.field_uint64 = 18446744073709551615ULL;
    showcase.field_sint32 = -2147483648;            // Uses zigzag encoding
    showcase.field_sint64 = -9223372036854775807LL; // Uses zigzag encoding
    showcase.field_bool   = true;
    showcase.field_string = "Hello, LitePB!";
    showcase.field_bytes  = { 0x01, 0x02, 0x03, 0xFF };

    // Fixed-width types
    showcase.field_fixed32  = 0xDEADBEEF;
    showcase.field_fixed64  = 0xCAFEBABEDEADBEEFULL;
    showcase.field_sfixed32 = -123456;
    showcase.field_sfixed64 = -9876543210LL;

    // Floating point
    showcase.field_float  = 3.14159f;
    showcase.field_double = 2.718281828459045;

    // Enum
    showcase.field_enum = Status::ACTIVE;

    // Nested message
    showcase.field_address.street   = "123 Main St";
    showcase.field_address.city     = "Anytown";
    showcase.field_address.zip_code = 12345;

    // Repeated fields
    showcase.repeated_int32  = { 1, 2, 3, 4, 5 };
    showcase.repeated_string = { "apple", "banana", "cherry" };

    Address addr1, addr2;
    addr1.street   = "456 Oak Ave";
    addr1.city     = "Springfield";
    addr1.zip_code = 67890;

    addr2.street   = "789 Pine Rd";
    addr2.city     = "Shelbyville";
    addr2.zip_code = 54321;

    showcase.repeated_addresses = { addr1, addr2 };

    // Unpacked repeated
    showcase.repeated_unpacked = { 10, 20, 30 };

    // Maps
    showcase.map_string_int["one"]   = 1;
    showcase.map_string_int["two"]   = 2;
    showcase.map_string_int["three"] = 3;

    Address map_addr;
    map_addr.street                     = "Map Street";
    map_addr.city                       = "Map City";
    map_addr.zip_code                   = 99999;
    showcase.map_string_message["home"] = map_addr;

    // Oneof (only one field can be set)
    showcase.test_oneof = int32_t(999);

    std::cout << "  ✓ Populated all field types" << std::endl;
    std::cout << std::endl;

    // Display the populated data
    std::cout << "Step 2: Displaying original data..." << std::endl;
    std::cout << "  Scalar types:" << std::endl;
    std::cout << "    int32:   " << showcase.field_int32 << std::endl;
    std::cout << "    int64:   " << showcase.field_int64 << std::endl;
    std::cout << "    uint32:  " << showcase.field_uint32 << std::endl;
    std::cout << "    uint64:  " << showcase.field_uint64 << std::endl;
    std::cout << "    sint32:  " << showcase.field_sint32 << " (zigzag)" << std::endl;
    std::cout << "    sint64:  " << showcase.field_sint64 << " (zigzag)" << std::endl;
    std::cout << "    bool:    " << (showcase.field_bool ? "true" : "false") << std::endl;
    std::cout << "    string:  " << showcase.field_string << std::endl;
    std::cout << "    bytes:   " << showcase.field_bytes.size() << " bytes" << std::endl;

    std::cout << "  Fixed-width types:" << std::endl;
    std::cout << "    fixed32:  0x" << std::hex << showcase.field_fixed32 << std::dec << std::endl;
    std::cout << "    fixed64:  0x" << std::hex << showcase.field_fixed64 << std::dec << std::endl;
    std::cout << "    sfixed32: " << showcase.field_sfixed32 << std::endl;
    std::cout << "    sfixed64: " << showcase.field_sfixed64 << std::endl;

    std::cout << "  Floating-point types:" << std::endl;
    std::cout << "    float:   " << std::fixed << std::setprecision(5) << showcase.field_float << std::endl;
    std::cout << "    double:  " << std::setprecision(15) << showcase.field_double << std::endl;

    std::cout << "  Complex types:" << std::endl;
    std::cout << "    enum:    " << (int) showcase.field_enum << " (ACTIVE)" << std::endl;
    std::cout << "    address: " << showcase.field_address.street << ", " << showcase.field_address.city << " "
              << showcase.field_address.zip_code << std::endl;
    std::cout << "    repeated_int32: [";
    for (size_t i = 0; i < showcase.repeated_int32.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << showcase.repeated_int32[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "    map_string_int size: " << showcase.map_string_int.size() << std::endl;
    std::cout << "    test_oneof (int32): ";
    if (std::holds_alternative<int32_t>(showcase.test_oneof)) {
        std::cout << std::get<int32_t>(showcase.test_oneof) << std::endl;
    }
    else {
        std::cout << "(not set)" << std::endl;
    }
    std::cout << std::endl;

    // Serialize
    std::cout << "Step 3: Serializing to binary format..." << std::endl;
    BufferOutputStream output;
    serialize(showcase, output);

    std::cout << "  Serialized size: " << output.size() << " bytes" << std::endl;
    std::cout << "  Binary data (hex):" << std::endl << "    ";
    print_bytes(output.data(), output.size());
    std::cout << std::endl;

    // Deserialize
    std::cout << "Step 4: Deserializing from binary format..." << std::endl;
    BufferInputStream input(output.data(), output.size());
    TypeShowcase decoded;
    bool parse_ok = parse(decoded, input);

    if (!parse_ok) {
        std::cout << "  ✗ ERROR: Failed to parse message!" << std::endl;
        return 1;
    }

    std::cout << "  ✓ Parsing successful!" << std::endl;
    std::cout << std::endl;

    // Verify all fields
    std::cout << "Step 5: Verifying all field types..." << std::endl;

    bool all_match = true;

    std::cout << "  Verifying scalar types..." << std::endl;
    if (!verify_scalar_types(showcase, decoded)) {
        all_match = false;
    }
    else {
        std::cout << "    ✓ All scalar types match" << std::endl;
    }

    std::cout << "  Verifying fixed-width types..." << std::endl;
    if (!verify_fixed_types(showcase, decoded)) {
        all_match = false;
    }
    else {
        std::cout << "    ✓ All fixed-width types match" << std::endl;
    }

    std::cout << "  Verifying complex types..." << std::endl;
    if (!verify_complex_types(showcase, decoded)) {
        all_match = false;
    }
    else {
        std::cout << "    ✓ All complex types match" << std::endl;
    }

    std::cout << std::endl;

    if (all_match) {
        std::cout << "=== ✓ SUCCESS: All types verified correctly! ===" << std::endl;
    }
    else {
        std::cout << "=== ✗ FAILURE: Some types did not match! ===" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "This example demonstrated ALL Protocol Buffer types:" << std::endl;
    std::cout << "  • Scalar types: int32, int64, uint32, uint64, sint32, sint64, bool, string, bytes" << std::endl;
    std::cout << "  • Fixed-width: fixed32, fixed64, sfixed32, sfixed64" << std::endl;
    std::cout << "  • Floating-point: float, double" << std::endl;
    std::cout << "  • Enum: Status enum" << std::endl;
    std::cout << "  • Nested messages: Address message" << std::endl;
    std::cout << "  • Repeated fields: packed and unpacked" << std::endl;
    std::cout << "  • Map fields: string→int32, string→message" << std::endl;
    std::cout << "  • Oneof: union type (one field active at a time)" << std::endl;

    return 0;
}
