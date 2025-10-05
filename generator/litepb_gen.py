#!/usr/bin/env python3
"""
LitePB Generator - Python implementation
Generates lightweight C++ code from .proto files for embedded systems.
"""

import os
import sys
import argparse
from pathlib import Path
from proto_parser import ProtoParser
from cpp_generator import CppGenerator


def check_dependencies():
    """Check if required dependencies are available."""
    import subprocess
    import shutil
    
    # Check for protoc
    if not shutil.which('protoc'):
        print("Error: protoc compiler not found. Please install protobuf.", file=sys.stderr)
        print("On Ubuntu/Debian: apt-get install protobuf-compiler", file=sys.stderr)
        print("On Mac: brew install protobuf", file=sys.stderr)
        print("On Nix: nix-env -i protobuf", file=sys.stderr)
        return False
    
    # Check for Python packages
    try:
        import google.protobuf
        import jinja2
    except ImportError as e:
        print(f"Error: Missing Python dependency: {e}", file=sys.stderr)
        print("Install with: pip install protobuf==5.28.0 Jinja2==3.1.4", file=sys.stderr)
        return False
    
    return True


def main():
    """Main entry point for the generator."""
    # Check dependencies first
    if not check_dependencies():
        return 1
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='LitePB Generator - Generate lightweight C++ code from .proto files'
    )
    parser.add_argument(
        'proto_files',
        nargs='+',
        help='Input .proto files to process'
    )
    parser.add_argument(
        '-o', '--output',
        default='.',
        help='Output directory for generated files (default: current directory)'
    )
    parser.add_argument(
        '-I', '--proto_path', '--include',
        action='append',
        default=[],
        dest='include',
        help='Add directory to the import path (can be specified multiple times)'
    )
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Initialize parser and generator
    proto_parser = ProtoParser(import_paths=args.include)
    cpp_gen = CppGenerator()
    
    # Process each proto file
    for proto_file in args.proto_files:
        try:
            print(f"Parsing {proto_file}...")
            
            # Check if file exists
            if not os.path.exists(proto_file):
                print(f"Error: File not found: {proto_file}", file=sys.stderr)
                sys.exit(1)
            
            # Parse the proto file
            proto_data = proto_parser.parse_proto_file(proto_file)
            
            # Generate output filenames
            base_name = os.path.basename(proto_file).replace('.proto', '')
            header_file = output_dir / f"{base_name}.pb.h"
            source_file = output_dir / f"{base_name}.pb.cpp"
            
            # Generate C++ code
            print(f"Generating {header_file}")
            header_content = cpp_gen.generate_header(proto_data, proto_file)
            
            print(f"Generating {source_file}")
            source_content = cpp_gen.generate_implementation(proto_data, proto_file)
            
            # Write output files
            with open(header_file, 'w') as f:
                f.write(header_content)
            
            with open(source_file, 'w') as f:
                f.write(source_content)
            
            print(f"Successfully generated files for {proto_file}")
            
        except Exception as e:
            print(f"Error processing {proto_file}: {e}", file=sys.stderr)
            import traceback
            traceback.print_exc()
            sys.exit(1)
    
    print("Code generation complete!")
    return 0


if __name__ == '__main__':
    sys.exit(main())