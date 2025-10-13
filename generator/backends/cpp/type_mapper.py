#!/usr/bin/env python3
"""
Type mapper for converting protobuf types to C++ types.
Uses protobuf enums directly instead of string conversions.
"""

from typing import Dict, Optional
from google.protobuf import descriptor_pb2 as pb2


class TypeMapper:
    """Maps protobuf types to C++ types and provides type utilities."""
    
    # Map from protobuf type enums to C++ types
    CPP_TYPE_MAP: Dict[int, str] = {
        pb2.FieldDescriptorProto.TYPE_DOUBLE: 'double',
        pb2.FieldDescriptorProto.TYPE_FLOAT: 'float',
        pb2.FieldDescriptorProto.TYPE_INT64: 'int64_t',
        pb2.FieldDescriptorProto.TYPE_UINT64: 'uint64_t',
        pb2.FieldDescriptorProto.TYPE_INT32: 'int32_t',
        pb2.FieldDescriptorProto.TYPE_FIXED64: 'uint64_t',
        pb2.FieldDescriptorProto.TYPE_FIXED32: 'uint32_t',
        pb2.FieldDescriptorProto.TYPE_BOOL: 'bool',
        pb2.FieldDescriptorProto.TYPE_STRING: 'std::string',
        pb2.FieldDescriptorProto.TYPE_BYTES: 'std::vector<uint8_t>',
        pb2.FieldDescriptorProto.TYPE_UINT32: 'uint32_t',
        pb2.FieldDescriptorProto.TYPE_SFIXED32: 'int32_t',
        pb2.FieldDescriptorProto.TYPE_SFIXED64: 'int64_t',
        pb2.FieldDescriptorProto.TYPE_SINT32: 'int32_t',
        pb2.FieldDescriptorProto.TYPE_SINT64: 'int64_t',
    }
    
    # Wire type mapping
    WIRE_TYPE_MAP: Dict[int, str] = {
        pb2.FieldDescriptorProto.TYPE_DOUBLE: 'litepb::WIRE_TYPE_FIXED64',
        pb2.FieldDescriptorProto.TYPE_FLOAT: 'litepb::WIRE_TYPE_FIXED32',
        pb2.FieldDescriptorProto.TYPE_INT64: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_UINT64: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_INT32: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_FIXED64: 'litepb::WIRE_TYPE_FIXED64',
        pb2.FieldDescriptorProto.TYPE_FIXED32: 'litepb::WIRE_TYPE_FIXED32',
        pb2.FieldDescriptorProto.TYPE_BOOL: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_STRING: 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        pb2.FieldDescriptorProto.TYPE_BYTES: 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        pb2.FieldDescriptorProto.TYPE_UINT32: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_SFIXED32: 'litepb::WIRE_TYPE_FIXED32',
        pb2.FieldDescriptorProto.TYPE_SFIXED64: 'litepb::WIRE_TYPE_FIXED64',
        pb2.FieldDescriptorProto.TYPE_SINT32: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_SINT64: 'litepb::WIRE_TYPE_VARINT',
        pb2.FieldDescriptorProto.TYPE_MESSAGE: 'litepb::WIRE_TYPE_LENGTH_DELIMITED',
        pb2.FieldDescriptorProto.TYPE_ENUM: 'litepb::WIRE_TYPE_VARINT',
    }
    
    # Default values for each type
    DEFAULT_VALUES: Dict[int, str] = {
        pb2.FieldDescriptorProto.TYPE_DOUBLE: '0.0',
        pb2.FieldDescriptorProto.TYPE_FLOAT: '0.0f',
        pb2.FieldDescriptorProto.TYPE_INT64: '0',
        pb2.FieldDescriptorProto.TYPE_UINT64: '0',
        pb2.FieldDescriptorProto.TYPE_INT32: '0',
        pb2.FieldDescriptorProto.TYPE_FIXED64: '0',
        pb2.FieldDescriptorProto.TYPE_FIXED32: '0',
        pb2.FieldDescriptorProto.TYPE_BOOL: 'false',
        pb2.FieldDescriptorProto.TYPE_STRING: '""',
        pb2.FieldDescriptorProto.TYPE_BYTES: '{}',
        pb2.FieldDescriptorProto.TYPE_UINT32: '0',
        pb2.FieldDescriptorProto.TYPE_SFIXED32: '0',
        pb2.FieldDescriptorProto.TYPE_SFIXED64: '0',
        pb2.FieldDescriptorProto.TYPE_SINT32: '0',
        pb2.FieldDescriptorProto.TYPE_SINT64: '0',
    }
    
    @classmethod
    def get_cpp_type(cls, field_type: int) -> str:
        """Get the C++ type for a protobuf type enum."""
        return cls.CPP_TYPE_MAP.get(field_type, '')
    
    @classmethod
    def get_field_cpp_type(cls, field: pb2.FieldDescriptorProto, file_proto: pb2.FileDescriptorProto) -> str:
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
        if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
            # Use the type name directly, qualified if needed
            type_name = field.type_name if field.HasField('type_name') else ''
            package = file_proto.package if file_proto.package else ''
            base_type = cls.qualify_type_name(type_name, package)
        else:
            base_type = cls.get_cpp_type(field.type)
        
        # Handle repeated fields
        if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
            if field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
                # Special case: repeated bytes is vector<vector<uint8_t>>
                return 'std::vector<std::vector<uint8_t>>'
            return f'std::vector<{base_type}>'
        
        # Handle optional fields
        syntax = file_proto.syntax if file_proto.syntax else 'proto2'
        if field.label == pb2.FieldDescriptorProto.LABEL_OPTIONAL:
            # Proto2 optional or proto3 explicit optional
            return f'std::optional<{base_type}>'
        
        return base_type
    
    @classmethod
    def get_map_cpp_type(cls, key_field: pb2.FieldDescriptorProto, value_field: pb2.FieldDescriptorProto, file_proto: pb2.FileDescriptorProto) -> str:
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
        key_type = cls.get_cpp_type(key_field.type)
        
        # Get value type
        if value_field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
            value_type_name = value_field.type_name if value_field.HasField('type_name') else ''
            package = file_proto.package if file_proto.package else ''
            value_type = cls.qualify_type_name(value_type_name, package)
        else:
            value_type = cls.get_cpp_type(value_field.type)
        
        return f'std::unordered_map<{key_type}, {value_type}>'
    
    @classmethod
    def get_oneof_cpp_type(cls, fields: list, file_proto: pb2.FileDescriptorProto) -> str:
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
            if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
                type_name = field.type_name if field.HasField('type_name') else ''
                package = file_proto.package if file_proto.package else ''
                field_type = cls.qualify_type_name(type_name, package)
            else:
                field_type = cls.get_cpp_type(field.type)
            variant_types.append(field_type)
        
        return f'std::variant<{", ".join(variant_types)}>'
    
    @classmethod
    def get_wire_type(cls, field_type: int) -> str:
        """Get the wire type for a protobuf type enum."""
        return cls.WIRE_TYPE_MAP.get(field_type, 'litepb::WIRE_TYPE_VARINT')
    
    @classmethod
    def get_default_value(cls, field: pb2.FieldDescriptorProto, file_proto: pb2.FileDescriptorProto) -> str:
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
            if field.type == pb2.FieldDescriptorProto.TYPE_STRING:
                # Escape the string properly
                escaped = default_val.replace('\\', '\\\\').replace('"', '\\"')
                return f'"{escaped}"'
            
            # Handle bool defaults
            if field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
                return 'true' if default_val.lower() == 'true' else 'false'
            
            # Handle enum defaults
            if field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                # Return the enum value name
                package = file_proto.package if file_proto.package else ''
                return cls.qualify_type_name(default_val, package)
            
            # For numeric types, return as-is
            return default_val
        
        # Return standard defaults
        if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
            return '{}'
        
        return cls.DEFAULT_VALUES.get(field.type, '{}')
    
    @classmethod
    def get_serialization_method(cls, field_type: int) -> str:
        """Get the ProtoWriter method name for serializing a type."""
        method_map: Dict[int, str] = {
            pb2.FieldDescriptorProto.TYPE_DOUBLE: 'write_double',
            pb2.FieldDescriptorProto.TYPE_FLOAT: 'write_float',
            pb2.FieldDescriptorProto.TYPE_INT64: 'write_varint',
            pb2.FieldDescriptorProto.TYPE_UINT64: 'write_varint',
            pb2.FieldDescriptorProto.TYPE_INT32: 'write_varint',
            pb2.FieldDescriptorProto.TYPE_FIXED64: 'write_fixed64',
            pb2.FieldDescriptorProto.TYPE_FIXED32: 'write_fixed32',
            pb2.FieldDescriptorProto.TYPE_BOOL: 'write_varint',
            pb2.FieldDescriptorProto.TYPE_STRING: 'write_string',
            pb2.FieldDescriptorProto.TYPE_BYTES: 'write_bytes',
            pb2.FieldDescriptorProto.TYPE_UINT32: 'write_varint',
            pb2.FieldDescriptorProto.TYPE_SFIXED32: 'write_sfixed32',
            pb2.FieldDescriptorProto.TYPE_SFIXED64: 'write_sfixed64',
            pb2.FieldDescriptorProto.TYPE_SINT32: 'write_sint32',
            pb2.FieldDescriptorProto.TYPE_SINT64: 'write_sint64',
        }
        return method_map.get(field_type, 'write_varint')

    @classmethod
    def get_deserialization_method(cls, field_type: int) -> str:
        """Get the ProtoReader method name for deserializing a type."""
        method_map: Dict[int, str] = {
            pb2.FieldDescriptorProto.TYPE_DOUBLE: 'read_double',
            pb2.FieldDescriptorProto.TYPE_FLOAT: 'read_float',
            pb2.FieldDescriptorProto.TYPE_INT64: 'read_varint',
            pb2.FieldDescriptorProto.TYPE_UINT64: 'read_varint',
            pb2.FieldDescriptorProto.TYPE_INT32: 'read_varint',
            pb2.FieldDescriptorProto.TYPE_FIXED64: 'read_fixed64',
            pb2.FieldDescriptorProto.TYPE_FIXED32: 'read_fixed32',
            pb2.FieldDescriptorProto.TYPE_BOOL: 'read_varint',
            pb2.FieldDescriptorProto.TYPE_STRING: 'read_string',
            pb2.FieldDescriptorProto.TYPE_BYTES: 'read_bytes',
            pb2.FieldDescriptorProto.TYPE_UINT32: 'read_varint',
            pb2.FieldDescriptorProto.TYPE_SFIXED32: 'read_fixed32',
            pb2.FieldDescriptorProto.TYPE_SFIXED64: 'read_fixed64',
            pb2.FieldDescriptorProto.TYPE_SINT32: 'read_sint32',
            pb2.FieldDescriptorProto.TYPE_SINT64: 'read_sint64',
        }
        return method_map.get(field_type, 'read_varint')

    @classmethod
    def needs_pointer(cls, field_type: int) -> bool:
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
    
    @classmethod
    def get_packed_size_expression(cls, field_type: int, item_name: str) -> str:
        """Get the expression to calculate the size of a packed field item."""
        if field_type in (pb2.FieldDescriptorProto.TYPE_INT32, pb2.FieldDescriptorProto.TYPE_INT64,
                         pb2.FieldDescriptorProto.TYPE_UINT32, pb2.FieldDescriptorProto.TYPE_UINT64,
                         pb2.FieldDescriptorProto.TYPE_BOOL, pb2.FieldDescriptorProto.TYPE_ENUM):
            return f'litepb::ProtoWriter::varint_size(static_cast<uint64_t>({item_name}))'
        elif field_type == pb2.FieldDescriptorProto.TYPE_SINT32:
            return f'litepb::ProtoWriter::sint32_size({item_name})'
        elif field_type == pb2.FieldDescriptorProto.TYPE_SINT64:
            return f'litepb::ProtoWriter::sint64_size({item_name})'
        elif field_type in (pb2.FieldDescriptorProto.TYPE_FIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED32, 
                           pb2.FieldDescriptorProto.TYPE_FLOAT):
            return f'litepb::ProtoWriter::fixed32_size()'
        elif field_type in (pb2.FieldDescriptorProto.TYPE_FIXED64, pb2.FieldDescriptorProto.TYPE_SFIXED64,
                           pb2.FieldDescriptorProto.TYPE_DOUBLE):
            return f'litepb::ProtoWriter::fixed64_size()'
        else:
            # Fallback
            return f'litepb::ProtoWriter::varint_size(static_cast<uint64_t>({item_name}))'
    
    @classmethod
    def get_default_check(cls, field: pb2.FieldDescriptorProto) -> Optional[str]:
        """Get condition to check if field has non-default value (proto3 optimization)."""
        field_name = field.name
        
        # Don't check messages - they always need to be encoded if present
        if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
            return None
        
        default_val = cls.DEFAULT_VALUES.get(field.type, None)
        if not default_val:
            return None
        
        # Special cases
        if field.type == pb2.FieldDescriptorProto.TYPE_STRING:
            return f'!value.{field_name}.empty()'
        elif field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
            return f'!value.{field_name}.empty()'
        elif field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
            return f'value.{field_name}'
        else:
            return f'value.{field_name} != {default_val}'
