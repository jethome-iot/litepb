"""PlatformIO pre-build script for LitePB code generation.

This script automatically generates C++ code from Protocol Buffer (.proto) files
during the PlatformIO build process. It searches for the litepb_gen generator,
processes proto files specified in platformio.ini, and outputs generated code
to the build directory.

Configuration in platformio.ini:
    custom_litepb_protos = path/to/file.proto path/to/other.proto
    custom_litepb_include_dirs = include/path1 include/path2
"""

from __future__ import annotations

Import("env")  # PlatformIO environment injection - must be first line

import logging
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(message)s",
)
logger = logging.getLogger(__name__)


class GeneratorError(Exception):
    """Raised when code generation fails."""
    pass


class GeneratorConfig:
    """Configuration for the LitePB code generator."""

    def __init__(self, env: Any) -> None:
        """Initialize configuration from PlatformIO environment.

        Args:
            env: PlatformIO environment object
        """
        self.project_dir = Path(env.subst("$PROJECT_DIR"))
        self.build_dir = Path(env.subst("$BUILD_DIR"))
        self.output_dir = self.build_dir / "litepb" / "generated"

        # Get custom options from platformio.ini
        protos_str = env.subst(env.GetProjectOption("custom_litepb_protos", ""))
        self.proto_files = [
            self.project_dir / proto for proto in protos_str.split() if proto
        ]

        # Check if RPC support is enabled
        use_rpc_str = env.subst(env.GetProjectOption("custom_litepb_use_rpc", "false"))
        use_rpc = use_rpc_str.lower() in ["true", "yes", "1", "on"]
        
        # Add RPC protocol protos if enabled
        if use_rpc:
            # Try to find the RPC protos in multiple locations
            # (handle both main project and example project structures)
            possible_paths = [
                self.project_dir / "proto" / "litepb",  # Main project
                self.project_dir / ".." / ".." / ".." / ".." / "proto" / "litepb",  # Example project
            ]
            
            rpc_proto_names = ["rpc_protocol.proto", "rpc_options.proto"]
            
            for base_path in possible_paths:
                if base_path.exists():
                    for proto_name in rpc_proto_names:
                        rpc_proto = base_path / proto_name
                        if rpc_proto.exists() and rpc_proto not in self.proto_files:
                            self.proto_files.append(rpc_proto)
                    # If we found the protos, stop searching
                    if any((base_path / name).exists() for name in rpc_proto_names):
                        break

        includes_str = env.subst(env.GetProjectOption("custom_litepb_include_dirs", ""))
        self.include_dirs = [
            self.project_dir / inc for inc in includes_str.split() if inc
        ]

        self.env = env


def find_generator_path(env: Any, project_dir: Path) -> Path:
    """Locate the litepb_gen generator script.

    Searches in multiple locations:
    1. Library dependencies (when LitePB is used as lib_deps)
    2. Project directory
    3. Relative paths for examples

    Args:
        env: PlatformIO environment object
        project_dir: Project root directory

    Returns:
        Path to the litepb_gen generator script

    Raises:
        GeneratorError: If generator cannot be found
    """
    search_paths: list[Path] = []

    # Check if LitePB is used as a library dependency
    for lib in env.get("LIB", []):
        lib_path = None
        if hasattr(lib, "path") and "litepb" in lib.path.lower():
            lib_path = Path(lib.path)
        elif hasattr(lib, "get_dir"):
            lib_dir = lib.get_dir()
            if "litepb" in lib_dir.lower():
                lib_path = Path(lib_dir)

        if lib_path:
            search_paths.append(lib_path / "litepb_gen")

    # Add additional search locations
    search_paths.extend([
        project_dir / "litepb_gen",
        project_dir / ".." / ".." / ".." / "litepb_gen",  # For examples at old location
        project_dir / ".." / ".." / ".." / ".." / "litepb_gen",  # For examples at new cpp location
        Path("litepb_gen").absolute(),
    ])

    # Find first existing generator
    for path in search_paths:
        if path.exists():
            return path

    raise GeneratorError(
        "Could not find litepb_gen generator script. "
        f"Searched in: {', '.join(str(p) for p in search_paths)}"
    )


