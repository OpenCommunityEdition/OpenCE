"""Target-first runtime oracle for scripted_hud_show_crosshair.

The executable function bytes come directly from each supplied COFF object.
The sole DIR32 relocation is patched in memory to a synthetic pointer owner;
no synthetic implementation or external API stub is executed.
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


TARGET_DEFAULT = ROOT / "build/split/source/interface/hud_weapon.obj"
OUTPUT_DEFAULT = ROOT / "scratch/hud-weapon-crosshair-runtime.json"
KNOWN_TARGET_SHA256 = {
	# Frozen January object before and after the in-place semantic/static rename.
	"760258800d59f9e12750a64cbe63479c7b1ee5a7e577f920328ca46e4fedfbb2",
	"5508c29cf4502ec0de18e2b5ba897796db5c87f4f8072ea86d24529d2899b401",
}
SYMBOL = "_scripted_hud_show_crosshair"
GLOBAL_NAMES = {"_bss_00453ac4", "_weapon_hud_globals"}
EXPECTED_RAW = bytes.fromhex(
	"558bec8a450884c0a1000000008b88e0010000740b83c9018988e00100005dc3"
	"83e1fe8988e00100005dc39090909090"
)
EXPECTED_NORMALIZED = "f106592d3b4916e99cc1f27585794057797128e64f6382b7740801c223f2748f"
EXPECTED_CODE_FLAGS = 0x60501020
EXPECTED_BSS_FLAGS = 0xC0300080
IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SCN_MEM_EXECUTE = 0x20000000

CODE = 0x100000
STOP = 0x110100
STACK = 0x200000
STACK_SIZE = 0x4000
SP = STACK + 0x2800
OWNER_SLOT = 0x300000
GLOBALS_PAGE = 0x400000
GLOBALS_SIZE = 0x1000
GLOBALS = GLOBALS_PAGE + 0x300
SCRIPT_FLAGS = GLOBALS + 0x1E0
SAVED_EBP = 0x0BADF00D

ARGUMENTS = (
	0x00000000,
	0x00000001,
	0x00000002,
	0x000000FF,
	0x00000100,
	0x00000101,
	0x12345600,
	0x12345680,
	0x7FFFFF00,
	0x80000000,
	0x80000001,
	0xFFFFFF00,
	0xFFFFFFFF,
)
FLAG_SEEDS = (
	0x00000000,
	0x00000001,
	0x00000002,
	0x00000003,
	0xFFFFFFFF,
	0xFFFFFFFE,
	0xA5A5A5A5,
	0x5A5A5A5A,
	0x80000000,
	0x7FFFFFFF,
	0x01020304,
	0x89ABCDEF,
)
PRESERVED = {
	UC_X86_REG_EBX: ("ebx", 0x2233BBCC),
	UC_X86_REG_ESI: ("esi", 0x5566EEFF),
	UC_X86_REG_EDI: ("edi", 0x6677FF00),
}


def sha256(path: Path) -> str:
	return hashlib.sha256(path.read_bytes()).hexdigest()


def load_owner(path: Path) -> dict:
	file_sha256 = sha256(path)
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
	assert int(info["size"]) == 48 and int(info["relocation_count"]) == 1, info
	assert info["normalized_sha256"] == EXPECTED_NORMALIZED, info
	assert raw == EXPECTED_RAW, raw.hex()
	assert int(symbol["value"]) == 0, symbol
	assert int(symbol["storage"]) == 2 and int(symbol["type"]) == 0x20, symbol
	assert selection == 1 and flags == EXPECTED_CODE_FLAGS, (selection, flags)
	required = IMAGE_SCN_CNT_CODE | IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_MEM_EXECUTE
	assert flags & required == required, flags
	relocation = info["relocations"][0]
	assert int(relocation["address"]) == 9, relocation
	assert int(relocation["type"]) == cc.IMAGE_REL_I386_DIR32, relocation
	resolved = relocation["target"]
	assert resolved[0:2] == ["defined-noncode", ".bss"], resolved
	assert resolved[2] in GLOBAL_NAMES and int(resolved[3]) == 0, resolved
	global_name = resolved[2]
	global_symbols = [
		item for item in obj["symbols"]
		if item["name"] == global_name and int(item["section"]) > 0
	]
	assert len(global_symbols) == 1, global_symbols
	global_symbol = global_symbols[0]
	global_section = obj["sections"][int(global_symbol["section"]) - 1]
	expected_storage = 2 if global_name == "_bss_00453ac4" else 3
	assert int(global_symbol["value"]) == 0, global_symbol
	assert int(global_symbol["storage"]) == expected_storage, global_symbol
	assert int(global_symbol["type"]) == 0, global_symbol
	assert global_section["name"] == ".bss", global_section
	assert int(global_section["size"]) == 4, global_section
	assert int(global_section["flags"]) == EXPECTED_BSS_FLAGS, global_section
	assert int(global_section["reloc_count"]) == 0, global_section
	assert int(global_section["flags"]) & IMAGE_SCN_CNT_UNINITIALIZED_DATA, global_section
	assert not int(global_section["flags"]) & IMAGE_SCN_CNT_INITIALIZED_DATA, global_section
	bss_raw = bytes(cc._section_bytes(obj, global_section))
	assert bss_raw == b"\0" * 4, bss_raw
	# The January splitter materializes four zero bytes for its synthetic BSS
	# section.  Ordinary compiler objects must leave this owner raw-less.
	bss_raw_present = bool(int(global_section["raw"]))
	if file_sha256 not in KNOWN_TARGET_SHA256:
		assert not bss_raw_present, global_section
	return {
		"path": str(path),
		"file_sha256": file_sha256,
		"size": int(info["size"]),
		"relocation_count": int(info["relocation_count"]),
		"normalized_sha256": info["normalized_sha256"],
		"raw_sha256": hashlib.sha256(raw).hexdigest(),
		"selection": selection,
		"code_flags": flags,
		"global_name": global_name,
		"global_storage": int(global_symbol["storage"]),
		"global_section_size": int(global_section["size"]),
		"global_section_flags": int(global_section["flags"]),
		"global_relocation_count": int(global_section["reloc_count"]),
		"global_raw_present": bss_raw_present,
		"global_raw_hex": bss_raw.hex(),
		"raw": raw,
	}


def run_case(code: bytes, argument: int, initial_flags: int) -> dict:
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x1000),
		(0x110000, 0x1000),
		(STACK, STACK_SIZE),
		(OWNER_SLOT, 0x1000),
		(GLOBALS_PAGE, GLOBALS_SIZE),
	):
		machine.mem_map(address, size)
	patched = bytearray(code)
	struct.pack_into("<I", patched, 9, OWNER_SLOT)
	machine.mem_write(CODE, bytes(patched))

	stack = bytearray((index * 37 + 11) & 0xFF for index in range(STACK_SIZE))
	struct.pack_into("<II", stack, SP - STACK, STOP, argument & 0xFFFFFFFF)
	machine.mem_write(STACK, bytes(stack))
	pointer_page = bytearray((index * 13 + 5) & 0xFF for index in range(0x1000))
	struct.pack_into("<I", pointer_page, 0, GLOBALS)
	machine.mem_write(OWNER_SLOT, bytes(pointer_page))
	global_page = bytearray((index * 29 + 17) & 0xFF for index in range(GLOBALS_SIZE))
	struct.pack_into("<I", global_page, SCRIPT_FLAGS - GLOBALS_PAGE, initial_flags)
	machine.mem_write(GLOBALS_PAGE, bytes(global_page))

	machine.reg_write(UC_X86_REG_EAX, 0x1122AABB)
	machine.reg_write(UC_X86_REG_ECX, 0x3344CCDD)
	machine.reg_write(UC_X86_REG_EDX, 0x4455DDEE)
	machine.reg_write(UC_X86_REG_EBP, SAVED_EBP)
	machine.reg_write(UC_X86_REG_ESP, SP)
	initial_df = 1 if (argument ^ initial_flags) & 1 else 0
	machine.reg_write(UC_X86_REG_EFLAGS, 0x202 | (initial_df << 10))
	for register, (_, value) in PRESERVED.items():
		machine.reg_write(register, value)

	trace: list[int] = []
	writes: list[list[int]] = []

	def code_hook(uc: u.Uc, address: int, _size: int, _user: object) -> None:
		trace.append(address)
		if address == STOP:
			uc.emu_stop()

	def write_hook(_uc: u.Uc, _access: int, address: int, size: int, value: int, _user: object) -> None:
		writes.append([address, size, value & 0xFFFFFFFF])

	machine.hook_add(u.UC_HOOK_CODE, code_hook)
	machine.hook_add(u.UC_HOOK_MEM_WRITE, write_hook)
	machine.emu_start(CODE, 0, count=32)

	expected_flags = (
		initial_flags | 1 if argument & 0xFF else initial_flags & 0xFFFFFFFE
	) & 0xFFFFFFFF
	expected_global = bytearray(global_page)
	struct.pack_into("<I", expected_global, SCRIPT_FLAGS - GLOBALS_PAGE, expected_flags)
	expected_stack = bytearray(stack)
	struct.pack_into("<I", expected_stack, SP - STACK - 4, SAVED_EBP)
	errors = []
	checks = {
		"ebp": (machine.reg_read(UC_X86_REG_EBP), SAVED_EBP),
		"esp": (machine.reg_read(UC_X86_REG_ESP), SP + 4),
		"eip": (machine.reg_read(UC_X86_REG_EIP), STOP),
	}
	for register, (name, expected) in PRESERVED.items():
		checks[name] = (machine.reg_read(register), expected)
	for name, (actual, expected) in checks.items():
		if actual != expected:
			errors.append(f"{name}: {actual:#x} != {expected:#x}")
	actual_flags = struct.unpack(
		"<I", machine.mem_read(SCRIPT_FLAGS, 4)
	)[0]
	if actual_flags != expected_flags:
		errors.append(f"flags: {actual_flags:#x} != {expected_flags:#x}")
	actual_df = 1 if machine.reg_read(UC_X86_REG_EFLAGS) & 0x400 else 0
	if actual_df != initial_df:
		errors.append(f"df: {actual_df} != {initial_df}")
	if bytes(machine.mem_read(GLOBALS_PAGE, GLOBALS_SIZE)) != bytes(expected_global):
		errors.append("globals or guard bytes changed outside script_flags")
	if bytes(machine.mem_read(OWNER_SLOT, 0x1000)) != bytes(pointer_page):
		errors.append("pointer owner page changed")
	if bytes(machine.mem_read(STACK, STACK_SIZE)) != bytes(expected_stack):
		errors.append("stack changed outside the normal saved-EBP slot")
	if bytes(machine.mem_read(CODE, len(patched))) != bytes(patched):
		errors.append("function code changed")
	write_shapes = [[address, size] for address, size, _value in writes]
	if write_shapes != [[SP - 4, 4], [SCRIPT_FLAGS, 4]]:
		errors.append(f"writes: {writes!r}")
	return {
		"argument": argument,
		"argument_low_byte": argument & 0xFF,
		"initial_flags": initial_flags,
		"expected_flags": expected_flags,
		"actual_flags": actual_flags,
		"initial_df": initial_df,
		"actual_df": actual_df,
		"trace": trace,
		"writes": writes,
		"errors": errors,
	}


def evaluate(code: bytes) -> list[dict]:
	return [
		run_case(code, argument, initial_flags)
		for argument in ARGUMENTS
		for initial_flags in FLAG_SEEDS
	]


def main() -> None:
	parser = argparse.ArgumentParser()
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--candidate", type=Path, action="append", default=[])
	parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
	args = parser.parse_args()

	target = load_owner(args.target.resolve())
	assert target["file_sha256"] in KNOWN_TARGET_SHA256, target
	owners = [target] + [load_owner(path.resolve()) for path in args.candidate]
	for owner in owners:
		cases = evaluate(owner.pop("raw"))
		failures = [case for case in cases if case["errors"]]
		assert not failures, (owner, failures[:3])
		owner["cases"] = len(cases)
		owner["behavior_sha256"] = hashlib.sha256(
			json.dumps(cases, sort_keys=True).encode("utf-8")
		).hexdigest()

	mutants = {}
	for name, offset, value in (
		("read_argument_second_byte", 5, 9),
		("set_bit_one_instead_of_zero", 0x17, 2),
		("clear_bits_zero_and_one", 0x22, 0xFC),
	):
		code = bytearray(EXPECTED_RAW)
		code[offset] = value
		cases = evaluate(bytes(code))
		detected = sum(bool(case["errors"]) for case in cases)
		assert detected > 0, (name, cases)
		mutants[name] = {"cases": len(cases), "detected": detected}

	behavior_hashes = {owner["behavior_sha256"] for owner in owners}
	assert len(behavior_hashes) == 1, owners
	result = {
		"script_sha256": sha256(SCRIPT),
		"owners": owners,
		"cases_per_owner": len(ARGUMENTS) * len(FLAG_SEEDS),
		"total_real_owner_cases": len(owners) * len(ARGUMENTS) * len(FLAG_SEEDS),
		"all_real_owners_pass": True,
		"negative_controls": mutants,
		"contract": {
			"boolean": "only the low byte of the 32-bit stack slot is read; zero clears and nonzero sets",
			"state": "only bit 0 of the dword at weapon_hud_globals+0x1E0 changes",
			"guard": "all other bytes in the 0x1E4 globals object and surrounding mapped guard page remain unchanged",
			"preserved": ["EBP", "EBX", "ESI", "EDI", "DF", "caller stack except saved-EBP slot"],
			"synthetic_code_executed": 0,
		},
	}
	args.output.parent.mkdir(parents=True, exist_ok=True)
	args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
	print(json.dumps(result, indent=2))


if __name__ == "__main__":
	main()
