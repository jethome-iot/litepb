#!/bin/bash
######################################################################
set -e
# For safer bash scripting; ignore if running on very old bash without -o pipefail support
set -o pipefail 2>/dev/null || true
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

# Ensure PlatformIO is available before proceeding
if ! command -v pio >/dev/null 2>&1; then
    echo "Error: 'pio' (PlatformIO CLI) not found in PATH." >&2
    echo "Install with: pip install platformio (or refer to project docs)." >&2
    exit 1
fi

EXAMPLES_DIR="$(realpath "${SCRIPT_DIR}/../examples/cpp/platformio")"

# Collect all directories (at any depth) containing a platformio.ini file.
# Use -print0 for NUL delim (portable) and derive the directory via dirname inside loop.
PLATFORMIO_PROJECTS=()
while IFS= read -r -d '' ini_file; do
    dir="$(dirname "${ini_file}")"
    PLATFORMIO_PROJECTS+=("$dir")
done < <(find "${EXAMPLES_DIR}" -type f -name 'platformio.ini' -print0)

TOTAL_PROJECTS=${#PLATFORMIO_PROJECTS[@]}
FAILED_PROJECTS=0

if [ "$TOTAL_PROJECTS" -eq 0 ]; then
    echo "No PlatformIO example projects were found under ${EXAMPLES_DIR}." >&2
    exit 1
fi

echo "======================================"
echo "LitePB PlatformIO Examples Runner"
echo "======================================"
echo ""
echo "Found ${TOTAL_PROJECTS} PlatformIO example projects to build and run."
echo ""
for PROJECT_DIR in "${PLATFORMIO_PROJECTS[@]}"; do
    echo "--------------------------------------"
    echo "Building project: ${PROJECT_DIR}"
    echo "--------------------------------------"
    pushd "$PROJECT_DIR" > /dev/null || exit 1
    
    # Build the project
    if pio run; then
        echo "✓ Build PASSED for ${PROJECT_DIR}"
        
        # Try to run if it's a native environment
        if pio run --target exec 2>/dev/null; then
            echo "✓ Execution PASSED for ${PROJECT_DIR}"
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
    echo "All ${TOTAL_PROJECTS} PlatformIO example projects built successfully!"
    exit 0
else
    echo "${FAILED_PROJECTS} out of ${TOTAL_PROJECTS} PlatformIO example projects failed to build."
    exit 1
fi
