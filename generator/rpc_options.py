#!/usr/bin/env python3
"""
RPC option constants for protobuf extension fields.
Defines field numbers for custom RPC options used in LitePB.

Note: These extension fields are for compile-time configuration only.
Runtime protocol fields like msg_id and message_type are defined in
RpcMessage (see rpc_protocol.proto) and handled by the wire protocol layer.
"""


class MethodOptions:
    """Extension field numbers for RPC method options.
    
    These are compile-time options specified in .proto files:
    - DEFAULT_TIMEOUT_MS: Default timeout for RPC calls in milliseconds
    - METHOD_ID: Unique identifier for the method within its service
    - FIRE_AND_FORGET: Whether the method is one-way (no response expected)
    """
    DEFAULT_TIMEOUT_MS = 50003
    METHOD_ID = 50004
    FIRE_AND_FORGET = 50005


class ServiceOptions:
    """Extension field numbers for RPC service options.
    
    These are compile-time options specified in .proto files:
    - SERVICE_ID: Unique identifier for the service
    """
    SERVICE_ID = 50006
