#!/usr/bin/env python3
"""The engine's golden frames: what the tests last drew (build/engine/golden)
against what has been approved (assets/golden/engine). Both stay on this
machine: the frames are drawn from the user's maps.

    python tools/engine_golden.py                # each frame: new, same or changed
    python tools/engine_golden.py approve NAME   # approve frames (names or prefixes)
    python tools/engine_golden.py approve --all

A frame is approved by copying it; the tests then compare each new drawing
with it, within their tolerance.
"""

import argparse
import filecmp
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DRAWN = ROOT / "build/engine/golden"
APPROVED = ROOT / "assets/golden/engine"


def status(drawn, approved):
    """Each drawn frame's name and whether it is new, the same file as its
    approved frame, or changed."""
    rows = []
    for frame in sorted(drawn.glob("*.png")):
        kept = approved / frame.name
        if not kept.exists():
            rows.append((frame.name, "new"))
        elif filecmp.cmp(frame, kept, shallow=False):
            rows.append((frame.name, "same"))
        else:
            rows.append((frame.name, "changed"))
    return rows


def chosen(drawn, names, every):
    """The drawn frames named (by name or prefix), or all of them."""
    frames = sorted(drawn.glob("*.png"))
    if every:
        return frames
    picked = [frame for frame in frames if any(frame.name == name or frame.name.startswith(name) for name in names)]
    return picked


def approve(drawn, approved, names, every=False):
    """Copies the chosen drawn frames over the approved ones, and says
    which."""
    frames = chosen(drawn, names, every)
    approved.mkdir(parents=True, exist_ok=True)
    for frame in frames:
        shutil.copy2(frame, approved / frame.name)
    return [frame.name for frame in frames]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command")
    approving = commands.add_parser("approve", help="approve drawn frames")
    approving.add_argument("names", nargs="*", help="frame names or prefixes")
    approving.add_argument("--all", action="store_true", help="every drawn frame")
    arguments = parser.parse_args(argv)
    if arguments.command == "approve":
        if not arguments.names and not arguments.all:
            parser.error("approve needs frame names or --all")
        names = approve(DRAWN, APPROVED, arguments.names, arguments.all)
        if not names:
            print("no drawn frame matches", file=sys.stderr)
            return 1
        for name in names:
            print(f"approved {name}")
        return 0
    rows = status(DRAWN, APPROVED)
    if not rows:
        print(f"no frames drawn yet ({DRAWN.relative_to(ROOT)})")
    for name, state in rows:
        print(f"{state:8} {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
