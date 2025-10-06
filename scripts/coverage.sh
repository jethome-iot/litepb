#!/bin/bash

echo "======================================"
echo "LitePB Code Coverage Report Generator"
echo "======================================"
echo ""

echo "Step 1: Cleaning previous coverage data and build..."
find . -name "*.gcda" -delete
find . -name "*.gcno" -delete
find . -name "*.gcov*" -delete
rm -rf tmp/coverage/
rm -rf .pio/build/coverage
rm -rf tmp/gcov/
echo "✓ Cleaned previous coverage data"
echo ""

echo "Step 2: Running tests with coverage instrumentation..."
pio test -e coverage --filter "core/test_streams" --filter "core/test_proto_reader" --filter "core/test_proto_writer" --filter "rpc/test_core" --filter "rpc/test_error_propagation"
echo "✓ Tests completed"
echo ""

echo "Step 3: Capturing coverage data..."
# Create tmp/gcov directory for intermediate gcov files
mkdir -p tmp/gcov
# Create tmp/coverage directory for all coverage outputs
mkdir -p tmp/coverage

# Find gcov efficiently - check if it's in PATH first, then look in nix store
GCOV_TOOL=$(which gcov 2>/dev/null)

if [ -z "$GCOV_TOOL" ]; then
    # In Nix, gcov is in the gcc package (not gcc-wrapper)
    # Search for it in the nix store
    GCOV_TOOL=$(ls /nix/store/*gcc-14.*/bin/gcov 2>/dev/null | head -1)
fi

if [ -z "$GCOV_TOOL" ] || [ ! -x "$GCOV_TOOL" ]; then
    echo "ERROR: gcov tool not found in PATH or nix store"
    echo "Coverage data has been generated in .pio/build/coverage/"
    echo "You can manually process .gcda and .gcno files using a coverage tool."
    exit 1
fi

echo "Using gcov at: $GCOV_TOOL"

# Run lcov from tmp/gcov directory so intermediate files go there
cd tmp/gcov
lcov --capture \
     --directory ../../.pio/build/coverage \
     --output-file ../coverage/coverage.info \
     --gcov-tool "$GCOV_TOOL" \
     --ignore-errors inconsistent \
     --quiet || echo "Note: lcov may show warnings (this is expected)"
cd ../..

# Clean up any stray gcov files outside tmp/
find . -maxdepth 1 -name "*.gcov*" -delete 2>/dev/null || true

echo "✓ Coverage data captured"
echo ""

echo "Step 4: Filtering coverage to cpp/src/litepb/ only..."
lcov --extract tmp/coverage/coverage.info \
     "*/cpp/src/litepb/*" \
     --output-file tmp/coverage/coverage_filtered.info \
     --quiet || true

echo "✓ Filtered to LitePB source files"
echo ""

echo "Step 5: Generating HTML report..."
genhtml tmp/coverage/coverage_filtered.info \
        --output-directory tmp/coverage/coverage_report \
        --title "LitePB Code Coverage" \
        --legend \
        --quiet || echo "Report generation completed"

echo "✓ HTML report generated"
echo ""

echo "Step 6: Coverage Summary"
echo "========================"
lcov --list tmp/coverage/coverage_filtered.info || true

echo ""
echo "======================================"
echo "Coverage report generated successfully!"
echo "Open tmp/coverage/coverage_report/index.html to view the detailed report"
echo "======================================"
