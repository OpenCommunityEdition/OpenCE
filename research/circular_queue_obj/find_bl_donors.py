#!/usr/bin/env python3
"""Find exact functions that preserve a false byte in BL across a long path."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import _section_bytes, load, section_info, section_infos_equal


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    split_root = root / "build" / "split"
    base_root = root / "build" / "base"
    hits = []
    for target_path in split_root.rglob("*.obj"):
        relative = target_path.relative_to(split_root)
        base_path = base_root / relative
        if not base_path.exists():
            continue
        try:
            target = load(target_path)
            base = load(base_path)
        except Exception:
            continue
        base_names = {item["name"] for item in base["symbols"]}
        for owner in target["symbols"]:
            name = owner["name"]
            if owner["section"] <= 0 or owner["type"] != 32 or name not in base_names:
                continue
            section = target["sections"][owner["section"] - 1]
            code = _section_bytes(target, section)
            # Match the precise lifetime motif: byte-zero BL near entry and
            # byte return from BL in a tail. Keep the window broad enough for
            # assert-heavy functions.
            motif = None
            for register, zero_bytes, return_bytes in (
                ("CL", b"\x32\xc9", b"\x8a\xc1"),
                ("DL", b"\x32\xd2", b"\x8a\xc2"),
                ("BL", b"\x32\xdb", b"\x8a\xc3"),
            ):
                zero_at = code[:0x80].find(zero_bytes)
                return_at = code.rfind(return_bytes)
                if zero_at >= 0 and return_at > zero_at:
                    motif = (register, zero_at, return_at)
                    break
            if motif is None:
                continue
            register, zero_at, return_at = motif
            try:
                exact = section_infos_equal(
                    section_info(target, name), section_info(base, name)
                )
            except Exception:
                exact = False
            hits.append(
                (
                    exact,
                    str(relative),
                    name,
                    len(code),
                    register,
                    zero_at,
                    return_at,
                )
            )
    for exact, relative, name, size, register, zero_at, return_at in sorted(
        hits, key=lambda item: (not item[0], item[1], item[2])
    ):
        print(
            f"exact={exact!s:<5} size={size:#06x} reg={register} zero={zero_at:#04x} "
            f"return={return_at:#04x} {relative}::{name}"
        )
    print(f"total={len(hits)} exact={sum(item[0] for item in hits)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
