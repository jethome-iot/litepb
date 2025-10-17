#!/bin/bash
######################################################################
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

# Change to project root directory (parent of scripts/)
cd "$(dirname "$0")/.." || exit 1

# File patterns to format
FILE_PATTERNS="*.h *.hpp *.cpp"

echo "Formatting C++ files..."

# Find and format all tracked C++ files
git ls-files $FILE_PATTERNS | while read -r file; do
    echo "Formatting: $file"
    clang-format -i "$file"
done

echo "Formatting complete!"
