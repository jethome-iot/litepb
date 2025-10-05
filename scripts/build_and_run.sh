#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GENERATOR_EXECUTABLE="$PROJECT_DIR/litepb_gen"

# Check if Python generator exists
if [ ! -f "$GENERATOR_EXECUTABLE" ]; then
    echo "Error: Generator not found at $GENERATOR_EXECUTABLE"
    exit 1
fi

# Run Python generator with options from command line arguments
"$GENERATOR_EXECUTABLE" "$@"