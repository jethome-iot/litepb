#!/bin/bash

echo "======================================"
echo "LitePB Clean Build Script"
echo "======================================"
echo ""

echo "Cleaning all build artifacts..."
echo ""

# Count directories before cleaning
PIO_COUNT=$(find . -name ".pio" -type d 2>/dev/null | wc -l)
BUILD_COUNT=$(find . -name "build" -type d 2>/dev/null | wc -l)
TMP_EXISTS=0
if [ -d "tmp" ]; then
    TMP_EXISTS=1
fi

# Remove all .pio directories at any level
if [ $PIO_COUNT -gt 0 ]; then
    echo "Removing $PIO_COUNT .pio directories..."
    find . -name ".pio" -type d -print0 | xargs -0 rm -rf
    echo "✓ Removed .pio directories"
else
    echo "No .pio directories found"
fi

# Remove all build directories at any level
if [ $BUILD_COUNT -gt 0 ]; then
    echo "Removing $BUILD_COUNT build directories..."
    find . -name "build" -type d -print0 | xargs -0 rm -rf
    echo "✓ Removed build directories"
else
    echo "No build directories found"
fi

# Remove top-level tmp directory
if [ $TMP_EXISTS -eq 1 ]; then
    echo "Removing tmp directory..."
    rm -rf tmp
    echo "✓ Removed tmp directory"
else
    echo "No tmp directory found"
fi

echo ""
echo "======================================"
echo "Clean complete!"
echo "======================================"