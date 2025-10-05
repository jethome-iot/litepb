#!/usr/bin/env python3
"""
Protobuf file parser using protoc compiler.
Parses .proto files and extracts descriptors for code generation.
"""

import os
import subprocess
import tempfile
from pathlib import Path
from google.protobuf import descriptor_pb2
from google.protobuf.descriptor_pool import DescriptorPool
from google.protobuf.descriptor import FieldDescriptor, Descriptor, EnumDescriptor
from typing import List, Dict, Optional, Any


class ProtoParser:
    """Parse .proto files using protoc and convert to internal structures."""
    
    def __init__(self, import_paths: Optional[List[str]] = None):
        """Initialize parser with optional import paths."""
        self.import_paths = import_paths or []
        self.descriptor_pool = DescriptorPool()
    
    def parse_proto_file(self, proto_path: str) -> Dict[str, Any]:
        """
        Parse a .proto file and return structured data.
        
        Args:
            proto_path: Path to the .proto file
            
        Returns:
            Dictionary containing parsed proto file data
        """
        # Use protoc to generate descriptor set
        descriptor_set = self._run_protoc(proto_path)
        
        # Parse the descriptor set
        file_descriptor_set = descriptor_pb2.FileDescriptorSet()
        file_descriptor_set.ParseFromString(descriptor_set)
        
        # Build descriptor pool with all files including extension definitions
        self.descriptor_pool = DescriptorPool()
        for file_proto in file_descriptor_set.file:
            try:
                self.descriptor_pool.Add(file_proto)
            except Exception:
                # File might already be in pool
                pass
        
        # Convert to internal structure
        result = []
        for file_proto in file_descriptor_set.file:
            result.append(self._convert_file_descriptor(file_proto))
        
        # Return the main file (last one in the set)
        return result[-1] if result else {}
    
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
    
    def _convert_file_descriptor(self, file_proto: descriptor_pb2.FileDescriptorProto) -> Dict[str, Any]:
        """Convert FileDescriptorProto to internal structure."""
        result = {
            'syntax': file_proto.syntax or 'proto2',
            'package': file_proto.package,
            'imports': [],
            'messages': [],
            'enums': [],
            'services': [],
            'options': {}
        }
        
        # Process imports
        for dep in file_proto.dependency:
            result['imports'].append({'path': dep, 'is_public': False})
        for pub_dep in file_proto.public_dependency:
            if pub_dep < len(file_proto.dependency):
                result['imports'][pub_dep]['is_public'] = True
        
        # Process top-level enums
        for enum_proto in file_proto.enum_type:
            result['enums'].append(self._convert_enum(enum_proto))
        
        # Process messages
        for msg_proto in file_proto.message_type:
            # Calculate full name for top-level messages
            full_name = f"{result['package']}.{msg_proto.name}" if result['package'] else msg_proto.name
            result['messages'].append(self._convert_message(msg_proto, result['syntax'], full_name))
        
        # Process services
        for service_proto in file_proto.service:
            result['services'].append(self._convert_service(service_proto, result['package']))
        
        # Process options
        if file_proto.HasField('options'):
            options = file_proto.options
            if options.HasField('java_package'):
                result['options']['java_package'] = options.java_package
            if options.HasField('java_outer_classname'):
                result['options']['java_outer_classname'] = options.java_outer_classname
        
        return result
    
    def _convert_message(self, msg_proto: descriptor_pb2.DescriptorProto, syntax: str = 'proto2', full_name: str = '') -> Dict[str, Any]:
        """Convert DescriptorProto to internal message structure."""
        message = {
            'name': msg_proto.name,
            'full_name': full_name,
            'fields': [],
            'oneofs': [],
            'maps': [],
            'nested_messages': [],
            'nested_enums': [],
            'reserved_numbers': [],
            'reserved_names': []
        }
        self._current_syntax = syntax
        
        # Track which fields belong to oneofs
        oneof_fields = {}
        for field_proto in msg_proto.field:
            if field_proto.HasField('oneof_index'):
                oneof_idx = field_proto.oneof_index
                if oneof_idx not in oneof_fields:
                    oneof_fields[oneof_idx] = []
                oneof_fields[oneof_idx].append(field_proto)
        
        # Process regular fields and detect maps
        for field_proto in msg_proto.field:
            if field_proto.HasField('oneof_index'):
                continue  # Skip oneof fields for now
            
            # Check if this is a map field
            if self._is_map_field(msg_proto, field_proto):
                map_field = self._convert_map_field(msg_proto, field_proto)
                if map_field:
                    message['maps'].append(map_field)
            else:
                message['fields'].append(self._convert_field(field_proto, syntax))
        
        # Process oneofs (but skip synthetic oneofs for proto3 optional fields)
        for idx, oneof_proto in enumerate(msg_proto.oneof_decl):
            if idx in oneof_fields:
                # Check if this is a synthetic oneof for proto3 optional field
                # Synthetic oneofs have a single field and the oneof name starts with '_'
                is_proto3_optional = (
                    len(oneof_fields[idx]) == 1 and 
                    oneof_proto.name.startswith('_')
                )
                
                if is_proto3_optional:
                    # This is a proto3 optional field - treat it as a regular optional field
                    field = self._convert_field(oneof_fields[idx][0], syntax)
                    field['label'] = 'OPTIONAL'  # Mark as explicit optional
                    message['fields'].append(field)
                else:
                    # This is a real oneof
                    oneof = {
                        'name': oneof_proto.name,
                        'fields': [self._convert_field(f, syntax) for f in oneof_fields[idx]]
                    }
                    message['oneofs'].append(oneof)
        
        # Process nested messages
        for nested_msg in msg_proto.nested_type:
            # Skip map entry messages
            if not nested_msg.options.map_entry:
                # Calculate full name for nested message
                nested_full_name = f"{full_name}.{nested_msg.name}" if full_name else nested_msg.name
                message['nested_messages'].append(self._convert_message(nested_msg, syntax, nested_full_name))
        
        # Process nested enums
        for nested_enum in msg_proto.enum_type:
            message['nested_enums'].append(self._convert_enum(nested_enum))
        
        # Process reserved fields
        for range_proto in msg_proto.reserved_range:
            for num in range(range_proto.start, range_proto.end):
                message['reserved_numbers'].append(num)
        
        message['reserved_names'].extend(msg_proto.reserved_name)
        
        return message
    
    def _convert_field(self, field_proto: descriptor_pb2.FieldDescriptorProto, syntax: str = 'proto2') -> Dict[str, Any]:
        """Convert FieldDescriptorProto to internal field structure."""
        field = {
            'name': field_proto.name,
            'type': self._convert_field_type(field_proto.type),
            'label': self._convert_field_label(field_proto.label, syntax),
            'number': field_proto.number,
            'type_name': field_proto.type_name if field_proto.HasField('type_name') else '',
            'default_value': None,
            'json_name': field_proto.json_name if field_proto.HasField('json_name') else field_proto.name
        }
        
        # Handle default values
        if field_proto.HasField('default_value'):
            field['default_value'] = field_proto.default_value
        
        # Handle packed option for repeated fields - only add if explicitly set
        if field_proto.HasField('options') and field_proto.options.HasField('packed'):
            field['packed'] = field_proto.options.packed
        
        return field
    
    def _convert_enum(self, enum_proto: descriptor_pb2.EnumDescriptorProto) -> Dict[str, Any]:
        """Convert EnumDescriptorProto to internal enum structure."""
        enum = {
            'name': enum_proto.name,
            'values': [],
            'allow_alias': False
        }
        
        # Check for allow_alias option
        if enum_proto.HasField('options') and enum_proto.options.HasField('allow_alias'):
            enum['allow_alias'] = enum_proto.options.allow_alias
        
        # Process enum values
        for value_proto in enum_proto.value:
            enum['values'].append({
                'name': value_proto.name,
                'number': value_proto.number
            })
        
        return enum
    
    def _convert_service(self, service_proto: descriptor_pb2.ServiceDescriptorProto, package: str) -> Dict[str, Any]:
        """Convert ServiceDescriptorProto to internal service structure."""
        service = {
            'name': service_proto.name,
            'full_name': f"{package}.{service_proto.name}" if package else service_proto.name,
            'methods': [],
            'options': {}
        }
        
        # Extract service_id from ServiceOptions (extension field 50006)
        service_id = None
        if service_proto.HasField('options'):
            options = service_proto.options
            serialized = options.SerializeToString()
            extension_values = self._parse_extension_fields(serialized)
            
            if 50006 in extension_values:
                service_id = extension_values[50006]
        
        service['options']['service_id'] = service_id
        
        # Process methods
        for method_proto in service_proto.method:
            service['methods'].append(self._convert_method(method_proto))
        
        return service
    
    def _convert_method(self, method_proto: descriptor_pb2.MethodDescriptorProto) -> Dict[str, Any]:
        """Convert MethodDescriptorProto to internal method structure."""
        method = {
            'name': method_proto.name,
            'input_type': method_proto.input_type,
            'output_type': method_proto.output_type,
            'client_streaming': method_proto.client_streaming,
            'server_streaming': method_proto.server_streaming,
            'options': {}
        }
        
        # Set default options
        direction = 'BIDIRECTIONAL'
        transport = 'LIGHTWEIGHT'
        default_timeout_ms = 5000
        method_id = None
        fire_and_forget = False
        
        # Extract custom options (litepb.rpc extension fields)
        # Extension field numbers: direction (50001), transport (50002), default_timeout_ms (50003), method_id (50004), fire_and_forget (50005)
        if method_proto.HasField('options'):
            options = method_proto.options
            
            # Parse extension fields from the serialized options
            # Extension fields are encoded as: (field_number << 3) | wire_type
            # For varint: wire_type = 0
            # Field 50001 -> tag = 400008 (0x61A88) -> varint encoded as 0x88, 0xB5, 0x18
            # Field 50002 -> tag = 400016 (0x61A90) -> varint encoded as 0x90, 0xB5, 0x18
            # Field 50003 -> tag = 400024 (0x61A98) -> varint encoded as 0x98, 0xB5, 0x18
            # Field 50004 -> tag = 400032 (0x61AA0) -> varint encoded as 0xA0, 0xB5, 0x18
            # Field 50005 -> tag = 400040 (0x61AA8) -> varint encoded as 0xA8, 0xB5, 0x18
            serialized = options.SerializeToString()
            extension_values = self._parse_extension_fields(serialized)
            
            if 50001 in extension_values:
                direction_value = extension_values[50001]
                if direction_value == 0:
                    direction = 'BIDIRECTIONAL'
                elif direction_value == 1:
                    direction = 'CLIENT_TO_SERVER'
                elif direction_value == 2:
                    direction = 'SERVER_TO_CLIENT'
            
            if 50002 in extension_values:
                transport_value = extension_values[50002]
                if transport_value == 0:
                    transport = 'LIGHTWEIGHT'
                elif transport_value == 1:
                    transport = 'GRPC'
            
            if 50003 in extension_values:
                default_timeout_ms = extension_values[50003]
            
            if 50004 in extension_values:
                method_id = extension_values[50004]
            
            if 50005 in extension_values:
                fire_and_forget = bool(extension_values[50005])
        
        method['options']['direction'] = direction
        method['options']['transport'] = transport
        method['options']['default_timeout_ms'] = default_timeout_ms
        method['options']['method_id'] = method_id
        method['options']['fire_and_forget'] = fire_and_forget
        
        return method
    
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
    
    def _is_map_field(self, msg_proto: descriptor_pb2.DescriptorProto, 
                     field_proto: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Check if a field is a map field."""
        if field_proto.label != 3:  # LABEL_REPEATED
            return False
        
        if field_proto.type != 11:  # TYPE_MESSAGE
            return False
        
        # Find the nested message type
        type_name = field_proto.type_name
        if type_name.startswith('.'):
            type_name = type_name[1:]
        
        # Look for the nested message with map_entry option
        for nested_msg in msg_proto.nested_type:
            if nested_msg.name == type_name.split('.')[-1]:
                return nested_msg.options.map_entry if nested_msg.HasField('options') else False
        
        return False
    
    def _convert_map_field(self, msg_proto: descriptor_pb2.DescriptorProto,
                          field_proto: descriptor_pb2.FieldDescriptorProto) -> Optional[Dict[str, Any]]:
        """Convert a map field to internal structure."""
        # Find the map entry message
        type_name = field_proto.type_name
        if type_name.startswith('.'):
            type_name = type_name[1:]
        
        entry_msg = None
        for nested_msg in msg_proto.nested_type:
            if nested_msg.name == type_name.split('.')[-1]:
                entry_msg = nested_msg
                break
        
        if not entry_msg or len(entry_msg.field) != 2:
            return None
        
        # Map entry should have exactly 2 fields: key and value
        key_field = None
        value_field = None
        
        for f in entry_msg.field:
            if f.number == 1:
                key_field = f
            elif f.number == 2:
                value_field = f
        
        if not key_field or not value_field:
            return None
        
        return {
            'name': field_proto.name,
            'number': field_proto.number,
            'key_type': self._convert_field_type(key_field.type),
            'value_type': self._convert_field_type(value_field.type),
            'value_type_name': value_field.type_name if value_field.HasField('type_name') else ''
        }
    
    def _convert_field_type(self, type_num: int) -> str:
        """Convert protobuf field type number to string."""
        type_map = {
            1: 'TYPE_DOUBLE',
            2: 'TYPE_FLOAT',
            3: 'TYPE_INT64',
            4: 'TYPE_UINT64',
            5: 'TYPE_INT32',
            6: 'TYPE_FIXED64',
            7: 'TYPE_FIXED32',
            8: 'TYPE_BOOL',
            9: 'TYPE_STRING',
            10: 'TYPE_GROUP',
            11: 'TYPE_MESSAGE',
            12: 'TYPE_BYTES',
            13: 'TYPE_UINT32',
            14: 'TYPE_ENUM',
            15: 'TYPE_SFIXED32',
            16: 'TYPE_SFIXED64',
            17: 'TYPE_SINT32',
            18: 'TYPE_SINT64'
        }
        return type_map.get(type_num, 'TYPE_UNKNOWN')
    
    def _convert_field_label(self, label_num: int, syntax: str = 'proto2') -> str:
        """Convert protobuf field label number to string.
        
        In proto2:
            1 = LABEL_OPTIONAL (optional keyword) -> 'OPTIONAL'
            2 = LABEL_REQUIRED (required keyword) -> 'REQUIRED'
            3 = LABEL_REPEATED (repeated keyword) -> 'REPEATED'
            
        In proto3:
            1 = LABEL_OPTIONAL (default for singular) -> 'IMPLICIT' (not truly optional)
            3 = LABEL_REPEATED (repeated keyword) -> 'REPEATED'
            Proto3 explicit optional fields are handled separately via synthetic oneofs
        """
        if label_num == 3:  # REPEATED
            return 'REPEATED'
        elif label_num == 2:  # REQUIRED (proto2 only)
            return 'REQUIRED'
        elif label_num == 1:  # OPTIONAL
            if syntax == 'proto3':
                # In proto3, label 1 is the default and doesn't mean optional
                return 'IMPLICIT'
            else:
                # In proto2, label 1 means optional keyword was used
                return 'OPTIONAL'
        return 'NONE'