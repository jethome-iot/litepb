#!/usr/bin/env python3
import sys
print("Python path:", sys.path)
print("Python version:", sys.version)

try:
    import google
    print("google module imported successfully")
    import google.protobuf
    print("google.protobuf module imported successfully")
    from google.protobuf import descriptor_pb2 as pb2
    print("descriptor_pb2 imported successfully")
except ImportError as e:
    print(f"Import error: {e}")

try:
    import jinja2
    print("jinja2 imported successfully")
except ImportError as e:
    print(f"Jinja2 import error: {e}")