import argparse
import subprocess
import shutil
import os
from pathlib import Path

parser = argparse.ArgumentParser(
    description="Script just to build the generators for a desired conan tool chain for cmake"
)
parser.add_argument(
    "-c", "--compiler", help="pass clang or gcc, msvc not tested and implemented yet", default="clang"
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
    "-lcxx",
    "--libcxx",
    help="Use libc++ instead of libstdc++11 for clang in case you pass your own profile which build all with libc++",
    action="store_const",
    const=True,
)
parser.add_argument(
    "-d",
    "--delete",
    help="Deletes the build directory, only for code navegation with clangd lsp",
    action="store_const",
    const=True,
)
parser.add_argument("-pf", "--profile", help="Set the conan profile you want")
parser.add_argument(
    "-ne",
    "--no-example",
    help="flag to not include example to build for development",
    action="store_const",
    const=True,
)

args = parser.parse_args()

default_clang_profiles_posix = {
    "Debug": "conan-profiles/posix/clang_debugCpp23",
    "Release": "conan-profiles/posix/clang_relCpp23",
}

default_clang_profiles_windows = {
    "Debug": "conan-profiles/windows/clang_debugCpp23",
    "Release": "conan-profiles/windows/clang_relCpp23",
}

default_gcc_profiles_posix = {
    "Debug": "conan-profiles/posix/gcc_debugCpp23",
    "Release": "conan-profiles/posix/gcc_relCpp23",
}

default_gcc_profiles_windows = {
    "Debug": "conan-profiles/windows/gcc_debugCpp23",
    "Release": "conan-profiles/windows/gcc_relCpp23",
}


# Build to use with ninja and clang
def run_process():
    script_dir = Path(__file__).resolve().parent
    build_type = "Debug"
    conan_profile = args.profile
    compiler_c = "clang"
    compiler_cxx = "clang++"

    if args.compiler == "gcc":
        compiler_c = "gcc"
        compiler_cxx = "g++"

    if args.build_type == "Release":
        build_type = "Release"
        if conan_profile is None:
            if compiler_c == "clang":
                if os.name == "posix":
                    conan_profile = default_clang_profiles_posix["Release"]
                elif os.name == "nt":
                    conan_profile = default_clang_profiles_windows["Release"]
            elif compiler_c == "gcc":
                if os.name == "posix":
                    conan_profile = default_gcc_profiles_posix["Release"]
                elif os.name == "nt":
                    conan_profile = default_gcc_profiles_windows["Release"]

    elif build_type == "Debug":
        if conan_profile is None:
            if compiler_c == "clang":
                if os.name == "posix":
                    conan_profile = default_clang_profiles_posix["Debug"]
                elif os.name == "nt":
                    conan_profile = default_clang_profiles_windows["Debug"]

            elif compiler_c == "gcc":
                if os.name == "posix":
                    conan_profile = default_gcc_profiles_posix["Debug"]
                elif os.name == "nt":
                    conan_profile = default_gcc_profiles_windows["Debug"]

    elif args.build_type != "Release" and args.build_type != "Debug":
        print(f"build_type: passing a rare argument, using {build_type}")

    build_dir = script_dir / "../build" / build_type

    build_system = "Unix Makefiles"

    if args.build_system == "ninja" or args.build_system == "Ninja":
        build_system = "Ninja"
    elif args.build_system != "ninja" and args.build_system != "make":
        print(f"build_system: not tested/supported by the library, using {build_system}")

    command_conan = [
        "conan",
        "install",
        ".",
        "--build=missing",
        f"-pr={conan_profile}",
    ]

    result = subprocess.run(command_conan, capture_output=False, text=True)

    if result.returncode == 0:
        print("--conan config done")
    else:
        print("--conan failed")
        return

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
        f"-DCMAKE_TOOLCHAIN_FILE={build_dir}/generators/conan_toolchain.cmake",
    ]

    if args.no_example is None:
        command.append("-DINCLUDE_EXAMPLE=On")

    if (
        args.libcxx
        or conan_profile in default_clang_profiles_posix.values()
        or conan_profile in default_clang_profiles_windows.values()
    ):
        command.append("-DUSE_LIB_CXX=On")

    result = subprocess.run(command, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- cmake config done")
    else:
        print("-- cmake failed")
        return

    print(
        f"-- Enviroment settings\n-- Compiler: {compiler_c}\n-- Build type: {build_type}\n-- Build system: {build_system}\n-- Conan profile: {conan_profile}"
    )
    if args.profile is None:
        print("-- Using a default profile")

    # Copy compile_commands.json to root for clangd
    source = Path(build_dir) / "compile_commands.json"

    if source.exists():
        print("-- compile_commands.json to copy")
        destination = script_dir / ".." / source.name
        _ = shutil.copy2(source, destination)
        source.unlink(missing_ok=True)
    else:
        print("-- compile_commands.json not found")

    if args.delete:
        os.remove(build_dir)


run_process()
