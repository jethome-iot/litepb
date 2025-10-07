#!/usr/bin/env python3
"""
Protobuf file parser using protoc compiler.
Parses .proto files and extracts descriptors for code generation.
"""

import os
import subprocess
import tempfile
from google.protobuf import descriptor_pb2
from typing import List, Dict, Optional
from rpc_options import RpcMethodOptions, RpcServiceOptions, MethodOptions, ServiceOptions


class ProtoParser:
    """Parse .proto files using protoc and return FileDescriptorProto."""
    
    def __init__(self, import_paths: Optional[List[str]] = None):
        """Initialize parser with optional import paths."""
        self.import_paths = import_paths or []
    
    def parse_proto_file(self, proto_path: str) -> descriptor_pb2.FileDescriptorProto:
        """
        Parse a .proto file and return FileDescriptorProto.
        
        Args:
            proto_path: Path to the .proto file
            
        Returns:
            FileDescriptorProto object for the parsed file
        """
        # Use protoc to generate descriptor set
        descriptor_set = self._run_protoc(proto_path)
        
        # Parse the descriptor set
        file_descriptor_set = descriptor_pb2.FileDescriptorSet()
        file_descriptor_set.ParseFromString(descriptor_set)
        
        # Return the main file (last one in the set)
        if file_descriptor_set.file:
            return file_descriptor_set.file[-1]
        
        # Return empty FileDescriptorProto if no files found
        return descriptor_pb2.FileDescriptorProto()
    
    def _run_protoc(self, proto_path: str) -> bytes:
        """Run protoc compiler to generate descriptor set."""
        with tempfile.NamedTemporaryFile(suffix='.desc', delete=False) as tmp:
            tmp_path = tmp.name
        
        try:
            # Build protoc command
            cmd = ['protoc']
            
            # Get absolute path and directory
            proto_abs_path = os.path.abspath(proto_path)
            proto_dir = os.path.dirname(proto_abs_path)
            proto_basename = os.path.basename(proto_abs_path)
            
            # Add import paths
            for import_path in self.import_paths:
                cmd.extend(['-I', import_path])
            
            # Add directory of the proto file as import path
            cmd.extend(['-I', proto_dir])
            
            # Output descriptor set
            cmd.extend(['--descriptor_set_out=' + tmp_path])
            
            # Include imports in descriptor set
            cmd.append('--include_imports')
            
            # Add the proto file (relative to its directory)
            cmd.append(proto_basename)
            
            # Run protoc
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                raise RuntimeError(f"protoc failed: {result.stderr}")
            
            # Read descriptor set
            with open(tmp_path, 'rb') as f:
                return f.read()
        finally:
            # Clean up temp file
            if os.path.exists(tmp_path):
                os.unlink(tmp_path)
    
    def _parse_extension_fields(self, data: bytes) -> Dict[int, int]:
        """Parse extension fields from serialized protobuf data."""
        extensions = {}
        i = 0
        
        while i < len(data):
            # Read varint tag
            tag, i = self._read_varint(data, i)
            field_number = tag >> 3
            wire_type = tag & 0x07
            
            if wire_type == 0:  # Varint
                value, i = self._read_varint(data, i)
                extensions[field_number] = value
            elif wire_type == 2:  # Length-delimited
                length, i = self._read_varint(data, i)
                i += length
            else:
                # Skip unknown wire types
                break
        
        return extensions
    
    def _read_varint(self, data: bytes, pos: int) -> tuple:
        """Read a varint from bytes at position pos, return (value, new_pos)."""
        result = 0
        shift = 0
        
        while pos < len(data):
            byte = data[pos]
            pos += 1
            result |= (byte & 0x7F) << shift
            if not (byte & 0x80):
                break
            shift += 7
        
        return result, pos
    
    def extract_method_options(self, method_proto: descriptor_pb2.MethodDescriptorProto) -> RpcMethodOptions:
        """Extract RPC method options from method descriptor."""
        if not method_proto.HasField('options'):
            return RpcMethodOptions()
        
        options_data = method_proto.options.SerializeToString()
        extensions = self._parse_extension_fields(options_data)
        
        default_timeout_ms = extensions.get(MethodOptions.DEFAULT_TIMEOUT_MS, 5000)
        method_id = extensions.get(MethodOptions.METHOD_ID, None)
        fire_and_forget = bool(extensions.get(MethodOptions.FIRE_AND_FORGET, 0))
        
        return RpcMethodOptions(
            method_id=method_id,
            default_timeout_ms=default_timeout_ms,
            fire_and_forget=fire_and_forget
        )
    
    def extract_service_options(self, service_proto: descriptor_pb2.ServiceDescriptorProto) -> RpcServiceOptions:
        """Extract RPC service options from service descriptor."""
        if not service_proto.HasField('options'):
            return RpcServiceOptions()
        
        options_data = service_proto.options.SerializeToString()
        extensions = self._parse_extension_fields(options_data)
        
        service_id = extensions.get(ServiceOptions.SERVICE_ID, None)
        
        return RpcServiceOptions(service_id=service_id)
