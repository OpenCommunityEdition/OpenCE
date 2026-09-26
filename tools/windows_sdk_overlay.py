#!/usr/bin/env python3
"""Expose the supplied Xbox SDK headers to the native Windows build.

The Xbox SDK and the Windows SDK both have winnt.h, winbase.h, dsound.h and
more, with different contents; the game needs the Xbox ones, so they must
come first in the include path. The Xbox SDK's C runtime headers (stdio.h,
math.h, ...) must not: the Windows build uses the Windows C runtime. This
copies every other Xbox SDK header into a directory of its own (the same
selection as the Linux overlay, tools/linux_sdk_overlay.py, including its C
version of winnt.h's assembly helpers). The supplied SDK is never modified.
"""

import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from linux_sdk_overlay import CRT_HEADERS, patch_winnt  # noqa: E402


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
        wanted[header.name.lower()] = header
    for existing in output.iterdir():
        if existing.name.lower() not in wanted:
            existing.unlink()
    for name, source in wanted.items():
        target = output / source.name
        if name == "winnt.h":
            # its shift helpers in C instead of x86 assembly (linux_sdk_overlay.py)
            target.write_text(patch_winnt(source.read_text(encoding="latin-1")), encoding="latin-1")
        elif not target.is_file() or target.stat().st_mtime < source.stat().st_mtime:
            shutil.copy2(source, target)
    return len(wanted)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--sdk-include", type=Path, default=Path("xbox/include"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stamp", type=Path, help="file to touch after a successful run")
    args = parser.parse_args()
    generate(args.sdk_include, args.output)
    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.write_text("ok\n", encoding="utf-8")


if __name__ == "__main__":
    main()
