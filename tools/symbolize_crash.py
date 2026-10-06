#!/usr/bin/env python3
"""Puts function names and source lines on the crash lines of a Windows
build's debug.txt or halo.log (port/windows/src/win32_crash.c):

    python tools/symbolize_crash.py debug.txt path/to/halo.exe

halo.exe must be the build that crashed, with its halo.pdb beside it (a
release's halo-windows-<config>-symbols.zip has the PDB; the zip of the
same name without -symbols has halo.exe). The lines come from
llvm-symbolizer, which reads the PDB. The crash reports that reach Sentry
need none of this: Sentry symbolizes them itself.
"""

import argparse
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

# the crash lines with an address in them (win32_crash.c's crash_filter)
BASE = re.compile(r"crash: halo\.exe at (?:0x)?([0-9A-Fa-f]+)")
EXCEPTION = re.compile(r"crash: exception \w+ at (?:0x)?([0-9A-Fa-f]+)")
CALLER = re.compile(r"crash: called from (?:0x)?([0-9A-Fa-f]+)")


def image_layout(executable: Path) -> tuple:
    """the address halo.exe is linked at and its size in memory (its PE
    optional header's ImageBase and SizeOfImage)"""
    data = executable.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0" or struct.unpack_from("<H", data, pe + 24)[0] != 0x10B:
        sys.exit(f"{executable} is not a 32-bit Windows executable")
    return struct.unpack_from("<I", data, pe + 24 + 28)[0], struct.unpack_from("<I", data, pe + 24 + 56)[0]


def symbolize(symbolizer: str, executable: Path, addresses: list) -> dict:
    """{address: [lines of llvm-symbolizer's answer]}"""
    if not addresses:
        return {}
    output = subprocess.run(
        [symbolizer, f"--obj={executable}", "--inlines", "--functions=linkage", "--relative-address"]
        + [hex(address) for address in addresses],
        capture_output=True, text=True, check=True).stdout
    answers = output.strip("\n").split("\n\n")
    return {address: answer.splitlines() for address, answer in zip(addresses, answers)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("log", type=Path, help="debug.txt or halo.log")
    parser.add_argument("executable", type=Path, help="the halo.exe that crashed (halo.pdb beside it)")
    args = parser.parse_args()

    symbolizer = shutil.which("llvm-symbolizer") or shutil.which("llvm-addr2line")
    if not symbolizer:
        sys.exit("llvm-symbolizer is not in the PATH (it comes with LLVM)")
    if not args.executable.with_suffix(".pdb").is_file():
        print(f"warning: no {args.executable.with_suffix('.pdb').name} beside {args.executable}", file=sys.stderr)
    linked, size = image_layout(args.executable)

    lines = args.log.read_text(encoding="utf-8", errors="replace").splitlines()
    # each crash's addresses, relative to where halo.exe was in that run (a
    # line before "halo.exe at" is from a build too old to say: linked base);
    # a caller's is its return address, one past the call
    base = linked
    wanted = {}
    for index, line in enumerate(lines):
        match = BASE.search(line)
        if match:
            base = int(match.group(1), 16)
            continue
        match = EXCEPTION.search(line)
        if match:
            wanted[index] = int(match.group(1), 16) - base
            continue
        match = CALLER.search(line)
        if match:
            wanted[index] = int(match.group(1), 16) - base - 1
    if not wanted:
        print(f"{args.log} has no crash lines")
        return 1
    answers = symbolize(symbolizer, args.executable, sorted({offset for offset in wanted.values() if 0 <= offset < size}))

    for index, line in enumerate(lines):
        print(line)
        if index not in wanted:
            continue
        answer = answers.get(wanted[index])
        if not answer or answer[0] == "??":
            print("        (not in halo.exe)")
            continue
        for function, location in zip(answer[0::2], answer[1::2]):
            print(f"        {function}  {location}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
