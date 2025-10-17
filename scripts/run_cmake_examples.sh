#!/bin/bash
######################################################################
set -e
# For safer bash scripting; ignore if running on very old bash without -o pipefail support
set -o pipefail 2>/dev/null || true
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

# Ensure CMake is available before proceeding
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: 'cmake' not found in PATH." >&2
    echo "Install CMake to run these examples." >&2
    exit 1
fi

EXAMPLES_DIR="$(realpath "${SCRIPT_DIR}/../examples/cpp/cmake")"

if [ ! -d "${EXAMPLES_DIR}" ]; then
    echo "No CMake examples directory found at ${EXAMPLES_DIR}." >&2
    exit 1
fi

# Collect all directories containing a CMakeLists.txt file
CMAKE_PROJECTS=()
while IFS= read -r -d '' cmake_file; do
    dir="$(dirname "${cmake_file}")"
    CMAKE_PROJECTS+=("$dir")
done < <(find "${EXAMPLES_DIR}" -mindepth 2 -maxdepth 2 -type f -name 'CMakeLists.txt' -print0)

TOTAL_PROJECTS=${#CMAKE_PROJECTS[@]}
FAILED_PROJECTS=0

if [ "$TOTAL_PROJECTS" -eq 0 ]; then
    echo "No CMake example projects were found under ${EXAMPLES_DIR}." >&2
    exit 1
fi

echo "======================================"
echo "LitePB CMake Examples Test Runner"
echo "======================================"
echo ""
echo "Found ${TOTAL_PROJECTS} CMake example projects to test."
echo ""

for PROJECT_DIR in "${CMAKE_PROJECTS[@]}"; do
    echo "--------------------------------------"
    echo "Building project: ${PROJECT_DIR}"
    echo "--------------------------------------"
    
    BUILD_DIR="${PROJECT_DIR}/build"
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    
    pushd "$PROJECT_DIR" > /dev/null || exit 1
    
    if cmake -B build && cmake --build build; then
        echo "✓ Build PASSED for ${PROJECT_DIR}"
        
        # Try to run the executable if it exists
        EXECUTABLE=$(find build -maxdepth 1 -type f -executable ! -name "*.so" ! -name "*.a" | head -n 1)
        if [ -n "$EXECUTABLE" ]; then
            echo "Running executable: $EXECUTABLE"
            if "$EXECUTABLE"; then
                echo "✓ Execution PASSED for ${PROJECT_DIR}"
            else
                echo "❌ Execution FAILED for ${PROJECT_DIR}"
                FAILED_PROJECTS=$((FAILED_PROJECTS + 1))
            fi
        fi
    else
        echo "❌ Build FAILED for ${PROJECT_DIR}"
        FAILED_PROJECTS=$((FAILED_PROJECTS + 1))
    fi
    
    popd > /dev/null || exit 1
    echo ""
done

echo "======================================"
if [ $FAILED_PROJECTS -eq 0 ]; then
    echo "All ${TOTAL_PROJECTS} CMake example projects built successfully!"
    exit 0
else
    echo "${FAILED_PROJECTS} out of ${TOTAL_PROJECTS} CMake example projects failed."
    exit 1
fi
