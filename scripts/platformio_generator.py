"""PlatformIO pre-build script for LitePB code generation.

This script automatically generates C++ code from Protocol Buffer (.proto) files
during the PlatformIO build process. It searches for the litepb_gen generator,
processes proto files specified in platformio.ini, and outputs generated code
to the build directory.

Configuration in platformio.ini:
    custom_litepb_protos = path/to/file.proto path/to/other.proto
    custom_litepb_protos = tests/proto/**/*.proto  # Glob patterns supported
    custom_litepb_include_dirs = include/path1 include/path2
    custom_litepb_use_rpc = true  # Enable RPC support
"""

from __future__ import annotations

Import("env")  # PlatformIO environment injection - must be first line

import glob
import logging
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional

# Module-level constants
LITEPB_DIR_NAME = "litepb"
GENERATED_DIR_NAME = "generated"
GENERATOR_SCRIPT_NAME = "litepb_gen"
RPC_PROTO_FILES = ["rpc_protocol.proto", "rpc_options.proto"]
RPC_PREPROCESSOR_DEFINE = "LITEPB_WITH_RPC"
PROTO_SUBDIR = "proto/litepb"
TRUE_VALUES = ["true", "yes", "1", "on"]

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
        self._env = env
        
        # Try to import projenv for global environment access
        try:
            Import("projenv")
            self._projenv = projenv
        except:
            self._projenv = None
        
        self.project_dir = Path(env.subst("$PROJECT_DIR"))
        self.build_dir = Path(env.subst("$BUILD_DIR"))
        self.output_dir = self.build_dir / LITEPB_DIR_NAME / GENERATED_DIR_NAME

        # Process proto files with glob support
        self.proto_files = self._process_proto_files()

        # Process include directories
        includes_str = env.subst(env.GetProjectOption("custom_litepb_include_dirs", ""))
        self.include_dirs = [
            self.project_dir / inc for inc in includes_str.split() if inc
        ]

        # Handle RPC support
        if self._detect_rpc_enabled():
            self._add_rpc_support()
            self._add_rpc_protos()

    def _process_proto_files(self) -> list[Path]:
        """Process proto file specifications with glob support.
        
        Returns:
            List of resolved proto file paths
        """
        proto_files = []
        protos_str = self._env.subst(self._env.GetProjectOption("custom_litepb_protos", ""))
        
        for proto_spec in protos_str.split():
            if not proto_spec:
                continue
                
            # Check if this is a glob pattern
            if any(char in proto_spec for char in ['*', '?', '[']):
                # Expand glob pattern relative to project directory
                pattern_path = self.project_dir / proto_spec
                matched_files = glob.glob(str(pattern_path), recursive=True)
                
                if matched_files:
                    logger.info(f"Glob pattern '{proto_spec}' matched {len(matched_files)} files")
                    proto_files.extend(Path(f) for f in matched_files)
                else:
                    logger.warning(f"Glob pattern '{proto_spec}' matched no files")
            else:
                # Regular file path
                proto_files.append(self.project_dir / proto_spec)
        
        return proto_files

    def _detect_rpc_enabled(self) -> bool:
        """Detect if RPC support should be enabled.
        
        Returns:
            True if RPC support should be enabled
        """
        # First check if LITEPB_WITH_RPC is already defined
        if RPC_PREPROCESSOR_DEFINE in self._env.get("CPPDEFINES", []):
            return True
        
        # Check if custom_litepb_use_rpc is set to true
        try:
            use_rpc_str = self._env.subst(
                self._env.GetProjectOption("custom_litepb_use_rpc", "false")
            )
            return use_rpc_str.lower() in TRUE_VALUES
        except:
            # GetProjectOption might fail when building as a library dependency
            # In that case, check if we're in an RPC example by looking at the project path
            return "rpc" in str(self.project_dir).lower()

    def _add_rpc_support(self) -> None:
        """Add RPC preprocessor definitions to the environments."""
        # Add to current environment
        if RPC_PREPROCESSOR_DEFINE not in self._env.get("CPPDEFINES", []):
            self._env.Append(CPPDEFINES=[RPC_PREPROCESSOR_DEFINE])
        
        # Add to project environment if available (for library builds)
        if self._projenv is not None:
            if RPC_PREPROCESSOR_DEFINE not in self._projenv.get("CPPDEFINES", []):
                self._projenv.Append(CPPDEFINES=[RPC_PREPROCESSOR_DEFINE])

    def _add_rpc_protos(self) -> None:
        """Add RPC protocol proto files to the list of files to generate."""
        # Try to find the RPC protos in multiple locations
        possible_paths = [
            self.project_dir / PROTO_SUBDIR,  # Main project
            self.project_dir.parent.parent.parent.parent / PROTO_SUBDIR,  # Example project
        ]
        
        for base_path in possible_paths:
            if base_path.exists():
                for proto_name in RPC_PROTO_FILES:
                    rpc_proto = base_path / proto_name
                    if rpc_proto.exists() and rpc_proto not in self.proto_files:
                        self.proto_files.append(rpc_proto)
                
                # If we found the protos, stop searching
                if any((base_path / name).exists() for name in RPC_PROTO_FILES):
                    break

    @property
    def env(self) -> Any:
        """Get the PlatformIO environment object."""
        return self._env


