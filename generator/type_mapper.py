#!/usr/bin/env python3
"""
Type mapper for converting protobuf types to C++ types.
"""

from typing import Dict, Any, Optional
from google.protobuf import descriptor_pb2


class TypeMapper:
    """Maps protobuf types to C++ types and provides type utilities."""
    
    # Map from protobuf types to C++ types
    CPP_TYPE_MAP = {
        'TYPE_DOUBLE': 'double',
        'TYPE_FLOAT': 'float',
        'TYPE_INT64': 'int64_t',
        'TYPE_UINT64': 'uint64_t',
        'TYPE_INT32': 'int32_t',
        'TYPE_FIXED64': 'uint64_t',
        'TYPE_FIXED32': 'uint32_t',
        'TYPE_BOOL': 'bool',
        'TYPE_STRING': 'std::string',
        'TYPE_BYTES': 'std::vector<uint8_t>',
        'TYPE_UINT32': 'uint32_t',
        'TYPE_SFIXED32': 'int32_t',
        'TYPE_SFIXED64': 'int64_t',
        'TYPE_SINT32': 'int32_t',
        'TYPE_SINT64': 'int64_t',
    }
    
    # Wire type mapping
    WIRE_TYPE_MAP = {
        'TYPE_DOUBLE': 'litepb::WIRE_TYPE_FIXED64',
        'TYPE_FLOAT': 'litepb::WIRE_TYPE_FIXED32',
        'TYPE_INT64': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_UINT64': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_INT32': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_FIXED64': 'litepb::WIRE_TYPE_FIXED64',
        'TYPE_FIXED32': 'litepb::WIRE_TYPE_FIXED32',
        'TYPE_BOOL': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_STRING': 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        'TYPE_BYTES': 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        'TYPE_UINT32': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_SFIXED32': 'litepb::WIRE_TYPE_FIXED32',
        'TYPE_SFIXED64': 'litepb::WIRE_TYPE_FIXED64',
        'TYPE_SINT32': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_SINT64': 'litepb::WIRE_TYPE_VARINT',
        'TYPE_MESSAGE': 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        'TYPE_ENUM': 'litepb::WIRE_TYPE_VARINT',
    }
    
    # Default values for each type
    DEFAULT_VALUES = {
        'TYPE_DOUBLE': '0.0',
        'TYPE_FLOAT': '0.0f',
        'TYPE_INT64': '0',
        'TYPE_UINT64': '0',
        'TYPE_INT32': '0',
        'TYPE_FIXED64': '0',
        'TYPE_FIXED32': '0',
        'TYPE_BOOL': 'false',
        'TYPE_STRING': '""',
        'TYPE_BYTES': '{}',
        'TYPE_UINT32': '0',
        'TYPE_SFIXED32': '0',
        'TYPE_SFIXED64': '0',
        'TYPE_SINT32': '0',
        'TYPE_SINT64': '0',
    }
    
    @classmethod
    def _field_type_to_string(cls, field_type: int) -> str:
        """Convert FieldDescriptorProto type enum to string name."""
        type_names = {
            descriptor_pb2.FieldDescriptorProto.TYPE_DOUBLE: 'TYPE_DOUBLE',
            descriptor_pb2.FieldDescriptorProto.TYPE_FLOAT: 'TYPE_FLOAT',
            descriptor_pb2.FieldDescriptorProto.TYPE_INT64: 'TYPE_INT64',
            descriptor_pb2.FieldDescriptorProto.TYPE_UINT64: 'TYPE_UINT64',
            descriptor_pb2.FieldDescriptorProto.TYPE_INT32: 'TYPE_INT32',
            descriptor_pb2.FieldDescriptorProto.TYPE_FIXED64: 'TYPE_FIXED64',
            descriptor_pb2.FieldDescriptorProto.TYPE_FIXED32: 'TYPE_FIXED32',
            descriptor_pb2.FieldDescriptorProto.TYPE_BOOL: 'TYPE_BOOL',
            descriptor_pb2.FieldDescriptorProto.TYPE_STRING: 'TYPE_STRING',
            descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE: 'TYPE_MESSAGE',
            descriptor_pb2.FieldDescriptorProto.TYPE_BYTES: 'TYPE_BYTES',
            descriptor_pb2.FieldDescriptorProto.TYPE_UINT32: 'TYPE_UINT32',
            descriptor_pb2.FieldDescriptorProto.TYPE_ENUM: 'TYPE_ENUM',
            descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED32: 'TYPE_SFIXED32',
            descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED64: 'TYPE_SFIXED64',
            descriptor_pb2.FieldDescriptorProto.TYPE_SINT32: 'TYPE_SINT32',
            descriptor_pb2.FieldDescriptorProto.TYPE_SINT64: 'TYPE_SINT64',
        }
        return type_names.get(field_type, f'TYPE_UNKNOWN_{field_type}')
    
    @classmethod
    def get_cpp_type(cls, proto_type: str) -> str:
        """Get the C++ type for a protobuf type."""
        return cls.CPP_TYPE_MAP.get(proto_type, '')
    
    @classmethod
    def get_field_cpp_type(cls, field: descriptor_pb2.FieldDescriptorProto, file_proto: descriptor_pb2.FileDescriptorProto) -> str:
        """
        Get the complete C++ type for a field, including containers.
        
        Args:
            field: Field descriptor
            file_proto: File descriptor for context
            
        Returns:
            Complete C++ type string
        """
        base_type = ''
        
        # Get base type
        if field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
            # Use the type name directly, qualified if needed
            type_name = field.type_name if field.HasField('type_name') else ''
            package = file_proto.package if file_proto.package else ''
            base_type = cls.qualify_type_name(type_name, package)
        else:
            field_type_str = cls._field_type_to_string(field.type)
            base_type = cls.get_cpp_type(field_type_str)
        
        # Handle repeated fields
        if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_BYTES:
                # Special case: repeated bytes is vector<vector<uint8_t>>
                return 'std::vector<std::vector<uint8_t>>'
            return f'std::vector<{base_type}>'
        
        # Handle optional fields
        syntax = file_proto.syntax if file_proto.syntax else 'proto2'
        if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL:
            # Proto2 optional or proto3 explicit optional
            return f'std::optional<{base_type}>'
        
        return base_type
    
    @classmethod
    def get_map_cpp_type(cls, key_field: descriptor_pb2.FieldDescriptorProto, value_field: descriptor_pb2.FieldDescriptorProto, file_proto: descriptor_pb2.FileDescriptorProto) -> str:
        """
        Get the C++ type for a map field.
        
        Args:
            key_field: Key field descriptor from map entry
            value_field: Value field descriptor from map entry
            file_proto: File descriptor for context
            
        Returns:
            C++ map type string
        """
        # Get key type
        key_type_str = cls._field_type_to_string(key_field.type)
        key_type = cls.get_cpp_type(key_type_str)
        
        # Get value type
        if value_field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
            value_type_name = value_field.type_name if value_field.HasField('type_name') else ''
            package = file_proto.package if file_proto.package else ''
            value_type = cls.qualify_type_name(value_type_name, package)
        else:
            value_type_str = cls._field_type_to_string(value_field.type)
            value_type = cls.get_cpp_type(value_type_str)
        
        return f'std::unordered_map<{key_type}, {value_type}>'
    
    @classmethod
    def get_oneof_cpp_type(cls, fields: list, file_proto: descriptor_pb2.FileDescriptorProto) -> str:
        """
        Get the C++ type for a oneof field (using std::variant).
        
        Args:
            fields: List of FieldDescriptorProto objects in the oneof
            file_proto: File descriptor for context
            
        Returns:
            C++ variant type string
        """
        variant_types = ['std::monostate']  # Default empty state
        
        for field in fields:
            if field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
                type_name = field.type_name if field.HasField('type_name') else ''
                package = file_proto.package if file_proto.package else ''
                field_type = cls.qualify_type_name(type_name, package)
            else:
                field_type_str = cls._field_type_to_string(field.type)
                field_type = cls.get_cpp_type(field_type_str)
            variant_types.append(field_type)
        
        return f'std::variant<{", ".join(variant_types)}>'
    
    @classmethod
    def get_wire_type(cls, proto_type: str) -> str:
        """Get the wire type for a protobuf type."""
        return cls.WIRE_TYPE_MAP.get(proto_type, 'litepb::WIRE_TYPE_VARINT')
    
    @classmethod
    def get_default_value(cls, field: descriptor_pb2.FieldDescriptorProto, file_proto: descriptor_pb2.FileDescriptorProto) -> str:
        """
        Get the default value for a field.
        
        Args:
            field: Field descriptor
            file_proto: File descriptor for context
            
        Returns:
            Default value as C++ code string
        """
        # Check for explicit default value
        if field.HasField('default_value'):
            default_val = field.default_value
            
            # Handle string defaults
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_STRING:
                # Escape the string properly
                escaped = default_val.replace('\\', '\\\\').replace('"', '\\"')
                return f'"{escaped}"'
            
            # Handle bool defaults
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_BOOL:
                return 'true' if default_val.lower() == 'true' else 'false'
            
            # Handle enum defaults
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_ENUM:
                # Return the enum value name
                package = file_proto.package if file_proto.package else ''
                return cls.qualify_type_name(default_val, package)
            
            # For numeric types, return as-is
            return default_val
        
        # Return standard defaults
        field_type_str = cls._field_type_to_string(field.type)
        if field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
            return '{}'
        
        return cls.DEFAULT_VALUES.get(field_type_str, '{}')
    
    @classmethod
    def get_serialization_method(cls, proto_type: str) -> str:
        """Get the ProtoWriter method name for serializing a type."""
        method_map = {
            'TYPE_DOUBLE': 'write_double',
            'TYPE_FLOAT': 'write_float',
            'TYPE_INT64': 'write_varint',
            'TYPE_UINT64': 'write_varint',
            'TYPE_INT32': 'write_varint',
            'TYPE_FIXED64': 'write_fixed64',
            'TYPE_FIXED32': 'write_fixed32',
            'TYPE_BOOL': 'write_varint',
            'TYPE_STRING': 'write_string',
            'TYPE_BYTES': 'write_bytes',
            'TYPE_UINT32': 'write_varint',
            'TYPE_SFIXED32': 'write_sfixed32',
            'TYPE_SFIXED64': 'write_sfixed64',
            'TYPE_SINT32': 'write_sint32',
            'TYPE_SINT64': 'write_sint64',
        }
        return method_map.get(proto_type, 'write_varint')

    @classmethod
    def get_deserialization_method(cls, proto_type: str) -> str:
        """Get the ProtoReader method name for deserializing a type."""
        method_map = {
            'TYPE_DOUBLE': 'read_double',
            'TYPE_FLOAT': 'read_float',
            'TYPE_INT64': 'read_varint',
            'TYPE_UINT64': 'read_varint',
            'TYPE_INT32': 'read_varint',
            'TYPE_FIXED64': 'read_fixed64',
            'TYPE_FIXED32': 'read_fixed32',
            'TYPE_BOOL': 'read_varint',
            'TYPE_STRING': 'read_string',
            'TYPE_BYTES': 'read_bytes',
            'TYPE_UINT32': 'read_varint',
            'TYPE_SFIXED32': 'read_fixed32',
            'TYPE_SFIXED64': 'read_fixed64',
            'TYPE_SINT32': 'read_sint32',
            'TYPE_SINT64': 'read_sint64',
        }
        return method_map.get(proto_type, 'read_varint')

    @classmethod
    def needs_pointer(cls, field_type: str) -> bool:
        """Check if a field type should be stored as a pointer."""
        # In LitePB, we use value semantics for everything
        # This could be changed to use pointers for large messages if needed
        return False

    @classmethod
    def qualify_type_name(cls, type_name: str, package: str = '', current_scope: str = '') -> str:
        """
        Qualify a protobuf type name for C++ usage.

        Args:
            type_name: The protobuf type name (possibly with leading dot)
            package: Current package name
            current_scope: Current message scope for resolving relative names

        Returns:
            Qualified C++ type name
        """
        if not type_name:
            return ''

        # Remove leading dot if present (fully qualified name)
        if type_name.startswith('.'):
            type_name = type_name[1:]

        # Convert dots to C++ namespace separators
        cpp_name = type_name.replace('.', '::')

        return cpp_name
