#!/usr/bin/env python3
"""Read-only disassembly helper for the October 2001 debug XBE."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parents[4]
sys.path.insert(
    0, str(WORKSPACE / "research" / "scenario_cross_build_capstone")
)
sys.path.insert(
    0, str(WORKSPACE / "research" / "rasterizer_alternate_january")
)

from capstone import CS_ARCH_X86, CS_MODE_32, Cs
from xbe_scan import Xbe


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("xbe", type=Path)
    parser.add_argument("va", type=lambda value: int(value, 0))
    parser.add_argument("size", type=lambda value: int(value, 0))
    args = parser.parse_args()

    image = Xbe(args.xbe)
    engine = Cs(CS_ARCH_X86, CS_MODE_32)
    for instruction in engine.disasm(image.read_va(args.va, args.size), args.va):
        print(
            f"{instruction.address:08x}  {instruction.bytes.hex(' '):<24} "
            f"{instruction.mnemonic:<8} {instruction.op_str}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
