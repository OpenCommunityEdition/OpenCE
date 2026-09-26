#!/usr/bin/env python3
"""Expose the supplied Xbox SDK headers to the native Linux build.

The XDK ships headers with DOS-style mixed-case names (``WinNT.h``) that its
own sources and ours include in several spellings (``winnt.h``,
``PSHPACK1.H``). Linux file systems are case sensitive, so this writes an
overlay directory of symlinks under every spelling that is needed.

The XDK's C runtime headers (stdio.h, math.h, ...) are deliberately left out:
the Linux build uses the host C library, extended by the shims in
``port/linux/include``. C++ headers are left out because the game is C.
The supplied SDK is never modified.
"""

import argparse
import sys
from pathlib import Path

# MSVC C runtime headers that the host libc (plus port/linux/include) replaces.
CRT_HEADERS = {
    "assert.h", "conio.h", "crtdbg.h", "ctype.h", "direct.h", "dos.h",
    "eh.h", "errno.h", "excpt.h", "fcntl.h", "float.h", "fpieee.h", "io.h",
    "iso646.h", "limits.h", "locale.h", "malloc.h", "math.h", "mbctype.h",
    "mbstring.h", "memory.h", "new.h", "process.h", "search.h", "setjmp.h",
    "setjmpex.h", "share.h", "signal.h", "stdarg.h", "stddef.h", "stdio.h",
    "stdlib.h", "string.h", "tchar.h", "time.h", "varargs.h", "wchar.h",
    "wctype.h",
    # Compiler intrinsics headers: clang provides its own.
    "emmintrin.h", "mmintrin.h", "xmmintrin.h",
    # C++ iostream era headers.
    "fstream.h", "iomanip.h", "ios.h", "iostream.h", "istream.h",
    "ostream.h", "stdexcpt.h", "stdiostr.h", "stl.h", "streamb.h",
    "strstrea.h", "typeinfo.h", "use_ansi.h", "useoldio.h", "xlocinfo.h",
    "ymath.h", "yvals.h", "xmath.h",
}


def spellings(name: str) -> set:
    return {name, name.lower(), name.upper()}


def generate(sdk_include: Path, output: Path) -> int:
    if not sdk_include.is_dir():
        sys.exit(
            f"{sdk_include} not found: extract the XDK's xbox folder into the "
            "repository root (see README.md)"
        )
    output.mkdir(parents=True, exist_ok=True)
    wanted = {}
    for header in sorted(sdk_include.iterdir()):
        if not header.is_file() or "." not in header.name:
            continue
        if header.name.lower() in CRT_HEADERS:
            continue
        for spelling in spellings(header.name):
            wanted[spelling] = header.resolve()

    # Remove stale links (an SDK header that disappeared or was excluded).
    for existing in output.iterdir():
        if existing.name not in wanted and existing.is_symlink():
            existing.unlink()
    for spelling, target in wanted.items():
        link = output / spelling
        if link.is_symlink() and link.resolve() == target:
            continue
        if link.is_symlink() or link.exists():
            link.unlink()
        link.symlink_to(target)
    return len(wanted)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--sdk-include", type=Path, default=Path("xbox/include"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--stamp", type=Path, help="file to touch after a successful run"
    )
    args = parser.parse_args()
    generate(args.sdk_include, args.output)
    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.write_text("ok\n", encoding="utf-8")


if __name__ == "__main__":
    main()
