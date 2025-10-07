#!/usr/bin/env python3
"""
Base classes for language-specific code generators.

This module defines the abstract interface that all language backends must implement.
Language generators are responsible for converting protobuf descriptors into
language-specific source code files.

To create a new language backend:
1. Create a new module under generator/backends/<language>/
2. Implement a class that inherits from LanguageGenerator
3. Implement the abstract methods: generate_header() and generate_implementation()
4. Use the google.protobuf.descriptor_pb2 types to access proto structure

Example:
    class MyLanguageGenerator(LanguageGenerator):
        def generate_header(self, file_proto, filename):
            # Generate interface/header file content
            return "// generated code..."
        
        def generate_implementation(self, file_proto, filename):
            # Generate implementation file content
            return "// generated code..."
"""

from abc import ABC, abstractmethod
from google.protobuf import descriptor_pb2


class LanguageGenerator(ABC):
    """
    Abstract base class for language-specific code generators.
    
    This class defines the contract that all language backends must implement
    to generate code from protobuf definitions. Subclasses should implement
    methods to generate both header/interface files and implementation files
    appropriate for the target language.
    """
    
    @abstractmethod
    def generate_header(self, file_proto: descriptor_pb2.FileDescriptorProto, filename: str) -> str:
        """
        Generate header/interface file content for the given proto file.
        
        This method should produce the language-specific header or interface code
        (e.g., .h for C++, .ts for TypeScript, .py stub for Python) from the
        protobuf file descriptor.
        
        Args:
            file_proto: The protobuf file descriptor containing messages, enums, and services
            filename: The name of the proto file being processed (e.g., "person.proto")
        
        Returns:
            A string containing the complete header/interface file content
        
        Raises:
            NotImplementedError: Must be implemented by subclasses
        """
        pass
    
    @abstractmethod
    def generate_implementation(self, file_proto: descriptor_pb2.FileDescriptorProto, filename: str) -> str:
        """
        Generate implementation file content for the given proto file.
        
        This method should produce the language-specific implementation code
        (e.g., .cpp for C++, .js for TypeScript, .py for Python) from the
        protobuf file descriptor.
        
        Args:
            file_proto: The protobuf file descriptor containing messages, enums, and services
            filename: The name of the proto file being processed (e.g., "person.proto")
        
        Returns:
            A string containing the complete implementation file content
        
        Raises:
            NotImplementedError: Must be implemented by subclasses
        """
        pass
