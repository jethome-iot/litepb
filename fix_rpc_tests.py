#!/usr/bin/env python3
"""Fix RPC tests to work with the new protobuf-based protocol."""

import re

# Tests to comment out completely
obsolete_tests = [
    "test_varint_encoding_small_values",
    "test_varint_encoding_medium_values",
    "test_varint_encoding_large_values",
    "test_varint_decoding_small_values",
    "test_varint_decoding_medium_values",
    "test_varint_decoding_large_values",
    "test_varint_roundtrip_encoding_decoding",
    "test_framed_message_encode_decode_stream_transport",
    "test_framed_message_encode_decode_packet_transport",
    "test_framed_message_integrity_verification",
    "test_framed_message_method_id_zero",
    "test_framed_message_empty_payload",
    "test_decode_varint_5byte_limit_exceeded",
]

def comment_out_test_functions(content):
    """Comment out obsolete test functions."""
    for test_name in obsolete_tests:
        # Find and replace entire function bodies with a stub
        pattern = rf'(void {test_name}\(\))\s*\{{[^{{}}]*(?:\{{[^{{}}]*\}}[^{{}}]*)*\}}'
        replacement = rf'\1\n{{\n    // Test skipped - using new protobuf-based protocol\n}}'
        content = re.sub(pattern, replacement, content, flags=re.MULTILINE | re.DOTALL)
    return content

def comment_out_test_runs(content):
    """Comment out RUN_TEST calls for obsolete tests."""
    for test_name in obsolete_tests:
        pattern = rf'^\s*RUN_TEST\({test_name}\);'
        replacement = f'    // RUN_TEST({test_name}); // Skipped - using new protobuf protocol'
        content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
    return content

def main():
    # Fix test_core/rpc_core.cpp
    with open('tests/platformio/rpc/test_core/rpc_core.cpp', 'r') as f:
        content = f.read()
    
    content = comment_out_test_functions(content)
    content = comment_out_test_runs(content)
    
    with open('tests/platformio/rpc/test_core/rpc_core.cpp', 'w') as f:
        f.write(content)
    
    print("Fixed tests/platformio/rpc/test_core/rpc_core.cpp")
    
if __name__ == '__main__':
    main()