"""Target-first ABI oracle for HUD's two-instruction get_return_eip helper.

Executable bytes come directly from each supplied COFF owner. The harness maps
two synthetic stop addresses to model distinct helper call return sites, but
executes no synthetic instructions. Two in-memory displacement mutations
validate the oracle; they are never written to an object or treated as source.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import unicorn as u
from unicorn.x86_const import (
	UC_X86_REG_EAX,
	UC_X86_REG_EBP,
	UC_X86_REG_EBX,
	UC_X86_REG_ECX,
	UC_X86_REG_EDI,
	UC_X86_REG_EFLAGS,
	UC_X86_REG_EIP,
	UC_X86_REG_EDX,
	UC_X86_REG_ESI,
	UC_X86_REG_ESP,
)


SCRIPT = Path(__file__).resolve()
ROOT = next(parent for parent in SCRIPT.parents if (parent / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


TARGET_DEFAULT = ROOT / "build/split/source/interface/hud_draw.obj"
OUTPUT_DEFAULT = ROOT / "scratch/hud-get-return-eip-runtime.json"
TARGET_SHA256 = "27d9adb322c237ea9114cd9a8e4d8b5457de517a7feef20ea6bd7158263b20db"
SYMBOL = "_get_return_eip"
EXPECTED_RAW = bytes.fromhex("8b4504c3" + "90" * 12)
EXPECTED_NORMALIZED = "6220b17edaa612ab088ef68837bec3423b5d5d13ee4e7351e3fc1dbc9e3e9a4a"
EXPECTED_FLAGS = 0x60501020
IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SCN_MEM_EXECUTE = 0x20000000

CODE = 0x100000
STOP_SITES = (0x110100, 0x110200)
STACK = 0x200000
STACK_SIZE = 0x4000
FRAME = STACK + 0x2800
HELPER_SP = FRAME - 0x80
DATA = 0x300000
SAVED_EBP = 0x0BADF00D
ARG_SENTINEL = 0xC001D00D

RETURN_VALUES = (
	0x00000000, 0x00000001, 0x0000FFFF, 0x7FFFFFFF,
	0x80000000, 0xFFFFFFFF, 0x01020304, 0x7FC12345,
	0xFFC54321, 0xDEADBEEF, 0xAAAAAAAA, 0x55555555,
)

PRESERVED = {
	UC_X86_REG_EBX: ("ebx", 0x2233BBCC),
	UC_X86_REG_ECX: ("ecx", 0x3344CCDD),
	UC_X86_REG_EDX: ("edx", 0x4455DDEE),
	UC_X86_REG_ESI: ("esi", 0x5566EEFF),
	UC_X86_REG_EDI: ("edi", 0x6677FF00),
}


def sha256(path: Path) -> str:
	return hashlib.sha256(path.read_bytes()).hexdigest()


def load_owner(path: Path) -> dict:
	obj = cc.load(path)
	matches = [
		symbol for symbol in obj["symbols"]
		if symbol["name"] == SYMBOL and int(symbol["section"]) > 0
	]
	assert len(matches) == 1, (path, matches)
	symbol = matches[0]
	section_number = int(symbol["section"])
	section = obj["sections"][section_number - 1]
	info = cc.section_info(obj, SYMBOL)
	raw = bytes(cc._section_bytes(obj, section))
	flags = int(section["flags"])
	selection = _section_aux_selection(obj, section_number)
	owner = {
		"path": str(path),
		"file_sha256": sha256(path),
		"size": int(info["size"]),
		"relocation_count": int(info["relocation_count"]),
		"normalized_sha256": info["normalized_sha256"],
		"raw_hex": raw.hex(),
		"value": int(symbol["value"]),
		"storage": int(symbol["storage"]),
		"symbol_type": int(symbol["type"]),
		"selection": selection,
		"flags": flags,
	}
	assert owner["size"] == 16 and owner["relocation_count"] == 0, owner
	assert owner["value"] == 0 and owner["storage"] == 2 and owner["symbol_type"] == 0x20, owner
	assert owner["normalized_sha256"] == EXPECTED_NORMALIZED and raw == EXPECTED_RAW, owner
	assert selection == 1 and flags == EXPECTED_FLAGS, owner
	required_flags = IMAGE_SCN_CNT_CODE | IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_MEM_EXECUTE
	assert flags & required_flags == required_flags, owner
	owner["raw"] = raw
	return owner


def run_case(code: bytes, outer_return: int, stop: int, eflags: int) -> dict:
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in ((CODE, 0x1000), (0x110000, 0x1000), (STACK, STACK_SIZE), (DATA, 0x1000)):
		machine.mem_map(address, size)
	machine.mem_write(CODE, code)
	stack_before = bytes((index * 37 + 11) & 0xFF for index in range(STACK_SIZE))
	data_before = bytes((index * 19 + 7) & 0xFF for index in range(0x1000))
	machine.mem_write(STACK, stack_before)
	machine.mem_write(DATA, data_before)
	machine.mem_write(HELPER_SP, struct.pack("<I", stop))
	machine.mem_write(FRAME, struct.pack("<III", SAVED_EBP, outer_return, ARG_SENTINEL))
	stack_before = bytes(machine.mem_read(STACK, STACK_SIZE))
	machine.reg_write(UC_X86_REG_EAX, outer_return ^ 0xA5A5A5A5)
	machine.reg_write(UC_X86_REG_EBP, FRAME)
	machine.reg_write(UC_X86_REG_ESP, HELPER_SP)
	for register, (_, value) in PRESERVED.items():
		machine.reg_write(register, value)
	machine.reg_write(UC_X86_REG_EFLAGS, eflags)

	trace: list[int] = []
	reads: list[list[int]] = []
	writes: list[list[int]] = []

	def code_hook(uc: u.Uc, address: int, _size: int, _user: object) -> None:
		trace.append(address)
		if address in STOP_SITES:
			uc.emu_stop()

	def memory_hook(_uc: u.Uc, access: int, address: int, size: int, value: int, _user: object) -> None:
		(writes if access == u.UC_MEM_WRITE else reads).append([address, size, value & 0xFFFFFFFF])

	machine.hook_add(u.UC_HOOK_CODE, code_hook)
	machine.hook_add(u.UC_HOOK_MEM_READ | u.UC_HOOK_MEM_WRITE, memory_hook)
	machine.emu_start(CODE, 0, count=8)

	errors = []
	checks = {
		"eax": (machine.reg_read(UC_X86_REG_EAX), outer_return),
		"ebp": (machine.reg_read(UC_X86_REG_EBP), FRAME),
		"esp": (machine.reg_read(UC_X86_REG_ESP), HELPER_SP + 4),
		"eip": (machine.reg_read(UC_X86_REG_EIP), stop),
		"eflags": (machine.reg_read(UC_X86_REG_EFLAGS), eflags),
	}
	for register, (name, expected) in PRESERVED.items():
		checks[name] = (machine.reg_read(register), expected)
	for name, (actual, expected) in checks.items():
		if actual != expected:
			errors.append(f"{name}: {actual:#x} != {expected:#x}")
	if trace != [CODE, CODE + 3, stop]:
		errors.append(f"trace: {trace!r}")
	if [[item[0], item[1]] for item in reads] != [[FRAME + 4, 4], [HELPER_SP, 4]]:
		errors.append(f"reads: {reads!r}")
	if writes:
		errors.append(f"writes: {writes!r}")
	if bytes(machine.mem_read(STACK, STACK_SIZE)) != stack_before:
		errors.append("checked stack changed")
	if bytes(machine.mem_read(DATA, 0x1000)) != data_before:
		errors.append("checked unrelated data changed")
	if bytes(machine.mem_read(CODE, len(code))) != code:
		errors.append("checked function body changed")
	return {"outer_return": outer_return, "helper_return_site": stop, "eflags": eflags, "errors": errors}


def evaluate(code: bytes) -> list[dict]:
	return [
		run_case(code, value, stop, 0x646 if site_index else 0x246)
		for site_index, stop in enumerate(STOP_SITES)
		for value in RETURN_VALUES
	]


def main() -> None:
	parser = argparse.ArgumentParser()
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--candidate", type=Path, action="append", default=[])
	parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
	args = parser.parse_args()

	target = load_owner(args.target.resolve())
	assert target["file_sha256"] == TARGET_SHA256, target
	owners = [target] + [load_owner(path.resolve()) for path in args.candidate]
	for owner in owners:
		cases = evaluate(owner.pop("raw"))
		assert not [case for case in cases if case["errors"]], (owner, cases)
		owner["cases"] = len(cases)
		owner["behavior_sha256"] = hashlib.sha256(json.dumps(cases, sort_keys=True).encode()).hexdigest()

	mutants = {}
	for name, displacement in (("ebp_plus_0", 0), ("ebp_plus_8", 8)):
		code = bytearray(EXPECTED_RAW)
		code[2] = displacement
		cases = evaluate(bytes(code))
		detected = sum(bool(case["errors"]) for case in cases)
		assert detected == len(cases), (name, cases)
		mutants[name] = {"cases": len(cases), "detected": detected}

	result = {
		"script_sha256": sha256(SCRIPT),
		"owners": owners,
		"cases_per_owner": len(STOP_SITES) * len(RETURN_VALUES),
		"all_real_owners_pass": True,
		"negative_controls": mutants,
		"contract": {
			"return_value": "full dword at caller EBP+4",
			"control_return": "dword at helper ESP, tested at two synthetic stop addresses",
			"executed_synthetic_instructions": 0,
			"machine_writes": 0,
			"preserved": [
				"EBP", "EBX", "ECX", "EDX", "ESI", "EDI", "EFLAGS",
				"checked stack", "checked unrelated data", "checked function body",
			],
		},
	}
	args.output.parent.mkdir(parents=True, exist_ok=True)
	args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
	print(json.dumps(result, indent=2))


if __name__ == "__main__":
	main()
