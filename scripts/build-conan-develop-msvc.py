import subprocess
import shutil
from pathlib import Path

import argparse

parser = argparse.ArgumentParser(
    description="Script just to build the generators for a desired conan tool chain for cmake"
)
parser.add_argument(
    "-c", "--compiler", help="pass clang or gcc, msvc not tested and implemented yet", default="clang"
)

args = parser.parse_args()


# Build to use with ninja and clang
def run_process():
    script_dir = Path(__file__).resolve().parent / ".." / ""

    print(f"script dir: {script_dir}")

    build_dir = script_dir / "build"

    compiler_c = "msvc"
    compilex_cxx = "msvc"

    profile = f"{script_dir}/conan-profiles/windows/msvc_debugCpp23"

    if args.compiler == "clang":
        compiler_c = "clang"
        compilex_cxx = "clang++"
        profile = f"{script_dir}/conan-profiles/windows/clang-cl_debugCpp23"

    command_conan = [
        "conan",
        "install",
        ".",
        "-s",
        "build_type=Debug",
        "--build=missing",
        f"-pr={profile}",
    ]

    result = subprocess.run(command_conan, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- conan config done")
    else:
        print("-- conan failed")
        return

    command_conan = [
        "conan",
        "install",
        ".",
        "-s",
        "build_type=Release",
        "--build=missing",
        f"-pr={profile}",
    ]

    result = subprocess.run(command_conan, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- conan config done")
    else:
        print("-- conan failed")
        return

    command = [
        "cmake",
        "-B",
        str(build_dir),
        "-DCMAKE_CXX_STANDARD=23",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=On",
        "-DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake",
    ]

    if compiler_c == "clang":
        command = [
            "cmake",
            "-B",
            str(build_dir),
            f"-DCMAKE_C_COMPILER={compiler_c}",
            f"-DCMAKE_CXX_COMPILER={compilex_cxx}",
            "-DCMAKE_CXX_STANDARD=23",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=On",
            "-DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake",
        ]

    command.append("-DINCLUDE_EXAMPLE=On")

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
