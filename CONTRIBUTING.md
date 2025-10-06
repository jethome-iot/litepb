# Contributing to LitePB

Thank you for your interest in contributing to LitePB! We welcome contributions from the community to help improve this lightweight Protocol Buffers implementation for embedded systems.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Code Style](#code-style)
- [Testing](#testing)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)
- [Development Workflow](#development-workflow)
- [Architecture Overview](#architecture-overview)

## Code of Conduct

### Our Standards

- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on constructive criticism
- Accept responsibility for mistakes
- Prioritize the project's best interests

### Unacceptable Behavior

- Harassment, discrimination, or offensive language
- Personal attacks or trolling
- Publishing private information without permission
- Any conduct inappropriate for a professional setting

## Getting Started

### Prerequisites

Before contributing, ensure you have:

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Python 3.7+ for the code generator
- Protocol Buffers compiler (`protoc`)
- PlatformIO Core (for running tests)
- Git for version control

### Installation

```bash
# Clone the repository
git clone https://github.com/jethome-iot/litepb.git
cd litepb

# Install PlatformIO Core (if not already installed)
pip install platformio

# Install Python dependencies for the generator
pip install -r generator/requirements.txt

# Verify installation
./litepb_gen --help
pio test --list-targets
```

## Development Setup

### Project Structure

```
litepb/
├── generator/          # Python code generator
│   ├── litepb_gen.py  # Main generator script
│   ├── cpp_generator.py # C++ code generation
│   └── proto_parser.py # Proto file parsing
├── cpp/               # C++ runtime library
│   ├── include/       # Public headers
│   └── src/           # Implementation files
├── tests/             # Test suite
│   ├── platformio/    # PlatformIO tests
│   └── interop/       # Interoperability tests
├── examples/          # Example projects
└── docs/              # Documentation
```

### Setting Up Your Development Environment

#### VSCode (Recommended)

```json
// .vscode/settings.json
{
    "C_Cpp.default.configurationProvider": "platformio.platformio-ide",
    "files.associations": {
        "*.pb.h": "cpp",
        "*.pb.cpp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_style": "file"
}
```

#### CLion

```xml
<!-- .idea/fileTemplates/LitePB Message.h -->
#pragma once

namespace ${NAMESPACE} {

struct ${NAME} {
    // Fields here
};

} // namespace ${NAMESPACE}
```

## Code Style

We use **clang-format** with a 4-space indentation style. A `.clang-format` file is provided in the repository root.

### C++ Code Style

#### General Rules

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Maximum 120 characters
- **Braces**: Opening brace on same line for functions and classes
- **Naming**:
  - Classes: `PascalCase`
  - Functions: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Namespaces: `lowercase`

#### Example

```cpp
namespace litepb {

class MessageParser {
public:
    explicit MessageParser(InputStream& stream) 
        : stream_(stream) {
        // Constructor body
    }
    
    bool parse_message(Message& msg) {
        if (!validate_header()) {
            return false;
        }
        
        return read_fields(msg);
    }
    
private:
    static constexpr size_t MAX_MESSAGE_SIZE = 1024;
    
    InputStream& stream_;
    
    bool validate_header() const {
        // Implementation
        return true;
    }
    
    bool read_fields(Message& msg) {
        // Implementation
        return true;
    }
};

} // namespace litepb
```

### Python Code Style

Follow PEP 8 with these additions:
- Maximum line length: 120 characters
- Use type hints for function signatures
- Document classes and functions with docstrings

```python
from typing import List, Optional

class ProtoGenerator:
    """Generates C++ code from Protocol Buffers definitions."""
    
    def __init__(self, output_dir: str):
        """
        Initialize the generator.
        
        Args:
            output_dir: Directory for generated files
        """
        self.output_dir = output_dir
    
    def generate(self, proto_file: str) -> bool:
        """
        Generate C++ code from a proto file.
        
        Args:
            proto_file: Path to the .proto file
            
        Returns:
            True if generation succeeded
        """
        # Implementation
        return True
```

### Formatting Your Code

Before submitting:

```bash
# Format C++ code
clang-format -i cpp/src/**/*.cpp cpp/include/**/*.h

# Format Python code
black generator/*.py --line-length 120

# Check formatting without modifying
clang-format -n cpp/src/**/*.cpp
```

## Testing

### Test Structure

Tests are organized into categories:

- **Unit Tests**: Test individual components
- **Integration Tests**: Test component interactions
- **Interop Tests**: Verify wire format compatibility
- **Platform Tests**: Platform-specific functionality

### Running Tests

#### All Tests

```bash
# Run all PlatformIO tests
pio test

# Run with verbose output
pio test -vvv
```

#### Specific Test Categories

```bash
# Core functionality tests
pio test -e test_core

# RPC tests
pio test -e test_rpc_core

# Serialization tests
pio test -f "*serialization*"

# Interoperability tests
cd tests/cpp/interop
./run_tests.sh
```

#### Writing New Tests

Create test files following the pattern:

```cpp
// tests/platformio/feature/test_feature.cpp
#include <unity.h>
#include "litepb/litepb.h"

void test_feature_basic() {
    // Arrange
    MyMessage msg;
    msg.field = 42;
    
    // Act
    litepb::BufferOutputStream output;
    bool result = litepb::serialize(msg, output);
    
    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(5, output.size());
}

void test_feature_edge_case() {
    // Test implementation
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_feature_basic);
    RUN_TEST(test_feature_edge_case);
    
    return UNITY_END();
}
```

### Test Coverage

Ensure your changes maintain or improve test coverage:

```bash
# Generate coverage report
./scripts/coverage.sh

# View coverage report
open tmp/coverage/coverage_report/index.html
```

Aim for:
- Minimum 80% line coverage for new code
- 100% coverage for critical paths
- Edge cases and error conditions tested

## Pull Request Process

### Before Creating a PR

1. **Fork and Clone**
   ```bash
   git clone https://github.com/your-username/litepb.git
   cd litepb
   git remote add upstream https://github.com/jethome-iot/litepb.git
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Changes**
   - Write clean, documented code
   - Add tests for new functionality
   - Update documentation as needed

4. **Test Thoroughly**
   ```bash
   # Run all tests
   pio test
   
   # Check formatting
   clang-format -n cpp/**/*.{h,cpp}
   
   # Run interop tests
   cd tests/cpp/interop && ./run_tests.sh
   ```

5. **Commit Changes**
   ```bash
   git add .
   git commit -m "feat: Add support for new feature
   
   - Detailed description of changes
   - Related issue numbers (fixes #123)"
   ```

### Commit Message Format

Follow conventional commits:

```
type(scope): Subject line

Body paragraph explaining the change in detail.

Footer with issue references
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Test additions or changes
- `chore`: Build process or auxiliary tool changes

### Creating the Pull Request

1. **Push to Your Fork**
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Open PR on GitHub**
   - Use a clear, descriptive title
   - Fill out the PR template completely
   - Link related issues
   - Add appropriate labels

3. **PR Description Template**
   ```markdown
   ## Description
   Brief description of changes
   
   ## Type of Change
   - [ ] Bug fix
   - [ ] New feature
   - [ ] Breaking change
   - [ ] Documentation update
   
   ## Testing
   - [ ] All tests pass
   - [ ] Added new tests
   - [ ] Updated documentation
   
   ## Related Issues
   Fixes #123
   ```

### Review Process

1. **Automated Checks**
   - CI builds must pass
   - All tests must pass
   - Code formatting check

2. **Code Review**
   - At least one maintainer approval required
   - Address all feedback constructively
   - Update PR based on review comments

3. **Merge**
   - Squash and merge for feature branches
   - Preserve commit history for large changes
   - Delete feature branch after merge

## Issue Guidelines

### Reporting Bugs

Use the bug report template and include:

```markdown
**Description**
Clear description of the bug

**To Reproduce**
1. Step-by-step instructions
2. Code snippet if applicable
3. Proto file definition

**Expected Behavior**
What should happen

**Actual Behavior**
What actually happens

**Environment**
- LitePB version: 
- Platform/OS: 
- Compiler: 
- PlatformIO version: 

**Additional Context**
Stack traces, error messages, etc.
```

### Feature Requests

```markdown
**Feature Description**
What feature would you like to see?

**Use Case**
Why is this feature needed?

**Proposed Solution**
How might this be implemented?

**Alternatives Considered**
Other approaches you've thought about

**Additional Context**
Examples, references, etc.
```

### Good First Issues

Look for issues labeled `good first issue`:
- Simple bug fixes
- Documentation improvements
- Test additions
- Small feature additions

## Development Workflow

### Adding a New Feature

1. **Design Phase**
   - Discuss in an issue first
   - Consider embedded system constraints
   - Ensure wire format compatibility

2. **Implementation**
   ```bash
   # Create feature branch
   git checkout -b feature/new-feature
   
   # Implement feature
   # Add to cpp/include/litepb/
   # Add to cpp/src/litepb/
   
   # Add tests
   # Create tests/platformio/test_new_feature/
   
   # Update documentation
   # Edit docs/API.md
   ```

3. **Generator Updates**
   ```python
   # If modifying code generation
   # Edit generator/cpp_generator.py
   # Update generator/type_mapper.py if needed
   ```

4. **Testing**
   ```bash
   # Test generated code
   ./litepb_gen test.proto -o test_output/
   
   # Verify compilation
   g++ -std=c++17 -I cpp/include test_output/*.cpp
   
   # Run tests
   pio test -e test_new_feature
   ```

### Debugging Tips

#### Using GDB with PlatformIO

```bash
# Debug native tests
pio test -e native --verbose
gdb .pio/build/native/program
```

#### Adding Debug Output

```cpp
#ifdef LITEPB_DEBUG
    printf("[DEBUG] Processing field %d\n", field_number);
#endif
```

Enable debug mode:
```bash
pio test -e native -D LITEPB_DEBUG
```

#### Memory Debugging

```cpp
// Use valgrind for memory leak detection
valgrind --leak-check=full .pio/build/native/program
```

## Architecture Overview

### Key Design Principles

1. **Zero Dependencies**: No external libraries in runtime
2. **Header-Only Option**: Can be used as header-only library
3. **Embedded-First**: Optimized for constrained environments
4. **Type Safety**: Compile-time type checking
5. **Wire Compatibility**: 100% protobuf wire format compatible

### Component Responsibilities

#### Generator (`generator/`)
- Parse .proto files using protoc
- Generate C++ structs and serialization code
- Handle service definitions for RPC

#### Core Library (`cpp/include/litepb/core/`)
- Stream abstractions
- Wire format encoding/decoding
- Low-level serialization primitives

#### RPC Layer (`cpp/include/litepb/rpc/`)
- Message routing and dispatch
- Transport abstraction
- Timeout and error handling

### Adding Protocol Features

To add support for new protobuf features:

1. **Update Parser**
   ```python
   # generator/proto_parser.py
   def parse_new_feature(self, descriptor):
       # Parse from protoc descriptor
   ```

2. **Update Type Mapping**
   ```python
   # generator/type_mapper.py
   def map_new_type(self, proto_type):
       # Map to C++ type
   ```

3. **Update Generator**
   ```python
   # generator/cpp_generator.py
   def generate_new_feature(self, feature):
       # Generate C++ code
   ```

4. **Add Runtime Support**
   ```cpp
   // cpp/include/litepb/new_feature.h
   template<typename T>
   class NewFeature {
       // Implementation
   };
   ```

5. **Add Tests**
   ```cpp
   // tests/platformio/test_new_feature/
   void test_new_feature() {
       // Test implementation
   }
   ```

## Release Process

### Version Numbering

We follow Semantic Versioning (MAJOR.MINOR.PATCH):
- **MAJOR**: Breaking API changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes

### Release Checklist

- [ ] All tests pass
- [ ] Documentation updated
- [ ] CHANGELOG updated
- [ ] Version bumped in library.json
- [ ] Tag created and pushed
- [ ] GitHub release created
- [ ] PlatformIO registry updated

## Getting Help

### Resources

- **Documentation**: [docs/](docs/)
- **Examples**: [examples/](examples/)
- **Issues**: [GitHub Issues](https://github.com/jethome-iot/litepb/issues)
- **Discussions**: [GitHub Discussions](https://github.com/jethome-iot/litepb/discussions)

### Contact

- Create an issue for bugs or features
- Start a discussion for questions
- Email: support@jethome.ru (for security issues only)

## Recognition

Contributors will be recognized in:
- GitHub contributors list
- Release notes for significant contributions
- CONTRIBUTORS.md file (for major contributors)

Thank you for contributing to LitePB!