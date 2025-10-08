#!/bin/bash

set -e

INTEROP_DIR="tests/cpp/interop"
INTEROP_BUILD_DIR="${INTEROP_DIR}/build"

mkdir -p "${INTEROP_BUILD_DIR}"
cd "${INTEROP_BUILD_DIR}"
cmake ..
make
./interop_tests


