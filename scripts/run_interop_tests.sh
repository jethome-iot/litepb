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
cmake ..
make
./interop_tests


