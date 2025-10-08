# Replit Configuration for LitePB

## Overview
LitePB is a lightweight C++ Protocol Buffer serialization and RPC framework designed for embedded systems. It offers wire format compatibility with standard Protocol Buffers, ensuring interoperability across different platforms. The project aims to provide a robust, efficient solution for message serialization and remote procedure calls in resource-constrained environments.

## Recent Changes
**October 2025 - Complete Generator Rewrite:**
- **Completely rewrote the Python generator from scratch** with clean, modular architecture
- **New extensible architecture** designed for easy addition of new language backends (TypeScript, Dart, etc.)
- **Core components:**
  - `core/parser.py`: Language-agnostic proto parsing using protoc
  - `core/descriptor_utils.py`: Helper functions for protobuf descriptors
  - `backends/base.py`: Abstract interface that all language generators must implement
  - `backends/cpp/`: Complete C++ backend with modular components
- **Key improvements:**
  - Separated language-agnostic parsing from language-specific code generation
  - Implemented proper topological sorting for dependency resolution
  - Fixed all C++ template instantiation ordering issues
  - Added forward declarations for Serializer specializations
  - Fixed wire format for maps with message values (proper length-delimited encoding)
- **Full protobuf feature support:**
  - All scalar types, repeated fields, maps, enums, nested messages, oneofs
  - Proto2 and proto3 syntax with proper semantics
  - RPC service generation with custom options support
- **Test results: ALL 170 tests passing (100% success rate)**
- **Ready for language expansion:** New languages can be added by implementing the `LanguageGenerator` interface

## User Preferences
Communication style: Simple, everyday language

**Development Practices:**
- Always set executable permissions (+x) for all scripts in `scripts/*.{sh,py}`
- Bash script standards:
  - Start all scripts with `set -e` (or `set -o pipefail` for pipeline-heavy scripts)
  - Import common error handling: `source "${SCRIPT_DIR}/lib/common.sh"`
  - This provides automatic error reporting with line numbers via `error_handler()` function
  - Use `SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"` for reliable path resolution
  - See existing scripts in `scripts/*.sh` for examples
- To run all PlatformIO targets from fresh, delete all build folders: `find . -name ".pio" -type d -print0 | xargs -0 rm -rf`
- Use `#pragma once` at the top of all C++ header files instead of traditional include guards - this is simpler, less error-prone, and supported by all modern compilers
- Run tests after each build: 1) `pio test` 2) `scripts/run_interop_tests.sh` 3) `scripts/run_examples_tests.sh`

## System Architecture
- **Build System**: PlatformIO with centralized directory configuration (src_dir = cpp/src, include_dir = cpp/include). Integrates with a Python-based code generator for Protocol Buffer files.
- **RPC System**: Optional feature (`custom_litepb_use_rpc`) with clean layer separation for transport addressing and message correlation. Features a protobuf-based protocol (`RpcMessage`) with versioning, explicit message types (REQUEST, RESPONSE, EVENT), and distinct error handling for RPC vs. application errors.
- **Serialization**: Wire format compatible with standard Protocol Buffers, supporting zigzag encoding, packed repeated fields, and map field serialization.
- **Code Generation**: A modular Python-based code generator (`generator/`) creates C++ files from `.proto` definitions. It uses Jinja2 templates, Google Protobuf descriptor objects, and supports custom RPC options. Generated files are not committed to the repository.
- **Code Style**: Enforced via `clang-format` with a `.clang-format` configuration (4-space indentation, left-aligned pointers, 132-character column limit).
- **Header Guards**: All C++ headers (manual and generated) use `#pragma once` for simplicity and modern compiler compatibility.
- **Containerized Development**: Utilizes a Docker environment (`ghcr.io/jethome-iot/litepb-dev:latest`) for consistent builds and CI/CD pipelines, including Python, build tools, code quality tools, and the `protoc` compiler.
- **Testing**: Comprehensive test suites for core functionality, serialization (proto2/proto3), and RPC, organized to mirror proto structure. Code coverage reports are generated.
- **CI/CD**: GitHub Actions workflows (`ci.yml`, `build-docker.yml`) manage continuous integration and deployment, performing format checks, running tests (PlatformIO, interoperability), and building examples.

## External Dependencies
- **PlatformIO**: Build automation, dependency management, and testing frameworks for embedded projects.
- **uv**: Python package manager.
- **clang-format**: C++ code formatting tool.
- **lcov/genhtml**: Tools for generating code coverage reports.
- **CMake**: Used for building standalone interoperability tests.
- **CPM**: C++ Package Manager for managing C++ dependencies in interoperability tests.
- **Google Protocol Buffers**: Used for message serialization and definition.
- **Jinja2**: Python templating engine used by the code generator.