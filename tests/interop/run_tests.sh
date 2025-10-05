#!/bin/bash
# Generate proto code, build, and run tests
cd "$(dirname "$0")"

# Help CMake find Nix-installed protobuf and abseil
PROTOC_PATH=$(which protoc 2>/dev/null)
if [ -n "$PROTOC_PATH" ]; then
    # Extract Nix store path from protoc location
    # e.g., /nix/store/xxx-protobuf-30.2/bin/protoc -> /nix/store/xxx-protobuf-30.2
    PROTOBUF_PREFIX=$(dirname $(dirname "$PROTOC_PATH"))
    export CMAKE_PREFIX_PATH="$PROTOBUF_PREFIX:$CMAKE_PREFIX_PATH"
    echo "Using Nix protobuf at: $PROTOBUF_PREFIX"
fi

# Try to find abseil-cpp without slow find command
# Check common Nix profile paths first
for path in /nix/store/*-abseil-cpp-*/; do
    if [ -d "$path" ]; then
        export CMAKE_PREFIX_PATH="$path:$CMAKE_PREFIX_PATH"
        echo "Using Nix abseil at: $path"
        break
    fi
done

mkdir -p build
cd build
cmake ..
make
./interop_tests