def find_generator_path(env: Any, project_dir: Path) -> Path:
    """Locate the litepb_gen generator script.

    Searches in multiple locations:
    1. Library directories (when LitePB is used as lib_deps)
    2. Project directory
    3. Parent directories for nested projects

    Args:
        env: PlatformIO environment object
        project_dir: Project root directory

    Returns:
        Path to the litepb_gen generator script

    Raises:
        GeneratorError: If generator cannot be found
    """
    search_paths: list[Path] = []

    # Check library directories from LIBSOURCE_DIRS
    lib_dirs = env.get("LIBSOURCE_DIRS", [])
    for lib_dir in lib_dirs:
        lib_path = Path(lib_dir)
        if lib_path.exists():
            # Look for litepb libraries
            for subdir in lib_path.iterdir():
                if subdir.is_dir() and LITEPB_DIR_NAME in subdir.name.lower():
                    search_paths.append(subdir / GENERATOR_SCRIPT_NAME)

    # Add project directory
    search_paths.append(project_dir / GENERATOR_SCRIPT_NAME)
    
    # Add parent directories for nested projects
    current = project_dir
    for _ in range(4):  # Check up to 4 levels up
        current = current.parent
        if current == current.parent:  # Reached root
            break
        search_paths.append(current / GENERATOR_SCRIPT_NAME)
    
    # Also check if it's in PATH (absolute path)
    search_paths.append(Path(GENERATOR_SCRIPT_NAME).resolve())

    # Find first existing generator
    for path in search_paths:
        if path.exists():
            return path

    raise GeneratorError(
        f"Could not find {GENERATOR_SCRIPT_NAME} generator script. "
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
    # Add the generated directory to include paths
    # This allows including generated files without the full path
    env.Append(CPPPATH=[str(output_dir)])


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
        try:
            relative_path = proto.relative_to(config.project_dir)
            logger.info(f"  - {relative_path}")
        except ValueError:
            # Proto file is outside project directory, show absolute path
            logger.info(f"  - {proto}")

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
    
    # Additional check: If we're building the library and the parent project needs RPC,
    # ensure LITEPB_WITH_RPC is defined for library compilation
    try:
        Import("projenv")
        parent_projenv = projenv
    except:
        parent_projenv = None
    
    if parent_projenv is not None:
        # Check if the parent project has RPC enabled
        try:
            parent_rpc = parent_projenv.GetProjectOption("custom_litepb_use_rpc", "false")
            if parent_rpc.lower() in TRUE_VALUES:
                # Ensure library gets compiled with RPC support
                if RPC_PREPROCESSOR_DEFINE not in build_env.get("CPPDEFINES", []):
                    build_env.Append(CPPDEFINES=[RPC_PREPROCESSOR_DEFINE])
        except:
            pass

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