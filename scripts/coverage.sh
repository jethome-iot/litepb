#!/bin/bash

# Exit on error and report where it happened
set -e
set -o pipefail

# Error handler to show where the script failed
error_handler() {
    local line_no=$1
    echo ""
    echo "ERROR: Script failed at line $line_no"
    echo "Check the command above this line for the error."
    exit 1
}

# Set up error trap
trap 'error_handler $LINENO' ERR

# Configuration Variables
ENV_NAME="native_coverage"
TMP_DIR="tmp/${ENV_NAME}"
GCOV_DIR="${TMP_DIR}/gcov"
COVERAGE_DIR="${TMP_DIR}/coverage"
COVERAGE_INFO="${COVERAGE_DIR}/coverage.info"
COVERAGE_FILTERED="${COVERAGE_DIR}/coverage_filtered.info"
COVERAGE_REPORT="${COVERAGE_DIR}/coverage_report"
BUILD_DIR=".pio/build/${ENV_NAME}"

# Coverage filter patterns - can add multiple patterns
COVERAGE_FILTERS=(
    "*/cpp/src/litepb/*"
    "*/cpp/include/litepb/*"
)

echo "======================================"
echo "LitePB Code Coverage Report Generator"
echo "======================================"
echo ""
echo "Environment: ${ENV_NAME}"
echo "Output directory: ${TMP_DIR}"
echo ""

echo "Step 1: Cleaning previous coverage data and build..."
# Clean gcov data files
find . -name "*.gcda" -delete
find . -name "*.gcno" -delete
find . -name "*.gcov*" -delete

# Clean output directories
rm -rf "${TMP_DIR}"
rm -rf "${BUILD_DIR}"

echo "✓ Cleaned previous coverage data"
echo ""

echo "Step 2: Running tests with coverage instrumentation..."
echo "Running: pio test -e ${ENV_NAME}"
pio test -e "${ENV_NAME}"
echo "✓ Tests completed"
echo ""

echo "Step 3: Capturing coverage data..."
# Create output directories
mkdir -p "${GCOV_DIR}"
mkdir -p "${COVERAGE_DIR}"

# Find gcov efficiently - check if it's in PATH first, then look in nix store
GCOV_TOOL=$(which gcov 2>/dev/null || true)

if [ -z "$GCOV_TOOL" ]; then
    # In Nix, gcov is in the gcc package (not gcc-wrapper)
    # Search for it in the nix store
    GCOV_TOOL=$(ls /nix/store/*gcc-14.*/bin/gcov 2>/dev/null | head -1)
fi

if [ -z "$GCOV_TOOL" ] || [ ! -x "$GCOV_TOOL" ]; then
    echo "ERROR: gcov tool not found in PATH or nix store"
    echo "Coverage data has been generated in ${BUILD_DIR}/"
    echo "You can manually process .gcda and .gcno files using a coverage tool."
    exit 1
fi

echo "Using gcov at: $GCOV_TOOL"

# Capture coverage data using lcov
echo "Capturing coverage data from ${BUILD_DIR}..."
lcov --capture \
     --directory "${BUILD_DIR}" \
     --output-file "${COVERAGE_INFO}" \
     --gcov-tool "$GCOV_TOOL" \
     --ignore-errors inconsistent \
     --base-directory . \
     --quiet

# Clean up any stray gcov files outside tmp/
find . -maxdepth 1 -name "*.gcov*" -delete 2>/dev/null || true

echo "✓ Coverage data captured"
echo ""

echo "Step 4: Filtering coverage to source files only..."
# Build the lcov extract command with all filter patterns
EXTRACT_CMD="lcov --extract ${COVERAGE_INFO}"
for filter in "${COVERAGE_FILTERS[@]}"; do
    EXTRACT_CMD="${EXTRACT_CMD} '${filter}'"
done
EXTRACT_CMD="${EXTRACT_CMD} --output-file ${COVERAGE_FILTERED} --quiet"

# Execute the extract command
eval "${EXTRACT_CMD}"

echo "✓ Filtered to LitePB source files"
echo ""

echo "Step 5: Generating HTML report..."
genhtml "${COVERAGE_FILTERED}" \
        --output-directory "${COVERAGE_REPORT}" \
        --title "LitePB Code Coverage" \
        --legend \
        --quiet

echo "✓ HTML report generated"
echo ""

echo "Step 6: Coverage Summary"
echo "========================"
lcov --list "${COVERAGE_FILTERED}"

echo ""
echo "======================================"
echo "Coverage report generated successfully!"
echo "Open ${COVERAGE_REPORT}/index.html to view the detailed report"
echo "======================================"