#include "litepb/litepb.h"
#include "person.pb.h"
#include <iostream>

using namespace basic;
using namespace litepb;

int main()
{
    std::cout << "=== LitePB Basic Serialization Example ===" << std::endl;
    std::cout << std::endl;

    // Step 1: Create and populate a Person message
    std::cout << "Step 1: Creating a Person message..." << std::endl;
    Person person;
    person.name  = "Alice Johnson";
    person.age   = 30;
    person.email = "alice@example.com";

    std::cout << "  Original data:" << std::endl;
    std::cout << "    Name:  " << person.name << std::endl;
    std::cout << "    Age:   " << person.age << std::endl;
    std::cout << "    Email: " << person.email << std::endl;
    std::cout << std::endl;

    // Step 2: Serialize (encode) the message to bytes
    std::cout << "Step 2: Serializing to bytes..." << std::endl;
    BufferOutputStream output;
    serialize(person, output);

    std::cout << "  Serialized size: " << output.size() << " bytes" << std::endl;
    std::cout << "  Bytes (hex): ";
    for (size_t i = 0; i < output.size(); ++i) {
        printf("%02x ", output.data()[i]);
    }
    std::cout << std::endl << std::endl;

    // Step 3: Deserialize (decode) the bytes back to a message
    std::cout << "Step 3: Deserializing from bytes..." << std::endl;
    BufferInputStream input(output.data(), output.size());
    Person decoded_person;
    bool parse_ok = parse(decoded_person, input);

    if (parse_ok) {
        std::cout << "  Parsing successful!" << std::endl;
        std::cout << "  Decoded data:" << std::endl;
        std::cout << "    Name:  " << decoded_person.name << std::endl;
        std::cout << "    Age:   " << decoded_person.age << std::endl;
        std::cout << "    Email: " << decoded_person.email << std::endl;
        std::cout << std::endl;

        // Step 4: Verify the data matches
        std::cout << "Step 4: Verifying data integrity..." << std::endl;
        bool matches =
            (person.name == decoded_person.name) && (person.age == decoded_person.age) && (person.email == decoded_person.email);

        if (matches) {
            std::cout << "  ✓ SUCCESS: Original and decoded data match perfectly!" << std::endl;
        }
        else {
            std::cout << "  ✗ ERROR: Data mismatch!" << std::endl;
            return 1;
        }
    }
    else {
        std::cout << "  ✗ ERROR: Failed to parse message!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrated:" << std::endl;
    std::cout << "  • Creating a Protocol Buffer message" << std::endl;
    std::cout << "  • Serializing (encoding) to binary format" << std::endl;
    std::cout << "  • Deserializing (decoding) back to a message" << std::endl;
    std::cout << "  • Verifying data integrity after round-trip" << std::endl;

    return 0;
}
