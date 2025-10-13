#!/usr/bin/env python3
"""
RPC option constants for protobuf extension fields.
Defines field numbers for custom RPC options used in LitePB.

Note: These extension fields are for compile-time configuration only.
Runtime protocol fields like msg_id and message_type are defined in
RpcMessage (see rpc_protocol.proto) and handled by the wire protocol layer.
"""

from dataclasses import dataclass
from typing import Optional
from enum import IntEnum


class CallDirection(IntEnum):
    """Call direction for RPC methods."""
    CLIENT_TO_SERVER = 0
    SERVER_TO_CLIENT = 1
    BIDIRECTIONAL = 2


@dataclass
class RpcMethodOptions:
    """Dataclass representing parsed RPC method option values."""
    method_id: Optional[int] = None
    default_timeout_ms: int = 5000
    direction: CallDirection = CallDirection.CLIENT_TO_SERVER
    is_event: bool = False


@dataclass
class RpcServiceOptions:
    """Dataclass representing parsed RPC service option values."""
    service_id: Optional[int] = None


class MethodOptions:
    """Extension field numbers for RPC method options.
    
    These are compile-time options specified in .proto files:
    - DEFAULT_TIMEOUT_MS: Default timeout for RPC calls in milliseconds
    - METHOD_ID: Unique identifier for the method within its service
    - DIRECTION: Call direction (CLIENT_TO_SERVER, SERVER_TO_CLIENT, BIDIRECTIONAL)
    - IS_EVENT: Whether the method is an event (no response expected)
    """
    DEFAULT_TIMEOUT_MS = 50003
    METHOD_ID = 50004
    DIRECTION = 50007
    IS_EVENT = 50008


class ServiceOptions:
    """Extension field numbers for RPC service options.
    
    These are compile-time options specified in .proto files:
    - SERVICE_ID: Unique identifier for the service
    """
    SERVICE_ID = 50006