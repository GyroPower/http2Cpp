import argparse
import subprocess

parser = argparse.ArgumentParser(description="Script to create and install a conan package with cmake chain")
parser.add_argument(
    "-pr",
    "--profile",
    help="Pass your profile from the profiles directory of conan or one defined outside with his path",
    default="conan-profiles/posix/clang_debugCpp23",
)

args = parser.parse_args()


# Build to use with ninja and clang
def run_process():
    profile = args.profile

    command = ["conan", "create", ".", "--build=missing", f"-pr={profile}"]

    result = subprocess.run(command, capture_output=False, text=True)

    if result.returncode == 0:
        print("-- conan create done")
    else:
        print("-- conan create failed")
        return


run_process()
