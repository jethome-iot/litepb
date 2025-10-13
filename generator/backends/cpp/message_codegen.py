#!/usr/bin/env python3
"""
Message and enum code generation for C++.
"""

from google.protobuf import descriptor_pb2 as pb2
from generator.backends.cpp.type_mapper import TypeMapper
from generator.backends.cpp.cpp_utils import CppUtils
from generator.backends.cpp.field_utils import FieldUtils


class MessageCodegen:
    """Generate C++ code for messages and enums."""
    
    def __init__(self, current_proto: pb2.FileDescriptorProto):
        """Initialize with current proto context."""
        self.current_proto = current_proto
    
    def generate_enum(self, enum_proto: pb2.EnumDescriptorProto, indent: int = 0) -> str:
        """Generate enum definition."""
        ind = '    ' * indent
        lines = []
        
        lines.append(f'{ind}enum class {enum_proto.name} : int32_t {{')
        
        for value in enum_proto.value:
            lines.append(f'{ind}    {value.name} = {value.number},')
        
        lines.append(f'{ind}}};')
        
        return '\n'.join(lines)
    
    def generate_message_declaration(self, message: pb2.DescriptorProto) -> str:
        """Generate forward declaration for a message and its nested types."""
        lines = []

        # Forward declare this message
        lines.append(f'struct {message.name};')

        # Recursively declare nested messages (except map entries)
        for nested_msg in message.nested_type:
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_lines = self.generate_message_declaration(nested_msg)
                lines.extend(nested_lines.split('\n'))

        return '\n'.join(lines)

    def generate_message_definition(self, message: pb2.DescriptorProto) -> str:
        """Generate complete definition for a message."""
        lines = []

        # Get context information
        package = self.current_proto.package if self.current_proto.package else ''
        package_ns = package.replace('.', '::') if package else ''
        # Build full message name from current context
        msg_fqn = f"{package}.{message.name}" if package else message.name
        msg_cpp_fqn = msg_fqn.replace('.', '::')
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'

        lines.append(f'struct {message.name} {{')

        # Nested enums first (but not map entries)
        for nested_enum in message.enum_type:
            enum_code = self.generate_enum(nested_enum, 1)
            lines.append(enum_code)
            lines.append('')

        # Forward declare all nested messages (except map entries)
        non_map_nested = [nt for nt in message.nested_type 
                         if not (nt.HasField('options') and nt.options.map_entry)]
        if non_map_nested:
            for nested_msg in non_map_nested:
                lines.append(f'    struct {nested_msg.name};')
            lines.append('')

        # Nested message definitions (now they can reference each other)
        for nested_msg in non_map_nested:
            nested_def = self.generate_message_definition(nested_msg)
            # Indent the nested definition
            nested_lines = nested_def.split('\n')
            for line in nested_lines:
                if line.strip():
                    lines.append(f'    {line}')
                else:
                    lines.append('')

        # Get regular fields (not in oneofs, not map entries)
        regular_fields = FieldUtils.get_non_oneof_fields(message)
        
        for field in regular_fields:
            # Determine if this field should use std::optional
            use_optional = FieldUtils.uses_optional(field, syntax)

            if field.type == pb2.FieldDescriptorProto.TYPE_GROUP:
                group_name = FieldUtils.get_group_type_name(field.type_name)
                if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{group_name}>'
                elif use_optional:
                    cpp_type = f'std::optional<{group_name}>'
                else:
                    cpp_type = group_name
            elif field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
                type_name = field.type_name
                qualified = TypeMapper.qualify_type_name(type_name, package, msg_fqn)
                simple_type = CppUtils.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)

                if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{simple_type}>'
                elif use_optional:
                    cpp_type = f'std::optional<{simple_type}>'
                else:
                    cpp_type = simple_type
            else:
                base_type = TypeMapper.get_cpp_type(field.type)

                if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{base_type}>'
                elif use_optional:
                    cpp_type = f'std::optional<{base_type}>'
                else:
                    cpp_type = base_type

            default_val = self._get_field_default(field, syntax)
            if default_val:
                lines.append(f'    {cpp_type} {field.name} = {default_val};')
            else:
                lines.append(f'    {cpp_type} {field.name};')

        # Map fields
        maps = FieldUtils.extract_maps_from_message(message)
        for map_field in maps:
            key_type = TypeMapper.get_cpp_type(map_field.key_field.type)

            if map_field.value_field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
                value_name = map_field.value_field.type_name
                qualified = TypeMapper.qualify_type_name(value_name, package, msg_fqn)
                value_type = CppUtils.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)
            else:
                value_type = TypeMapper.get_cpp_type(map_field.value_field.type)

            cpp_type = f'std::unordered_map<{key_type}, {value_type}>'
            lines.append(f'    {cpp_type} {map_field.name};')

        # Oneof fields - deduplicate types to avoid std::variant errors
        oneofs = FieldUtils.extract_oneofs_from_message(message)
        for oneof in oneofs:
            variant_types = ['std::monostate']
            seen_types = set()
            
            for field in oneof.fields:
                if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
                    type_name = field.type_name
                    qualified = TypeMapper.qualify_type_name(type_name, package, msg_fqn)
                    field_type = CppUtils.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)
                else:
                    field_type = TypeMapper.get_cpp_type(field.type)
                
                if field_type not in seen_types:
                    variant_types.append(field_type)
                    seen_types.add(field_type)

            oneof_type = f'std::variant<{", ".join(variant_types)}>'
            lines.append(f'    {oneof_type} {oneof.name};')

        # Add unknown fields member for forward/backward compatibility
        lines.append('')
        lines.append('    // Unknown field preservation for forward/backward compatibility')
        lines.append('    litepb::UnknownFieldSet unknown_fields;')

        lines.append('};')

        return '\n'.join(lines)

    def _get_field_default(self, field: pb2.FieldDescriptorProto, syntax: str) -> str:
        """Get default value for field initialization."""
        # Repeated fields don't need initialization (vector has default constructor)
        if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
            return ''
        
        # Fields wrapped in std::optional don't need initialization
        if FieldUtils.uses_optional(field, syntax):
            return ''
        
        if field.HasField('default_value'):
            # Use TypeMapper.get_default_value with descriptor objects
            return TypeMapper.get_default_value(field, self.current_proto)
        
        # For proto3 implicit fields and proto2 required fields, provide default values
        if field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
            return f'static_cast<{TypeMapper.qualify_type_name(field.type_name, "")}>(0)'
        
        return TypeMapper.DEFAULT_VALUES.get(field.type, '')
