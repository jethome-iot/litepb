#!/usr/bin/env python3
"""
LitePB RPC generator for C++.
Generates RPC service stubs (client, server, and registration helpers) from protobuf service definitions.
"""

import os
from typing import List
from jinja2 import Environment, FileSystemLoader
from google.protobuf import descriptor_pb2 as pb2

from ..base import RpcGenerator
from ...core.proto_parser import ProtoParser
from ...core.rpc_options import CallDirection


class LitePBRpcGenerator(RpcGenerator):
    """Generate RPC service stubs for LitePB C++ backend."""
    
    def __init__(self):
        """Initialize the RPC generator with templates."""
        self.parser = ProtoParser()
        self.setup_templates()
    
    def setup_templates(self):
        """Set up Jinja2 templates for RPC code generation."""
        template_dir = os.path.join(os.path.dirname(__file__), 'templates')
        self.env = Environment(loader=FileSystemLoader(template_dir))
    
    def generate_services(
        self, 
        services: List[pb2.ServiceDescriptorProto], 
        file_proto: pb2.FileDescriptorProto,
        namespace_prefix: str
    ) -> str:
        """Generate all RPC services."""
        if not services:
            return ''
        
        lines = []
        lines.append('// RPC Service Stubs')
        for service in services:
            lines.append(self.generate_rpc_service(service, namespace_prefix))
        
        return '\n'.join(lines)
    
    def generate_rpc_service(self, service: pb2.ServiceDescriptorProto, namespace_prefix: str) -> str:
        """Generate RPC service stubs with unified base classes."""
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
        
        # Categorize methods based on direction and event type
        client_to_server_rpcs = []
        server_to_client_events = []
        bidirectional_methods = []
        
        for method in service.method:
            method_options = self.parser.extract_method_options(method)
            
            
            if method_options.direction == CallDirection.CLIENT_TO_SERVER:
                if method_options.is_event:
                    # Client-to-server event (no response)
                    client_to_server_rpcs.append((method, method_options))
                else:
                    # Standard RPC
                    client_to_server_rpcs.append((method, method_options))
            elif method_options.direction == CallDirection.SERVER_TO_CLIENT:
                server_to_client_events.append((method, method_options))
            else:  # BIDIRECTIONAL
                bidirectional_methods.append((method, method_options))
        
        # Generate unified Server base class
        lines.append(f'// Server base class for {service_name} (service_id={service_id})')
        lines.append(f'class {service_name}Server {{')
        lines.append('public:')
        lines.append(f'    explicit {service_name}Server(litepb::RpcChannel& channel)')
        lines.append('        : channel_(channel) {')
        lines.append('        // Auto-register all handlers in constructor')
        lines.append(f'        register_handlers();')
        lines.append('    }')
        lines.append('')
        lines.append(f'    virtual ~{service_name}Server() = default;')
        lines.append('')
        
        # Generate pure virtual handler methods for RPCs from client
        if client_to_server_rpcs or bidirectional_methods:
            lines.append('    // Pure virtual RPC handlers - must be implemented by derived class')
            for method, options in client_to_server_rpcs + bidirectional_methods:
                if not options.is_event:
                    method_name = method.name
                    input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                    output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
                    
                    # Check if return type is void/empty
                    if self._is_void_type(method.output_type):
                        lines.append(f'    virtual void handle{method_name}(const {input_type}& request) = 0;')
                    else:
                        lines.append(f'    virtual litepb::Result<{output_type}> handle{method_name}(const {input_type}& request) = 0;')
                    lines.append('')
        
        # Generate virtual event handlers for events from client
        for method, options in client_to_server_rpcs:
            if options.is_event:
                method_name = method.name
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                lines.append(f'    virtual void on{method_name}(const {input_type}& event) {{}}')
                lines.append('')
        
        # Generate concrete event emitters for server-to-client events
        if server_to_client_events or bidirectional_methods:
            lines.append('    // Concrete event emitters')
            for method, options in server_to_client_events + bidirectional_methods:
                if options.is_event:
                    method_name = method.name
                    input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                    
                    lines.append(f'    // service_id={service_id}, method_id={options.method_id}')
                    lines.append(f'    bool emit{method_name}(const {input_type}& event) {{')
                    lines.append(f'        return channel_.send_event<{input_type}>({service_id}, {options.method_id}, event);')
                    lines.append('    }')
                    lines.append('')
        
        lines.append('protected:')
        lines.append('    litepb::RpcChannel& channel_;')
        lines.append('')
        lines.append('private:')
        lines.append('    void register_handlers() {')
        
        # Register all handlers
        for method, options in client_to_server_rpcs + bidirectional_methods:
            method_name = method.name
            method_id = options.method_id
            input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
            output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
            
            if options.is_event:
                # Register event handler
                lines.append(f'        // Register event handler for {method_name}')
                lines.append(f'        channel_.on_event<{input_type}>({service_id}, {method_id},')
                lines.append(f'            [this](const {input_type}& event) {{')
                lines.append(f'                this->on{method_name}(event);')
                lines.append('            });')
            else:
                # Register RPC handler
                lines.append(f'        // Register RPC handler for {method_name}')
                if self._is_void_type(method.output_type):
                    # Void return type - need to wrap in Result
                    lines.append(f'        channel_.on<{input_type}, google::protobuf::Empty>(')
                    lines.append(f'            {service_id}, {method_id},')
                    lines.append(f'            [this](const {input_type}& request) {{')
                    lines.append(f'                this->handle{method_name}(request);')
                    lines.append(f'                return litepb::Result<google::protobuf::Empty>{{}};')
                    lines.append('            });')
                else:
                    lines.append(f'        channel_.on<{input_type}, {output_type}>(')
                    lines.append(f'            {service_id}, {method_id},')
                    lines.append(f'            [this](const {input_type}& request) {{')
                    lines.append(f'                return this->handle{method_name}(request);')
                    lines.append('            });')
            lines.append('')
        
        lines.append('    }')
        lines.append('};')
        lines.append('')
        
        # Generate unified Client base class
        lines.append(f'// Client base class for {service_name} (service_id={service_id})')
        lines.append(f'class {service_name}Client {{')
        lines.append('public:')
        lines.append(f'    explicit {service_name}Client(litepb::RpcChannel& channel)')
        lines.append('        : channel_(channel) {')
        lines.append('        // Auto-register event handlers in constructor')
        lines.append(f'        register_event_handlers();')
        lines.append('    }')
        lines.append('')
        lines.append(f'    virtual ~{service_name}Client() = default;')
        lines.append('')
        
        # Generate concrete RPC call methods
        if client_to_server_rpcs or bidirectional_methods:
            lines.append('    // Concrete RPC call methods')
            for method, options in client_to_server_rpcs + bidirectional_methods:
                if not options.is_event:
                    method_name = method.name
                    method_id = options.method_id
                    input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                    output_type = self._get_qualified_type_name(method.output_type, namespace_prefix)
                    default_timeout = options.default_timeout_ms
                    
                    lines.append(f'    // service_id={service_id}, method_id={method_id}')
                    if self._is_void_type(method.output_type):
                        # Simplified API for void returns
                        lines.append(f'    void {method_name}(const {input_type}& request,')
                        lines.append(f'                      std::function<void(bool success)> callback,')
                        lines.append(f'                      uint32_t timeout_ms = {default_timeout}) {{')
                        lines.append(f'        channel_.call<{input_type}, google::protobuf::Empty>(')
                        lines.append(f'            {service_id}, {method_id}, request,')
                        lines.append(f'            [callback](const litepb::Result<google::protobuf::Empty>& result) {{')
                        lines.append(f'                callback(result.error.code == litepb::RpcError::OK);')
                        lines.append('            }, timeout_ms);')
                    else:
                        lines.append(f'    void {method_name}(const {input_type}& request,')
                        lines.append(f'                      std::function<void(const litepb::Result<{output_type}>&)> callback,')
                        lines.append(f'                      uint32_t timeout_ms = {default_timeout}) {{')
                        lines.append(f'        channel_.call<{input_type}, {output_type}>(')
                        lines.append(f'            {service_id}, {method_id}, request, callback, timeout_ms);')
                    lines.append('    }')
                    lines.append('')
        
        # Generate concrete event emitters for client-to-server events
        for method, options in client_to_server_rpcs:
            if options.is_event:
                method_name = method.name
                method_id = options.method_id
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append(f'    // service_id={service_id}, method_id={method_id}')
                lines.append(f'    bool emit{method_name}(const {input_type}& event) {{')
                lines.append(f'        return channel_.send_event<{input_type}>({service_id}, {method_id}, event);')
                lines.append('    }')
                lines.append('')
        
        # Generate virtual event handlers with empty defaults
        if server_to_client_events or bidirectional_methods:
            lines.append('    // Virtual event handlers - can be overridden by derived class')
            for method, options in server_to_client_events + bidirectional_methods:
                if options.is_event:
                    method_name = method.name
                    input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                    
                    lines.append(f'    virtual void on{method_name}(const {input_type}& event) {{')
                    lines.append('        // Default empty implementation - override in derived class')
                    lines.append('    }')
                    lines.append('')
        
        lines.append('protected:')
        lines.append('    litepb::RpcChannel& channel_;')
        lines.append('')
        lines.append('private:')
        lines.append('    void register_event_handlers() {')
        
        # Register event handlers
        for method, options in server_to_client_events + bidirectional_methods:
            if options.is_event:
                method_name = method.name
                method_id = options.method_id
                input_type = self._get_qualified_type_name(method.input_type, namespace_prefix)
                
                lines.append(f'        // Register event handler for {method_name}')
                lines.append(f'        channel_.on_event<{input_type}>({service_id}, {method_id},')
                lines.append(f'            [this](const {input_type}& event) {{')
                lines.append(f'                this->on{method_name}(event);')
                lines.append('            });')
                lines.append('')
        
        lines.append('    }')
        lines.append('};')
        lines.append('')
        
        # Generate backward compatibility registration helpers (optional, for migration)
        lines.append(f'// Backward compatibility helper (deprecated)')
        lines.append(f'[[deprecated("Use {service_name}Server constructor instead")]]')
        lines.append(f'inline void register_{self._to_snake_case(service_name)}(')
        lines.append(f'    litepb::RpcChannel& channel,')
        lines.append(f'    {service_name}Server& server) {{')
        lines.append('    // Server now auto-registers in constructor')
        lines.append('    (void)channel;')
        lines.append('    (void)server;')
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
    
    def _is_void_type(self, type_name: str) -> bool:
        """Check if the type is void or Empty."""
        # Remove leading dot if present
        if type_name.startswith('.'):
            type_name = type_name[1:]
        
        void_types = [
            'google.protobuf.Empty',
            'Empty',
            'void'  # Support explicit void
        ]
        
        return type_name in void_types or type_name.endswith('::Empty') or type_name.endswith('.Empty')
    
    def _to_snake_case(self, name: str) -> str:
        """Convert CamelCase to snake_case."""
        result = []
        for i, char in enumerate(name):
            if char.isupper() and i > 0:
                result.append('_')
            result.append(char.lower())
        return ''.join(result)