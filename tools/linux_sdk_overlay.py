#!/usr/bin/env python3
"""Expose the supplied Xbox SDK headers to the native Linux build.

The XDK ships headers with DOS-style mixed-case names (``WinNT.h``) that its
own sources and ours include in several spellings (``winnt.h``,
``PSHPACK1.H``). Linux file systems are case sensitive, so this writes an
overlay directory of symlinks under every spelling that is needed.

The XDK's C runtime headers (stdio.h, math.h, ...) are deliberately left out:
the Linux build uses the host C library, extended by the shims in
``port/linux/include``. C++ headers are left out because the game is C.

winnt.h is the one header with inline assembly: three 64-bit shift helpers
written in x86 ``__asm``. The native ports have no assembly (and the Android
build is not x86), so the overlay holds a copy of it whose helpers are
written in C (``patch_winnt``, also used by tools/windows_sdk_overlay.py).
The supplied SDK is never modified.
"""

import argparse
import re
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


# winnt.h's shift helpers, in C: the shift count is taken modulo 32 as the
# x86 shld/shrd/sar instructions the SDK's versions use do
C_BODIES = {
    "Int64ShllMod32": "return Value << (ShiftCount & 31);",
    "Int64ShraMod32": "return Value >> (ShiftCount & 31);",
    "Int64ShrlMod32": "return Value >> (ShiftCount & 31);",
}


def patch_winnt(text: str) -> str:
    """winnt.h with its three ``__asm`` helpers written in C."""
    for name, body in C_BODIES.items():
        pattern = re.compile(
            r"(" + name + r"\s*\([^)]*\)\s*\{)\s*__asm\s*\{[^}]*\}\s*(\})",
            re.MULTILINE,
        )
        text, count = pattern.subn(lambda m: m.group(1) + "\n    " + body + "\n" + m.group(2), text)
        if count != 1:
            sys.exit(f"winnt.h: expected one definition of {name}, found {count}")
    if re.search(r"__asm\s*\{", text):
        sys.exit("winnt.h: unexpected inline assembly left after patching")
    return text


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

    # winnt.h: a patched copy instead of a link
    source = next((p for p in sdk_include.iterdir() if p.name.lower() == "winnt.h"), None)
    if source is not None:
        patched = patch_winnt(source.read_text(encoding="latin-1"))
        for spelling in spellings(source.name):
            link = output / spelling
            if link.is_symlink() or link.exists():
                link.unlink()
            link.write_text(patched, encoding="latin-1")
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
