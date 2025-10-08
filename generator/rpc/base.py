#!/usr/bin/env python3
"""
Base classes for RPC code generators.

This module defines the abstract interface that all RPC backends must implement.
RPC generators are responsible for converting protobuf service definitions into
RPC framework-specific client and server code.

To create a new RPC backend:
1. Create a new module under generator/rpc/<framework>/
2. Implement a class that inherits from RpcGenerator
3. Implement the abstract method: generate_services()
4. Use the google.protobuf.descriptor_pb2 types to access service structure

Example:
    class MyRpcGenerator(RpcGenerator):
        def generate_services(self, services, file_proto, namespace_prefix):
            # Generate RPC client/server stubs
            return "// generated RPC code..."
"""

from abc import ABC, abstractmethod
from typing import List
from google.protobuf import descriptor_pb2 as pb2


class RpcGenerator(ABC):
    """
    Abstract base class for RPC code generators.
    
    This class defines the contract that all RPC backends must implement
    to generate RPC client and server stubs from protobuf service definitions.
    Subclasses should implement methods to generate framework-specific code
    for handling remote procedure calls.
    """
    
    @abstractmethod
    def generate_services(
        self, 
        services: List[pb2.ServiceDescriptorProto], 
        file_proto: pb2.FileDescriptorProto,
        namespace_prefix: str
    ) -> str:
        """
        Generate RPC service stubs (client and server) for all services.
        
        This method should produce RPC framework-specific code for:
        - Client stubs to call remote methods
        - Server interfaces to implement remote methods
        - Registration/routing logic for the RPC framework
        
        Args:
            services: List of service descriptors from the proto file
            file_proto: The complete file descriptor (for accessing package, options, etc.)
            namespace_prefix: The namespace/package prefix for type qualification
                             (e.g., "mypackage" or "mypackage::subpackage")
        
        Returns:
            A string containing the complete RPC service code (clients, servers, registration)
        
        Raises:
            NotImplementedError: Must be implemented by subclasses
        """
        pass
