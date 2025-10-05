#!/bin/bash

# Change to project root directory (parent of scripts/)
cd "$(dirname "$0")/.." || exit 1

# File patterns to check
FILE_PATTERNS="*.h *.hpp *.cpp"

echo "Checking C++ file formatting..."

# Collect all files that need formatting
files_with_errors=()

# Find and check all tracked C++ files
while IFS= read -r file; do
    if ! clang-format --dry-run -Werror "$file" 2>&1 > /dev/null; then
        files_with_errors+=("$file")
    fi
done < <(git ls-files $FILE_PATTERNS)

# Check if we had any errors
if [ ${#files_with_errors[@]} -gt 0 ]; then
    echo "❌ Format check failed! The following files need formatting:"
    for file in "${files_with_errors[@]}"; do
        echo "  - $file"
    done
    echo ""
    echo "Please run './scripts/format.sh' to fix formatting."
    exit 1
fi

echo "✅ All files are properly formatted!"
exit 0
