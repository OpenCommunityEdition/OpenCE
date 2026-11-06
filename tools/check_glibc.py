#!/usr/bin/env python3
"""Fails when a program or library needs a glibc newer than a given version.

    python3 tools/check_glibc.py 2.29 bin/halo bin/libSDL3.so.0

The AppImages use the glibc of the computer they run on, so the newest glibc
symbol version their files name (what `readelf -V` shows) is the oldest
computer they start on. port/linux/README.md ("AppImage and Flatpak
packages") gives these versions; this keeps them true: a new dependency or a
newer build computer would otherwise raise one without anyone noticing.
"""

import os
import re
import subprocess
import sys

VERSION = re.compile(r"\bGLIBC_(\d+(?:\.\d+)+)\b")


def newest(text):
    """The newest GLIBC_x.y in readelf -V output, as a tuple of ints, or ()"""
    found = [tuple(int(part) for part in match.split(".")) for match in VERSION.findall(text)]
    return max(found, default=())


def parse_limit(text):
    return tuple(int(part) for part in text.split("."))


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    limit = parse_limit(argv[0])
    failed = False
    for path in argv[1:]:
        output = subprocess.run([os.environ.get("READELF", "readelf"), "-V", path], capture_output=True, text=True, check=True).stdout
        version = newest(output)
        shown = ".".join(map(str, version)) if version else "none"
        if version and version > limit:
            print(f"{path}: needs glibc {shown}, more than {argv[0]}", file=sys.stderr)
            failed = True
        else:
            print(f"{path}: glibc {shown} (at most {argv[0]})")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
