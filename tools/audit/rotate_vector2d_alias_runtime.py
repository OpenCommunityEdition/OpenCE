#!/usr/bin/env python3
"""Bounded real-byte replay of the January rotate_vector2d alias contract.

The January target, the single grounded scalar-temporary correctness trial,
and the preserved pre-repair ordinary body all execute from their COFF bytes.
The old ordinary body is retained solely as a genuine negative control.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import unicorn as u
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

FUNCTION = "_rotate_vector2d"
OBJECTS = {
    "target": ROOT / (
        "scratch/path-obstacles-tangents-before-20260906/build/split/"
        "source/ai/path_obstacles.obj"
    ),
    "repaired": ROOT / (
        "scratch/path-obstacles-tangents-rotate-correctness-first-gate-"
        "20260906.obj"
    ),
    "old_negative": ROOT / "scratch/path-obstacles-tangents-first-gate-20260906.obj",
}
PINS = {
    "target": "f438f10d52492b5dd4ae35691a50cb10240d30ad01f61a81be57542b0e224a29",
    "repaired": "dba2594c50f311dc64f68d7e351f44d2114b82647278a40906c448287a965cd1",
    "old_negative": "c9b4cfac532f7fcb018d980a6cac8da1ee00f17e0e6c0e85717a4613917e6029",
}
TARGET_NORMALIZED = "9a9864975806d5697fb568f3ea1aadee7f934119b5ebcd0b4aad5edb307aec3a"
OLD_NORMALIZED = "58f40bb118ff928d858eaa95d3b90a4f661add7e86967e059d5a5bf5693e6310"

CODE, STOP, DATA, STACK = 0x100000, 0x101000, 0x200000, 0x300000
V, RESULT, SP = DATA + 0x100, DATA + 0x200, STACK + 0x2000
FPCWS = (0x027F, 0x037F)
SAVED = {
    UC_X86_REG_EBP: 0x12345678,
    UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A,
    UC_X86_REG_EDI: 0x456789AB,
}

CASES = [
    ("generic", 0x3F800000, 0x40000000, 0x3F19999A, 0x3F4CCCCD),
    ("identity", 0xBF400000, 0x40A00000, 0x00000000, 0x3F800000),
    ("quarter_turn", 0x3F800000, 0x40000000, 0x3F800000, 0x00000000),
    ("negative_quarter", 0x3F800000, 0x40000000, 0xBF800000, 0x00000000),
    ("signed_zero", 0x80000000, 0x00000000, 0x80000000, 0x3F800000),
    ("subnormal", 0x00000001, 0x80000001, 0x3F000000, 0x3F000000),
    ("large_finite", 0x7F7FFFFF, 0xFF7FFFFF, 0x3F3504F3, 0x3F3504F3),
    ("qnan_vector", 0x7FC12345, 0x3F800000, 0x3F000000, 0x3F000000),
    ("qnan_sine", 0x3F800000, 0x40000000, 0x7FC54321, 0x3F800000),
    ("positive_infinity", 0x7F800000, 0x3F800000, 0x3F000000, 0x3F000000),
]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_body(role: str) -> tuple[bytes, dict]:
    path = OBJECTS[role]
    actual = digest(path)
    if actual != PINS[role]:
        raise AssertionError(f"{role} SHA mismatch: {actual} != {PINS[role]}")
    obj = cc.load(path)
    symbol = cc.symbol(obj, FUNCTION)
    section = obj["sections"][symbol["section"] - 1]
    info = cc.section_info_by_number(obj, symbol["section"])
    expected = OLD_NORMALIZED if role == "old_negative" else TARGET_NORMALIZED
    if info["size"] != 48 or info["relocation_count"] != 0:
        raise AssertionError(f"unexpected {role} section shape: {info}")
    if info["normalized_sha256"] != expected:
        raise AssertionError(
            f"unexpected {role} normalized SHA: {info['normalized_sha256']} != {expected}"
        )
    body = bytes(obj["data"][section["raw"] : section["raw"] + section["size"]])
    return body, info


def run(body: bytes, bits: tuple[int, int, int, int], control: int, alias: bool) -> dict:
    vx, vy, sine, cosine = bits
    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    cpu.mem_map(CODE, 0x2000)
    cpu.mem_map(DATA, 0x1000)
    cpu.mem_map(STACK, 0x4000)
    cpu.mem_write(CODE, body)
    cpu.mem_write(STOP, b"\xF4")
    data_before = bytes(((index * 29 + 7) & 0xFF) for index in range(0x1000))
    stack_before = bytes(((index * 17 + 3) & 0xFF) for index in range(0x4000))
    cpu.mem_write(DATA, data_before)
    cpu.mem_write(STACK, stack_before)
    vector_bytes = struct.pack("<II", vx, vy)
    cpu.mem_write(V, vector_bytes)
    result = V if alias else RESULT
    frame = struct.pack("<IIIII", STOP, V, sine, cosine, result)
    cpu.mem_write(SP, frame)

    writes = []
    def on_code(uc, address, size, unused):
        if address == STOP:
            uc.emu_stop()
        elif not (CODE <= address and address + size <= CODE + len(body)):
            raise AssertionError(f"code boundary 0x{address:x}+{size}")

    def on_write(uc, access, address, size, value, unused):
        in_stack = STACK <= address and address + size <= SP
        in_result = result <= address and address + size <= result + 8
        if not (in_stack or in_result):
            raise AssertionError(f"write boundary 0x{address:x}+{size}")
        writes.append([address - result if in_result else address - SP, size])

    hooks = [
        cpu.hook_add(u.UC_HOOK_CODE, on_code),
        cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write),
    ]
    cpu.reg_write(UC_X86_REG_ESP, SP)
    cpu.reg_write(UC_X86_REG_EAX, 0xA5A51234)
    cpu.reg_write(UC_X86_REG_ECX, 0x56789ABC)
    cpu.reg_write(UC_X86_REG_EDX, 0x6789ABCD)
    cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
    cpu.reg_write(UC_X86_REG_FPCW, control)
    cpu.reg_write(UC_X86_REG_FPSW, 0)
    cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
    initial_tag = cpu.reg_read(UC_X86_REG_FPTAG)
    for register, value in SAVED.items():
        cpu.reg_write(register, value)
    try:
        cpu.emu_start(CODE, STOP, count=1000)
    finally:
        for hook in hooks:
            cpu.hook_del(hook)

    if cpu.reg_read(UC_X86_REG_EIP) != STOP:
        raise AssertionError("function did not return to STOP")
    if cpu.reg_read(UC_X86_REG_ESP) != SP + 4:
        raise AssertionError("ESP contract failed")
    if cpu.reg_read(UC_X86_REG_EAX) != result:
        raise AssertionError("result-pointer EAX contract failed")
    for register, value in SAVED.items():
        if cpu.reg_read(register) != value:
            raise AssertionError(f"nonvolatile register {register} changed")
    if cpu.reg_read(UC_X86_REG_FPCW) != control:
        raise AssertionError("FPCW changed")
    if cpu.reg_read(UC_X86_REG_FPTAG) != initial_tag:
        raise AssertionError("x87 tag changed")
    if cpu.reg_read(UC_X86_REG_FPSW) & 0x3800:
        raise AssertionError("x87 stack not empty")
    if cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400:
        raise AssertionError("direction flag set")
    if bytes(cpu.mem_read(SP + 4, 16)) != frame[4:]:
        raise AssertionError("arguments mutated")

    output = bytes(cpu.mem_read(result, 8))
    expected_data = bytearray(data_before)
    expected_data[V - DATA : V - DATA + 8] = vector_bytes
    expected_data[result - DATA : result - DATA + 8] = output
    if bytes(cpu.mem_read(DATA, 0x1000)) != bytes(expected_data):
        raise AssertionError("data guard or input mutation")
    actual_stack = bytes(cpu.mem_read(STACK, 0x4000))
    start = SP - STACK
    if actual_stack[: start - 4] != stack_before[: start - 4]:
        raise AssertionError("lower stack guard changed")
    if actual_stack[start + len(frame) :] != stack_before[start + len(frame) :]:
        raise AssertionError("upper stack guard changed")
    return {"output_hex": output.hex(), "writes": writes}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default=str(ROOT / "scratch/rotate-vector2d-alias-runtime-20260906.json"),
    )
    args = parser.parse_args()
    bodies = {}
    infos = {}
    for role in OBJECTS:
        bodies[role], infos[role] = load_body(role)
    if bodies["target"] != bodies["repaired"]:
        raise AssertionError("repaired bytes are not exactly the January target bytes")
    if bodies["target"] == bodies["old_negative"]:
        raise AssertionError("old negative control unexpectedly equals target")

    rows = []
    old_alias_mismatches = []
    executions = 0
    for control in FPCWS:
        for name, *bits_list in CASES:
            bits = tuple(bits_list)
            results = {}
            for role, body in bodies.items():
                results[role] = {
                    "nonalias": run(body, bits, control, False),
                    "in_place": run(body, bits, control, True),
                }
                executions += 2
            target_same = (
                results["target"]["nonalias"]["output_hex"]
                == results["target"]["in_place"]["output_hex"]
            )
            repaired_same = (
                results["repaired"]["nonalias"]["output_hex"]
                == results["repaired"]["in_place"]["output_hex"]
            )
            target_repaired = results["target"] == results["repaired"]
            old_same = (
                results["old_negative"]["nonalias"]["output_hex"]
                == results["old_negative"]["in_place"]["output_hex"]
            )
            if not (target_same and repaired_same and target_repaired):
                raise AssertionError(f"positive alias contract failed: {name}/0x{control:04x}")
            if not old_same:
                old_alias_mismatches.append(f"{name}/0x{control:04x}")
            rows.append({
                "case": name,
                "fpcw": f"0x{control:04x}",
                "input_hex": struct.pack("<IIII", *bits).hex(),
                "target_repaired_output_hex": results["target"]["nonalias"]["output_hex"],
                "target_in_place_equals_nonalias": target_same,
                "repaired_in_place_equals_nonalias": repaired_same,
                "target_equals_repaired_full_trace": target_repaired,
                "old_negative_nonalias_hex": results["old_negative"]["nonalias"]["output_hex"],
                "old_negative_in_place_hex": results["old_negative"]["in_place"]["output_hex"],
                "old_negative_in_place_equals_nonalias": old_same,
            })
    required = {f"generic/0x{control:04x}" for control in FPCWS}
    if not required.issubset(old_alias_mismatches):
        raise AssertionError("genuine old-byte negative control did not reject generic aliasing")

    report = {
        "status": "PASS",
        "scope": "real rotate_vector2d bytes only; bounded inputs and two explicit FPCW modes",
        "objects": {
            role: {
                "path": str(path.relative_to(ROOT)).replace("\\", "/"),
                "sha256": PINS[role],
                "size": infos[role]["size"],
                "relocation_count": infos[role]["relocation_count"],
                "normalized_sha256": infos[role]["normalized_sha256"],
            }
            for role, path in OBJECTS.items()
        },
        "target_and_repaired_section_bytes_identical": True,
        "cases": len(CASES),
        "fpcw_modes": [f"0x{control:04x}" for control in FPCWS],
        "executions": executions,
        "old_negative_alias_mismatch_count": len(old_alias_mismatches),
        "old_negative_alias_mismatches": old_alias_mismatches,
        "checks": [
            "exact output bytes",
            "in-place output equals nonalias output",
            "write boundaries and input/data/stack guards",
            "EAX result pointer and literal return ESP",
            "nonvolatile registers, direction flag, FPCW and empty x87 stack",
            "genuine pre-repair object-byte negative control",
        ],
        "rows": rows,
        "limitation": "Finite replay; it proves the repaired section is byte-identical to January and demonstrates the old body's alias failure, not an exhaustive IEEE-754 proof.",
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    }
    output = Path(args.output)
    if not output.is_absolute():
        output = ROOT / output
    output = output.resolve()
    if not output.is_relative_to((ROOT / "scratch").resolve()):
        raise ValueError("report output must stay under scratch")
    if output.exists():
        raise FileExistsError(f"refusing to overwrite report: {output}")
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "status": report["status"],
        "output": str(output.relative_to(ROOT)).replace("\\", "/"),
        "executions": executions,
        "old_negative_alias_mismatch_count": len(old_alias_mismatches),
        "script_sha256": report["script_sha256"],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
