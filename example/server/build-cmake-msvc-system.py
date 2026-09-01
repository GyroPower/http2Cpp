import argparse
import subprocess
import shutil
import os
from pathlib import Path

parser = argparse.ArgumentParser(description="Script just to build the generators for a desired cmake build")
parser.add_argument("-c", "--compiler", help="pass clang or gcc, msvc not tested yet", default="clang")
# parser.add_argument(
#     "-bt", "--build-type", help="Pass 'Debug' or 'Release' for the type or build", default="Debug"
# )
# parser.add_argument(
#     "-bs", "--build-system", help="Pass ninja or make for the build system to generate", default="make"
# )
# parser.add_argument(
#     "-lcxx", "--libcxx", help="Use libc++ instead of libstdc++11", action="store_const", const=True
# )
parser.add_argument(
    "-ne",
    "--no-example",
    help="flag to not include example to build for development",
    action="store_const",
    const=True,
)

parser.add_argument("-d", "--delete", help="Deletes the build directory", action="store_const", const=True)

args = parser.parse_args()


# Build to use with ninja and clang
def run_process():

    script_dir = Path(__file__).resolve().parent

    print(f"script dir: {script_dir}")

    build_dir = script_dir / "build"

    compiler_c = "msvc"
    compilex_cxx = "msvc"

    print(f"compiler: {args.compiler}")

    if args.compiler == "clang":
        compiler_c = "clang"
        compilex_cxx = "clang++"

    if compiler_c == "clang":
        print("clang compiler")
        command = [
            "cmake",
            "-B",
            str(build_dir),
            f"-DCMAKE_C_COMPILER={compiler_c}",
            f"-DCMAKE_CXX_COMPILER={compilex_cxx}",
            "-DCMAKE_CXX_STANDARD=23",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=On",
            "-T",
            "ClangCL",
        ]

        print("Command overwritted")

    else:
        command = [
            "cmake",
            "-B",
            str(build_dir),
            "-DCMAKE_CXX_STANDARD=23",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=On",
        ]

    print(f"Command: {command}")

    result = subprocess.run(command, capture_output=False, text=True)

    source = Path(build_dir) / "compile_commands.json"

    if source.exists():
        print("-- compile_commands.json to copy")
        destination = script_dir / ".." / source.name
        _ = shutil.copy2(source, destination)
        source.unlink(missing_ok=True)
    else:
        print("-- compile_commands.json not found")


run_process()
