#!/usr/bin/env bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"

echo "Generating all test proto files..."

# Generate serialization test protos
echo "Generating serialization test protos..."
for proto in tests/proto/serialization/*.proto; do
    echo "  Processing: $(basename "$proto")"
    ./litepb_gen -o tests/cpp/platformio/serialization/test_packages/proto -I tests/proto -I proto "$proto"
done

# Generate RPC test protos
echo "Generating RPC test protos..."
for proto in tests/proto/rpc/*.proto; do
    echo "  Processing: $(basename "$proto")"
    ./litepb_gen -o tests/cpp/platformio/rpc/test_packages/proto -I tests/proto -I proto "$proto"
done

# Generate example protos
echo "Generating example protos..."
./litepb_gen -o examples/cpp/rpc/litepb_rpc/proto -I proto -I examples/cpp/rpc/litepb_rpc/proto examples/cpp/rpc/litepb_rpc/proto/switch.proto

echo "Code generation complete!"