#!/usr/bin/env python3
"""
Data models for C++ code generation.
Replaces dict-based structures with proper dataclasses.
"""

from dataclasses import dataclass
from typing import List, Optional
from google.protobuf import descriptor_pb2 as pb2


@dataclass
class MapFieldInfo:
    """Information about a protobuf map field."""
    name: str
    number: int
    key_field: pb2.FieldDescriptorProto
    value_field: pb2.FieldDescriptorProto


@dataclass
class OneofInfo:
    """Information about a protobuf oneof field."""
    name: str
    fields: List[pb2.FieldDescriptorProto]
