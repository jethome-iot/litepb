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
from ...core.proto_parser import ProtoParser


class CppGenerator(LanguageGenerator):
    """Generate C++ code from parsed protobuf structures."""
    
    def __init__(self):
        """Initialize the generator with templates."""
        self.current_proto = None  # Track current proto for context (FileDescriptorProto)
        self.parser = ProtoParser()  # For extracting RPC options
        self.setup_templates()
    
    @property
    def rpc_generator(self):
        """Lazy-load RPC generator only when services are detected."""
        if not hasattr(self, '_rpc_generator'):
            from ...rpc.litepb.generator import LitePBRpcGenerator
            self._rpc_generator = LitePBRpcGenerator()
        return self._rpc_generator
    
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
            # Convert "path/to/file.proto" to "path/to/file.pb.h"
            include_path = dependency.replace('.proto', '.pb.h')
            import_includes.append(include_path)
        
        # Sort messages in topological order to avoid forward declaration issues
        messages = list(file_proto.message_type)
        sorted_messages = CppUtils.topological_sort_messages(messages)
        
        # Generate RPC services using the RPC generator
        services = list(file_proto.service)
        namespace_prefix = CppUtils.get_namespace_prefix(file_proto.package if file_proto.package else '')
        rpc_services_code = self.rpc_generator.generate_services(services, file_proto, namespace_prefix)

        # Prepare context
        context = {
            'header_guard': CppUtils.get_header_guard(filename),
            'package': file_proto.package if file_proto.package else '',
            'namespace_parts': CppUtils.get_namespace_parts(file_proto.package if file_proto.package else ''),
            'namespace_prefix': namespace_prefix,
            'imports': import_includes,
            'enums': list(file_proto.enum_type),
            'messages': sorted_messages,
            'services': services,
            'rpc_services_code': rpc_services_code,
        }
        
        return template.render(**context)
    
    def generate_implementation(self, file_proto: pb2.FileDescriptorProto, filename: str) -> str:
        """Generate C++ implementation file content."""
        self.current_proto = file_proto  # Set context for type generation
        template = self.env.get_template('source.j2')
        
        # Prepare context
        context = {
            'include_file': CppUtils.get_include_filename(filename),
            'namespace_prefix': CppUtils.get_namespace_prefix(file_proto.package if file_proto.package else ''),
            'messages': list(file_proto.message_type),
        }
        
        return template.render(**context)
    
    # Template helper methods - these are called by Jinja templates with current_proto already set
    
    def generate_enum(self, enum_proto: pb2.EnumDescriptorProto, indent: int = 0) -> str:
        """Generate enum definition."""
        assert self.current_proto is not None, "current_proto must be set before generating enum"
        message_codegen = MessageCodegen(self.current_proto)
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
        message_codegen = MessageCodegen(self.current_proto)
        return message_codegen.generate_message_declaration(message)

    def generate_message_definition(self, message: pb2.DescriptorProto) -> str:
        """Generate complete definition for a message."""
        assert self.current_proto is not None, "current_proto must be set before generating message definition"
        message_codegen = MessageCodegen(self.current_proto)
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
