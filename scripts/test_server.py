#
# Needs h2load binary to do the testing.
#
# It's recommended to use Release build for better numbers of course and disable loggin to the console
# on your own code
#

import subprocess


def run_process():
    command_h2load = [
        "h2load",
        "-n",
        "1000000",
        "-c",
        "100",
        "-m",
        "30",
        "-t",
        "1",
        "https://localhost:8082/",
    ]

    result = subprocess.run(command_h2load, capture_output=False, text=True)

    if result.returncode == 0:
        print("Success")


run_process()
