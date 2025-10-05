#!/bin/bash
GCOV_PATH=$(find /nix/store/*gcc*14.2* -name "gcov" -type f 2>/dev/null | head -1)
exec "$GCOV_PATH" "$@"
