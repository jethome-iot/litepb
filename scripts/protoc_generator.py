Import("env")
import subprocess
import os
import sys
import shutil

print("======================================")
print("Protoc code generation script started")
print("======================================")

project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")

output_dir = os.path.join(project_dir, "test/proto/test_interop/protoc")

def check_protoc():
    protoc_path = shutil.which("protoc")
    if not protoc_path:
        print("Error: protoc compiler not found. Please install protobuf compiler.")
        sys.exit(1)
    print(f"Found protoc: {protoc_path}")
    
    result = subprocess.run([protoc_path, "--version"], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"Protoc version: {result.stdout.strip()}")
    return protoc_path

def generate_protoc_code():
    protoc_protos = env.subst(env.GetProjectOption("custom_protoc_protos", ""))
    protoc_include_dirs = env.subst(env.GetProjectOption("custom_protoc_include_dirs", ""))
    
    if not protoc_protos:
        print("No proto files specified for protoc generation.")
        return
    
    protoc_path = check_protoc()
    
    if os.path.exists(output_dir):
        print(f"Cleaning output directory: {output_dir}")
        for f in os.listdir(output_dir):
            file_path = os.path.join(output_dir, f)
            if os.path.isfile(file_path):
                os.remove(file_path)
    else:
        print(f"Creating output directory: {output_dir}")
    
    os.makedirs(output_dir, exist_ok=True)
    
    env.Append(CPPFLAGS=[f'-I{output_dir}'])
    env.Append(CPPPATH=output_dir)
    
    for proto in protoc_protos.split():
        proto_path = os.path.join(project_dir, proto)
        if not os.path.isfile(proto_path):
            print(f"Error: Proto file {proto_path} does not exist")
            sys.exit(1)
        
        proto_dir = os.path.dirname(proto_path)
        
        include_args = []
        if protoc_include_dirs:
            for inc_dir in protoc_include_dirs.split():
                include_path = os.path.join(project_dir, inc_dir)
                if not os.path.isdir(include_path):
                    print(f"Warning: Include directory {include_path} does not exist")
                include_args.extend([f'-I{include_path}'])
        
        include_args.append(f'-I{proto_dir}')
        
        cmd = [protoc_path, f'--cpp_out={output_dir}'] + include_args + [proto_path]
        print(f"Running command: {' '.join(cmd)}")
        
        result = subprocess.run(cmd, cwd=project_dir)
        if result.returncode != 0:
            print(f"Error: Generating code from {proto} failed")
            sys.exit(1)
        
        print(f"Successfully generated C++ code from {proto}")
    
    generated_files = [f for f in os.listdir(output_dir) if f.endswith('.cc') or f.endswith('.h')]
    print(f"Generated files: {', '.join(generated_files)}")

print(f"project_dir         : {project_dir}")
print(f"build_dir           : {build_dir}")
print(f"output_dir          : {output_dir}")
print("======================================")

generate_protoc_code()

print("======================================")
print("Protoc code generation script finished")
print("======================================")
