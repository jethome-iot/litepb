#!/usr/bin/env python3
"""
C++ specific utilities for code generation.
"""

import os
from typing import List
from google.protobuf import descriptor_pb2 as pb2


class CppUtils:
    """C++ specific utility functions."""
    
    @staticmethod
    def get_header_guard(filename: str) -> str:
        """Generate header guard from filename."""
        base = os.path.basename(filename).replace('.proto', '').upper()
        return f'LITEPB_GENERATED_{base}_H'
    
    @staticmethod
    def get_include_filename(filename: str) -> str:
        """Get include filename from proto filename."""
        return os.path.basename(filename).replace('.proto', '.pb.h')
    
    @staticmethod
    def get_namespace_parts(package: str) -> List[str]:
        """Split package into namespace parts."""
        if not package:
            return []
        return package.split('.')
    
    @staticmethod
    def get_namespace_prefix(package: str) -> str:
        """Get full namespace prefix for qualified names."""
        if not package:
            return ''
        return package.replace('.', '::')
    
    @staticmethod
    def qualify_type_name(type_name: str, package: str = '', current_scope: str = '') -> str:
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
    
    @staticmethod
    def simplify_type_in_context(full_type: str, current_ns: str = '', current_msg: str = '') -> str:
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
    
    @staticmethod
    def topological_sort_messages(messages: List[pb2.DescriptorProto]) -> List[pb2.DescriptorProto]:
        """Sort messages in topological order to avoid forward declaration issues."""
        # Create a map for quick lookup
        msg_map = {msg.name: msg for msg in messages}

        # Build dependency graph
        dependencies = {}
        for msg in messages:
            dependencies[msg.name] = CppUtils._get_message_dependencies(msg, messages)

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

    @staticmethod
    def _get_message_dependencies(message: pb2.DescriptorProto, all_messages: List[pb2.DescriptorProto]) -> List[str]:
        """Get list of message names that this message depends on."""
        dependencies = set()

        # Check all fields
        for field in message.field:
            if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
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
