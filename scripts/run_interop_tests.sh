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

# Set up environment to help CMake find Nix-installed packages
export CMAKE_PREFIX_PATH="${NIX_PROFILES// /:}:${CMAKE_PREFIX_PATH}"
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"

# Configure with explicit instructions to use pkg-config for finding abseil
cmake .. \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" \
  -DCMAKE_FIND_USE_CMAKE_PATH=YES \
  -DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=YES \
  -DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=YES \
  -DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=YES

make
./interop_tests


