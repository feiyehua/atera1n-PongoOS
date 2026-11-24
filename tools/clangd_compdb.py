# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2026 feiyehua

"""Generate clangd's compilation database from the Makefile's compiler flags."""

import json
from pathlib import Path
import subprocess
import sys


def compile_commands(directory, sources, command):
    if command[:3] == ["xcrun", "-sdk", "iphoneos"]:
        compiler = subprocess.check_output(
            [*command[:3], "--find", command[3]], text=True
        ).strip()
        command = [compiler, *command[4:]]

    return [
        {
            "directory": str(directory),
            "file": source,
            "arguments": [*command, "-c", source],
        }
        for source in sources
    ]


def main():
    groups = [[]]
    for argument in sys.argv[1:]:
        if argument == "--":
            groups.append([])
        else:
            groups[-1].append(argument)

    firmware_sources, firmware_command, host_sources, host_command = groups
    directory = Path.cwd()
    entries = compile_commands(directory, firmware_sources, firmware_command)
    entries.extend(compile_commands(directory, host_sources, host_command))
    (directory / "compile_commands.json").write_text(
        json.dumps(entries, indent=2) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
