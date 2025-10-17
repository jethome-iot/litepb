#!/bin/bash

set -e

echo "======================================"
echo "LitePB Clean Build Script"
echo "======================================"
echo ""

echo "Cleaning all build artifacts..."
echo ""

# Count directories and files before cleaning
PIO_COUNT=$(find . -name ".pio" -type d 2>/dev/null | wc -l)
BUILD_COUNT=$(find . -name "build" -type d 2>/dev/null | wc -l)
PB_H_COUNT=$(find . -name "*.pb.h" -type f 2>/dev/null | wc -l)
PB_CPP_COUNT=$(find . -name "*.pb.cpp" -type f 2>/dev/null | wc -l)
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

# Remove all generated .pb.h files
if [ $PB_H_COUNT -gt 0 ]; then
    echo "Removing $PB_H_COUNT .pb.h files..."
    find . -name "*.pb.h" -type f -print0 | xargs -0 rm -f
    echo "✓ Removed .pb.h files"
else
    echo "No .pb.h files found"
fi

# Remove all generated .pb.cpp files
if [ $PB_CPP_COUNT -gt 0 ]; then
    echo "Removing $PB_CPP_COUNT .pb.cpp files..."
    find . -name "*.pb.cpp" -type f -print0 | xargs -0 rm -f
    echo "✓ Removed .pb.cpp files"
else
    echo "No .pb.cpp files found"
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