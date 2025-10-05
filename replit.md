# Replit Configuration for LitePB

## Overview
LitePB is a lightweight Protocol Buffer serialization and RPC framework for embedded systems. It provides a C++ implementation for encoding/decoding Protocol Buffer messages with wire format compatibility with standard Protocol Buffers, enabling interoperability across platforms.

## User Preferences
Communication style: Simple, everyday language

**Development Practices:**
- Always set executable permissions (+x) for all scripts in `scripts/**/*.{sh,py}`

## System Architecture
- **Build System**: PlatformIO with consistent workflow between local development and CI/CD
- **RPC System**: 8-byte source/destination addressing supporting direct, broadcast, and wildcard messaging
- **Error Handling**: Two-layer system with RPC-layer errors and application-specific error codes
- **Serialization**: Wire format compatible with standard Protocol Buffers including zigzag encoding, packed repeated fields, and map field serialization
- **Code Style**: Enforced via clang-format (4-space indentation, left-aligned pointers, 132-character column limit)

## External Dependencies
- **PlatformIO**: Build automation, dependency management, and testing frameworks
- **uv**: Python package management
- **clang-format**: C++ code formatting
- **lcov/genhtml**: Code coverage report generation
- **CMake**: Building standalone interoperability tests
- **CPM**: Managing C++ dependencies for interoperability tests

## Project Documentation
For full project documentation, see [README.md](README.md)

## Replit-Specific Workflows

**Core Tests:**
- **PlatformIO Tests** - Runs all tests (core + examples) with auto-regeneration
- **Code Coverage** - Generates coverage reports (93.8% line coverage, 100% function coverage)

**Example Workflows:**
- **LitePB RPC Example** - Multi-service RPC example (Sensor + Switch services)
- **LitePB RPC Example Tests** - 38 tests across 4 test suites

## Code Formatting

All C++ code follows strict formatting rules enforced by clang-format:

**Configuration:**
- `.clang-format` - Defines code style rules (4-space indentation, left-aligned pointers, 132-character column limit)
- Consistent formatting across all contributors

**Local Formatting Scripts:**
- **`scripts/format.sh`** - Automatically formats all C++ files (*.h, *.hpp, *.cpp)
- **`scripts/format_check.sh`** - Checks formatting without modifying files; lists files that need formatting

**CI Enforcement:**
- Format Check job runs first in CI pipeline for fast feedback
- Automatically rejects PRs with formatting violations
- Clear error messages show which files need formatting

**Usage:**
```bash
# Format all files
./scripts/format.sh

# Check formatting (exit 0 if OK, exit 1 with list of files if not)
./scripts/format_check.sh
```

## GitHub Sync Scripts

Generic scripts for syncing from private Replit repo to public GitHub repo:

**scripts/dev/github_sync_initial.sh** - One-time initial push to public repo
- Auto-detects repo names (strips `-replit` suffix for public)
- Filters out Replit-specific files using `.github_sync_filter`
- Shows preview of files to be synced before confirmation
- Pushes directly to public master branch
- Usage: `./scripts/dev/github_sync_initial.sh`

**scripts/dev/github_sync_to_public.sh** - Ongoing syncs via PR
- Auto-detects repo names
- Syncs from private dev → public master (via PR)
- Shows preview of files to be synced before confirmation
- Usage: `./scripts/dev/github_sync_to_public.sh`

**.github_sync_filter** - List of Replit-specific files/directories to exclude
- Located at project root as hidden file
- Excludes `scripts/dev/` to prevent syncing dev tools

Both scripts support:
- `-h` / `--help` for usage info
- `--dry-run` to preview changes
- `--yes` to skip confirmation prompts
- File preview showing what will be synced (after filtering)
- Verbose output with section headers and status indicators

## Current Status

### Library Manifest
- **library.json**: PlatformIO-compliant manifest with schema validation, proper keywords array, examples listing, and export rules
- Successfully validated with `pio pkg pack` command
- Ready for PlatformIO Registry publication

### Serialization Examples
- **Basic Example** (`examples/serialization/basic/`): Person message demo, 36-byte serialization
- **All Types Example** (`examples/serialization/all_types/`): Comprehensive protobuf types showcase, 335-byte serialization

### Test Coverage
- **Overall**: 93.8% line coverage (320/341 lines), 100% function coverage (38/38 functions)
- **Test Suites**: 146 tests across 5 core test suites + 38 RPC example tests

### CI/CD
- All 5 GitHub Actions jobs passing (Format Check, PlatformIO Tests, Test Interop, Build Examples, Code Coverage)
- Format checking enforces clang-format style rules on all C++ code
- Reusable composite action for setup reduces workflow duplication