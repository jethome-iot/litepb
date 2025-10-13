#!/usr/bin/env python3
"""Simple wrapper to run the generator with proper paths."""
import sys
import os

# Add generator directory to path
sys.path.insert(0, '/home/runner/workspace/generator')

# Now import and run the main generator
from litepb_gen import main

if __name__ == '__main__':
    sys.exit(main())