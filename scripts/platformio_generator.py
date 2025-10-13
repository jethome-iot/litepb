Import("env")
from pathlib import Path
import sys, inspect, os, shlex
from glob import glob

global_env = DefaultEnvironment()

try:
    Import("projenv")
except:
    projenv = None

################################################################################
# https://docs.platformio.org/en/latest/manifests/library-json/fields/build/extrascript.html
#
# env         - Private flags (only for the current "SomeLib" source files)
# projenv     - Operate with the project environment (files located in the `src` folder)
# global_env  - flags to the Global environment (project `src` files, frameworks)
################################################################################

PIOENV = env["PIOENV"]
BUILD_TYPE = env.GetBuildType()
PROJECT_DIR = Path(env["PROJECT_DIR"])
BUILD_DIR = Path(env.subst(env["BUILD_DIR"]))
OUTDIR_DIR = BUILD_DIR / "litepb" / "generated"   # output dir as requested

def _resolve_script_path():
    # 1. Normal case
    try:
        return Path(__file__).resolve()  # noqa
    except NameError:
        pass

    # 2. Inspect call stack
    for frame_info in inspect.stack():
        f = frame_info.filename
        if f and os.path.exists(f):
            return Path(f).resolve()

    # 3. Fallback to argv
    if sys.argv and sys.argv[0] and os.path.exists(sys.argv[0]):
        return Path(sys.argv[0]).resolve()

    raise RuntimeError("[litepb] Cannot determine script path (no __file__, stack, or argv)")

SCRIPT_PATH = _resolve_script_path()
LITEPB_ROOT = SCRIPT_PATH.parents[1]
LITEPB_SRC_PATH = LITEPB_ROOT / "cpp" / "src"
LITEPB_GENERATOR_DIR = LITEPB_ROOT / "generator"

GEN = LITEPB_ROOT / "./litepb_gen"
if not GEN.exists():
    print(f"[litepb] ERROR: Missing codegen script/executable: {GEN}")
    Exit(1)


################################################################################
#print("*" * 60 + env.Dump() + "*" * 60)
print(f"[litepb] Environment : {PIOENV}, Build type: {BUILD_TYPE}")
print(f"[litepb] Project dir : {PROJECT_DIR}")
print(f"[litepb] Build dir   : {BUILD_DIR}")
print(f"[litepb] LitePB dir  : {LITEPB_ROOT}")
print(f"[litepb] Generator   : {GEN}")

################################################################################
# Read options
custom_litepb_protos = env.GetProjectOption("custom_litepb_protos", "").split()
custom_litepb_include_dirs = env.GetProjectOption("custom_litepb_include_dirs", "").split()
################################################################################

# Expand glob patterns in custom_litepb_protos (supports **, *.proto, etc.)
proto_files = []
seen = set()
for pat in custom_litepb_protos:
    abs_pat = (PROJECT_DIR / pat)
    # glob() handles wildcards; recursive=True enables **
    matches = [
        str(Path(m).resolve())
        for m in glob(str(abs_pat), recursive=True)
    ]
    if not matches:
        # If the pattern has no wildcard and is a real file, accept it
        if abs_pat.exists():
            matches = [str(abs_pat.resolve())]
        else:
            print(f"[litepb] WARN: pattern/file not found: {pat}")
    for m in sorted(matches):
        if m not in seen:
            seen.add(m)
            proto_files.append(m)

# Nothing to do? Exit gracefully.
if not proto_files:
    print("[litepb] No proto files matched by custom_litepb_protos; skipping codegen setup.")
