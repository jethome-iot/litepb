#!/bin/bash
######################################################################
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

echo "======================================="
echo "Running LitePB PlatformIO Unit Tests"
echo "======================================="
echo ""

# Run PlatformIO tests
pio test
