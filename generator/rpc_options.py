#!/usr/bin/env python3
"""
RPC option constants for protobuf extension fields.
Defines field numbers for custom RPC options used in LitePB.
"""


class MethodOptions:
    """Extension field numbers for RPC method options."""
    DEFAULT_TIMEOUT_MS = 50003
    METHOD_ID = 50004
    FIRE_AND_FORGET = 50005


class ServiceOptions:
    """Extension field numbers for RPC service options."""
    SERVICE_ID = 50006
