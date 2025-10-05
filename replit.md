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

## Docker Environment

**Docker Files Location:**
- Docker infrastructure is organized in the `docker/` directory
- `docker/Dockerfile` - Container definition for development environment

**Containerized Build System:**
- All GitHub Actions CI jobs run in a Docker container for consistent, fast builds
- Eliminates setup overhead (~1-2 minutes per job saved)
- Official image: `ghcr.io/jethome-iot/litepb-dev:latest` (used by all CI runs)

**Container Contents:**
- Python 3.11 with PlatformIO, protobuf, jinja2
- Build tools: gcc, g++, make, CMake, build-essential
- Code quality: clang-format, lcov, genhtml
- Protocol Buffers: protoc compiler with libprotobuf-dev, libabsl-dev
- Non-root user (builder) for security

**Docker Image:**
- **Official Image**: `ghcr.io/jethome-iot/litepb-dev:latest`
- **Build Trigger**: Updates when `docker/Dockerfile` changes
- **Versioning**: Tagged with both `latest` and commit SHA
- **Usage**: All CI jobs use this image for consistency

**Benefits:**
- Consistent environment across GitHub Actions and local development
- Faster CI (no dependency installation time)
- Easy dependency versioning
- Same tools everywhere

## Project Documentation
For full project documentation, see [README.md](README.md)

## Replit-Specific Workflows

**Core Tests:**
- **PlatformIO Tests** - Runs all tests (core + examples) with auto-regeneration
- **Code Coverage** - Generates HTML coverage reports in `tmp/coverage/coverage_report/`

**Example Workflows:**
- **LitePB RPC Example** - Multi-service RPC example (Sensor + Switch services)
- **LitePB RPC Example Tests** - Comprehensive test suites for RPC functionality

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


## Current Status

### RPC Protocol (Version 1)
- **Protobuf-based Protocol**: RpcMessage with protocol versioning for backward compatibility
- **Clean Layer Separation**: Transport handles addressing (src/dst/msg_id), RPC handles protocol
- **Explicit Message Types**: REQUEST, RESPONSE, EVENT replacing magic values
- **Error Separation**: RPC errors (timeout, transport) vs application errors
- **All Tests Passing**: 298/298 test cases successful after refactoring

### Library Manifest
- **library.json**: PlatformIO-compliant manifest with schema validation, proper keywords array, examples listing, and export rules
- Successfully validated with `pio pkg pack` command
- Ready for PlatformIO Registry publication

### Serialization Examples
- **Basic Example** (`examples/serialization/basic/`): Person message demo, 36-byte serialization
- **All Types Example** (`examples/serialization/all_types/`): Comprehensive protobuf types showcase, 335-byte serialization

### Test Coverage
- **Coverage Reports**: Generated in `tmp/coverage/` directory structure
- **Report Organization**: Separate directories for core and RPC coverage analysis
- **Comprehensive Test Suite**: Full coverage across core functionality, protocol features, and RPC implementation

### CI/CD
- **GitHub Actions**: 5 jobs (Format Check, PlatformIO Tests, Test Interop, Build Examples, Code Coverage)
- **Environment**: Ubuntu 24.04 GitHub-hosted runners
- **Container**: `ghcr.io/jethome-iot/litepb-dev:latest` Docker image for consistent builds
- **Code Quality**: Enforced via clang-format style rules on all C++ code
- **Build Speed**: Docker-based CI eliminates dependency installation overhead
- **Fork Support**: Fork repositories use the main repository's Docker image