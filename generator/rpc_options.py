#!/usr/bin/env python3
"""
RPC options helper classes and constants.
Defines extension field numbers and helper classes for RPC method and service options.
"""


class ExtensionFieldNumbers:
    """Extension field numbers for RPC options."""
    DEFAULT_TIMEOUT_MS = 50003
    METHOD_ID = 50004
    SERVICE_ID = 50006


class MethodOptions:
    """Helper class for RPC method options."""
    def __init__(self):
        self.method_id = None
        self.default_timeout_ms = 5000


class ServiceOptions:
    """Helper class for RPC service options."""
    def __init__(self):
        self.service_id = None
