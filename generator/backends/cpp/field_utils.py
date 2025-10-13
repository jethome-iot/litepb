#!/usr/bin/env python3
"""
Field inspection utilities for protobuf descriptors.
"""

from typing import List
from google.protobuf import descriptor_pb2 as pb2
from generator.backends.cpp.models import MapFieldInfo, OneofInfo


class FieldUtils:
    """Utilities for inspecting and working with protobuf field descriptors."""
    
    @staticmethod
    def is_message_type(field: pb2.FieldDescriptorProto) -> bool:
        """Check if field is a message type."""
        return field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE
    
    @staticmethod
    def is_enum_type(field: pb2.FieldDescriptorProto) -> bool:
        """Check if field is an enum type."""
        return field.type == pb2.FieldDescriptorProto.TYPE_ENUM
    
    @staticmethod
    def is_repeated(field: pb2.FieldDescriptorProto) -> bool:
        """Check if field is repeated."""
        return field.label == pb2.FieldDescriptorProto.LABEL_REPEATED
    
    @staticmethod
    def is_map_field(message_proto: pb2.DescriptorProto, 
                     field_proto: pb2.FieldDescriptorProto) -> bool:
        """Check if field is a map field (TYPE_MESSAGE with map_entry option)."""
        if field_proto.type != pb2.FieldDescriptorProto.TYPE_MESSAGE:
            return False
        
        # Find the nested type for this field
        type_name = field_proto.type_name.split('.')[-1]
        for nested_type in message_proto.nested_type:
            if nested_type.name == type_name:
                return nested_type.options.map_entry if nested_type.HasField('options') else False
        return False
    
    @staticmethod
    def uses_optional(field: pb2.FieldDescriptorProto, syntax: str) -> bool:
        """Determine if a field should use std::optional."""
        if syntax == 'proto2':
            # Proto2: REQUIRED and OPTIONAL fields use std::optional
            return field.label in (pb2.FieldDescriptorProto.LABEL_REQUIRED,
                                   pb2.FieldDescriptorProto.LABEL_OPTIONAL)
        else:  # proto3
            # Proto3: Only explicitly optional fields use std::optional
            return field.proto3_optional if hasattr(field, 'proto3_optional') else False
    
    @staticmethod
    def extract_maps_from_message(message: pb2.DescriptorProto) -> List[MapFieldInfo]:
        """Extract map field information from a message descriptor."""
        maps = []
        
        # Find map entry nested types
        map_entries = {}
        for nested_type in message.nested_type:
            if nested_type.HasField('options') and nested_type.options.map_entry:
                map_entries[nested_type.name] = nested_type
        
        # Find fields that use these map entries
        for field in message.field:
            if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
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
                        maps.append(MapFieldInfo(
                            name=field.name,
                            number=field.number,
                            key_field=key_field,
                            value_field=value_field
                        ))
        
        return maps
    
    @staticmethod
    def extract_oneofs_from_message(message: pb2.DescriptorProto) -> List[OneofInfo]:
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
                oneofs.append(OneofInfo(
                    name=oneof_decl.name,
                    fields=oneof_fields_map[idx]
                ))
        
        return oneofs
    
    @staticmethod
    def get_non_oneof_fields(message: pb2.DescriptorProto) -> List[pb2.FieldDescriptorProto]:
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
            if field.type == pb2.FieldDescriptorProto.TYPE_MESSAGE:
                type_name = field.type_name.split('.')[-1]
                if type_name in map_entry_names:
                    continue
            non_oneof_fields.append(field)
        
        return non_oneof_fields
    
    @staticmethod
    def is_field_packed(field: pb2.FieldDescriptorProto, syntax: str) -> bool:
        """Determine if a repeated field should be packed."""
        # Only repeated fields can be packed
        if field.label != pb2.FieldDescriptorProto.LABEL_REPEATED:
            return False
        
        # Strings, bytes, messages are NEVER packed
        if field.type in (pb2.FieldDescriptorProto.TYPE_STRING, 
                         pb2.FieldDescriptorProto.TYPE_BYTES, 
                         pb2.FieldDescriptorProto.TYPE_MESSAGE, 
                         pb2.FieldDescriptorProto.TYPE_GROUP):
            return False
        
        # Check if field has explicit packed option
        if field.HasField('options') and field.options.HasField('packed'):
            return field.options.packed
        
        # Proto3: All numeric repeated fields are packed by default
        # Proto2: Only packed if field has [packed = true] option (which we checked above)
        if syntax == 'proto3':
            # In proto3, numeric/enum repeated fields are packed by default
            return field.type in (
                pb2.FieldDescriptorProto.TYPE_INT32, pb2.FieldDescriptorProto.TYPE_INT64,
                pb2.FieldDescriptorProto.TYPE_UINT32, pb2.FieldDescriptorProto.TYPE_UINT64,
                pb2.FieldDescriptorProto.TYPE_SINT32, pb2.FieldDescriptorProto.TYPE_SINT64,
                pb2.FieldDescriptorProto.TYPE_BOOL, pb2.FieldDescriptorProto.TYPE_ENUM,
                pb2.FieldDescriptorProto.TYPE_FIXED32, pb2.FieldDescriptorProto.TYPE_SFIXED32,
                pb2.FieldDescriptorProto.TYPE_FIXED64, pb2.FieldDescriptorProto.TYPE_SFIXED64,
                pb2.FieldDescriptorProto.TYPE_FLOAT, pb2.FieldDescriptorProto.TYPE_DOUBLE
            )
        else:
            # Proto2: not packed by default (only if explicit)
            return False
    
    @staticmethod
    def get_group_type_name(type_name: str) -> str:
        """Extract the group type name from a type_name."""
        if type_name.startswith('.'):
            type_name = type_name[1:]
        # Groups are typically named like "MessageName.GroupName"
        parts = type_name.split('.')
        return parts[-1] if parts else 'UnknownGroup'
