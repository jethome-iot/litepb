# Basic Serialization Example (CMake)

This example demonstrates basic Protocol Buffer serialization using LitePB with CMake.

## What This Example Shows

- Creating a simple Protocol Buffer message (`Person`)
- Serializing the message to binary format
- Deserializing the binary data back to a message
- Verifying data integrity after round-trip serialization

## Building and Running

### Prerequisites

- CMake 3.14 or higher
- C++11 compatible compiler
- LitePB generator (`litepb_gen` in the project root)

### Build Instructions

```bash
# From this directory
cmake -B build
cmake --build build

# Run the example
./build/basic_serialization
```

### Expected Output

The program will:
1. Create a `Person` message with sample data
2. Serialize it to bytes and display the hex representation
3. Deserialize the bytes back to a message
4. Verify that the original and decoded data match

## Project Structure

```
basic_serialization/
├── proto/
│   └── person.proto      # Protocol Buffer definition
├── src/
│   └── main.cpp          # Example application
├── CMakeLists.txt        # CMake build configuration
└── README.md            # This file
```

## How It Works

1. **CMakeLists.txt** automatically invokes `litepb_gen` to generate C++ code from `person.proto`
2. The generated code (`person.pb.h` and `person.pb.cpp`) is placed in the `build/generated/` directory
3. The executable is linked with the LitePB static library
4. All you need to do is write your application code!
