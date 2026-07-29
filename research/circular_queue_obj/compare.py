#!/usr/bin/env python3
"""Concise strict comparison/disassembly for circular_queue.obj."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORKSPACE = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
try:
    import capstone
except ModuleNotFoundError:
    sys.path.insert(
        0, str(WORKSPACE / "research" / "scenario_cross_build_capstone")
    )

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from tools.coff_compare import (
    _section_bytes,
    load,
    relocation_infos_equal,
    section_info,
    section_infos_equal,
    symbol,
)


FUNCTIONS = (
    "_circular_queue_reset",
    "_circular_queue_validate",
    "_circular_queue_new",
    "_circular_queue_delete",
    "_circular_queue_size",
    "_circular_queue_free_space",
    "_circular_queue_queue_data",
    "_circular_queue_dequeue_data",
)


def raw_function(obj: dict, name: str) -> bytes:
    owner = symbol(obj, name)
    section = obj["sections"][owner["section"] - 1]
    return _section_bytes(obj, section)


def normalized_function(obj: dict, name: str) -> bytes:
    """Return the section bytes with relocation payloads zeroed."""
    raw = bytearray(raw_function(obj, name))
    for relocation in section_info(obj, name)["relocations"]:
        address = relocation["address"]
        raw[address : address + 4] = b"\0\0\0\0"
    return bytes(raw)


def first_difference(left: bytes, right: bytes) -> int | None:
    for index, (left_byte, right_byte) in enumerate(zip(left, right)):
        if left_byte != right_byte:
            return index
    if len(left) != len(right):
        return min(len(left), len(right))
    return None


def differing_byte_count(left: bytes, right: bytes) -> int:
    common = sum(left_byte != right_byte for left_byte, right_byte in zip(left, right))
    return common + abs(len(left) - len(right))


def concise_summary(target: dict, base: dict) -> None:
    name = "_circular_queue_dequeue_data"
    target_info = section_info(target, name)
    base_info = section_info(base, name)
    target_bytes = normalized_function(target, name)
    base_bytes = normalized_function(base, name)
    first = first_difference(target_bytes, base_bytes)
    first_relocation_difference = None
    for index, (left, right) in enumerate(
        zip(target_info["relocations"], base_info["relocations"])
    ):
        if left != right:
            first_relocation_difference = {
                "index": index,
                "target": left,
                "base": right,
            }
            break
    if (
        first_relocation_difference is None
        and len(target_info["relocations"]) != len(base_info["relocations"])
    ):
        first_relocation_difference = {
            "index": min(
                len(target_info["relocations"]), len(base_info["relocations"])
            ),
            "target_count": len(target_info["relocations"]),
            "base_count": len(base_info["relocations"]),
        }
    print(
        f"{name}: exact={section_infos_equal(target_info, base_info)} "
        f"T={target_info['size']}/{target_info['relocation_count']} "
        f"B={base_info['size']}/{base_info['relocation_count']} "
        f"relocs={relocation_infos_equal(target_info['relocations'], base_info['relocations'])} "
        f"Tsha={target_info['normalized_sha256']} "
        f"Bsha={base_info['normalized_sha256']} "
        f"first={None if first is None else hex(first)} "
        f"diff_bytes={differing_byte_count(target_bytes, base_bytes)} "
        f"first_reloc={first_relocation_difference}"
    )


def print_disassembly(label: str, obj: dict, name: str) -> None:
    info = section_info(obj, name)
    reloc_by_address = {
        relocation["address"]: relocation for relocation in info["relocations"]
    }
    engine = Cs(CS_ARCH_X86, CS_MODE_32)
    print(f"\n[{label}] {name}")
    for instruction in engine.disasm(raw_function(obj, name), 0):
        relocation = reloc_by_address.get(instruction.address + 1)
        suffix = ""
        if relocation is not None:
            suffix = f" ; RELOC {relocation['target']}"
        print(
            f"{instruction.address:04x}  {instruction.bytes.hex(' '):<24}"
            f" {instruction.mnemonic:<8} {instruction.op_str}{suffix}"
        )


def main() -> int:
    target_path = Path(sys.argv[1])
    base_path = Path(sys.argv[2])
    target = load(target_path)
    base = load(base_path)

    if "--summary" in sys.argv[3:]:
        concise_summary(target, base)
        return 0

    for name in FUNCTIONS:
        target_info = section_info(target, name)
        base_info = section_info(base, name)
        print(
            f"{name}: exact={section_infos_equal(target_info, base_info)} "
            f"T={target_info['size']}/{target_info['relocation_count']} "
            f"B={base_info['size']}/{base_info['relocation_count']} "
            f"relocs={relocation_infos_equal(target_info['relocations'], base_info['relocations'])} "
            f"Tsha={target_info['normalized_sha256'][:16]} "
            f"Bsha={base_info['normalized_sha256'][:16]}"
        )

    print_disassembly("TARGET", target, "_circular_queue_dequeue_data")
    print_disassembly("BASE", base, "_circular_queue_dequeue_data")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
