#!/usr/bin/env python3
"""Run an MSVC compile and make its /showIncludes paths usable by ninja.

On a Linux host the compiler (through wibo or wine) reports headers as
Windows paths: "source/cseries\\cseries.h", "xbox/include\\POPPACK.H", or absolute
"z:\\home\\...\\main.h" in lower case.
Ninja's deps=msvc records them verbatim, finds no such files on a
case-sensitive file system with '/' separators, and so rebuilds every object
on every run. This rewrites each "Note: including file:" line to the real
on-disk path; everything else passes through unchanged.

Usage: msvc_deps_filter.py <compiler command...>
"""

import os
import subprocess
import sys
from functools import lru_cache
from typing import Dict, Optional

INCLUDE_PREFIX = "Note: including file:"


@lru_cache(maxsize=None)
def directory_entries(directory: str) -> Dict[str, str]:
    try:
        return {name.lower(): name for name in os.listdir(directory or ".")}
    except OSError:
        return {}


@lru_cache(maxsize=None)
def resolve(path: str) -> Optional[str]:
    """Return the on-disk spelling of a Windows-style path, or None."""
    path = path.replace("\\", "/")
    # wibo and wine expose the host root as drive Z:
    if len(path) > 2 and path[1] == ":" and path[0] in "zZ" and path[2] == "/":
        path = path[2:]
    if os.path.exists(path):
        return relative(path)
    absolute = path.startswith("/")
    resolved = "/" if absolute else ""
    for component in (part for part in path.split("/") if part):
        if component in (".", ".."):
            resolved = os.path.join(resolved, component)
            continue
        actual = directory_entries(resolved).get(component.lower())
        if actual is None:
            return None
        resolved = os.path.join(resolved, actual)
    return relative(resolved)


def relative(path: str) -> str:
    """Keep paths inside the checkout relative, like the others ninja sees."""
    if os.path.isabs(path):
        candidate = os.path.relpath(path)
        if not candidate.startswith(".."):
            return candidate
    return path


def filter_line(line: str) -> str:
    if not line.startswith(INCLUDE_PREFIX):
        return line
    reported = line[len(INCLUDE_PREFIX):].strip()
    actual = resolve(reported)
    if actual is None:
        return line
    return f"{INCLUDE_PREFIX} {actual}\n"


def main() -> int:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    process = subprocess.Popen(
        sys.argv[1:],
        stdout=subprocess.PIPE,
        encoding="latin-1",
        errors="replace",
    )
    assert process.stdout is not None
    for line in process.stdout:
        sys.stdout.write(filter_line(line))
    return process.wait()


if __name__ == "__main__":
    sys.exit(main())
