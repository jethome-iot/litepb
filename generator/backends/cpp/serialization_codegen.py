#!/usr/bin/env python3
"""
Serialization and deserialization code generation for C++.
"""

from typing import List, Dict
from google.protobuf import descriptor_pb2 as pb2
from generator.backends.cpp.type_mapper import TypeMapper
from generator.backends.cpp.field_utils import FieldUtils
from generator.backends.cpp.models import MapFieldInfo, OneofInfo


class SerializationCodegen:
    """Generate C++ serialization/deserialization code."""
    
    def __init__(self, current_proto: pb2.FileDescriptorProto):
        """Initialize with current proto context."""
        self.current_proto = current_proto
    
    def _collect_all_nested(self, message: pb2.DescriptorProto, ns_prefix: str, result: dict) -> None:
        """Recursively collect all nested messages into a dict."""
        for nested_msg in message.nested_type:
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_prefix = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
                full_name = f'{nested_prefix}::{nested_msg.name}'
                result[full_name] = (nested_msg, nested_prefix)
                self._collect_all_nested(nested_msg, nested_prefix, result)
    
    def _build_dependency_graph(self, all_msgs: dict) -> dict:
        """Build dependency graph: msg_name -> set of messages it depends on."""
        deps = {name: set() for name in all_msgs}
        
        for full_name, (msg, prefix) in all_msgs.items():
            # First, add dependencies for nested types - parent depends on its nested types
            # This ensures nested types are generated before their parent
            for nested_msg in msg.nested_type:
                if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                    nested_full_name = f'{full_name}::{nested_msg.name}'
                    if nested_full_name in all_msgs:
                        deps[full_name].add(nested_full_name)
            
            # Then check all fields including those in nested types and map fields
            for field in msg.field:
                if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
                    # Get the type name
                    type_name = field.type_name
                    # Remove leading dot if present
                    if type_name.startswith('.'):
                        type_name = type_name[1:]
                    
                    # Handle different name formats
                    dep_name = self._find_message_in_all_msgs(type_name, all_msgs, full_name)
                    if dep_name and dep_name != full_name:  # Don't add self-dependencies
                        deps[full_name].add(dep_name)
        
        return deps
    
    def _find_message_in_all_msgs(self, type_name: str, all_msgs: dict, current_msg: str) -> str:
        """Find a message type name in all_msgs dict, handling nested types."""
        # Remove leading dot if present
        if type_name.startswith('.'):
            type_name = type_name[1:]
        
        # If it has a package prefix, remove it for matching
        if self.current_proto.package and type_name.startswith(self.current_proto.package + '.'):
            type_name = type_name[len(self.current_proto.package) + 1:]
        
        # Convert proto path to C++ namespace format
        cpp_type = type_name.replace('.', '::')
        
        # Try to find exact match first
        for msg_name in all_msgs:
            if msg_name == cpp_type:
                return msg_name
            # Check if it ends with this type (for namespace-prefixed types)
            if msg_name.endswith('::' + cpp_type):
                return msg_name
        
        # If type is just a simple name, look for it in nested types of current message
        if '.' not in type_name and '::' not in type_name:
            # Check if it's a sibling type in the same parent
            if '::' in current_msg:
                parent = current_msg.rsplit('::', 1)[0]
                sibling = f'{parent}::{type_name}'
                if sibling in all_msgs:
                    return sibling
            
            # Check with namespace prefix
            if self.current_proto.package:
                ns_prefix = self.current_proto.package.replace('.', '::')
                prefixed = f'{ns_prefix}::{type_name}'
                if prefixed in all_msgs:
                    return prefixed
        
        return ""
    
    def _topological_sort(self, deps: dict, all_msgs: dict) -> List[tuple]:
        """Topological sort to generate messages in dependency order (dependencies first)."""
        result = []
        visited = set()
        visiting = set()  # For cycle detection
        
        def visit(node):
            if node in visited:
                return
            if node in visiting:
                # Cycle detected, but we'll continue anyway
                return
            visiting.add(node)
            # Visit dependencies first - these are the types this message depends on
            for dep in deps.get(node, []):
                if dep in all_msgs:  # Only visit if it's in our set of messages
                    visit(dep)
            visiting.remove(node)
            visited.add(node)
            # Then add this node
            result.append(all_msgs[node])
        
        for node in sorted(deps.keys()):  # Sort for consistent ordering
            visit(node)
        
        return result
    
    def _collect_nested_messages_reverse(self, message: pb2.DescriptorProto, ns_prefix: str, result: List[tuple]) -> None:
        """Collect nested messages in reverse declaration order (later siblings first)."""
        for nested_msg in reversed(message.nested_type):
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_prefix = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
                # First add this message
                result.append((nested_msg, nested_prefix))
                # Then recursively collect its children
                self._collect_nested_messages_reverse(nested_msg, nested_prefix, result)
    
    def generate_all_serializer_forward_declarations(self, messages: List[pb2.DescriptorProto], ns_prefix: str) -> str:
        """Generate forward declarations for all serializers."""
        lines = []
        
        # Collect all messages including nested ones
        all_msgs = []
        for message in messages:
            self._collect_messages_for_forward_decl(message, ns_prefix, all_msgs)
        
        # Generate forward declarations
        for msg_type, msg in all_msgs:
            # Skip map entry types - they don't need serializers
            if msg.HasField('options') and msg.options.map_entry:
                continue
            lines.append(f'template<> class Serializer<{msg_type}>;')
        
        return '\n'.join(lines)
    
    def _collect_messages_for_forward_decl(self, message: pb2.DescriptorProto, ns_prefix: str, result: List[tuple]) -> None:
        """Recursively collect all messages for forward declarations."""
        msg_type = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
        result.append((msg_type, message))
        
        for nested_msg in message.nested_type:
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_prefix = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
                self._collect_messages_for_forward_decl(nested_msg, nested_prefix, result)
    
    def generate_all_serializers(self, messages: List[pb2.DescriptorProto], ns_prefix: str, inline: bool) -> str:
        """Generate all serializers for a list of messages in proper dependency order."""
        lines = []
        
        # Collect all messages including nested ones with depth-first ordering
        # This ensures nested types are collected before their parents
        all_msgs_list = []
        for message in messages:
            self._collect_messages_depth_first(message, ns_prefix, all_msgs_list)
        
        # Convert to dict for dependency graph building
        all_msgs = {full_name: (msg, prefix) for full_name, msg, prefix in all_msgs_list}
        
        # Build dependency graph for ALL messages
        deps = self._build_dependency_graph(all_msgs)
        
        # Sort in topological order (dependencies first)
        sorted_msgs = self._topological_sort(deps, all_msgs)
        
        # Generate serializers in dependency order
        for msg, prefix in sorted_msgs:
            # Skip map entry types - they don't need serializers
            if msg.HasField('options') and msg.options.map_entry:
                continue
            lines.append(self._generate_single_serializer(msg, prefix, inline))
            lines.append('')
        
        return '\n'.join(lines)
    
    def _collect_messages_depth_first(self, message: pb2.DescriptorProto, ns_prefix: str, result: List[tuple]) -> None:
        """Collect messages depth-first, ensuring nested types come before parents."""
        # First, recursively collect all nested messages
        for nested_msg in message.nested_type:
            if not (nested_msg.HasField('options') and nested_msg.options.map_entry):
                nested_prefix = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
                self._collect_messages_depth_first(nested_msg, nested_prefix, result)
        
        # Then add the parent message
        full_name = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
        result.append((full_name, message, ns_prefix))

    def generate_serializer_spec(self, message: pb2.DescriptorProto, ns_prefix: str, inline: bool) -> str:
        """Generate serializer specialization for a message and its nested messages."""
        lines = []
        
        # Collect all messages with their fully qualified names
        all_msgs = {}
        full_name = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
        all_msgs[full_name] = (message, ns_prefix)
        self._collect_all_nested(message, ns_prefix, all_msgs)
        
        # Build dependency graph  
        deps = self._build_dependency_graph(all_msgs)
        
        # Sort in topological order (dependencies first)
        sorted_msgs = self._topological_sort(deps, all_msgs)
        
        # Generate serializers in dependency order
        for msg, prefix in sorted_msgs:
            # Skip map entry types - they don't need serializers
            if msg.HasField('options') and msg.options.map_entry:
                continue
            lines.append(self._generate_single_serializer(msg, prefix, inline))
            lines.append('')
        
        return '\n'.join(lines)
    
    def _generate_single_serializer(self, message: pb2.DescriptorProto, ns_prefix: str, inline: bool) -> str:
        """Generate a single serializer specialization without recursion."""
        lines = []
        msg_type = f'{ns_prefix}::{message.name}' if ns_prefix else message.name
        inline_str = 'inline ' if inline else ''
        
        lines.append(f'template<>')
        lines.append(f'class Serializer<{msg_type}> {{')
        lines.append(f'public:')
        
        # Serialize method
        lines.append(f'    {inline_str}static bool serialize(const {msg_type}& value, litepb::OutputStream& stream) {{')
        lines.append('        litepb::ProtoWriter writer(stream);')
        
        # Get non-oneof, non-map fields
        regular_fields = FieldUtils.get_non_oneof_fields(message)
        
        # Generate write code for each regular field
        for field in regular_fields:
            lines.append(self.generate_field_write(field, message))
        
        # Generate write code for maps
        maps = FieldUtils.extract_maps_from_message(message)
        for map_field in maps:
            lines.append(self.generate_map_write(map_field, message))
        
        # Generate write code for oneofs
        oneofs = FieldUtils.extract_oneofs_from_message(message)
        for oneof in oneofs:
            lines.append(self.generate_oneof_write(oneof, message))
        
        # Write unknown fields at the end
        lines.append('        // Serialize unknown fields for forward/backward compatibility')
        lines.append('        if (!value.unknown_fields.empty()) {')
        lines.append('            if (!value.unknown_fields.serialize_to(stream)) return false;')
        lines.append('        }')
        
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
            for field in oneof.fields:
                lines.append(self.generate_oneof_field_read(field, oneof, message))
        
        lines.append('                default: {')
        lines.append('                    // Capture unknown field for forward/backward compatibility')
        lines.append('                    std::vector<uint8_t> unknown_data;')
        lines.append('                    if (!reader.capture_unknown_field(wire_type, unknown_data)) return false;')
        lines.append('                    ')
        lines.append('                    // Store in UnknownFieldSet based on wire type')
        lines.append('                    switch (wire_type) {')
        lines.append('                        case litepb::WIRE_TYPE_VARINT: {')
        lines.append('                            // Decode varint from captured data')
        lines.append('                            uint64_t varint_value = 0;')
        lines.append('                            size_t shift = 0;')
        lines.append('                            for (size_t i = 0; i < unknown_data.size() && i < 10; ++i) {')
        lines.append('                                varint_value |= static_cast<uint64_t>(unknown_data[i] & 0x7F) << shift;')
        lines.append('                                if ((unknown_data[i] & 0x80) == 0) break;')
        lines.append('                                shift += 7;')
        lines.append('                            }')
        lines.append('                            value.unknown_fields.add_varint(field_number, varint_value);')
        lines.append('                            break;')
        lines.append('                        }')
        lines.append('                        case litepb::WIRE_TYPE_FIXED32: {')
        lines.append('                            if (unknown_data.size() >= 4) {')
        lines.append('                                uint32_t fixed32_value = ')
        lines.append('                                    static_cast<uint32_t>(unknown_data[0]) |')
        lines.append('                                    (static_cast<uint32_t>(unknown_data[1]) << 8) |')
        lines.append('                                    (static_cast<uint32_t>(unknown_data[2]) << 16) |')
        lines.append('                                    (static_cast<uint32_t>(unknown_data[3]) << 24);')
        lines.append('                                value.unknown_fields.add_fixed32(field_number, fixed32_value);')
        lines.append('                            }')
        lines.append('                            break;')
        lines.append('                        }')
        lines.append('                        case litepb::WIRE_TYPE_FIXED64: {')
        lines.append('                            if (unknown_data.size() >= 8) {')
        lines.append('                                uint64_t fixed64_value = 0;')
        lines.append('                                for (int i = 0; i < 8; ++i) {')
        lines.append('                                    fixed64_value |= static_cast<uint64_t>(unknown_data[i]) << (i * 8);')
        lines.append('                                }')
        lines.append('                                value.unknown_fields.add_fixed64(field_number, fixed64_value);')
        lines.append('                            }')
        lines.append('                            break;')
        lines.append('                        }')
        lines.append('                        case litepb::WIRE_TYPE_LENGTH_DELIMITED: {')
        lines.append('                            // Extract actual data (skip length prefix)')
        lines.append('                            size_t pos = 0;')
        lines.append('                            uint64_t len = 0;')
        lines.append('                            int shift = 0;')
        lines.append('                            while (pos < unknown_data.size() && pos < 10) {')
        lines.append('                                len |= static_cast<uint64_t>(unknown_data[pos] & 0x7F) << shift;')
        lines.append('                                if ((unknown_data[pos] & 0x80) == 0) {')
        lines.append('                                    pos++;')
        lines.append('                                    break;')
        lines.append('                                }')
        lines.append('                                shift += 7;')
        lines.append('                                pos++;')
        lines.append('                            }')
        lines.append('                            if (pos < unknown_data.size()) {')
        lines.append('                                value.unknown_fields.add_length_delimited(field_number, ')
        lines.append('                                    unknown_data.data() + pos, unknown_data.size() - pos);')
        lines.append('                            }')
        lines.append('                            break;')
        lines.append('                        }')
        lines.append('                        case litepb::WIRE_TYPE_START_GROUP: {')
        lines.append('                            value.unknown_fields.add_group(field_number, ')
        lines.append('                                unknown_data.data(), unknown_data.size());')
        lines.append('                            break;')
        lines.append('                        }')
        lines.append('                        default:')
        lines.append('                            break;')
        lines.append('                    }')
        lines.append('                    break;')
        lines.append('                }')
        lines.append('            }')
        lines.append('        }')
        lines.append('        return true;')
        lines.append('    }')
        
        lines.append('};')
        
        return '\n'.join(lines)
    
    def generate_field_write(self, field: pb2.FieldDescriptorProto, message: pb2.DescriptorProto) -> str:
        """Generate write code for a field."""
        field_num = field.number
        field_name = field.name
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'

        # Check if field uses std::optional wrapper
        use_optional_field = FieldUtils.uses_optional(field, syntax)

        if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
            # Repeated field - check if it should be packed
            lines = []
            
            if FieldUtils.is_field_packed(field, syntax):
                # Packed encoding
                lines.append(f'        if (!value.{field_name}.empty()) {{')
                lines.append(f'            // Calculate packed size')
                lines.append(f'            size_t packed_size = 0;')
                lines.append(f'            for (const auto& item : value.{field_name}) {{')
                
                # Add size calculation based on field type
                size_expr = TypeMapper.get_packed_size_expression(field.type, 'item')
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
                write_code = self._generate_packed_write_value(field.type, 'item')
                lines.append(f'                {write_code}')
                lines.append(f'            }}')
                lines.append(f'        }}')
            else:
                # Unpacked encoding
                lines.append(f'        for (const auto& item : value.{field_name}) {{')
                wire_type = TypeMapper.get_wire_type(field.type)
                lines.append(f'            writer.write_tag({field_num}, {wire_type});')
                
                if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
                    # For messages, we need to write the length first
                    lines.append(f'            {{')
                    lines.append(f'                litepb::BufferOutputStream temp_stream;')
                    lines.append(f'                if (!litepb::Serializer<std::decay_t<decltype(item)>>::serialize(item, temp_stream)) return false;')
                    lines.append(f'                writer.write_varint(temp_stream.size());')
                    lines.append(f'                stream.write(temp_stream.data(), temp_stream.size());')
                    lines.append(f'            }}')
                elif field.type == pb2.FieldDescriptorProto.TYPE_GROUP:
                    # GROUP is deprecated and not length-delimited
                    lines.append(f'            if (!litepb::Serializer<std::decay_t<decltype(item)>>::serialize(item, stream)) return false;')
                elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                    lines.append(f'            writer.write_varint(static_cast<uint64_t>(item));')
                elif field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
                    method = TypeMapper.get_serialization_method(field.type)
                    lines.append(f'            writer.{method}(item.data(), item.size());')
                else:
                    method = TypeMapper.get_serialization_method(field.type)
                    lines.append(f'            writer.{method}(item);')
                lines.append(f'        }}')
            
            return '\n'.join(lines)
        
        # Optional or singular field
        lines = []
        
        if use_optional_field:
            # Field with std::optional wrapper
            lines.append(f'        if (value.{field_name}.has_value()) {{')
            wire_type = TypeMapper.get_wire_type(field.type)
            lines.append(f'            writer.write_tag({field_num}, {wire_type});')
            
            if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
                # For messages, we need to write the length first
                lines.append(f'            {{')
                lines.append(f'                litepb::BufferOutputStream temp_stream;')
                lines.append(f'                if (!litepb::Serializer<std::decay_t<decltype(value.{field_name}.value())>>::serialize(value.{field_name}.value(), temp_stream)) return false;')
                lines.append(f'                writer.write_varint(temp_stream.size());')
                lines.append(f'                stream.write(temp_stream.data(), temp_stream.size());')
                lines.append(f'            }}')
            elif field.type == pb2.FieldDescriptorProto.TYPE_GROUP:
                # GROUP is deprecated and not length-delimited
                lines.append(f'            if (!litepb::Serializer<std::decay_t<decltype(value.{field_name}.value())>>::serialize(value.{field_name}.value(), stream)) return false;')
            elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append(f'            writer.write_varint(static_cast<uint64_t>(value.{field_name}.value()));')
            elif field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'            writer.{method}(value.{field_name}.value().data(), value.{field_name}.value().size());')
            else:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'            writer.{method}(value.{field_name}.value());')
            lines.append(f'        }}')
        else:
            # Proto3 singular field or proto2 required field
            # Skip default values in proto3
            if syntax == 'proto3':
                default_check = TypeMapper.get_default_check(field)
                if default_check:
                    lines.append(f'        if ({default_check}) {{')
                else:
                    lines.append(f'        {{')
            else:
                lines.append(f'        {{')
            
            wire_type = TypeMapper.get_wire_type(field.type)
            lines.append(f'            writer.write_tag({field_num}, {wire_type});')
            
            if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
                # For messages, we need to write the length first
                lines.append(f'            {{')
                lines.append(f'                litepb::BufferOutputStream temp_stream;')
                lines.append(f'                if (!litepb::Serializer<std::decay_t<decltype(value.{field_name})>>::serialize(value.{field_name}, temp_stream)) return false;')
                lines.append(f'                writer.write_varint(temp_stream.size());')
                lines.append(f'                stream.write(temp_stream.data(), temp_stream.size());')
                lines.append(f'            }}')
            elif field.type == pb2.FieldDescriptorProto.TYPE_GROUP:
                # GROUP is deprecated and not length-delimited
                lines.append(f'            if (!litepb::Serializer<std::decay_t<decltype(value.{field_name})>>::serialize(value.{field_name}, stream)) return false;')
            elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append(f'            writer.write_varint(static_cast<uint64_t>(value.{field_name}));')
            elif field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'            writer.{method}(value.{field_name}.data(), value.{field_name}.size());')
            else:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'            writer.{method}(value.{field_name});')
            lines.append(f'        }}')
        
        return '\n'.join(lines)

    def _generate_packed_write_value(self, field_type: int, item_name: str) -> str:
        """Generate code to write a single value in packed format."""
        if field_type == pb2.FieldDescriptorProto.TYPE_ENUM:
            return f'writer.write_varint(static_cast<uint64_t>({item_name}));'
        elif field_type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
            unsigned_type = 'uint32_t' if field_type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
            method = 'write_fixed32' if field_type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'write_fixed64'
            return f'{unsigned_type} temp; std::memcpy(&temp, &{item_name}, sizeof(temp)); writer.{method}(temp);'
        else:
            method = TypeMapper.get_serialization_method(field_type)
            if method == 'write_varint':
                return f'writer.{method}(static_cast<uint64_t>({item_name}));'
            else:
                return f'writer.{method}({item_name});'
    
    def generate_map_write(self, map_field: MapFieldInfo, message: pb2.DescriptorProto) -> str:
        """Generate write code for a map field."""
        lines = []
        lines.append(f'        for (const auto& [key, val] : value.{map_field.name}) {{')
        lines.append(f'            writer.write_tag({map_field.number}, litepb::WIRE_TYPE_LENGTH_DELIMITED);')
        lines.append(f'            ')
        lines.append(f'            // Calculate entry size')
        lines.append(f'            size_t entry_size = 0;')
        
        # Key size
        key_wire = TypeMapper.get_wire_type(map_field.key_field.type)
        key_method = TypeMapper.get_serialization_method(map_field.key_field.type)
        lines.append(f'            entry_size += litepb::ProtoWriter::varint_size((1 << 3) | {key_wire});')
        if key_method == 'write_string':
            lines.append(f'            entry_size += litepb::ProtoWriter::varint_size(key.size()) + key.size();')
        elif key_method == 'write_varint':
            lines.append(f'            entry_size += litepb::ProtoWriter::varint_size(static_cast<uint64_t>(key));')
        elif key_method in ('write_fixed32', 'write_sfixed32'):
            lines.append(f'            entry_size += 4;')
        elif key_method in ('write_fixed64', 'write_sfixed64'):
            lines.append(f'            entry_size += 8;')
        
        # Value size
        val_wire = TypeMapper.get_wire_type(map_field.value_field.type)
        val_method = TypeMapper.get_serialization_method(map_field.value_field.type)
        lines.append(f'            entry_size += litepb::ProtoWriter::varint_size((2 << 3) | {val_wire});')
        if map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
            lines.append(f'            // Message value size calculated during write')
            lines.append(f'            litepb::BufferOutputStream msg_stream;')
            lines.append(f'            litepb::Serializer<std::decay_t<decltype(val)>>::serialize(val, msg_stream);')
            lines.append(f'            entry_size += litepb::ProtoWriter::varint_size(msg_stream.size()) + msg_stream.size();')
        elif val_method == 'write_string':
            lines.append(f'            entry_size += litepb::ProtoWriter::varint_size(val.size()) + val.size();')
        elif val_method == 'write_varint':
            lines.append(f'            entry_size += litepb::ProtoWriter::varint_size(static_cast<uint64_t>(val));')
        elif val_method in ('write_fixed32', 'write_sfixed32'):
            lines.append(f'            entry_size += 4;')
        elif val_method in ('write_fixed64', 'write_sfixed64'):
            lines.append(f'            entry_size += 8;')
        
        lines.append(f'            ')
        lines.append(f'            writer.write_varint(entry_size);')
        lines.append(f'            ')
        lines.append(f'            // Write key')
        lines.append(f'            writer.write_tag(1, {key_wire});')
        lines.append(f'            writer.{key_method}({"static_cast<uint64_t>(key)" if key_method == "write_varint" else "key"});')
        lines.append(f'            ')
        lines.append(f'            // Write value')
        lines.append(f'            writer.write_tag(2, {val_wire});')
        if map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
            lines.append(f'            writer.write_varint(msg_stream.size());')
            lines.append(f'            stream.write(msg_stream.data(), msg_stream.size());')
        elif map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
            lines.append(f'            writer.write_varint(static_cast<uint64_t>(val));')
        elif val_method == 'write_bytes':
            lines.append(f'            writer.{val_method}(val.data(), val.size());')
        elif val_method == 'write_varint':
            lines.append(f'            writer.{val_method}(static_cast<uint64_t>(val));')
        else:
            lines.append(f'            writer.{val_method}(val);')
        lines.append(f'        }}')
        
        return '\n'.join(lines)
    
    def generate_oneof_write(self, oneof: OneofInfo, message: pb2.DescriptorProto) -> str:
        """Generate write code for a oneof field."""
        lines = []
        lines.append(f'        std::visit([&](const auto& oneof_val) {{')
        lines.append(f'            using T = std::decay_t<decltype(oneof_val)>;')
        lines.append(f'            if constexpr (!std::is_same_v<T, std::monostate>) {{')
        
        for i, field in enumerate(oneof.fields):
            wire_type = TypeMapper.get_wire_type(field.type)
            field_num = field.number
            
            if i == 0:
                lines.append(f'                if constexpr (std::is_same_v<T, {self._get_oneof_field_cpp_type(field)}>) {{')
            else:
                lines.append(f'                }} else if constexpr (std::is_same_v<T, {self._get_oneof_field_cpp_type(field)}>) {{')
            
            lines.append(f'                    writer.write_tag({field_num}, {wire_type});')
            
            if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_GROUP):
                lines.append(f'                    litepb::Serializer<T>::serialize(oneof_val, stream);')
            elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append(f'                    writer.write_varint(static_cast<uint64_t>(oneof_val));')
            elif field.type == pb2.FieldDescriptorProto.TYPE_BYTES:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'                    writer.{method}(oneof_val.data(), oneof_val.size());')
            elif field.type == pb2.FieldDescriptorProto.TYPE_STRING:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'                    writer.{method}(oneof_val);')
            else:
                method = TypeMapper.get_serialization_method(field.type)
                lines.append(f'                    writer.{method}(oneof_val);')
        
        if oneof.fields:
            lines.append(f'                }}')
        
        lines.append(f'            }}')
        lines.append(f'        }}, value.{oneof.name});')
        
        return '\n'.join(lines)
    
    def _get_oneof_field_cpp_type(self, field: pb2.FieldDescriptorProto) -> str:
        """Get C++ type for a oneof field alternative."""
        if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
            type_name = field.type_name
            return TypeMapper.qualify_type_name(type_name, "")
        else:
            return TypeMapper.get_cpp_type(field.type)
    
    def generate_field_read(self, field: pb2.FieldDescriptorProto, message: pb2.DescriptorProto) -> str:
        """Generate read case for a field."""
        field_num = field.number
        field_name = field.name
        syntax = self.current_proto.syntax if self.current_proto.syntax else 'proto2'
        use_optional = FieldUtils.uses_optional(field, syntax)
        
        lines = []
        lines.append(f'                case {field_num}: {{')
        
        if field.label == pb2.FieldDescriptorProto.LABEL_REPEATED:
            # Check if packed
            if FieldUtils.is_field_packed(field, syntax):
                lines.append(f'                    if (wire_type == litepb::WIRE_TYPE_LENGTH_DELIMITED) {{')
                lines.append(f'                        // Packed repeated field')
                lines.append(f'                        uint64_t length;')
                lines.append(f'                        if (!reader.read_varint(length)) return false;')
                lines.append(f'                        size_t end_pos = reader.position() + length;')
                lines.append(f'                        while (reader.position() < end_pos) {{')
                self._generate_packed_read_code(lines, field.type, field_name)
                lines.append(f'                        }}')
                lines.append(f'                    }} else {{')
                lines.append(f'                        // Unpacked (for backward compat)')
                self._generate_unpacked_read_code(lines, field.type, field_name)
                lines.append(f'                    }}')
            else:
                # Always unpacked
                self._generate_unpacked_read_code(lines, field.type, field_name)
        elif use_optional:
            # Optional field
            if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_GROUP):
                lines.append(f'                    // Read length-delimited message')
                lines.append(f'                    uint64_t msg_length;')
                lines.append(f'                    if (!reader.read_varint(msg_length)) return false;')
                lines.append(f'                    ')
                lines.append(f'                    // Read message bytes into buffer')
                lines.append(f'                    std::vector<uint8_t> msg_buffer(msg_length);')
                lines.append(f'                    if (!stream.read(msg_buffer.data(), msg_length)) return false;')
                lines.append(f'                    ')
                lines.append(f'                    // Create a stream from the buffer and parse')
                lines.append(f'                    decltype(value.{field_name})::value_type temp;')
                lines.append(f'                    litepb::BufferInputStream msg_stream(msg_buffer.data(), msg_buffer.size());')
                lines.append(f'                    if (!litepb::Serializer<std::decay_t<decltype(temp)>>::parse(temp, msg_stream)) return false;')
                lines.append(f'                    value.{field_name} = std::move(temp);')
            elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append(f'                    uint64_t enum_val;')
                lines.append(f'                    if (!reader.read_varint(enum_val)) return false;')
                lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})::value_type>(enum_val);')
            else:
                self._generate_simple_read_to_optional(lines, field.type, field_name)
        else:
            # Required or singular
            if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_GROUP):
                lines.append(f'                    // Read length-delimited message')
                lines.append(f'                    uint64_t msg_length;')
                lines.append(f'                    if (!reader.read_varint(msg_length)) return false;')
                lines.append(f'                    ')
                lines.append(f'                    // Read message bytes into buffer')
                lines.append(f'                    std::vector<uint8_t> msg_buffer(msg_length);')
                lines.append(f'                    if (!stream.read(msg_buffer.data(), msg_length)) return false;')
                lines.append(f'                    ')
                lines.append(f'                    // Create a stream from the buffer and parse')
                lines.append(f'                    litepb::BufferInputStream msg_stream(msg_buffer.data(), msg_buffer.size());')
                lines.append(f'                    if (!litepb::Serializer<std::decay_t<decltype(value.{field_name})>>::parse(value.{field_name}, msg_stream)) return false;')
            elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
                lines.append(f'                    uint64_t enum_val;')
                lines.append(f'                    if (!reader.read_varint(enum_val)) return false;')
                lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})>(enum_val);')
            else:
                method = TypeMapper.get_deserialization_method(field.type)
                if field.type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
                    unsigned_type = 'uint32_t' if field.type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
                    lines.append(f'                    {unsigned_type} temp_unsigned;')
                    lines.append(f'                    if (!reader.{method}(temp_unsigned)) return false;')
                    lines.append(f'                    std::memcpy(&value.{field_name}, &temp_unsigned, sizeof(value.{field_name}));')
                elif method == 'read_varint':
                    lines.append(f'                    uint64_t temp_varint;')
                    lines.append(f'                    if (!reader.{method}(temp_varint)) return false;')
                    if field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
                        lines.append(f'                    value.{field_name} = (temp_varint != 0);')
                    else:
                        lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})>(temp_varint);')
                else:
                    lines.append(f'                    if (!reader.{method}(value.{field_name})) return false;')
        
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
    
    def _generate_packed_read_code(self, lines: List[str], field_type: int, field_name: str) -> None:
        """Generate code to read a single value in packed format (inside a loop)."""
        cpp_type = TypeMapper.get_cpp_type(field_type)
        method = TypeMapper.get_deserialization_method(field_type)
        
        if field_type == pb2.FieldDescriptorProto.TYPE_ENUM:
            lines.append(f'                            uint64_t enum_val;')
            lines.append(f'                            if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                            value.{field_name}.push_back(static_cast<decltype(value.{field_name})::value_type>(enum_val));')
        elif method == 'read_varint':
            lines.append(f'                            uint64_t temp_varint;')
            lines.append(f'                            if (!reader.{method}(temp_varint)) return false;')
            if field_type == pb2.FieldDescriptorProto.TYPE_BOOL:
                lines.append(f'                            value.{field_name}.push_back(temp_varint != 0);')
            else:
                lines.append(f'                            value.{field_name}.push_back(static_cast<{cpp_type}>(temp_varint));')
        elif method in ('read_sint32', 'read_sint64'):
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            if (!reader.{method}(temp)) return false;')
            lines.append(f'                            value.{field_name}.push_back(temp);')
        elif field_type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
            unsigned_type = 'uint32_t' if field_type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
            lines.append(f'                            {unsigned_type} temp_unsigned;')
            lines.append(f'                            if (!reader.{method}(temp_unsigned)) return false;')
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
            lines.append(f'                            value.{field_name}.push_back(temp);')
        else:
            lines.append(f'                            {cpp_type} temp;')
            lines.append(f'                            if (!reader.{method}(temp)) return false;')
            lines.append(f'                            value.{field_name}.push_back(temp);')
    
    def _generate_unpacked_read_code(self, lines: List[str], field_type: int, field_name: str) -> None:
        """Generate code to read a single value in unpacked format."""
        cpp_type = TypeMapper.get_cpp_type(field_type)
        method = TypeMapper.get_deserialization_method(field_type)
        
        if field_type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_GROUP):
            lines.append(f'                    // Read length-delimited message')
            lines.append(f'                    uint64_t msg_length;')
            lines.append(f'                    if (!reader.read_varint(msg_length)) return false;')
            lines.append(f'                    ')
            lines.append(f'                    // Read message bytes into buffer')
            lines.append(f'                    std::vector<uint8_t> msg_buffer(msg_length);')
            lines.append(f'                    if (!stream.read(msg_buffer.data(), msg_length)) return false;')
            lines.append(f'                    ')
            lines.append(f'                    // Create a stream from the buffer and parse')
            lines.append(f'                    decltype(value.{field_name})::value_type temp;')
            lines.append(f'                    litepb::BufferInputStream msg_stream(msg_buffer.data(), msg_buffer.size());')
            lines.append(f'                    if (!litepb::Serializer<std::decay_t<decltype(temp)>>::parse(temp, msg_stream)) return false;')
            lines.append(f'                    value.{field_name}.push_back(std::move(temp));')
        elif field_type == pb2.FieldDescriptorProto.TYPE_ENUM:
            lines.append(f'                        uint64_t enum_val;')
            lines.append(f'                        if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                        value.{field_name}.push_back(static_cast<decltype(value.{field_name})::value_type>(enum_val));')
        elif method == 'read_varint':
            lines.append(f'                        uint64_t temp_varint;')
            lines.append(f'                        if (!reader.{method}(temp_varint)) return false;')
            if field_type == pb2.FieldDescriptorProto.TYPE_BOOL:
                lines.append(f'                        value.{field_name}.push_back(temp_varint != 0);')
            else:
                lines.append(f'                        value.{field_name}.push_back(static_cast<{cpp_type}>(temp_varint));')
        elif method in ('read_sint32', 'read_sint64'):
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        if (!reader.{method}(temp)) return false;')
            lines.append(f'                        value.{field_name}.push_back(temp);')
        elif field_type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
            unsigned_type = 'uint32_t' if field_type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
            lines.append(f'                        {unsigned_type} temp_unsigned;')
            lines.append(f'                        if (!reader.{method}(temp_unsigned)) return false;')
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
            lines.append(f'                        value.{field_name}.push_back(temp);')
        else:
            lines.append(f'                        {cpp_type} temp;')
            lines.append(f'                        if (!reader.{method}(temp)) return false;')
            lines.append(f'                        value.{field_name}.push_back(temp);')
    
    def _generate_simple_read_to_optional(self, lines: List[str], field_type: int, field_name: str) -> None:
        """Generate read code for simple types into optional."""
        method = TypeMapper.get_deserialization_method(field_type)
        
        if field_type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
            unsigned_type = 'uint32_t' if field_type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
            lines.append(f'                    {unsigned_type} temp_unsigned;')
            lines.append(f'                    if (!reader.{method}(temp_unsigned)) return false;')
            lines.append(f'                    decltype(value.{field_name})::value_type temp;')
            lines.append(f'                    std::memcpy(&temp, &temp_unsigned, sizeof(temp));')
            lines.append(f'                    value.{field_name} = temp;')
        elif method == 'read_varint':
            lines.append(f'                    uint64_t temp_varint;')
            lines.append(f'                    if (!reader.{method}(temp_varint)) return false;')
            if field_type == pb2.FieldDescriptorProto.TYPE_BOOL:
                lines.append(f'                    value.{field_name} = (temp_varint != 0);')
            else:
                lines.append(f'                    value.{field_name} = static_cast<decltype(value.{field_name})::value_type>(temp_varint);')
        else:
            lines.append(f'                    decltype(value.{field_name})::value_type temp;')
            lines.append(f'                    if (!reader.{method}(temp)) return false;')
            lines.append(f'                    value.{field_name} = temp;')
    
    def generate_map_read(self, map_field: MapFieldInfo, message: pb2.DescriptorProto) -> str:
        """Generate read case for a map field."""
        lines = []
        lines.append(f'                case {map_field.number}: {{')
        lines.append(f'                    // Read map entry')
        lines.append(f'                    uint64_t entry_length;')
        lines.append(f'                    if (!reader.read_varint(entry_length)) return false;')
        lines.append(f'                    ')
        
        # Declare key and value variables
        key_cpp_type = TypeMapper.get_cpp_type(map_field.key_field.type)
        if map_field.value_field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_ENUM):
            val_cpp_type = TypeMapper.qualify_type_name(map_field.value_field.type_name, "")
        else:
            val_cpp_type = TypeMapper.get_cpp_type(map_field.value_field.type)
        
        lines.append(f'                    {key_cpp_type} entry_key{{}};')
        lines.append(f'                    {val_cpp_type} entry_val{{}};')
        lines.append(f'                    ')
        lines.append(f'                    size_t entry_end = reader.position() + entry_length;')
        lines.append(f'                    while (reader.position() < entry_end) {{')
        lines.append(f'                        uint32_t entry_field;')
        lines.append(f'                        litepb::WireType entry_wire;')
        lines.append(f'                        if (!reader.read_tag(entry_field, entry_wire)) return false;')
        lines.append(f'                        ')
        lines.append(f'                        if (entry_field == 1) {{  // key')
        
        # Read key
        key_method = TypeMapper.get_deserialization_method(map_field.key_field.type)
        if map_field.key_field.type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
            unsigned_type = 'uint32_t' if map_field.key_field.type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
            lines.append(f'                            {unsigned_type} temp_unsigned;')
            lines.append(f'                            if (!reader.{key_method}(temp_unsigned)) return false;')
            lines.append(f'                            std::memcpy(&entry_key, &temp_unsigned, sizeof(entry_key));')
        elif key_method == 'read_varint':
            lines.append(f'                            uint64_t temp;')
            lines.append(f'                            if (!reader.{key_method}(temp)) return false;')
            if map_field.key_field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
                lines.append(f'                            entry_key = (temp != 0);')
            else:
                lines.append(f'                            entry_key = static_cast<{key_cpp_type}>(temp);')
        else:
            lines.append(f'                            if (!reader.{key_method}(entry_key)) return false;')
        
        lines.append(f'                        }} else if (entry_field == 2) {{  // value')
        
        # Read value
        if map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
            lines.append(f'                            // Read length-delimited message')
            lines.append(f'                            uint64_t msg_length;')
            lines.append(f'                            if (!reader.read_varint(msg_length)) return false;')
            lines.append(f'                            ')
            lines.append(f'                            // Read message bytes into buffer')
            lines.append(f'                            std::vector<uint8_t> msg_buffer(msg_length);')
            lines.append(f'                            if (!stream.read(msg_buffer.data(), msg_length)) return false;')
            lines.append(f'                            ')
            lines.append(f'                            // Create a stream from the buffer and parse')
            lines.append(f'                            litepb::BufferInputStream msg_stream(msg_buffer.data(), msg_buffer.size());')
            lines.append(f'                            if (!litepb::Serializer<{val_cpp_type}>::parse(entry_val, msg_stream)) return false;')
        elif map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
            lines.append(f'                            uint64_t temp;')
            lines.append(f'                            if (!reader.read_varint(temp)) return false;')
            lines.append(f'                            entry_val = static_cast<{val_cpp_type}>(temp);')
        else:
            val_method = TypeMapper.get_deserialization_method(map_field.value_field.type)
            if map_field.value_field.type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
                unsigned_type = 'uint32_t' if map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
                lines.append(f'                            {unsigned_type} temp_unsigned;')
                lines.append(f'                            if (!reader.{val_method}(temp_unsigned)) return false;')
                lines.append(f'                            std::memcpy(&entry_val, &temp_unsigned, sizeof(entry_val));')
            elif val_method == 'read_varint':
                lines.append(f'                            uint64_t temp;')
                lines.append(f'                            if (!reader.{val_method}(temp)) return false;')
                if map_field.value_field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
                    lines.append(f'                            entry_val = (temp != 0);')
                else:
                    lines.append(f'                            entry_val = static_cast<{val_cpp_type}>(temp);')
            else:
                lines.append(f'                            if (!reader.{val_method}(entry_val)) return false;')
        
        lines.append(f'                        }} else {{')
        lines.append(f'                            if (!reader.skip_field(entry_wire)) return false;')
        lines.append(f'                        }}')
        lines.append(f'                    }}')
        lines.append(f'                    ')
        lines.append(f'                    value.{map_field.name}[entry_key] = entry_val;')
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
    
    def generate_oneof_field_read(self, field: pb2.FieldDescriptorProto, oneof: OneofInfo, message: pb2.DescriptorProto) -> str:
        """Generate read case for a field within a oneof."""
        oneof_name = oneof.name
        field_num = field.number
        
        lines = []
        lines.append(f'                case {field_num}: {{')
        
        cpp_type = self._get_oneof_field_cpp_type(field)
        
        if field.type in (pb2.FieldDescriptorProto.TYPE_MESSAGE, pb2.FieldDescriptorProto.TYPE_GROUP):
            lines.append(f'                    {cpp_type} oneof_val;')
            lines.append(f'                    if (!litepb::Serializer<{cpp_type}>::parse(oneof_val, stream)) return false;')
            lines.append(f'                    value.{oneof_name} = std::move(oneof_val);')
        elif field.type == pb2.FieldDescriptorProto.TYPE_ENUM:
            lines.append(f'                    uint64_t enum_val;')
            lines.append(f'                    if (!reader.read_varint(enum_val)) return false;')
            lines.append(f'                    value.{oneof_name} = static_cast<{cpp_type}>(enum_val);')
        else:
            method = TypeMapper.get_deserialization_method(field.type)
            if field.type in (pb2.FieldDescriptorProto.TYPE_SFIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED64):
                unsigned_type = 'uint32_t' if field.type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'uint64_t'
                read_method = 'read_fixed32' if field.type == pb2.FieldDescriptorProto.TYPE_SFIXED32 else 'read_fixed64'
                lines.append(f'                    {unsigned_type} temp_unsigned;')
                lines.append(f'                    if (!reader.{read_method}(temp_unsigned)) return false;')
                lines.append(f'                    {cpp_type} oneof_val;')
                lines.append(f'                    std::memcpy(&oneof_val, &temp_unsigned, sizeof(oneof_val));')
                lines.append(f'                    value.{oneof_name} = oneof_val;')
            elif method == 'read_varint':
                lines.append(f'                    uint64_t temp_varint;')
                lines.append(f'                    if (!reader.{method}(temp_varint)) return false;')
                if field.type == pb2.FieldDescriptorProto.TYPE_BOOL:
                    lines.append(f'                    value.{oneof_name} = (temp_varint != 0);')
                else:
                    lines.append(f'                    value.{oneof_name} = static_cast<{cpp_type}>(temp_varint);')
            else:
                lines.append(f'                    {cpp_type} oneof_val;')
                lines.append(f'                    if (!reader.{method}(oneof_val)) return false;')
                lines.append(f'                    value.{oneof_name} = oneof_val;')
        
        lines.append('                    break;')
        lines.append('                }')
        
        return '\n'.join(lines)
