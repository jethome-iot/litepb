#!/usr/bin/env python3
"""
C++ code generator for LitePB.
Generates C++ header and implementation files from protobuf descriptors.
"""

import os
from typing import List
from jinja2 import Environment, FileSystemLoader
from google.protobuf import descriptor_pb2 as pb2

from ..base import LanguageGenerator
from .cpp_utils import CppUtils
from .message_codegen import MessageCodegen
from .serialization_codegen import SerializationCodegen
from .type_mapper import TypeMapper
from ...core.proto_parser import ProtoParser


class CppGenerator(LanguageGenerator):
    """Generate C++ code from parsed protobuf structures."""
    
    def __init__(self, namespace_prefix: str = ''):
        """Initialize the generator with templates.
        
        Args:
            namespace_prefix: Optional prefix to add to all namespaces (e.g., "litepb_")
        """
        self.current_proto = None  # Track current proto for context (FileDescriptorProto)
        self.namespace_prefix = namespace_prefix
        self.parser = ProtoParser()
        self.setup_templates()
    
    def setup_templates(self):
        """Set up Jinja2 templates for code generation."""
        template_dir = os.path.join(os.path.dirname(__file__), 'templates')
        self.env = Environment(loader=FileSystemLoader(template_dir))
        
        # Register custom functions - they'll be called with current_proto context
        self.env.globals['generate_enum'] = self.generate_enum
        self.env.globals['generate_message'] = self.generate_message
        self.env.globals['generate_message_declaration'] = self.generate_message_declaration
        self.env.globals['generate_message_definition'] = self.generate_message_definition
        self.env.globals['generate_serializer_spec'] = self.generate_serializer_spec
        self.env.globals['generate_serializer_impl'] = self.generate_serializer_impl
    
    def generate_header(self, file_proto: pb2.FileDescriptorProto, filename: str) -> str:
        """Generate C++ header file content."""
        self.current_proto = file_proto  # Set context for type generation
        template = self.env.get_template('header.j2')
        
        # Convert imports to include paths
        import_includes = []
        for dependency in file_proto.dependency:
            # Skip rpc_options.proto - it's only for compile-time extension field definitions
            if 'rpc_options.proto' in dependency:
                continue
            # Handle well-known type imports specially
            if 'google/protobuf/' in dependency and dependency.endswith('.proto'):
                # Well-known types are handled by our own headers
                continue
            # Convert "path/to/file.proto" to "path/to/file.pb.h"
            include_path = dependency.replace('.proto', '.pb.h')
            import_includes.append(include_path)
        
        # Sort messages in topological order to avoid forward declaration issues
        messages = list(file_proto.message_type)
        sorted_messages = CppUtils.topological_sort_messages(messages)
        
        # Get namespace prefix before using it
        namespace_prefix = CppUtils.get_namespace_prefix(file_proto.package if file_proto.package else '', self.namespace_prefix)
        
        # For serialization, we need just the package namespace path without the wrapper
        # When there's a wrapper namespace, serializers should reference types without it
        package_ns = file_proto.package.replace('.', '::') if file_proto.package else ''
        
        # Create serialization codegen instance for global serializer generation
        serialization_codegen = SerializationCodegen(file_proto, self.namespace_prefix)
        serializer_forward_declarations = serialization_codegen.generate_all_serializer_forward_declarations(sorted_messages, package_ns)
        serializers_code = serialization_codegen.generate_all_serializers(sorted_messages, package_ns, True)
        
        # Prepare context
        context = {
            'header_guard': CppUtils.get_header_guard(filename),
            'package': file_proto.package if file_proto.package else '',
            'namespace_parts': CppUtils.get_namespace_parts(file_proto.package if file_proto.package else '', self.namespace_prefix),
            'namespace_prefix': self.namespace_prefix,  # Pass the raw prefix string for wrapper namespace
            'imports': import_includes,
            'enums': list(file_proto.enum_type),
            'messages': sorted_messages,
            'serializer_forward_declarations': serializer_forward_declarations,
            'serializers_code': serializers_code,
        }
        
        return template.render(**context)
    
    def generate_implementation(self, file_proto: pb2.FileDescriptorProto, filename: str) -> str:
        """Generate C++ implementation file content."""
        self.current_proto = file_proto  # Set context for type generation
        template = self.env.get_template('source.j2')
        
        # Prepare context
        context = {
            'include_file': CppUtils.get_include_filename(filename),
            'namespace_prefix': CppUtils.get_namespace_prefix(file_proto.package if file_proto.package else '', self.namespace_prefix),
            'messages': list(file_proto.message_type),
        }
        
        return template.render(**context)
    
    # Template helper methods - these are called by Jinja templates with current_proto already set
    
    def generate_enum(self, enum_proto: pb2.EnumDescriptorProto, indent: int = 0) -> str:
        """Generate enum definition."""
        assert self.current_proto is not None, "current_proto must be set before generating enum"
        message_codegen = MessageCodegen(self.current_proto, self.namespace_prefix)
        return message_codegen.generate_enum(enum_proto, indent)
    
    def generate_message(self, message: pb2.DescriptorProto, indent: int = 0) -> str:
        """Generate message struct definition (legacy method, prefer generate_message_definition)."""
        # Just delegate to generate_message_definition and adjust indentation
        definition = self.generate_message_definition(message)
        if indent == 0:
            return definition
        
        # Add indentation
        ind = '    ' * indent
        lines = definition.split('\n')
        return '\n'.join(f'{ind}{line}' if line.strip() else '' for line in lines)
    
    def generate_message_declaration(self, message: pb2.DescriptorProto) -> str:
        """Generate forward declaration for a message and its nested types."""
        assert self.current_proto is not None, "current_proto must be set before generating message declaration"
        message_codegen = MessageCodegen(self.current_proto, self.namespace_prefix)
        return message_codegen.generate_message_declaration(message)

    def generate_message_definition(self, message: pb2.DescriptorProto) -> str:
        """Generate complete definition for a message."""
        assert self.current_proto is not None, "current_proto must be set before generating message definition"
        message_codegen = MessageCodegen(self.current_proto, self.namespace_prefix)
        return message_codegen.generate_message_definition(message)

    def generate_serializer_spec(self, message: pb2.DescriptorProto, ns_prefix: str, inline: bool) -> str:
        """Generate serializer specialization for a message and its nested messages."""
        assert self.current_proto is not None, "current_proto must be set before generating serializer spec"
        serialization_codegen = SerializationCodegen(self.current_proto)
        return serialization_codegen.generate_serializer_spec(message, ns_prefix, inline)
    
    def generate_serializer_impl(self, message: pb2.DescriptorProto, ns_prefix: str) -> str:
        """Generate serializer implementation (non-inline version)."""
        # All serializers are now inline in the header, so no need for separate implementations
        return ""
