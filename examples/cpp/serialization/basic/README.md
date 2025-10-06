# Basic Serialization Example

Simple demonstration of LitePB serialization and deserialization.

## Description

This example shows how to:
- Create a Protocol Buffer message (Person with name, age, email)
- Serialize the message to binary format
- Deserialize back to a message
- Verify the data integrity

## Building and Running

```bash
# From this directory
pio run -t exec

# Or from project root
cd examples/serialization/basic
pio run -t exec
```

## Expected Output

```
=== LitePB Basic Serialization Example ===

Step 1: Creating Person message...
  Name: Alice Smith
  Age: 30
  Email: alice@example.com

Step 2: Serializing to binary format...
  Serialized size: 36 bytes

Step 3: Deserializing from binary format...
  ✓ Parsing successful!

Step 4: Verifying deserialized data...
  ✓ Name matches: Alice Smith
  ✓ Age matches: 30
  ✓ Email matches: alice@example.com

=== ✓ SUCCESS: Data serialized and deserialized correctly! ===
```

## Files

- `src/main.cpp` - Main example code
- `proto/person.proto` - Protocol Buffer definition
- `platformio.ini` - Build configuration