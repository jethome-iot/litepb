# Replit Configuration for LitePB

## Overview
LitePB is a lightweight C++ Protocol Buffer serialization and RPC framework designed for embedded systems. It offers wire format compatibility with standard Protocol Buffers, ensuring interoperability across different platforms. The project aims to provide a robust, efficient solution for message serialization and remote procedure calls in resource-constrained environments.

## User Preferences
Communication style: Simple, everyday language

**Development Practices:**
- Always set executable permissions (+x) for all scripts in `scripts/*.{sh,py}`
- All bash scripts should include `set -e` at the top to exit immediately if any command fails, preventing cascading errors
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