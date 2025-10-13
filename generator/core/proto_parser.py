#!/usr/bin/env python3
"""
Protobuf file parser using protoc compiler.
Parses .proto files and extracts descriptors for code generation.
"""

import os
import subprocess
import tempfile
from typing import List, Dict, Optional
from google.protobuf import descriptor_pb2 as pb2


class ProtoParser:
    """Parse .proto files using protoc and return FileDescriptorProto."""
    
    def __init__(self, import_paths: Optional[List[str]] = None):
        """Initialize parser with optional import paths."""
        self.import_paths = import_paths or []
    
    def parse_proto_file(self, proto_path: str) -> pb2.FileDescriptorProto:
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
        file_descriptor_set = pb2.FileDescriptorSet()
        file_descriptor_set.ParseFromString(descriptor_set)
        
        # Return the main file (last one in the set)
        if file_descriptor_set.file:
            return file_descriptor_set.file[-1]
        
        # Return empty FileDescriptorProto if no files found
        return pb2.FileDescriptorProto()
    
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
    
