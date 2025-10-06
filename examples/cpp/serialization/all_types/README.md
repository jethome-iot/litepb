# All Types Showcase

Comprehensive demonstration of ALL Protocol Buffer types supported by LitePB.

## Overview

This example demonstrates every Protocol Buffer field type with complete verification:
- All scalar types (int32, int64, uint32, uint64, sint32, sint64, bool, string, bytes)
- Fixed-width types (fixed32, fixed64, sfixed32, sfixed64, float, double)
- Complex types (enum, nested messages, repeated fields, maps, oneofs)

## Building and Running

```bash
# From this directory
pio run -t exec

# Or from project root  
cd examples/serialization/all_types
pio run -t exec
```

## Expected Output

Shows creation, serialization (335 bytes), deserialization, and verification of all types:

```
=== LitePB All Types Showcase ===

Step 1: Creating TypeShowcase message with all field types...
Step 2: Displaying original data...
Step 3: Serializing to binary format...
  Serialized size: 335 bytes
Step 4: Deserializing from binary format...
  ✓ Parsing successful!
Step 5: Verifying all field types...
  ✓ All scalar types match
  ✓ All fixed-width types match
  ✓ All complex types match

=== ✓ SUCCESS: All types verified correctly! ===
```

## Files

- `src/main.cpp` - Comprehensive example code
- `proto/showcase.proto` - Complete Protocol Buffer definition
- `platformio.ini` - Build configuration