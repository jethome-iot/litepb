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
        regular_methods = []
        fire_and_forget_methods = []
        
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