else:
    proto_include_paths = []
    for d in custom_litepb_include_dirs:
        p = (PROJECT_DIR / d).resolve() if not os.path.isabs(d) else Path(d).resolve()
        if p.exists():
            proto_include_paths.append(str(p))
        else:
            print(f"[litepb] WARN: Directory not found: {d}")

    # Add litepb_proto_root to custom_litepb_include_dirs if not already present
    if str(litepb_proto_root) not in proto_include_paths:
        proto_include_paths.insert(0, str(litepb_proto_root))

    # If custom_litepb_use_rpc is set -
    # need to add $LITEPB_ROOT/proto/litepb/rpc_protocol.proto" to proto_files
    if custom_litepb_use_rpc:
        print(f"[litepb] RPC enabled")
        rpc_proto = litepb_proto_root / "litepb" / "rpc_protocol.proto"
        proto_files.append(str(rpc_proto.resolve()))

    # Discover *additional* .proto files inside include dirs so SCons can track imports.
    # This makes rebuilds more accurate when included files change.
    dep_proto_files_set = set(proto_files)
    for pf in proto_files:
        dep_proto_files_set.add(str(Path(pf).resolve()))

    for inc in proto_include_paths:
        for found in Path(inc).rglob("*.proto"):
            dep_proto_files_set.add(str(found.resolve()))

    # Print dep_proto_files for debugging
    print(f"[litepb] Dep proto files ({len(dep_proto_files_set)}):")
    for p in sorted(dep_proto_files_set):
        print("   -", p)

    # Compute expected outputs (.pb.h / .pb.cpp) for each listed proto.
    # NOTE: If you may have colliding filenames from different folders,
    # consider encoding folder names into the output filename.
    def _stem_no_ext(p):
        return Path(p).name[:-6] if str(p).endswith(".proto") else Path(p).stem

    targets = []
    for pf in proto_files:
        stem = _stem_no_ext(pf)
        targets.append(str((OUTDIR_DIR / f"{stem}.pb.h").resolve()))
        targets.append(str((OUTDIR_DIR / f"{stem}.pb.cpp").resolve()))

    # Print targets for debugging
    print(f"[litepb] Expected targets ({len(targets)}):")
    for t in sorted(targets):
        print("   -", t)

    def _codegen_action(target, source, env):
        import subprocess
        # Ensure output dir exists
        OUTDIR_DIR.mkdir(parents=True, exist_ok=True)

        # Build command:
        #   litepb_gen -I include1 -I include2 -o outdir file1.proto file2.proto ...
        cmd = [str(GEN)]

        # Append include dirs (-I)
        for inc in proto_include_paths:
            cmd += ["-I", inc]

        # Output dir
        cmd += ["-o", str(OUTDIR_DIR)]

        # Proto files (only the main ones you listed)
        cmd += proto_files

        print("[litepb] Codegen:", " ".join(shlex.quote(x) for x in cmd))
        subprocess.check_call(cmd)
        return 0

    # Wire up the builder: sources = all .proto that can affect generation,
    # targets = expected outputs for the configured protos
    codegen = env.Command(
        target=targets,
        source=sorted(dep_proto_files_set),
        action=_codegen_action
    )

    # Ensure program build waits for codegen
    # (works across frameworks; adjust if you use a different final target)
    env.Depends("$BUILD_DIR/${PROGNAME}.elf", codegen)

    # Ensure codegen is re-run if this script or any file in the generator dir changes
    _excluded = {".venv", "__pycache__"}
    extra_deps = (
        str(SCRIPT_PATH.resolve()),
        *(
            str(p)
            for p in LITEPB_GENERATOR_DIR.rglob("*")
            if p.is_file() and not any(part in _excluded for part in p.parts)
        )
    )
    print(f"[litepb] Extra dependencies: {len(extra_deps)} files:")
    for d in extra_deps:
       print("   -", d)
    env.Depends(codegen, extra_deps)


    # Tell the compiler where to find generated sources
    env.Append(CPPPATH=[str(OUTDIR_DIR)])
    #env.Append(CPPPATH=[LITEPB_SRC_PATH.resolve()]) # our own sources

    # If custom_litepb_use_rpc is enabled, add definition LITEPB_WITH_RPC
    if custom_litepb_use_rpc:
        env.Append(CPPDEFINES=["LITEPB_WITH_RPC"])
        projenv.Append(CPPDEFINES=["LITEPB_WITH_RPC"]) if projenv else None

    print(f"[litepb] Enabled. Outputs -> {OUTDIR_DIR}")
    print(f"[litepb] Protos ({len(proto_files)}):")
    for pf in proto_files:
        print("   -", pf)
    print(f"[litepb] Include dirs ({len(proto_include_paths)}):")
    for inc in proto_include_paths:
        print("   -", inc)