def clean_output_directory(output_dir: Path) -> None:
    """Remove all files from the output directory.

    Args:
        output_dir: Directory to clean
    """
    if output_dir.exists():
        logger.info(f"Cleaning output directory: {output_dir}")
        shutil.rmtree(output_dir)
        output_dir.mkdir(parents=True)
    else:
        logger.info("Output directory does not exist, creating it.")
        output_dir.mkdir(parents=True)


def generate_proto_file(
    generator_path: Path,
    proto_file: Path,
    output_dir: Path,
    include_dirs: list[Path],
) -> None:
    """Generate C++ code from a single proto file.

    Args:
        generator_path: Path to litepb_gen generator
        proto_file: Proto file to process
        output_dir: Directory for generated code
        include_dirs: Additional include directories for imports

    Raises:
        GeneratorError: If proto file doesn't exist or generation fails
    """
    if not proto_file.exists():
        raise GeneratorError(f"Proto file does not exist: {proto_file}")

    # Build command with include arguments
    cmd = [str(generator_path)]

    for inc_dir in include_dirs:
        if not inc_dir.is_dir():
            logger.warning(f"Include directory does not exist: {inc_dir}")
        cmd.extend(["-I", str(inc_dir)])

    cmd.extend(["-o", str(output_dir), str(proto_file)])

    logger.info(f"Running command: {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=False)
    if result.returncode != 0:
        raise GeneratorError(
            f"Code generation failed for {proto_file.name} "
            f"(exit code: {result.returncode})"
        )


def setup_compiler_paths(env: Any, output_dir: Path) -> None:
    """Add generated code directory to compiler include paths.

    Args:
        env: PlatformIO environment object
        output_dir: Directory containing generated code
    """
    env.Append(CPPFLAGS=[f"-I{output_dir}"])
    env.Append(CPPPATH=str(output_dir))


def generate_all_protos(config: GeneratorConfig, generator_path: Path) -> None:
    """Generate C++ code for all configured proto files.

    Args:
        config: Generator configuration
        generator_path: Path to litepb_gen generator

    Raises:
        GeneratorError: If generation fails for any proto file
    """
    if not config.proto_files:
        logger.info("No proto files specified for generation.")
        return

    # Log the proto files that will be generated
    logger.info(f"Proto files to generate: {len(config.proto_files)}")
    for proto in config.proto_files:
        logger.info(f"  - {proto.relative_to(config.project_dir) if proto.is_absolute() else proto}")

    clean_output_directory(config.output_dir)
    setup_compiler_paths(config.env, config.output_dir)

    for proto_file in config.proto_files:
        generate_proto_file(
            generator_path,
            proto_file,
            config.output_dir,
            config.include_dirs,
        )


def main(build_env: Any) -> None:
    """Main entry point for the code generation script.
    
    Args:
        build_env: PlatformIO environment object
    """
    logger.info("=" * 38)
    logger.info("LitePB code generation script started")
    logger.info("=" * 38)

    try:
        config = GeneratorConfig(build_env)
        generator_path = find_generator_path(build_env, config.project_dir)

        logger.info(f"project_dir         : {config.project_dir}")
        logger.info(f"build_dir           : {config.build_dir}")
        logger.info(f"generator_path      : {generator_path}")
        logger.info("=" * 38)

        generate_all_protos(config, generator_path)

        logger.info("=" * 38)
        logger.info("LitePB code generation script finished")
        logger.info("=" * 38)

    except GeneratorError as e:
        logger.error(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        sys.exit(1)


# Execute main when script is loaded by PlatformIO
if __name__ == "__main__":
    main(env)
else:
    # PlatformIO runs this as a module, not as __main__
    main(env)
