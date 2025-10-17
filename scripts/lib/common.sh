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