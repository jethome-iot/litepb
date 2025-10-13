# Replit Configuration for LitePB

## Overview
LitePB is a lightweight C++ Protocol Buffer serialization library designed for embedded systems. It offers wire format compatibility with standard Protocol Buffers, ensuring interoperability across different platforms. The project provides a robust, efficient solution for message serialization in resource-constrained environments with zero external dependencies.

## User Preferences
Communication style: Simple, everyday language

**Development Practices:**
- Always use the `litepb_gen` wrapper script in the project root to run the generator - never run Python directly from the generator/ folder
- Always set executable permissions (+x) for all scripts in `scripts/*.{sh,py}`
- Bash script standards:
  - Start all scripts with `set -e` (or `set -o pipefail` for pipeline-heavy scripts)
  - Import common error handling: `source "${SCRIPT_DIR}/lib/common.sh"`
  - This provides automatic error reporting with line numbers via `error_handler()` function
  - Use `SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"` for reliable path resolution
  - See existing scripts in `scripts/*.sh` for examples
- To run all PlatformIO targets from fresh, delete all build folders: `find . -name ".pio" -type d -print0 | xargs -0 rm -rf`
- Use `#pragma once` at the top of all C++ header files instead of traditional include guards - this is simpler, less error-prone, and supported by all modern compilers
- Run tests after each build: 1) `pio test` 2) `scripts/run_interop_tests.sh` 3) `scripts/run_platformio_examples.sh` (builds and runs examples) 4) `scripts/run_cmake_examples.sh` (builds and runs examples)

## System Architecture
- **Build System**: Supports both PlatformIO (for embedded development) and CMake (for general C++ projects). PlatformIO uses centralized directory configuration (src_dir = cpp/src, include_dir = cpp/include). CMake configuration in `cmake/CMakeLists.txt` builds LitePB as a static library with proper installation and packaging support. Both integrate with the Python-based code generator for Protocol Buffer files.
- **Serialization**: Wire format compatible with standard Protocol Buffers, supporting zigzag encoding, packed repeated fields, and map field serialization.
- **Code Generation**: A modular Python-based code generator (`generator/`) creates C++ files from `.proto` definitions using Jinja2 templates and Google Protobuf descriptors. The generator has an extensible architecture with language-agnostic parsing (`core/proto_parser.py`) and pluggable language backends (`backends/`). Currently supports C++, designed for easy addition of TypeScript, Dart, and other languages.
- **Code Style**: Enforced via `clang-format` with a `.clang-format` configuration (4-space indentation, left-aligned pointers, 132-character column limit).
- **Header Guards**: All C++ headers (manual and generated) use `#pragma once` for simplicity and modern compiler compatibility.
- **Containerized Development**: Utilizes a Docker environment (`ghcr.io/jethome-iot/litepb-dev:latest`) for consistent builds and CI/CD pipelines, including Python, build tools, code quality tools, and the `protoc` compiler.
- **Testing**: Comprehensive test suites including 177 PlatformIO unit tests and 10 interoperability tests. Code coverage reports are generated.
- **CI/CD**: GitHub Actions workflows (`ci.yml`, `build-docker.yml`) manage continuous integration and deployment, performing format checks, running tests, and building examples.

## External Dependencies
- **PlatformIO**: Build automation, dependency management, and testing frameworks for embedded projects.
- **uv**: Python package manager.
- **clang-format**: C++ code formatting tool.
- **lcov/genhtml**: Tools for generating code coverage reports.
- **CMake**: Used for building LitePB as a static library, CMake examples, and standalone interoperability tests.
- **Google Protocol Buffers**: Used for code generation via `protoc` compiler.
- **Jinja2**: Python templating engine used by the code generator.
