#!/usr/bin/env python3
"""Fix all RPC tests to work with the new protobuf-based protocol."""

import os
import re
import glob

def fix_transport_interface(content):
    """Fix Transport interface to use new signature."""
    # Fix send method
    content = re.sub(
        r'bool send\(const uint8_t\* data, size_t len\) override',
        'bool send(const uint8_t* data, size_t len, uint16_t src_addr, uint16_t dst_addr, uint32_t msg_id) override',
        content
    )
    
    # Fix recv method
    content = re.sub(
        r'size_t recv\(uint8_t\* buffer, size_t max_len\) override',
        'size_t recv(uint8_t* buffer, size_t max_len, uint16_t& src_addr, uint16_t& dst_addr, uint32_t& msg_id) override',
        content
    )
    
    return content

def fix_custom_error(content):
    """Replace CUSTOM_ERROR with TRANSPORT_ERROR."""
    content = content.replace('litepb::RpcError::CUSTOM_ERROR', 'litepb::RpcError::TRANSPORT_ERROR')
    return content

def fix_app_code(content):
    """Remove references to app_code."""
    # Remove app_code assignments
    content = re.sub(r'error\.app_code\s*=\s*[^;]+;', '// app_code removed', content)
    content = re.sub(r'result\.error\.app_code\s*=\s*[^;]+;', '// app_code removed', content)
    
    # Remove app_code checks  
    content = re.sub(r'TEST_ASSERT_EQUAL_UINT32\([^,]+,\s*[^.]+\.app_code\);?', '// app_code check removed', content)
    
    return content

def stub_obsolete_functions(content, test_file):
    """Stub out obsolete test functions."""
    obsolete_patterns = [
        r'(void test_[^(]*varint[^(]*\(\))\s*\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}',
        r'(void test_[^(]*framed_message[^(]*\(\))\s*\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}',
        r'(void test_[^(]*FramedMessage[^(]*\(\))\s*\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}',
    ]
    
    for pattern in obsolete_patterns:
        content = re.sub(
            pattern,
            r'\1\n{\n    // Test skipped - using new protobuf-based protocol\n}',
            content,
            flags=re.MULTILINE | re.DOTALL
        )
    
    return content

def comment_run_tests(content):
    """Comment out RUN_TEST calls for obsolete tests."""
    patterns = [
        r'RUN_TEST\(test_[^)]*varint[^)]*\)',
        r'RUN_TEST\(test_[^)]*framed_message[^)]*\)',
        r'RUN_TEST\(test_[^)]*FramedMessage[^)]*\)',
    ]
    
    for pattern in patterns:
        content = re.sub(
            f'^\\s*{pattern};?',
            lambda m: f'    // {m.group(0)} // Skipped - using new protobuf protocol',
            content,
            flags=re.MULTILINE
        )
    
    return content

def fix_test_file(filepath):
    """Fix a single test file."""
    print(f"Processing {filepath}...")
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Apply fixes
    content = fix_transport_interface(content)
    content = fix_custom_error(content)
    content = fix_app_code(content)
    content = stub_obsolete_functions(content, filepath)
    content = comment_run_tests(content)
    
    if content != original_content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"  Fixed {filepath}")
    else:
        print(f"  No changes needed for {filepath}")

def main():
    # Fix all RPC test files
    test_files = [
        'tests/platformio/rpc/test_core/rpc_core.cpp',
        'tests/platformio/rpc/test_error_propagation/error_propagation.cpp',
        'tests/platformio/rpc/test_integration/rpc_integration.cpp'
    ]
    
    # Also fix example test files
    example_test_files = glob.glob('tests/platformio/examples/rpc/litepb_rpc/*/**.cpp', recursive=True)
    test_files.extend(example_test_files)
    
    for filepath in test_files:
        if os.path.exists(filepath):
            fix_test_file(filepath)

if __name__ == '__main__':
    main()