#!/usr/bin/env python3
"""Expose the supplied Xbox SDK headers to the Android build.

This is the Linux overlay (tools/linux_sdk_overlay.py) with one difference:
the Android guest is AArch64 code, where x86 inline assembly cannot compile,
and winnt.h defines three 64-bit shift helpers with ``__asm`` bodies. The
overlay replaces that header with a copy whose helpers are written in C.
Every other header is still a symlink to the unmodified SDK file, and the
supplied SDK itself is never modified.
"""

import argparse
import re
from pathlib import Path

from linux_sdk_overlay import generate, spellings

C_BODIES = {
    "Int64ShllMod32": "return Value << (ShiftCount & 31);",
    "Int64ShraMod32": "return Value >> (ShiftCount & 31);",
    "Int64ShrlMod32": "return Value >> (ShiftCount & 31);",
}


def patch_winnt(text: str) -> str:
    for name, body in C_BODIES.items():
        pattern = re.compile(
            r"(" + name + r"\s*\([^)]*\)\s*\{)\s*__asm\s*\{[^}]*\}\s*(\})",
            re.MULTILINE,
        )
        text, count = pattern.subn(lambda m: m.group(1) + "\n    " + body + "\n" + m.group(2), text)
        if count != 1:
            raise SystemExit(f"winnt.h: expected one definition of {name}, found {count}")
    if re.search(r"__asm\s*\{", text):
        raise SystemExit("winnt.h: unexpected inline assembly left after patching")
    return text


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--sdk-include", type=Path, default=Path("xbox/include"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stamp", type=Path, help="file to touch after a successful run")
    args = parser.parse_args()

    generate(args.sdk_include, args.output)
    source = next(p for p in args.sdk_include.iterdir() if p.name.lower() == "winnt.h")
    patched = patch_winnt(source.read_text(encoding="latin-1"))
    for spelling in spellings(source.name):
        link = args.output / spelling
        if link.is_symlink() or link.exists():
            link.unlink()
        link.write_text(patched, encoding="latin-1")
    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.write_text("ok\n", encoding="utf-8")


if __name__ == "__main__":
    main()
