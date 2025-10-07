#!/usr/bin/env python3
"""
C++ code generator for LitePB.
Generates C++ header and implementation files from protobuf descriptors.
"""

import os
from typing import Dict, Any, List, Optional
from jinja2 import Environment, DictLoader
from google.protobuf import descriptor_pb2
from type_mapper import TypeMapper
from rpc_options import MethodOptions, ServiceOptions
from proto_parser import ProtoParser


class CppGenerator:
    """Generate C++ code from parsed protobuf structures."""
    
    def __init__(self):
        """Initialize the generator with templates."""
        self.type_mapper = TypeMapper()
        self.current_proto = None  # Track current proto for context (FileDescriptorProto)
        self.parser = ProtoParser()  # For extracting RPC options
        self.setup_templates()
    
    # Helper methods for working with protobuf descriptor enums
    @staticmethod
    def is_message_type(field: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Check if field is a message type."""
        return field.type == descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE
    
    @staticmethod
    def is_enum_type(field: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Check if field is an enum type."""
        return field.type == descriptor_pb2.FieldDescriptorProto.TYPE_ENUM
    
    @staticmethod
    def is_repeated(field: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Check if field is repeated."""
        return field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED
    
    @staticmethod
    def is_map_field(message_proto: descriptor_pb2.DescriptorProto, 
                     field_proto: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Check if field is a map field (TYPE_MESSAGE with map_entry option)."""
        if field_proto.type != descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE:
            return False
        
        # Find the nested type for this field
        type_name = field_proto.type_name.split('.')[-1]
        for nested_type in message_proto.nested_type:
            if nested_type.name == type_name:
                return nested_type.options.map_entry if nested_type.HasField('options') else False
        return False
    
    @staticmethod
    def get_field_type_name(field_type: int) -> str:
        """Convert field type enum to string name (e.g., TYPE_MESSAGE)."""
        # Map the enum value to its string name
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
            descriptor_pb2.FieldDescriptorProto.TYPE_GROUP: 'TYPE_GROUP',
            descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE: 'TYPE_MESSAGE',
            descriptor_pb2.FieldDescriptorProto.TYPE_BYTES: 'TYPE_BYTES',
            descriptor_pb2.FieldDescriptorProto.TYPE_UINT32: 'TYPE_UINT32',
            descriptor_pb2.FieldDescriptorProto.TYPE_ENUM: 'TYPE_ENUM',
            descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED32: 'TYPE_SFIXED32',
            descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED64: 'TYPE_SFIXED64',
            descriptor_pb2.FieldDescriptorProto.TYPE_SINT32: 'TYPE_SINT32',
            descriptor_pb2.FieldDescriptorProto.TYPE_SINT64: 'TYPE_SINT64',
        }
        return type_names[field_type] if field_type in type_names else f'TYPE_UNKNOWN_{field_type}'
    
    @staticmethod
    def get_field_label_name(field_label: int) -> str:
        """Convert field label enum to string name (e.g., LABEL_REPEATED)."""
        label_names = {
            descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL: 'LABEL_OPTIONAL',
            descriptor_pb2.FieldDescriptorProto.LABEL_REQUIRED: 'LABEL_REQUIRED',
            descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED: 'LABEL_REPEATED',
        }
        return label_names[field_label] if field_label in label_names else f'LABEL_UNKNOWN_{field_label}'
    
    @staticmethod
    def uses_optional(field: descriptor_pb2.FieldDescriptorProto, syntax: str) -> bool:
        """Determine if a field should use std::optional."""
        if syntax == 'proto2':
            # Proto2: REQUIRED and OPTIONAL fields use std::optional
            return field.label in (descriptor_pb2.FieldDescriptorProto.LABEL_REQUIRED,
                                   descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL)
        else:  # proto3
            # Proto3: Only explicitly optional fields use std::optional
            # Check for proto3_optional flag (set on explicit optional fields)
            return field.proto3_optional if hasattr(field, 'proto3_optional') else False
    
    def setup_templates(self):
        """Set up Jinja2 templates for code generation."""
        templates = {
            'header': '''#pragma once

#include "litepb/litepb.h"
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <optional>
#include <variant>
#include <unordered_map>
#include <algorithm>
#include <utility>
{%- if services %}
#include <functional>
#include "litepb/rpc/channel.h"
#include "litepb/rpc/error.h"
{% endif %}

{%- for import in imports %}
#include "{{ import }}"
{% endfor %}

{% if package %}
{% for ns in namespace_parts %}
namespace {{ ns }} {
{% endfor %}

{% endif %}
{%- for enum in enums %}
{{ generate_enum(enum) }}
{% endfor %}

{%- if messages %}
// Forward declarations
{%- for message in messages %}
struct {{ message.name }};
{% endfor %}

{% endif %}
{%- for message in messages %}
{{ generate_message_definition(message) }}
{% endfor %}

{%- if services %}
// RPC Service Stubs
{% for service in services %}
{{ generate_rpc_service(service, namespace_prefix) }}
{% endfor %}
{% endif %}

{% if package %}
{% for ns in namespace_parts|reverse %}
}  // namespace {{ ns }}
{% endfor %}

{% endif %}
// Serializer specializations
namespace litepb {

{% for message in messages %}
{{ generate_serializer_spec(message, namespace_prefix, true) }}
{% endfor %}

}  // namespace litepb
''',
            'source': '''#include "litepb/litepb.h"
#include "{{ include_file }}"

namespace litepb {

{% for message in messages %}
{{ generate_serializer_impl(message, namespace_prefix) }}
{% endfor %}

}  // namespace litepb
'''
        }
        
        self.env = Environment(loader=DictLoader(templates))
        
        # Register custom functions
        self.env.globals['generate_enum'] = self.generate_enum
        self.env.globals['generate_message'] = self.generate_message
        self.env.globals['generate_message_declaration'] = self.generate_message_declaration
        self.env.globals['generate_message_definition'] = self.generate_message_definition
        self.env.globals['generate_serializer_spec'] = self.generate_serializer_spec
        self.env.globals['generate_serializer_impl'] = self.generate_serializer_impl
        self.env.globals['generate_rpc_service'] = self.generate_rpc_service
    
    def generate_header(self, file_proto: descriptor_pb2.FileDescriptorProto, filename: str) -> str:
        """Generate C++ header file content."""
        self.current_proto = file_proto  # Set context for type generation
        template = self.env.get_template('header')
        
        # Convert imports to include paths
        import_includes = []
        for dependency in file_proto.dependency:
            # Skip rpc_options.proto - it's only for compile-time extension field definitions
            # (Runtime fields like msg_id and message_type are in rpc_protocol.proto/RpcMessage)
            if 'rpc_options.proto' in dependency:
                continue
            # Convert "path/to/file.proto" to "path/to/file.pb.h"
            include_path = dependency.replace('.proto', '.pb.h')
            import_includes.append(include_path)
        
        # Sort messages in topological order to avoid forward declaration issues
        messages = list(file_proto.message_type)
        sorted_messages = self._topological_sort_messages(messages)

        # Prepare context
        context = {
            'header_guard': self.get_header_guard(filename),
            'package': file_proto.package if file_proto.package else '',
            'namespace_parts': self.get_namespace_parts(file_proto.package if file_proto.package else ''),
            'namespace_prefix': self.get_namespace_prefix(file_proto.package if file_proto.package else ''),
            'imports': import_includes,
            'enums': list(file_proto.enum_type),
            'messages': sorted_messages,
            'services': list(file_proto.service),
        }
        
        return template.render(**context)
    
    def generate_implementation(self, file_proto: descriptor_pb2.FileDescriptorProto, filename: str) -> str:
        """Generate C++ implementation file content."""
        self.current_proto = file_proto  # Set context for type generation
        template = self.env.get_template('source')
        
        # Prepare context
        context = {
            'include_file': self.get_include_filename(filename),
            'namespace_prefix': self.get_namespace_prefix(file_proto.package if file_proto.package else ''),
            'messages': list(file_proto.message_type),
        }
        
        return template.render(**context)
    
    def generate_enum(self, enum_proto: descriptor_pb2.EnumDescriptorProto, indent: int = 0) -> str:
        """Generate enum definition."""
        ind = '    ' * indent
        lines = []
        
        lines.append(f'{ind}enum class {enum_proto.name} : int32_t {{')
        
        for value in enum_proto.value:
            lines.append(f'{ind}    {value.name} = {value.number},')
        
        lines.append(f'{ind}}};')
        
        return '\n'.join(lines)
    
    def simplify_type_in_context(self, full_type: str, current_ns: str = '', current_msg: str = '') -> str:
        """Simplify a fully qualified type based on current context."""
        # Check if we're in a nested message and the type is a sibling
        if current_msg and '::' in current_msg:
            # We're in a nested message, extract the parent
            parent_parts = current_msg.rsplit('::', 1)
            if len(parent_parts) == 2:
                parent_msg = parent_parts[0]
                # Check if the full_type is also nested in the same parent
                if full_type.startswith(parent_msg + '::'):
                    # Extract what comes after the parent
                    sibling_name = full_type[len(parent_msg) + 2:]
                    # If it's a direct child (no more ::), it's a sibling
                    if '::' not in sibling_name:
                        return sibling_name
        
        # If nested in current message, drop message prefix
        if current_msg and full_type.startswith(current_msg + '::'):
            return full_type[len(current_msg) + 2:]

        # If in same namespace, drop namespace prefix
        if current_ns and full_type.startswith(current_ns + '::'):
            return full_type[len(current_ns) + 2:]
        
        return full_type
    
    def generate_message(self, message: descriptor_pb2.DescriptorProto, indent: int = 0) -> str:
        """Generate message struct definition (legacy method, prefer generate_message_definition)."""
        # Just delegate to generate_message_definition and adjust indentation
        definition = self.generate_message_definition(message)
        if indent == 0:
            return definition
        
        # Add indentation
        ind = '    ' * indent
        lines = definition.split('\n')
        return '\n'.join(f'{ind}{line}' if line.strip() else '' for line in lines)
    
    def _get_group_type_name(self, type_name: str) -> str:
        """Extract the group type name from a type_name."""
        if type_name.startswith('.'):
            type_name = type_name[1:]
        # Groups are typically named like "MessageName.GroupName"
        parts = type_name.split('.')
        return parts[-1] if parts else 'UnknownGroup'

    def generate_message_declaration(self, message: descriptor_pb2.DescriptorProto) -> str:
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

    def generate_message_definition(self, message: descriptor_pb2.DescriptorProto) -> str:
        """Generate complete definition for a message."""
        assert self.current_proto is not None, "current_proto must be set before calling generate_message_definition"
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
        regular_fields = self._get_non_oneof_fields(message)
        
        for field in regular_fields:
            field_type = self.get_field_type_name(field.type)

            # Determine if this field should use std::optional
            use_optional = self.uses_optional(field, syntax)

            if field_type == 'TYPE_GROUP':
                group_name = self._get_group_type_name(field.type_name)
                if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{group_name}>'
                elif use_optional:
                    cpp_type = f'std::optional<{group_name}>'
                else:
                    cpp_type = group_name
            elif field_type in ('TYPE_MESSAGE', 'TYPE_ENUM'):
                type_name = field.type_name
                qualified = TypeMapper.qualify_type_name(type_name, package, msg_fqn)
                simple_type = self.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)

                if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{simple_type}>'
                elif use_optional:
                    cpp_type = f'std::optional<{simple_type}>'
                else:
                    cpp_type = simple_type
            else:
                base_type = TypeMapper.get_cpp_type(field_type)

                if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
                    cpp_type = f'std::vector<{base_type}>'
                elif use_optional:
                    cpp_type = f'std::optional<{base_type}>'
                else:
                    cpp_type = base_type

            default_val = self.get_field_default(field)
            if default_val:
                lines.append(f'    {cpp_type} {field.name} = {default_val};')
            else:
                lines.append(f'    {cpp_type} {field.name};')

        # Map fields
        maps = self._extract_maps_from_message(message)
        for map_field in maps:
            key_type = TypeMapper.get_cpp_type(map_field['key_type'])

            if map_field['value_type'] in ('TYPE_MESSAGE', 'TYPE_ENUM'):
                value_name = map_field.get('value_type_name', '')
                qualified = TypeMapper.qualify_type_name(value_name, package, msg_fqn)
                value_type = self.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)
            else:
                value_type = TypeMapper.get_cpp_type(map_field['value_type'])

            cpp_type = f'std::unordered_map<{key_type}, {value_type}>'
            lines.append(f'    {cpp_type} {map_field["name"]};')

        # Oneof fields - deduplicate types to avoid std::variant errors
        oneofs = self._extract_oneofs_from_message(message)
        for oneof in oneofs:
            variant_types = ['std::monostate']
            seen_types = set()
            
            for field in oneof['fields']:
                if field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
                    type_name = field.type_name
                    qualified = TypeMapper.qualify_type_name(type_name, package, msg_fqn)
                    field_type = self.simplify_type_in_context(qualified, package_ns, msg_cpp_fqn)
                else:
                    field_type = TypeMapper.get_cpp_type(self.get_field_type_name(field.type))
                
                if field_type not in seen_types:
                    variant_types.append(field_type)
                    seen_types.add(field_type)

            oneof_type = f'std::variant<{", ".join(variant_types)}>'
            lines.append(f'    {oneof_type} {oneof["name"]};')

        lines.append('};')

        return '\n'.join(lines)

    def generate_serializer_spec(self, message: descriptor_pb2.DescriptorProto, ns_prefix: str, inline: bool) -> str:
        """Generate serializer specialization for a message and its nested messages."""
        lines = []
        
        # First, recursively generate serializers for nested messages (but not map entries)
        for nested_msg in message.nested_type:
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_prefix = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
                lines.append(self.generate_serializer_spec(nested_msg, nested_prefix, inline))
                lines.append('')
        
        # Then generate serializer for this message
        msg_type = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
        inline_str = 'inline ' if inline else ''
        
        lines.append(f'template<>')
        lines.append(f'struct Serializer<{msg_type}> {{')
        
        # Serialize method
        lines.append(f'    {inline_str}static bool serialize(const {msg_type}& value, litepb::OutputStream& stream) {{')
        lines.append('        litepb::ProtoWriter writer(stream);')
        
        # Get non-oneof, non-map fields
        regular_fields = self._get_non_oneof_fields(message)
        
        # Generate write code for each regular field
        for field in regular_fields:
            lines.append(self.generate_field_write(field, message))
        
        # Generate write code for maps
        maps = self._extract_maps_from_message(message)
        for map_field in maps:
            lines.append(self.generate_map_write(map_field, message))
        
        # Generate write code for oneofs
        oneofs = self._extract_oneofs_from_message(message)
        for oneof in oneofs:
            lines.append(self.generate_oneof_write(oneof, message))
        
        lines.append('        return true;')
        lines.append('    }')
        lines.append('')
        
        # Parse method
        lines.append(f'    {inline_str}static bool parse({msg_type}& value, litepb::InputStream& stream) {{')
        lines.append('        litepb::ProtoReader reader(stream);')
        lines.append('        uint32_t field_number;')
        lines.append('        litepb::WireType wire_type;')
        lines.append('        while (reader.read_tag(field_number, wire_type)) {')
        lines.append('            ')
        lines.append('            switch (field_number) {')
        
        # Generate read cases for each regular field
        for field in regular_fields:
            lines.append(self.generate_field_read(field, message))
        
        # Generate read cases for maps
        for map_field in maps:
            lines.append(self.generate_map_read(map_field, message))
        
        # Generate read cases for oneofs
        for oneof in oneofs:
            for field in oneof['fields']:
                lines.append(self.generate_oneof_field_read(field, oneof, message))
        
        lines.append('                default:')
        lines.append('                    if (!reader.skip_field(wire_type)) return false;')
        lines.append('                    break;')
        lines.append('            }')
        lines.append('        }')
        lines.append('        return true;')
        lines.append('    }')
        
        lines.append('};')
        
        return '\n'.join(lines)
    
    def generate_serializer_impl(self, message: Dict[str, Any], ns_prefix: str) -> str:
        """Generate serializer implementation (non-inline version)."""
        # All serializers are now inline in the header, so no need for separate implementations
        return ""
    
    def generate_field_write(self, field: descriptor_pb2.FieldDescriptorProto, message: descriptor_pb2.DescriptorProto) -> str:
        """Generate write code for a field."""
        assert self.current_proto is not None, "current_proto must be set before calling generate_field_write"
        field_num = field.number
        field_name = field.name
        field_type = self.get_field_type_name(field.type)
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'

        # Check if field uses std::optional wrapper
        use_optional_field = self.uses_optional(field, syntax)

        if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
            # Repeated field - check if it should be packed
            lines = []
            
            if self.is_field_packed(field):
                # Packed encoding
                lines.append(f'        if (!value.{field_name}.empty()) {{')
                lines.append(f'            // Calculate packed size')
                lines.append(f'            size_t packed_size = 0;')
                lines.append(f'            for (const auto& item : value.{field_name}) {{')
                
                # Add size calculation based on field type
                size_expr = self.get_packed_size_expression(field_type, 'item')
                lines.append(f'                packed_size += {size_expr};')
                lines.append(f'            }}')
                lines.append(f'            ')
                lines.append(f'            // Write tag with LENGTH_DELIMITED wire type')
                lines.append(f'            writer.write_tag({field_num}, litepb::WIRE_TYPE_LENGTH_DELIMITED);')
                lines.append(f'            writer.write_varint(packed_size);')
                lines.append(f'            ')
                lines.append(f'            // Write all values without tags')
                lines.append(f'            for (const auto& item : value.{field_name}) {{')
                
                # Add write code based on field type
                if field_type == 'TYPE_ENUM':
                    lines.append(f'                writer.write_varint(static_cast<uint64_t>(item));')
                else:
                    method = TypeMapper.get_serialization_method(field_type)
                    if method == 'write_varint':
                        if field_type == 'TYPE_BOOL':
                            lines.append(f'                writer.{method}(item ? 1 : 0);')
                        else:
                            lines.append(f'                writer.{method}(static_cast<uint64_t>(item));')
                    else:
                        lines.append(f'                writer.{method}(item);')
                
                lines.append(f'            }}')
                lines.append(f'        }}')
            else:
                # Unpacked encoding (original logic)
                lines.append(f'        for (const auto& item : value.{field_name}) {{')
                lines.append(f'            writer.write_tag({field_num}, {TypeMapper.get_wire_type(field_type)});')
                
                if field_type == 'TYPE_MESSAGE':
                    lines.append(f'            {{')
                    lines.append(f'                litepb::BufferOutputStream msg_stream;')
                    lines.append(f'                litepb::serialize(item, msg_stream);')
                    lines.append(f'                writer.write_bytes(msg_stream.data(), msg_stream.size());')
                    lines.append(f'            }}')
                elif field_type == 'TYPE_ENUM':
                    lines.append(f'            writer.write_varint(static_cast<uint64_t>(item));')
                elif field_type == 'TYPE_GROUP':
                    lines.append(f'            // TODO: Implement group serialization')
                else:
                    method = TypeMapper.get_serialization_method(field_type)
                    if method == 'write_varint':
                        # Cast to uint64_t for varint types
                        if field_type == 'TYPE_BOOL':
                            lines.append(f'            writer.{method}(item ? 1 : 0);')
                        else:
                            lines.append(f'            writer.{method}(static_cast<uint64_t>(item));')
                    elif method == 'write_bytes':
                        lines.append(f'            writer.{method}(item.data(), item.size());')
                    else:
                        lines.append(f'            writer.{method}(item);')
                
                lines.append('        }')
            
            return '\n'.join(lines)
        
        elif use_optional_field:
            # Fields that use std::optional (proto2 optional/required or proto3 explicit optional)
            lines = []
            lines.append(f'        if (value.{field_name}.has_value()) {{')
            lines.append(f'            writer.write_tag({field_num}, {TypeMapper.get_wire_type(field_type)});')
            
            if field_type == 'TYPE_MESSAGE':
                lines.append(f'            {{')
                lines.append(f'                litepb::BufferOutputStream msg_stream;')
                lines.append(f'                litepb::serialize(value.{field_name}.value(), msg_stream);')
                lines.append(f'                writer.write_bytes(msg_stream.data(), msg_stream.size());')
                lines.append(f'            }}')
            elif field_type == 'TYPE_ENUM':
                lines.append(f'            writer.write_varint(static_cast<uint64_t>(value.{field_name}.value()));')
            elif field_type == 'TYPE_GROUP':
                lines.append(f'            // TODO: Implement group serialization')
            else:
                method = TypeMapper.get_serialization_method(field_type)
                if method == 'write_varint':
                    # Cast to uint64_t for varint types
                    if field_type == 'TYPE_BOOL':
                        lines.append(f'            writer.{method}(value.{field_name}.value() ? 1 : 0);')
                    else:
                        lines.append(f'            writer.{method}(static_cast<uint64_t>(value.{field_name}.value()));')
                elif method == 'write_bytes':
                    lines.append(f'            writer.{method}(value.{field_name}.value().data(), value.{field_name}.value().size());')
                elif method == 'write_string':
                    lines.append(f'            writer.{method}(value.{field_name}.value());')
                else:
                    lines.append(f'            writer.{method}(value.{field_name}.value());')
            
            lines.append('        }')
            return '\n'.join(lines)
        
        else:
            # Regular field (proto3 style)
            lines = []
            
            # Check if field has non-default value (proto3 optimization)
            msg_syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'
            if msg_syntax == 'proto3':
                default_check = self.get_default_check(field)
                if default_check:
                    lines.append(f'        if ({default_check}) {{')
                    indent = '    '
                else:
                    indent = ''
            else:
                indent = ''
            
            lines.append(f'        {indent}writer.write_tag({field_num}, {TypeMapper.get_wire_type(field_type)});')
            
            if field_type == 'TYPE_MESSAGE':
                lines.append(f'        {indent}{{')
                lines.append(f'        {indent}    litepb::BufferOutputStream msg_stream;')
                lines.append(f'        {indent}    litepb::serialize(value.{field_name}, msg_stream);')
                lines.append(f'        {indent}    writer.write_bytes(msg_stream.data(), msg_stream.size());')
                lines.append(f'        {indent}}}')
            elif field_type == 'TYPE_ENUM':
                lines.append(f'        {indent}writer.write_varint(static_cast<uint64_t>(value.{field_name}));')
            elif field_type == 'TYPE_GROUP':
                lines.append(f'        {indent}// TODO: Implement group serialization')
            else:
                method = TypeMapper.get_serialization_method(field_type)
                if method == 'write_varint':
                    # Cast to uint64_t for varint types
                    if field_type == 'TYPE_BOOL':
                        lines.append(f'        {indent}writer.{method}(value.{field_name} ? 1 : 0);')
                    else:
                        lines.append(f'        {indent}writer.{method}(static_cast<uint64_t>(value.{field_name}));')
                elif method == 'write_bytes':
                    lines.append(f'        {indent}writer.{method}(value.{field_name}.data(), value.{field_name}.size());')
                else:
                    lines.append(f'        {indent}writer.{method}(value.{field_name});')
            
            if msg_syntax == 'proto3' and self.get_default_check(field):
                lines.append('        }')
            
            return '\n'.join(lines)
    
    def generate_field_read(self, field: descriptor_pb2.FieldDescriptorProto, message: descriptor_pb2.DescriptorProto) -> str:
        """Generate read case for a field."""
        assert self.current_proto is not None, "current_proto must be set before calling generate_field_read"
        field_num = field.number
        field_name = field.name
        field_type = self.get_field_type_name(field.type)
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'

        # Check if field uses std::optional wrapper
        use_optional_field = self.uses_optional(field, syntax)

        lines = []
        lines.append(f'                case {field_num}: {{')
        
        if field.label == descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED:
            # Repeated field - check if it can be packed
            can_be_packed = self.is_field_packed(field) or field_type in (
                'TYPE_INT32', 'TYPE_INT64', 'TYPE_UINT32', 'TYPE_UINT64',
                'TYPE_SINT32', 'TYPE_SINT64', 'TYPE_BOOL', 'TYPE_ENUM',
                'TYPE_FIXED32', 'TYPE_SFIXED32', 'TYPE_FIXED64', 'TYPE_SFIXED64',
                'TYPE_FLOAT', 'TYPE_DOUBLE'
            )
            
            if can_be_packed and field_type not in ('TYPE_MESSAGE', 'TYPE_GROUP', 'TYPE_STRING', 'TYPE_BYTES'):
                # Field can be packed - handle both packed and unpacked formats
                lines.append(f'                    if (wire_type == litepb::WIRE_TYPE_LENGTH_DELIMITED) {{')
                lines.append(f'                        // Packed format')
                lines.append(f'                        uint64_t length;')
                lines.append(f'                        if (!reader.read_varint(length)) return false;')
                lines.append(f'                        size_t end_pos = reader.position() + length;')
                lines.append(f'                        while (reader.position() < end_pos) {{')
                
                # Add read code for packed format
                self._generate_packed_read_code(lines, field_type, field_name)
                
                lines.append(f'                        }}')
                lines.append(f'                    }} else {{')
                lines.append(f'                        // Unpacked format (backwards compatibility)')
                
                # Add read code for unpacked format
                self._generate_unpacked_read_code(lines, field_type, field_name)
                
                lines.append(f'                    }}')
            else:
                # Field cannot be packed (messages, strings, bytes) - use unpacked format only
                if field_type == 'TYPE_MESSAGE':
                    lines.append(f'                    std::vector<uint8_t> msg_data;')
                    lines.append(f'                    if (!reader.read_bytes(msg_data)) return false;')
                    lines.append(f'                    litepb::BufferInputStream msg_stream(msg_data.data(), msg_data.size());')
                    lines.append(f'                    value.{field_name}.emplace_back();')
                    lines.append(f'                    if (!litepb::parse(value.{field_name}.back(), msg_stream)) return false;')
                elif field_type == 'TYPE_GROUP':
                    lines.append(f'                    // TODO: Implement group deserialization')
                    lines.append(f'                    if (!reader.skip_field(wire_type)) return false;')
                else:
                    # Unpacked format for non-packable types
                    self._generate_unpacked_read_code(lines, field_type, field_name)
        
        elif use_optional_field:
            # Fields that use std::optional (proto2 optional/required or proto3 explicit optional)
            if field_type == 'TYPE_MESSAGE':
                lines.append(f'                    std::vector<uint8_t> msg_data;')
                lines.append(f'                    if (!reader.read_bytes(msg_data)) return false;')
                lines.append(f'                    litepb::BufferInputStream msg_stream(msg_data.data(), msg_data.size());')
                lines.append(f'                    value.{field_name}.emplace();')
                lines.append(f'                    if (!litepb::parse(value.{field_name}.value(), msg_stream)) return false;')
            elif field_type == 'TYPE_ENUM':
                lines.append(f'                    uint64_t enum_val;')
                lines.append(f'                    if (!reader.read_varint(enum_val)) return false;')
                lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})::value_type>(enum_val);')
            elif field_type == 'TYPE_GROUP':
                lines.append(f'                    // TODO: Implement group deserialization')
                lines.append(f'                    if (!reader.skip_field(wire_type)) return false;')
            else:
                cpp_type = TypeMapper.get_cpp_type(field_type)
                method = TypeMapper.get_deserialization_method(field_type)
                if method == 'read_varint':
                    # Read varint as uint64_t then cast to actual type
                    lines.append(f'                    uint64_t temp_varint;')
                    lines.append(f'                    if (!reader.{method}(temp_varint)) return false;')
                    if field_type == 'TYPE_BOOL':
                        lines.append(f'                    value.{field_name} = (temp_varint != 0);')
                    else:
                        lines.append(f'                    value.{field_name} = static_cast<{cpp_type}>(temp_varint);')
                elif field_type in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
                    # For signed fixed types, read as unsigned then reinterpret
                    unsigned_type = 'uint32_t' if field_type == 'TYPE_SFIXED32' else 'uint64_t'
                    lines.append(f'                    {unsigned_type} temp_unsigned;')
                    lines.append(f'                    if (!reader.{method}(temp_unsigned)) return false;')
                    lines.append(f'                    {cpp_type} temp;')
                    lines.append(f'                    std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
                    lines.append(f'                    value.{field_name} = temp;')
                else:
                    lines.append(f'                    {cpp_type} temp;')
                    lines.append(f'                    if (!reader.{method}(temp)) return false;')
                    lines.append(f'                    value.{field_name} = temp;')
        
        else:
            # Regular field (proto3 style)
            if field_type == 'TYPE_MESSAGE':
                lines.append(f'                    std::vector<uint8_t> msg_data;')
                lines.append(f'                    if (!reader.read_bytes(msg_data)) return false;')
                lines.append(f'                    litepb::BufferInputStream msg_stream(msg_data.data(), msg_data.size());')
                lines.append(f'                    if (!litepb::parse(value.{field_name}, msg_stream)) return false;')
            elif field_type == 'TYPE_ENUM':
                lines.append(f'                    uint64_t enum_val;')
                lines.append(f'                    if (!reader.read_varint(enum_val)) return false;')
                lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})>(enum_val);')
            elif field_type == 'TYPE_GROUP':
                lines.append(f'                    // TODO: Implement group deserialization')
                lines.append(f'                    if (!reader.skip_field(wire_type)) return false;')
            else:
                cpp_type = TypeMapper.get_cpp_type(field_type)
                method = TypeMapper.get_deserialization_method(field_type)
                if method == 'read_varint':
                    # Read varint as uint64_t then cast to actual type
                    lines.append(f'                    uint64_t temp_varint;')
                    lines.append(f'                    if (!reader.{method}(temp_varint)) return false;')
                    if field_type == 'TYPE_BOOL':
                        lines.append(f'                    value.{field_name} = (temp_varint != 0);')
                    else:
                        lines.append(f'                    value.{field_name} = static_cast<{cpp_type}>(temp_varint);')
                elif field_type in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
                    # For signed fixed types, read as unsigned then reinterpret
                    unsigned_type = 'uint32_t' if field_type == 'TYPE_SFIXED32' else 'uint64_t'
                    lines.append(f'                    {unsigned_type} temp_unsigned;')
                    lines.append(f'                    if (!reader.{method}(temp_unsigned)) return false;')
                    lines.append(f'                    {cpp_type} temp;')
                    lines.append(f'                    std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
                    lines.append(f'                    value.{field_name} = temp;')
                else:
                    lines.append(f'                    {cpp_type} temp;')
                    lines.append(f'                    if (!reader.{method}(temp)) return false;')
                    lines.append(f'                    value.{field_name} = temp;')
        
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
    
    def generate_map_write(self, map_field: Dict[str, Any], message: descriptor_pb2.DescriptorProto) -> str:
        """Generate write code for a map field."""
        # Maps are encoded as repeated message fields
        lines = []
        field_num = map_field['number']
        field_name = map_field['name']
        
        lines.append(f'        for (const auto& [key, val] : value.{field_name}) {{')
        lines.append(f'            writer.write_tag({field_num}, litepb::WIRE_TYPE_LENGTH_DELIMITED);')
        lines.append('            // Write map entry as a message')
        lines.append('            litepb::BufferOutputStream entry_stream;')
        lines.append('            litepb::ProtoWriter entry_writer(entry_stream);')
        
        # Write key (field 1)
        key_method = TypeMapper.get_serialization_method(map_field['key_type'])
        lines.append(f'            entry_writer.write_tag(1, {TypeMapper.get_wire_type(map_field["key_type"])});')
        if key_method == 'write_varint':
            if map_field['key_type'] == 'TYPE_BOOL':
                lines.append(f'            entry_writer.{key_method}(key ? 1 : 0);')
            else:
                lines.append(f'            entry_writer.{key_method}(static_cast<uint64_t>(key));')
        else:
            lines.append(f'            entry_writer.{key_method}(key);')
        
        # Write value (field 2)
        value_type = map_field['value_type']
        if value_type == 'TYPE_MESSAGE':
            lines.append(f'            entry_writer.write_tag(2, litepb::WIRE_TYPE_LENGTH_DELIMITED);')
            lines.append(f'            {{')
            lines.append(f'                litepb::BufferOutputStream val_stream;')
            lines.append(f'                litepb::serialize(val, val_stream);')
            lines.append(f'                entry_writer.write_bytes(val_stream.data(), val_stream.size());')
            lines.append(f'            }}')
        elif value_type == 'TYPE_ENUM':
            lines.append(f'            entry_writer.write_tag(2, litepb::WIRE_TYPE_VARINT);')
            lines.append(f'            entry_writer.write_varint(static_cast<uint64_t>(val));')
        elif value_type == 'TYPE_BYTES':
            lines.append(f'            entry_writer.write_tag(2, litepb::WIRE_TYPE_LENGTH_DELIMITED);')
            lines.append(f'            entry_writer.write_bytes(val.data(), val.size());')
        else:
            val_method = TypeMapper.get_serialization_method(value_type)
            lines.append(f'            entry_writer.write_tag(2, {TypeMapper.get_wire_type(value_type)});')
            if val_method == 'write_varint':
                if value_type == 'TYPE_BOOL':
                    lines.append(f'            entry_writer.{val_method}(val ? 1 : 0);')
                else:
                    lines.append(f'            entry_writer.{val_method}(static_cast<uint64_t>(val));')
            else:
                lines.append(f'            entry_writer.{val_method}(val);')
        
        lines.append('            writer.write_bytes(entry_stream.data(), entry_stream.size());')
        lines.append('        }')
        
        return '\n'.join(lines)
    
    def generate_map_read(self, map_field: Dict[str, Any], message: descriptor_pb2.DescriptorProto) -> str:
        """Generate read case for a map field."""
        lines = []
        field_num = map_field['number']
        field_name = map_field['name']
        
        lines.append(f'                case {field_num}: {{')
        lines.append('                    // Read map entry')
        lines.append('                    std::vector<uint8_t> entry_data;')
        lines.append('                    if (!reader.read_bytes(entry_data)) return false;')
        lines.append('                    litepb::BufferInputStream entry_stream(entry_data.data(), entry_data.size());')
        lines.append('                    litepb::ProtoReader entry_reader(entry_stream);')
        
        # Declare key and value variables
        key_type = TypeMapper.get_cpp_type(map_field['key_type'])
        if map_field['value_type'] in ('TYPE_MESSAGE', 'TYPE_ENUM'):
            pkg = self.current_proto.package if self.current_proto and self.current_proto.package else ''
            value_type = TypeMapper.qualify_type_name(map_field['value_type_name'], pkg)
        else:
            value_type = TypeMapper.get_cpp_type(map_field['value_type'])
        
        lines.append(f'                    {key_type} key{{}};')
        lines.append(f'                    {value_type} val{{}};')
        
        # Parse entry fields
        lines.append('                    uint32_t entry_field;')
        lines.append('                    litepb::WireType entry_wire_type;')
        lines.append('                    while (entry_reader.read_tag(entry_field, entry_wire_type)) {')
        lines.append('                        if (entry_field == 1) {')
        
        # Read key
        key_method = TypeMapper.get_deserialization_method(map_field['key_type'])
        if key_method == 'read_varint':
            lines.append('                            uint64_t key_varint;')
            lines.append(f'                            if (!entry_reader.{key_method}(key_varint)) return false;')
            lines.append(f'                            key = static_cast<{key_type}>(key_varint);')
        elif key_method in ('read_fixed32', 'read_fixed64'):
            # Use temporary for fixed types to match expected type
            temp_type = 'uint32_t' if key_method == 'read_fixed32' else 'uint64_t'
            lines.append(f'                            {temp_type} temp;')
            lines.append(f'                            if (!entry_reader.{key_method}(temp)) return false;')
            lines.append(f'                            key = static_cast<{key_type}>(temp);')
        else:
            lines.append(f'                            if (!entry_reader.{key_method}(key)) return false;')
        
        lines.append('                        } else if (entry_field == 2) {')
        
        # Read value
        if map_field['value_type'] == 'TYPE_MESSAGE':
            lines.append('                            std::vector<uint8_t> val_data;')
            lines.append('                            if (!entry_reader.read_bytes(val_data)) return false;')
            lines.append('                            litepb::BufferInputStream val_stream(val_data.data(), val_data.size());')
            lines.append('                            if (!litepb::parse(val, val_stream)) return false;')
        elif map_field['value_type'] == 'TYPE_ENUM':
            lines.append('                            uint64_t enum_val;')
            lines.append('                            if (!entry_reader.read_varint(enum_val)) return false;')
            lines.append('                            val = static_cast<decltype(val)>(enum_val);')
        elif map_field['value_type'] in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
            val_method = TypeMapper.get_deserialization_method(map_field['value_type'])
            # Use unsigned temp for fixed types to avoid type mismatch
            temp_type = 'uint32_t' if map_field['value_type'] == 'TYPE_SFIXED32' else 'uint64_t'
            lines.append(f'                            {temp_type} temp;')
            lines.append(f'                            if (!entry_reader.{val_method}(temp)) return false;')
            lines.append(f'                            val = static_cast<{value_type}>(temp);')
        else:
            val_method = TypeMapper.get_deserialization_method(map_field['value_type'])
            if val_method == 'read_varint':
                lines.append('                            uint64_t val_varint;')
                lines.append(f'                            if (!entry_reader.{val_method}(val_varint)) return false;')
                lines.append(f'                            val = static_cast<{value_type}>(val_varint);')
            elif val_method in ('read_fixed32', 'read_fixed64'):
                # Use temporary for fixed types to match expected type
                temp_type = 'uint32_t' if val_method == 'read_fixed32' else 'uint64_t'
                lines.append(f'                            {temp_type} temp;')
                lines.append(f'                            if (!entry_reader.{val_method}(temp)) return false;')
                lines.append(f'                            val = static_cast<{value_type}>(temp);')
            else:
                lines.append(f'                            if (!entry_reader.{val_method}(val)) return false;')
        
        lines.append('                        }')
        lines.append('                    }')
        lines.append(f'                    value.{field_name}[key] = val;')
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
    
    def generate_oneof_write(self, oneof: Dict[str, Any], message: descriptor_pb2.DescriptorProto) -> str:
        """Generate write code for a oneof field."""
        lines = []
        oneof_name = oneof['name']
        
        lines.append(f'        std::visit([&writer, &stream](const auto& oneof_val) {{')
        lines.append('            using T = std::decay_t<decltype(oneof_val)>;')
        
        for i, field in enumerate(oneof['fields']):
            pkg = self.current_proto.package if self.current_proto and self.current_proto.package else ''
            field_type_cpp = TypeMapper.get_cpp_type(self.get_field_type_name(field.type)) if self.get_field_type_name(field.type) not in ('TYPE_MESSAGE', 'TYPE_ENUM') else TypeMapper.qualify_type_name(field.type_name, pkg)
            lines.append(f'            {"else " if i > 0 else ""}if constexpr (std::is_same_v<T, {field_type_cpp}>) {{')
            lines.append(f'                writer.write_tag({field.number}, {TypeMapper.get_wire_type(self.get_field_type_name(field.type))});')
            
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE:
                lines.append('                litepb::serialize(oneof_val, stream);')
            elif field.type == descriptor_pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append('                writer.write_varint(static_cast<uint64_t>(oneof_val));')
            else:
                method = TypeMapper.get_serialization_method(self.get_field_type_name(field.type))
                if method == 'write_bytes':
                    lines.append(f'                writer.{method}(oneof_val.data(), oneof_val.size());')
                elif method == 'write_varint':
                    if self.get_field_type_name(field.type) == 'TYPE_BOOL':
                        lines.append(f'                writer.{method}(oneof_val ? 1 : 0);')
                    else:
                        lines.append(f'                writer.{method}(static_cast<uint64_t>(oneof_val));')
                else:
                    lines.append(f'                writer.{method}(oneof_val);')
            
            lines.append('            }')
        
        lines.append(f'        }}, value.{oneof_name});')
        
        return '\n'.join(lines)
    
    def generate_oneof_field_read(self, field: descriptor_pb2.FieldDescriptorProto, oneof: Dict[str, Any], message: descriptor_pb2.DescriptorProto) -> str:
        """Generate read case for a oneof field."""
        lines = []
        field_num = field.number
        oneof_name = oneof['name']
        
        lines.append(f'                case {field_num}: {{')
        
        if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE:
            pkg = self.current_proto.package if self.current_proto and self.current_proto.package else ''
            field_type = TypeMapper.qualify_type_name(field.type_name, pkg)
            lines.append(f'                    {field_type} oneof_val;')
            lines.append('                    if (!litepb::parse(oneof_val, stream)) return false;')
            lines.append(f'                    value.{oneof_name} = oneof_val;')
        elif field.type == descriptor_pb2.FieldDescriptorProto.TYPE_ENUM:
            pkg = self.current_proto.package if self.current_proto and self.current_proto.package else ''
            field_type = TypeMapper.qualify_type_name(field.type_name, pkg)
            lines.append('                    uint64_t enum_val;')
            lines.append('                    if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                    value.{oneof_name} = static_cast<{field_type}>(enum_val);')
        else:
            cpp_type = TypeMapper.get_cpp_type(self.get_field_type_name(field.type))
            method = TypeMapper.get_deserialization_method(self.get_field_type_name(field.type))
            if method == 'read_varint':
                lines.append('                    uint64_t oneof_varint;')
                lines.append(f'                    if (!reader.{method}(oneof_varint)) return false;')
                if self.get_field_type_name(field.type) == 'TYPE_BOOL':
                    lines.append(f'                    value.{oneof_name} = (oneof_varint != 0);')
                else:
                    lines.append(f'                    value.{oneof_name} = static_cast<{cpp_type}>(oneof_varint);')
            elif self.get_field_type_name(field.type) in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
                # sfixed types need to read as unsigned then convert
                temp_type = 'uint32_t' if self.get_field_type_name(field.type) == 'TYPE_SFIXED32' else 'uint64_t'
                read_method = 'read_fixed32' if self.get_field_type_name(field.type) == 'TYPE_SFIXED32' else 'read_fixed64'
                lines.append(f'                    {temp_type} temp_unsigned;')
                lines.append(f'                    if (!reader.{read_method}(temp_unsigned)) return false;')
                lines.append(f'                    {cpp_type} oneof_val;')
                lines.append(f'                    std::memcpy(&oneof_val, &temp_unsigned, sizeof(oneof_val));')
                lines.append(f'                    value.{oneof_name} = oneof_val;')
            else:
                lines.append(f'                    {cpp_type} oneof_val;')
                lines.append(f'                    if (!reader.{method}(oneof_val)) return false;')
                lines.append(f'                    value.{oneof_name} = oneof_val;')
        
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
    
    def _extract_maps_from_message(self, message: descriptor_pb2.DescriptorProto) -> List[Dict[str, Any]]:
        """Extract map field information from a message descriptor."""
        maps = []
        
        # Find map entry nested types
        map_entries = {}
        for nested_type in message.nested_type:
            if nested_type.HasField('options') and nested_type.options.map_entry:
                map_entries[nested_type.name] = nested_type
        
        # Find fields that use these map entries
        for field in message.field:
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE:
                type_name = field.type_name.split('.')[-1]
                if type_name in map_entries:
                    map_entry = map_entries[type_name]
                    # Map entries have exactly 2 fields: key (number 1) and value (number 2)
                    key_field = None
                    value_field = None
                    for entry_field in map_entry.field:
                        if entry_field.number == 1:
                            key_field = entry_field
                        elif entry_field.number == 2:
                            value_field = entry_field
                    
                    if key_field and value_field:
                        maps.append({
                            'name': field.name,
                            'number': field.number,
                            'key_type': self.get_field_type_name(key_field.type),
                            'value_type': self.get_field_type_name(value_field.type),
                            'value_type_name': value_field.type_name if value_field.type in (
                                descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE,
                                descriptor_pb2.FieldDescriptorProto.TYPE_ENUM
                            ) else ''
                        })
        
        return maps
    
    def _extract_oneofs_from_message(self, message: descriptor_pb2.DescriptorProto) -> List[Dict[str, Any]]:
        """Extract oneof information from a message descriptor."""
        oneofs = []
        
        # Build a map of oneof index to fields
        oneof_fields_map = {}
        for field in message.field:
            if field.HasField('oneof_index'):
                # Proto3 optional fields have proto3_optional=True and use synthetic oneofs internally
                # They should NOT be treated as oneof fields
                is_proto3_optional = field.proto3_optional if hasattr(field, 'proto3_optional') else False
                if is_proto3_optional:
                    continue  # Skip proto3 optional fields
                
                oneof_idx = field.oneof_index
                if oneof_idx not in oneof_fields_map:
                    oneof_fields_map[oneof_idx] = []
                oneof_fields_map[oneof_idx].append(field)
        
        # Create oneof structures
        for idx, oneof_decl in enumerate(message.oneof_decl):
            if idx in oneof_fields_map:
                oneofs.append({
                    'name': oneof_decl.name,
                    'fields': oneof_fields_map[idx]
                })
        
        return oneofs
    
    def _get_non_oneof_fields(self, message: descriptor_pb2.DescriptorProto) -> List[descriptor_pb2.FieldDescriptorProto]:
        """Get all fields that are not part of a oneof or map entry."""
        # Get map entry names
        map_entry_names = set()
        for nested_type in message.nested_type:
            if nested_type.HasField('options') and nested_type.options.map_entry:
                map_entry_names.add(nested_type.name)
        
        # Filter fields
        non_oneof_fields = []
        for field in message.field:
            # Skip oneof fields (but NOT proto3 optional fields which use synthetic oneofs)
            if field.HasField('oneof_index'):
                # Proto3 optional fields have proto3_optional=True and use synthetic oneofs internally
                # They should be treated as regular optional fields, not oneofs
                is_proto3_optional = field.proto3_optional if hasattr(field, 'proto3_optional') else False
                if not is_proto3_optional:
                    continue  # This is a real oneof field, skip it
            # Skip map entry fields
            if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE:
                type_name = field.type_name.split('.')[-1]
                if type_name in map_entry_names:
                    continue
            non_oneof_fields.append(field)
        
        return non_oneof_fields
    
    def _get_message_dependencies(self, message: descriptor_pb2.DescriptorProto, all_messages: List[descriptor_pb2.DescriptorProto]) -> List[str]:
        """Get list of message names that this message depends on."""
        dependencies = set()

        # Check all fields
        for field in message.field:
            if field.type in (descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE, descriptor_pb2.FieldDescriptorProto.TYPE_ENUM):
                type_name = field.type_name
                # Extract the simple message name from the full type name
                if type_name.startswith('.'):
                    type_name = type_name[1:]
                # Convert from proto format (package.MessageName) to just MessageName
                simple_name = type_name.split('.')[-1]

                # Only add dependency if it's another message in this file
                for msg in all_messages:
                    if msg.name == simple_name and msg.name != message.name:
                        dependencies.add(simple_name)

        return list(dependencies)

    def _topological_sort_messages(self, messages: List[descriptor_pb2.DescriptorProto]) -> List[descriptor_pb2.DescriptorProto]:
        """Sort messages in topological order to avoid forward declaration issues."""
        # Create a map for quick lookup
        msg_map = {msg.name: msg for msg in messages}

        # Build dependency graph
        dependencies = {}
        for msg in messages:
            dependencies[msg.name] = self._get_message_dependencies(msg, messages)

        # Perform topological sort using Kahn's algorithm
        sorted_names = []
        in_degree = {name: 0 for name in msg_map.keys()}

        # Calculate in-degrees
        for name, deps in dependencies.items():
            for dep in deps:
                if dep in in_degree:
                    in_degree[name] += 1

        # Start with nodes having no dependencies
        queue = [name for name, degree in in_degree.items() if degree == 0]

        while queue:
            current = queue.pop(0)
            sorted_names.append(current)

            # Reduce in-degree for dependent nodes
            for name, deps in dependencies.items():
                if current in deps:
                    in_degree[name] -= 1
                    if in_degree[name] == 0:
                        queue.append(name)

        # Check for cycles
        if len(sorted_names) != len(messages):
            # If there are cycles, just return original order
            # In practice, protobuf messages with circular dependencies should use pointers
            return messages

        # Return messages in sorted order
        return [msg_map[name] for name in sorted_names]

    def get_field_default(self, field: descriptor_pb2.FieldDescriptorProto) -> Optional[str]:
        """Get default value for field initialization."""
        if self.get_field_label_name(field.label) in ('LABEL_REPEATED', 'LABEL_OPTIONAL'):
            return None
        
        if field.HasField('default_value'):
            # Use TypeMapper.get_default_value with descriptor objects
            assert self.current_proto is not None, "current_proto must be set before calling get_field_default"
            return TypeMapper.get_default_value(field, self.current_proto)
        
        # For proto3, most types have implicit defaults
        if field.type == descriptor_pb2.FieldDescriptorProto.TYPE_ENUM:
            return f'static_cast<{TypeMapper.qualify_type_name(field.type_name, "")}>(0)'
        
        return TypeMapper.DEFAULT_VALUES.get(self.get_field_type_name(field.type))
    
    def get_default_check(self, field: descriptor_pb2.FieldDescriptorProto) -> Optional[str]:
        """Get condition to check if field has non-default value (proto3 optimization)."""
        field_name = field.name
        field_type = self.get_field_type_name(field.type)
        
        # Don't check messages - they always need to be encoded if present
        if field_type == 'TYPE_MESSAGE':
            return None
        
        default_val = TypeMapper.DEFAULT_VALUES.get(field_type, None)
        if not default_val:
            return None
        
        # Special cases
        if field_type == 'TYPE_STRING':
            return f'!value.{field_name}.empty()'
        elif field_type == 'TYPE_BYTES':
            return f'!value.{field_name}.empty()'
        elif field_type == 'TYPE_BOOL':
            return f'value.{field_name}'
        else:
            return f'value.{field_name} != {default_val}'
    
    def is_field_packed(self, field: descriptor_pb2.FieldDescriptorProto) -> bool:
        """Determine if a repeated field should be packed."""
        assert self.current_proto is not None, "current_proto must be set before calling is_field_packed"
        # Only repeated fields can be packed
        if self.get_field_label_name(field.label) != 'LABEL_REPEATED':
            return False
        
        field_type = self.get_field_type_name(field.type)
        
        # Strings, bytes, messages are NEVER packed
        if field_type in ('TYPE_STRING', 'TYPE_BYTES', 'TYPE_MESSAGE', 'TYPE_GROUP'):
            return False
        
        # Check if field has explicit packed option
        if field.HasField('options') and field.options.HasField('packed'):
            return field.options.packed
        
        # Proto3: All numeric repeated fields are packed by default
        # Proto2: Only packed if field has [packed = true] option (which we checked above)
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'
        if syntax == 'proto3':
            # In proto3, numeric/enum repeated fields are packed by default
            return field_type in (
                'TYPE_INT32', 'TYPE_INT64', 'TYPE_UINT32', 'TYPE_UINT64',
                'TYPE_SINT32', 'TYPE_SINT64', 'TYPE_BOOL', 'TYPE_ENUM',
                'TYPE_FIXED32', 'TYPE_SFIXED32', 'TYPE_FIXED64', 'TYPE_SFIXED64',
                'TYPE_FLOAT', 'TYPE_DOUBLE'
            )
        else:
            # Proto2: not packed by default (only if explicit)
            return False
    
    def get_packed_size_expression(self, field_type: str, item_name: str) -> str:
        """Get the expression to calculate the size of a packed field item."""
        if field_type in ('TYPE_INT32', 'TYPE_INT64', 'TYPE_UINT32', 'TYPE_UINT64', 'TYPE_BOOL', 'TYPE_ENUM'):
            return f'litepb::ProtoWriter::varint_size(static_cast<uint64_t>({item_name}))'
        elif field_type in ('TYPE_SINT32'):
            return f'litepb::ProtoWriter::sint32_size({item_name})'
        elif field_type in ('TYPE_SINT64'):
            return f'litepb::ProtoWriter::sint64_size({item_name})'
        elif field_type in ('TYPE_FIXED32', 'TYPE_SFIXED32', 'TYPE_FLOAT'):
            return f'litepb::ProtoWriter::fixed32_size()'
        elif field_type in ('TYPE_FIXED64', 'TYPE_SFIXED64', 'TYPE_DOUBLE'):
            return f'litepb::ProtoWriter::fixed64_size()'
        else:
            # Fallback
            return f'litepb::ProtoWriter::varint_size(static_cast<uint64_t>({item_name}))'
    
    def _generate_packed_read_code(self, lines: List[str], field_type: str, field_name: str) -> None:
        """Generate code to read a single value in packed format (inside a loop)."""
        cpp_type = TypeMapper.get_cpp_type(field_type)
        method = TypeMapper.get_deserialization_method(field_type)
        
        if field_type == 'TYPE_ENUM':
            lines.append(f'                            uint64_t enum_val;')
            lines.append(f'                            if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                            value.{field_name}.push_back(static_cast<decltype(value.{field_name})::value_type>(enum_val));')
        elif method == 'read_varint':
            lines.append(f'                            uint64_t temp_varint;')
            lines.append(f'                            if (!reader.{method}(temp_varint)) return false;')
            if field_type == 'TYPE_BOOL':
                lines.append(f'                            value.{field_name}.push_back(temp_varint != 0);')
            else:
                lines.append(f'                            value.{field_name}.push_back(static_cast<{cpp_type}>(temp_varint));')
        elif method in ('read_sint32', 'read_sint64'):
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            if (!reader.{method}(temp)) return false;')
            lines.append(f'                            value.{field_name}.push_back(temp);')
        elif field_type in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
            unsigned_type = 'uint32_t' if field_type == 'TYPE_SFIXED32' else 'uint64_t'
            lines.append(f'                            {unsigned_type} temp_unsigned;')
            lines.append(f'                            if (!reader.{method}(temp_unsigned)) return false;')
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
            lines.append(f'                            value.{field_name}.push_back(temp);')
        else:
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            if (!reader.{method}(temp)) return false;')
            lines.append(f'                            value.{field_name}.push_back(temp);')
    
    def _generate_unpacked_read_code(self, lines: List[str], field_type: str, field_name: str) -> None:
        """Generate code to read a single value in unpacked format."""
        cpp_type = TypeMapper.get_cpp_type(field_type)
        method = TypeMapper.get_deserialization_method(field_type)
        
        if field_type == 'TYPE_ENUM':
            lines.append(f'                        uint64_t enum_val;')
            lines.append(f'                        if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                        value.{field_name}.push_back(static_cast<decltype(value.{field_name})::value_type>(enum_val));')
        elif method == 'read_varint':
            lines.append(f'                        uint64_t temp_varint;')
            lines.append(f'                        if (!reader.{method}(temp_varint)) return false;')
            if field_type == 'TYPE_BOOL':
                lines.append(f'                        value.{field_name}.push_back(temp_varint != 0);')
            else:
                lines.append(f'                        value.{field_name}.push_back(static_cast<{cpp_type}>(temp_varint));')
        elif method in ('read_sint32', 'read_sint64'):
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        if (!reader.{method}(temp)) return false;')
            lines.append(f'                        value.{field_name}.push_back(temp);')
        elif field_type in ('TYPE_SFIXED32', 'TYPE_SFIXED64'):
            unsigned_type = 'uint32_t' if field_type == 'TYPE_SFIXED32' else 'uint64_t'
            lines.append(f'                        {unsigned_type} temp_unsigned;')
            lines.append(f'                        if (!reader.{method}(temp_unsigned)) return false;')
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
            lines.append(f'                        value.{field_name}.push_back(temp);')
        else:
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        if (!reader.{method}(temp)) return false;')
            lines.append(f'                        value.{field_name}.push_back(temp);')
    
    def get_header_guard(self, filename: str) -> str:
        """Generate header guard from filename."""
        base = os.path.basename(filename).replace('.proto', '').upper()
        return f'LITEPB_GENERATED_{base}_H'
    
    def get_include_filename(self, filename: str) -> str:
        """Get include filename from proto filename."""
        return os.path.basename(filename).replace('.proto', '.pb.h')
    
    def get_namespace_parts(self, package: str) -> List[str]:
        """Split package into namespace parts."""
        if not package:
            return []
        return package.split('.')
    
    def get_namespace_prefix(self, package: str) -> str:
        """Get full namespace prefix for qualified names."""
        if not package:
            return ''
        return package.replace('.', '::')
    
    def generate_rpc_service(self, service: descriptor_pb2.ServiceDescriptorProto, namespace_prefix: str) -> str:
        """Generate RPC service stubs (client, server interface, and registration function)."""
        lines = []
        service_name = service.name
        
        # Extract service options using parser
        service_options = self.parser.extract_service_options(service)
        
        # Validate service_id: must be present and > 0
        service_id = service_options.service_id
        if service_id is None:
            raise RuntimeError(
                f"Service '{service_name}' is missing required 'service_id' option. "
                f"Add: option (litepb.rpc.service_id) = <unique_id>;"
            )
        if service_id == 0:
            raise RuntimeError(
                f"Service '{service_name}' has invalid service_id=0. "
                f"service_id must be greater than 0."
            )
        
        # Generate SERVICE_ID constant
        service_id_constant = f'{service_name.upper()}_SERVICE_ID'
        lines.append(f'#define {service_id_constant} {service_id}')
        lines.append('')
        
        # Validate method_id: all methods must have method_id set
        method_ids = {}
        for method in service.method:
            method_options = self.parser.extract_method_options(method)
            method_id = method_options.method_id
            if method_id is None:
                raise RuntimeError(
                    f"Method '{method.name}' in service '{service_name}' is missing required "
                    f"'method_id' option. Add: option (litepb.rpc.method_id) = <unique_id>;"
                )
            
            # Check for duplicate method_ids within the service
            if method_id in method_ids:
                raise RuntimeError(
                    f"Duplicate method_id {method_id} in service '{service_name}': "
                    f"methods '{method_ids[method_id]}' and '{method.name}' have the same ID"
                )
            method_ids[method_id] = method.name
        
        # Separate methods by type (RPC vs fire-and-forget)
        regular_methods = []  # Request-response RPC methods
        fire_and_forget_methods = []  # Fire-and-forget events
        
        for method in service.method:
            method_options = self.parser.extract_method_options(method)
            is_fire_and_forget = method_options.fire_and_forget
            
            if is_fire_and_forget:
                fire_and_forget_methods.append(method)
            else:
                regular_methods.append(method)
        
        # Generate client stub class
        lines.append(f'// Client stub for {service_name} (service_id={service_id})')
        lines.append(f'class {service_name}Client {{')
        lines.append('public:')
        lines.append(f'    explicit {service_name}Client(litepb::RpcChannel& channel)')
        lines.append('        : channel_(channel) {}')
        lines.append('')
        
        # Generate client RPC methods
        for method in regular_methods:
            method_options = self.parser.extract_method_options(method)
            method_name = method.name
            method_id = method_options.method_id
            input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
            output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
            default_timeout = method_options.default_timeout_ms
            
            lines.append(f'    // service_id={service_id}, method_id={method_id}')
            lines.append(f'    void {method_name}(const {input_type}& request,')
            lines.append(f'                      std::function<void(const litepb::Result<{output_type}>&)> callback,')
            lines.append(f'                      uint32_t timeout_ms = {default_timeout},')
            lines.append(f'                      uint64_t dst_addr = 0) {{')
            lines.append(f'        channel_.call_internal<{input_type}, {output_type}>(')
            lines.append(f'            {service_id}, {method_id}, request, callback, timeout_ms, dst_addr);')
            lines.append('    }')
            lines.append('')
        
        # Generate fire-and-forget event senders
        if fire_and_forget_methods:
            lines.append('    // Fire-and-forget event senders')
            for method in fire_and_forget_methods:
                method_options = self.parser.extract_method_options(method)
                method_name = method.name
                method_id = method_options.method_id
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append(f'    // service_id={service_id}, method_id={method_id}')
                lines.append(f'    bool {method_name}(const {input_type}& event, uint64_t dst_addr = 0) {{')
                lines.append(f'        return channel_.send_event<{input_type}>({service_id}, {method_id}, event, dst_addr);')
                lines.append('    }')
                lines.append('')
        
        # Generate event handler registration
        if fire_and_forget_methods:
            lines.append('    // Event handler registration')
            for method in fire_and_forget_methods:
                method_options = self.parser.extract_method_options(method)
                method_name = method.name
                method_id = method_options.method_id
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append(f'    // service_id={service_id}, method_id={method_id}')
                lines.append(f'    void on{method_name}(std::function<void(uint64_t, const {input_type}&)> handler) {{')
                lines.append(f'        channel_.on_event<{input_type}>({service_id}, {method_id}, handler);')
                lines.append('    }')
                lines.append('')
        
        lines.append('private:')
        lines.append('    litepb::RpcChannel& channel_;')
        lines.append('};')
        lines.append('')
        
        # Generate server interface class (only for regular RPC methods, not fire-and-forget)
        lines.append(f'// Server interface for {service_name}')
        lines.append(f'class {service_name}Server {{')
        lines.append('public:')
        lines.append(f'    virtual ~{service_name}Server() = default;')
        lines.append('')
        
        # Generate server virtual methods for regular RPC
        for method in regular_methods:
            method_name = method.name
            input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
            output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
            
            lines.append(f'    virtual litepb::Result<{output_type}> {method_name}(')
            lines.append(f'        uint64_t src_addr,')
            lines.append(f'        const {input_type}& request) = 0;')
            lines.append('')
        
        lines.append('};')
        lines.append('')
        
        # Generate EventServer interface (for fire-and-forget events)
        if fire_and_forget_methods:
            lines.append(f'// EventServer interface for {service_name}')
            lines.append(f'class {service_name}EventServer {{')
            lines.append('public:')
            lines.append(f'    virtual ~{service_name}EventServer() = default;')
            lines.append('')
            
            # Generate handler methods for all fire-and-forget events
            for method in fire_and_forget_methods:
                method_name = method.name
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append(f'    virtual void {method_name}Handler(uint64_t src_addr, const {input_type}& msg) {{}}')
                lines.append('')
            
            lines.append('};')
            lines.append('')
        
        # Generate registration helper function (only for regular RPC methods, not fire-and-forget)
        lines.append(f'// Registration helper for {service_name} (service_id={service_id})')
        lines.append(f'inline void register_{self._to_snake_case(service_name)}(')
        lines.append(f'    litepb::RpcChannel& channel,')
        lines.append(f'    {service_name}Server& server) {{')
        
        # Register each regular RPC method
        for method in regular_methods:
            method_options = self.parser.extract_method_options(method)
            method_name = method.name
            method_id = method_options.method_id
            input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
            output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
            
            lines.append('')
            lines.append(f'    // service_id={service_id}, method_id={method_id}')
            lines.append(f'    channel.on_internal<{input_type}, {output_type}>(')
            lines.append(f'        {service_id}, {method_id},')
            lines.append(f'        [&server](uint64_t src_addr, const {input_type}& request) {{')
            lines.append(f'            return server.{method_name}(src_addr, request);')
            lines.append('        });')
        
        lines.append('}')
        lines.append('')
        
        # Generate event registration helper (for fire-and-forget events)
        if fire_and_forget_methods:
            lines.append(f'// Event registration helper for {service_name} (service_id={service_id})')
            lines.append(f'inline void register_{self._to_snake_case(service_name)}_events(')
            lines.append(f'    litepb::RpcChannel& channel,')
            lines.append(f'    {service_name}EventServer* handler) {{')
            lines.append('    if (handler) {')
            
            # Register each fire-and-forget event
            for method in fire_and_forget_methods:
                method_options = self.parser.extract_method_options(method)
                method_name = method.name
                method_id = method_options.method_id
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append('')
                lines.append(f'        // service_id={service_id}, method_id={method_id}')
                lines.append(f'        channel.on_event<{input_type}>(')
                lines.append(f'            {service_id}, {method_id},')
                lines.append(f'            [handler](uint64_t src_addr, const {input_type}& msg) {{')
                lines.append(f'                handler->{method_name}Handler(src_addr, msg);')
                lines.append('            });')
            
            lines.append('    }')
            lines.append('}')
            lines.append('')
        
        return '\n'.join(lines)
    
    def _get_qualified_type_name(self, type_name: str, namespace_prefix: str) -> str:
        """Get qualified type name for RPC methods."""
        # Remove leading dot if present
        if type_name.startswith('.'):
            type_name = type_name[1:]
        
        # Convert proto package notation to C++ namespace notation
        type_name = type_name.replace('.', '::')
        
        # If the type is in the same package, we can use it directly
        # Otherwise, we need the full qualification
        if namespace_prefix and type_name.startswith(namespace_prefix + '::'):
            # Type is in the same namespace, can use unqualified name
            return type_name[len(namespace_prefix) + 2:]
        
        return type_name
    
    def _to_snake_case(self, name: str) -> str:
        """Convert CamelCase to snake_case."""
        result = []
        for i, char in enumerate(name):
            if char.isupper() and i > 0:
                result.append('_')
            result.append(char.lower())
        return ''.join(result)
