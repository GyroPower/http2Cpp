import argparse
import subprocess
import shutil
from pathlib import Path

parser = argparse.ArgumentParser(
    description="Script just to build and install the library for a desired cmake config"
)
parser.add_argument(
    "-c", "--compiler", help="pass clang or gcc, msvc not tested and implemented yet", default="gcc"
)
parser.add_argument(
    "-bt", "--build-type", help="Pass 'Debug' or 'Release' for the type or build", default="Debug"
)
parser.add_argument(
    "-bs",
    "--build-system",
    help="Pass ninja or make for the build system to generate, others not implemented",
    default="make",
)
parser.add_argument(
    "--cmake-prefix-path",
    help="Pass the path to where install the library, not need if the Enviroment variable is set for cmake",
)
parser.add_argument(
    "-lcxx", "--libcxx", help="Use libc++ instead of libstdc++11", action="store_const", const=True
)


args = parser.parse_args()


# Build to use with ninja and clang
def run_process():
    cmake_prefix_path = None
    if args.cmake_prefix_path:
        cmake_prefix_path = Path(args.cmake_prefix_path)

    print(f"-- compiler: {args.compiler}")
    print(f"-- build-system: {args.build_system}")
    print(f"-- build_type: {args.build_type}")
    print(f"-- cmake_prefix_path: {args.cmake_prefix_path}")

    compiler_c = "gcc"
    compiler_cxx = "g++"

    if args.compiler == "clang":
        compiler_c = "clang"
        compiler_cxx = "clang++"
    elif args.compiler != "clang" or args.compiler != "gcc":
        compiler_c = "gcc"
        compiler_cxx = "g++"
        print(f"-- Error: compiler not contempled for build of library, using: {compiler_c}")

    script_dir = Path(__file__).resolve().parent
    build_type = "Debug"

    if args.build_type == "Release":
        build_type = "Release"
    elif args.build_type != "Release" and args.build_type != "Debug":
        print(f"build_type: passing a rare argument, using {build_type}")

    build_dir = script_dir / "build" / build_type

    build_system = "Unix Makefiles"

    if args.build_system == "ninja" or args.build_system == "Ninja":
        build_system = "Ninja"
    elif args.build_system != "ninja" and args.build_system != "make":
        print(f"build_system: not tested/supported by the library, using {build_system}")

    script_dir = Path(__file__).resolve().parent

    build_dir = script_dir / "../build" / build_type

    command = [
        "cmake",
        "-G",
        f"{build_system}",
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_C_COMPILER={compiler_c}",
        f"-DCMAKE_CXX_COMPILER={compiler_cxx}",
        "-DCMAKE_CXX_STANDARD=23",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=On",
    ]

    if cmake_prefix_path is not None:
        command.append(f"-DCMAKE_INSTALL_PREFIX={cmake_prefix_path}")

    if args.libcxx and not compiler_c == "gcc":
        command.append("-DUSE_LIB_CXX=On")

    result = subprocess.run(command, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- cmake config done")
    else:
        print("-- cmake failed")
        return

    # Copy compile_commands.json to root for clangd
    source = Path(build_dir / "compile_commands.json")

    if source.exists():
        print("-- compile_commands.json to copy")
        destination = script_dir / ".." / source.name
        _ = shutil.copy2(source, destination)
        source.unlink(missing_ok=True)
    else:
        print("-- compile_commands.json not found")

    command = ["cmake", "--build", f"{build_dir}", "--target", "install"]

    result = subprocess.run(command, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- install done")
    else:
        print("-- install failed")

    print(
        f"-- Enviroment settings\n-- Compiler: {compiler_c}\n-- Build type: {build_type}\n-- Build system: {build_system}"
    )

    if args.libcxx:
        print("-- libc++ use: True")


run_process()
