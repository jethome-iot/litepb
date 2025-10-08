#!/bin/bash
######################################################################
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

INTEROP_DIR="tests/cpp/interop"
INTEROP_BUILD_DIR="${INTEROP_DIR}/build"

mkdir -p "${INTEROP_BUILD_DIR}"
cd "${INTEROP_BUILD_DIR}"

# Check if we're in a Nix environment and apply special settings if needed
if [ -n "$NIX_PROFILES" ]; then
    echo "Detected Nix environment, configuring CMake to find Nix packages..."
    # Set up environment to help CMake find Nix-installed packages
    export CMAKE_PREFIX_PATH="${NIX_PROFILES// /:}:${CMAKE_PREFIX_PATH}"
    
    # Configure with explicit instructions to use pkg-config for finding libraries
    cmake .. \
      -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" \
      -DCMAKE_FIND_USE_CMAKE_PATH=YES \
      -DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=YES \
      -DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=YES \
      -DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=YES
else
    # Standard cmake for non-Nix environments
    cmake ..
fi

make
./interop_tests


