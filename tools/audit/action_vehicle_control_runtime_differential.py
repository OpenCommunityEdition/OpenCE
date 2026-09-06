"""Target-first oracle for exactly ``action_vehicle_control``.

This oracle executes the real i386 COFF function bytes from the frozen January
target, the canonical first natural reconciled candidate object, and the final
owner-correct object.
The only modeled callees are the function's genuine existing dependencies:
``datum_get`` (through the authentic ``actor_get`` macro) and
``actor_path_has_path``.  The stubs record their cdecl arguments and order.

Every case starts with a fully seeded 0x800-byte actor-memory envelope.  That
is the modeled checked region, not a claim about ``sizeof(actor_datum)``.
Validation is byte-exact over the whole envelope, including all bytes outside
the narrow expected writes.  It also checks target write order, complete
caller frame, ESP, EIP, DF, and the i386 nonvolatile registers.
Target/candidate comparison additionally includes the volatile registers.

The oracle writes only its requested scratch JSON report.  It does not compile,
invoke a build system, or modify source, configuration, or frozen inputs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

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
ROOT = next(
	parent for parent in SCRIPT.parents
	if (parent / "tools/coff_compare.py").is_file()
)
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


TARGET_DEFAULT = (
	ROOT
	/ "scratch/action-vehicle-fable-control-before-20260906/files/build/split/source/ai/action_vehicle.obj"
)
FIRST_DEFAULT = ROOT / "scratch/action-vehicle-fable-control-first-20260906.obj"
ACTUAL_DEFAULT = (
	ROOT / "scratch/action-vehicle-fable-control-final-20260906/source/ai/action_vehicle.obj"
)
MANIFEST_DEFAULT = ROOT / "scratch/action-vehicle-fable-control-final-20260906.json"
SOURCE_DEFAULT = (
	ROOT / "scratch/action-vehicle-fable-control-final-20260906/files/source/ai/action_vehicle.c"
)
OWNER_HEADER_DEFAULT = (
	ROOT / "scratch/action-vehicle-fable-control-final-20260906/files/source/ai/actions.h"
)
OWNER_CONSUMER_DEFAULT = (
	ROOT / "scratch/action-vehicle-fable-control-final-20260906/files/source/ai/actions.c"
)
OUTPUT_DEFAULT = (
	ROOT / "scratch/action-vehicle-control-runtime-differential-report.json"
)

DEFAULT_HASHES = {
	"target": "49f00a0115cecf578e51102fc33f665f876a4fafcb78ee3b53520545406ef769",
	"first": "6bc330954c197c8a5dc54c3ed269e39b5d894edafc4d0a4f265b235686b46c2f",
	"actual": "0f80fc38c93779cce5e03eceea3dffa9788386a497300ac6c18e307c66c4f82c",
	"manifest": "081017c7ca9d82bf5b240e64a27115ec3d187901a7f2b04f77e291182b8ddf7b",
	"source": "a45f810e182ca07d962da22deb8f9ea0063382650205248d74f66530f7a7725b",
	"owner_header": "4cd690d9f45ef445b6fc651e6fe96ea725eb576f9161111fafa6b0a58e96754d",
	"owner_consumer": "df430c426ffacf6c7872bc9b8a6b45f325950c7fe2e79477930d39c9673c5975",
}

FUNCTION = {
	"symbol": "_action_vehicle_control",
	"size": 176,
	"relocation_count": 3,
	"normalized_sha256": "81612c85cf900b15c2de8ea6d52646f5727a96fbea5e2f13018e6f9613fcd293",
	"storage": 2,
	"symbol_type": 0x20,
	"selection": 1,
}

EXPECTED_RELOCATIONS = [
	[4, cc.IMAGE_REL_I386_DIR32, "global:_actor_data"],
	[17, cc.IMAGE_REL_I386_REL32, "api:_datum_get"],
	[89, cc.IMAGE_REL_I386_REL32, "api:_actor_path_has_path"],
]

CODE = 0x100000
STOP = 0x10F000
STUBS = 0x200000
GLOBALS = 0x300000
ACTOR_DATA_SLOT = GLOBALS + 0x100
ACTOR_DATA_VALUE = 0x2468ACE0
ACTOR = 0x400000
ACTOR_SIZE = 0x800
STACK_PAGE = 0x500000
ENTRY_SP = STACK_PAGE + 0xC000
CALLER_WINDOW_SIZE = 0x80

LOCK_FACING_OFFSET = 0x0C8
DESTINATION_FACING_OFFSET = 0x0D8
PRIMARY_PRIORITY_OFFSET = 0x3E8
PRIMARY_DIRECTION_TYPE_OFFSET = 0x3EC
PRIMARY_DIRECTION_VECTOR_OFFSET = 0x3F0
IDLE_LOOK_TYPE_OFFSET = 0x3FC
DIVE_INTO_COVER_OFFSET = 0x424
EMERGE_FROM_COVER_OFFSET = 0x425
STATIONARY_CROUCH_OFFSET = 0x426
MOVING_CROUCH_OFFSET = 0x427
PANICKED_OFFSET = 0x428
SHOOT_AT_TARGET_OFFSET = 0x454

STUB_NAMES = ("_datum_get", "_actor_path_has_path")
STUB_ADDRESSES = {
	name: STUBS + 0x100 * (index + 1)
	for index, name in enumerate(STUB_NAMES)
}

SAVED_REGISTERS = {
	UC_X86_REG_EBP: 0x1122AABB,
	UC_X86_REG_EBX: 0x2233BBCC,
	UC_X86_REG_ESI: 0x3344CCDD,
	UC_X86_REG_EDI: 0x4455DDEE,
}
SAVED_REGISTER_NAMES = {
	UC_X86_REG_EBP: "ebp",
	UC_X86_REG_EBX: "ebx",
	UC_X86_REG_ESI: "esi",
	UC_X86_REG_EDI: "edi",
}
VOLATILE_REGISTERS = {
	UC_X86_REG_EAX: "eax",
	UC_X86_REG_ECX: "ecx",
	UC_X86_REG_EDX: "edx",
}

VECTOR_PATTERNS = (
	(0x3F800000, 0xC0000000, 0x00000000),
	(0x80000000, 0x7F800000, 0xFF800000),
	(0x7FC12345, 0xFFC54321, 0x00000001),
	(0xDEADBEEF, 0x01020304, 0x89ABCDEF),
	(0x3EAAAAAB, 0xBF000000, 0x41200000),
	(0xFFFFFFFF, 0x7FFFFFFF, 0x80000001),
)

HANDLES = (
	0x00000000,
	0x00000001,
	0x7FFFFFFF,
	0x80000000,
	0xFFFFFFFF,
	0x12345678,
	0xDEADBEEF,
	0x0000FFFF,
	0xFFFF0000,
	0x13572468,
	0xCAFEBABE,
	0x01020304,
	0xFEEDFACE,
	0xAAAAAAAA,
	0x55555555,
	0x00010001,
	0xFFFE0002,
	0x31415926,
)


def p16(value: int) -> bytes:
	return struct.pack("<H", value & 0xFFFF)


def p32(value: int) -> bytes:
	return struct.pack("<I", value & 0xFFFFFFFF)


def read32(machine: u.Uc, address: int) -> int:
	return struct.unpack("<I", machine.mem_read(address, 4))[0]


def put16(data: bytearray, offset: int, value: int) -> None:
	data[offset : offset + 2] = p16(value)


def put32(data: bytearray, offset: int, value: int) -> None:
	data[offset : offset + 4] = p32(value)


def sha256_bytes(data: bytes) -> str:
	return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
	return sha256_bytes(path.read_bytes())


def seeded_bytes(size: int, salt: int) -> bytes:
	return bytes(((index * 37 + salt * 53 + 11) & 0xFF) for index in range(size))


def differing_ranges(actual: bytes, expected: bytes) -> list[list[int]]:
	indices = [
		index
		for index, (actual_byte, expected_byte) in enumerate(zip(actual, expected))
		if actual_byte != expected_byte
	]
	if not indices:
		return []
	ranges = []
	start = previous = indices[0]
	for index in indices[1:]:
		if index != previous + 1:
			ranges.append([start, previous + 1])
			start = index
		previous = index
	ranges.append([start, previous + 1])
	return ranges


def machine_argument(machine: u.Uc, index: int) -> int:
	return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def cases() -> list[dict]:
	result = []
	path_bytes = (0x00, 0x01, 0x02, 0x7F, 0x80, 0xFE, 0xFF, 0x00)
	for index, path_byte in enumerate(path_bytes):
		result.append({
			"name": "unlocked_path_%02x_%02d" % (path_byte, index),
			"seed": index + 1,
			"handle": HANDLES[index],
			"lock_facing": 0,
			"path_byte": path_byte,
			"vector_words": VECTOR_PATTERNS[index % len(VECTOR_PATTERNS)],
		})
	lock_bytes = (0x01, 0x02, 0x7F, 0x80, 0xFE, 0xFF, 0x55, 0xAA, 0x10, 0x40)
	for local_index, lock_byte in enumerate(lock_bytes):
		index = len(result)
		result.append({
			"name": "locked_%02x_%02d" % (lock_byte, local_index),
			"seed": index + 1,
			"handle": HANDLES[index],
			"lock_facing": lock_byte,
			"path_byte": (0x00, 0x01, 0x80, 0xFF)[local_index % 4],
			"vector_words": VECTOR_PATTERNS[index % len(VECTOR_PATTERNS)],
		})
	return result


def relocation_identity(name: str) -> tuple[str, int]:
	if name == "_actor_data":
		return "global:_actor_data", ACTOR_DATA_SLOT
	if name in STUB_ADDRESSES:
		return "api:" + name, STUB_ADDRESSES[name]
	raise AssertionError(("unexpected action_vehicle_control relocation", name))


def prepare_function(path: Path, role: str) -> dict:
	obj = cc.load(path)
	functions = function_sections(obj)
	if FUNCTION["symbol"] not in functions:
		raise AssertionError((role, "missing function", FUNCTION["symbol"]))
	function = functions[FUNCTION["symbol"]]
	symbol = cc.symbol(obj, FUNCTION["symbol"])
	section = obj["sections"][function.section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))
	fingerprint = []
	for relocation in function.info["relocations"]:
		target = relocation.get("symbolic_target", relocation["target"])
		if not isinstance(target, (list, tuple)) or target[0] != "symbol":
			raise AssertionError((role, "unresolved relocation", target))
		name, addend = target[1:]
		identity, address = relocation_identity(name)
		fingerprint.append([relocation["address"], relocation["type"], identity])
		address += addend
		if relocation["type"] == cc.IMAGE_REL_I386_REL32:
			address -= CODE + relocation["address"] + 4
		elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
			raise AssertionError((role, "unsupported relocation", relocation["type"]))
		raw[relocation["address"] : relocation["address"] + 4] = p32(address)
	undefined = {}
	for name in ("_actor_data", "_datum_get", "_actor_path_has_path"):
		matches = [item for item in obj["symbols"] if item["name"] == name]
		if len(matches) != 1:
			raise AssertionError((role, "dependency symbol cardinality", name, matches))
		dependency = matches[0]
		undefined[name] = {
			"section": dependency["section"],
			"value": dependency["value"],
			"storage": dependency["storage"],
			"symbol_type": dependency["type"],
		}
	return {
		"role": role,
		"path": path,
		"object": obj,
		"raw": bytes(raw),
		"info": function.info,
		"symbol": symbol,
		"selection": _section_aux_selection(obj, symbol["section"]),
		"relocation_fingerprint": fingerprint,
		"undefined_dependencies": undefined,
	}


def section_guard(prepared: dict) -> dict:
	info = prepared["info"]
	symbol = prepared["symbol"]
	undefined = prepared["undefined_dependencies"]
	checks = {
		"size_176": info["size"] == FUNCTION["size"],
		"relocations_3": info["relocation_count"] == FUNCTION["relocation_count"],
		"normalized_sha256": info["normalized_sha256"] == FUNCTION["normalized_sha256"],
		"relocation_fingerprint": prepared["relocation_fingerprint"] == EXPECTED_RELOCATIONS,
		"public_external_storage_2": symbol["storage"] == FUNCTION["storage"],
		"function_type_0x20": symbol["type"] == FUNCTION["symbol_type"],
		"comdat_noduplicates_1": prepared["selection"] == FUNCTION["selection"],
		"dependencies_are_undefined_externals": all(
			facts["section"] == 0 and facts["value"] == 0 and facts["storage"] == 2
			for facts in undefined.values()
		),
	}
	failed = [name for name, passed in checks.items() if not passed]
	if failed:
		raise AssertionError((prepared["role"], "section guard", failed))
	return checks


def initial_actor(case: dict) -> bytes:
	actor = bytearray(seeded_bytes(ACTOR_SIZE, case["seed"]))
	actor[LOCK_FACING_OFFSET] = case["lock_facing"]
	for index, word in enumerate(case["vector_words"]):
		put32(actor, DESTINATION_FACING_OFFSET + 4 * index, word)
	return bytes(actor)


def expected_actor(case: dict, original: bytes) -> bytes:
	actor = bytearray(original)
	if case["lock_facing"]:
		put16(actor, PRIMARY_PRIORITY_OFFSET, 4)
		put16(actor, PRIMARY_DIRECTION_TYPE_OFFSET, 4)
		actor[
			PRIMARY_DIRECTION_VECTOR_OFFSET : PRIMARY_DIRECTION_VECTOR_OFFSET + 12
		] = actor[DESTINATION_FACING_OFFSET : DESTINATION_FACING_OFFSET + 12]
	elif case["path_byte"]:
		put16(actor, PRIMARY_PRIORITY_OFFSET, 3)
		put16(actor, PRIMARY_DIRECTION_TYPE_OFFSET, 0)
	else:
		put16(actor, PRIMARY_PRIORITY_OFFSET, 0)
	for offset in (
		SHOOT_AT_TARGET_OFFSET,
		STATIONARY_CROUCH_OFFSET,
		MOVING_CROUCH_OFFSET,
		PANICKED_OFFSET,
		DIVE_INTO_COVER_OFFSET,
		EMERGE_FROM_COVER_OFFSET,
	):
		actor[offset] = 0
	put16(actor, IDLE_LOOK_TYPE_OFFSET, 4)
	return bytes(actor)


def expected_events(case: dict) -> list[dict]:
	events = [{
		"api": "_datum_get",
		"stack_depth": 28,
		"args": [ACTOR_DATA_VALUE, case["handle"]],
	}]
	if not case["lock_facing"]:
		events.append({
			"api": "_actor_path_has_path",
			"stack_depth": 24,
			"args": [case["handle"]],
		})
	return events


def expected_actor_writes(case: dict) -> list[dict]:
	writes = []
	def append(offset: int, size: int, value: int) -> None:
		writes.append({"offset": offset, "size": size, "value": value})
	if case["lock_facing"]:
		append(PRIMARY_PRIORITY_OFFSET, 2, 4)
		append(PRIMARY_DIRECTION_TYPE_OFFSET, 2, 4)
		for index, word in enumerate(case["vector_words"]):
			append(PRIMARY_DIRECTION_VECTOR_OFFSET + 4 * index, 4, word)
	elif case["path_byte"]:
		append(PRIMARY_PRIORITY_OFFSET, 2, 3)
		append(PRIMARY_DIRECTION_TYPE_OFFSET, 2, 0)
	else:
		append(PRIMARY_PRIORITY_OFFSET, 2, 0)
	for offset in (
		SHOOT_AT_TARGET_OFFSET,
		STATIONARY_CROUCH_OFFSET,
		MOVING_CROUCH_OFFSET,
		PANICKED_OFFSET,
		DIVE_INTO_COVER_OFFSET,
		EMERGE_FROM_COVER_OFFSET,
	):
		append(offset, 1, 0)
	append(IDLE_LOOK_TYPE_OFFSET, 2, 4)
	return writes


def execute_case(prepared: dict, case: dict) -> dict:
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x10000),
		(STUBS, 0x10000),
		(GLOBALS, 0x10000),
		(ACTOR, 0x1000),
		(STACK_PAGE, 0x10000),
	):
		machine.mem_map(address, size)
	machine.mem_write(CODE, prepared["raw"])
	for address in STUB_ADDRESSES.values():
		machine.mem_write(address, b"\xc3")
	machine.mem_write(ACTOR_DATA_SLOT, p32(ACTOR_DATA_VALUE))

	original_actor = initial_actor(case)
	machine.mem_write(ACTOR, original_actor)
	stack_seed = seeded_bytes(0x1000, case["seed"] + 91)
	machine.mem_write(ENTRY_SP - 0x800, stack_seed)
	frame = p32(STOP) + p32(case["handle"])
	machine.mem_write(ENTRY_SP, frame)
	expected_caller_window = bytes(machine.mem_read(ENTRY_SP, CALLER_WINDOW_SIZE))

	machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
	machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
	machine.reg_write(UC_X86_REG_EAX, 0xA0A1A2A3)
	machine.reg_write(UC_X86_REG_ECX, 0xC0C1C2C3)
	machine.reg_write(UC_X86_REG_EDX, 0xD0D1D2D3)
	for register, value in SAVED_REGISTERS.items():
		machine.reg_write(register, value)

	events = []
	actor_writes = []
	hook_errors = []

	def record_write(unused_machine, unused_access, address, size, value, unused_data):
		if ACTOR <= address and address + size <= ACTOR + ACTOR_SIZE:
			actor_writes.append({
				"offset": address - ACTOR,
				"size": size,
				"value": value & ((1 << (size * 8)) - 1),
			})

	def record_event(api: str, arguments: list[int]) -> None:
		events.append({
			"api": api,
			"stack_depth": ENTRY_SP - machine.reg_read(UC_X86_REG_ESP),
			"args": arguments,
		})

	def execute_stub(unused_machine, address, unused_size, unused_data):
		name = next(
			(candidate for candidate, stub in STUB_ADDRESSES.items() if stub == address),
			None,
		)
		if name is None:
			hook_errors.append("execution entered unknown stub 0x%08x" % address)
			machine.emu_stop()
			return
		try:
			if name == "_datum_get":
				data = machine_argument(machine, 0)
				handle = machine_argument(machine, 1)
				record_event(name, [data, handle])
				if data != ACTOR_DATA_VALUE:
					raise AssertionError(("datum_get data owner", data, ACTOR_DATA_VALUE))
				if handle != case["handle"]:
					raise AssertionError(("datum_get handle", handle, case["handle"]))
				machine.reg_write(UC_X86_REG_EAX, ACTOR)
				machine.reg_write(UC_X86_REG_ECX, 0xC1C2C3C4)
				machine.reg_write(UC_X86_REG_EDX, 0xD1D2D3D4)
			elif name == "_actor_path_has_path":
				handle = machine_argument(machine, 0)
				record_event(name, [handle])
				if handle != case["handle"]:
					raise AssertionError(("path handle", handle, case["handle"]))
				machine.reg_write(
					UC_X86_REG_EAX,
					0xA5B6C700 | (case["path_byte"] & 0xFF),
				)
				machine.reg_write(UC_X86_REG_ECX, 0xC5C6C7C8)
				machine.reg_write(UC_X86_REG_EDX, 0xD5D6D7D8)
		except (AssertionError, u.UcError) as error:
			hook_errors.append(repr(error))
			machine.emu_stop()

	machine.hook_add(u.UC_HOOK_MEM_WRITE, record_write)
	machine.hook_add(
		u.UC_HOOK_CODE,
		execute_stub,
		begin=STUBS,
		end=STUBS + 0xFFFF,
	)
	execution_error = None
	try:
		machine.emu_start(CODE, STOP, count=1000)
	except u.UcError as error:
		execution_error = str(error)

	actor = bytes(machine.mem_read(ACTOR, ACTOR_SIZE))
	caller_window = bytes(machine.mem_read(ENTRY_SP, CALLER_WINDOW_SIZE))
	return {
		"execution_error": execution_error,
		"hook_errors": hook_errors,
		"events": events,
		"actor_writes": actor_writes,
		"actor": actor,
		"original_actor": original_actor,
		"caller_window": caller_window,
		"expected_caller_window": expected_caller_window,
		"eip": machine.reg_read(UC_X86_REG_EIP),
		"esp": machine.reg_read(UC_X86_REG_ESP),
		"eflags": machine.reg_read(UC_X86_REG_EFLAGS),
		"saved_registers": {
			SAVED_REGISTER_NAMES[register]: machine.reg_read(register)
			for register in SAVED_REGISTERS
		},
		"volatile_registers": {
			name: machine.reg_read(register)
			for register, name in VOLATILE_REGISTERS.items()
		},
	}


def validate_outcome(case: dict, outcome: dict) -> list[str]:
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected = expected_actor(case, outcome["original_actor"])
	if outcome["actor"] != expected:
		errors.append(
			"seeded actor-memory envelope differs at %s"
			% differing_ranges(outcome["actor"], expected)
		)
	if outcome["events"] != expected_events(case):
		errors.append("external-call argument/order trace differs")
	if outcome["actor_writes"] != expected_actor_writes(case):
		errors.append("actor write order/value trace differs")
	if outcome["caller_window"] != outcome["expected_caller_window"]:
		errors.append("caller return/argument/tail window was modified")
	if outcome["eip"] != STOP:
		errors.append("ordinary return did not reach STOP")
	if outcome["esp"] != ENTRY_SP + 4:
		errors.append(
			"cdecl stack imbalance: 0x%08x != 0x%08x"
			% (outcome["esp"], ENTRY_SP + 4)
		)
	if outcome["eflags"] & 0x400:
		errors.append("direction flag left set")
	for register, expected_value in SAVED_REGISTERS.items():
		name = SAVED_REGISTER_NAMES[register]
		if outcome["saved_registers"][name] != expected_value:
			errors.append("nonvolatile register %s was not preserved" % name)
	return errors


def canonical_outcome(outcome: dict) -> dict:
	return {
		"events": outcome["events"],
		"actor_writes": outcome["actor_writes"],
		"actor_sha256": sha256_bytes(outcome["actor"]),
		"changed_ranges": differing_ranges(outcome["actor"], outcome["original_actor"]),
		"caller_window_sha256": sha256_bytes(outcome["caller_window"]),
		"eip": outcome["eip"],
		"esp": outcome["esp"],
		"direction_flag": bool(outcome["eflags"] & 0x400),
		"saved_registers": outcome["saved_registers"],
		"volatile_registers": outcome["volatile_registers"],
	}


def concise_difference(actual: dict, expected: dict) -> dict:
	components = [key for key in expected if actual.get(key) != expected[key]]
	result = {"differing_components": components}
	for key in ("events", "actor_writes", "actor_sha256", "esp", "direction_flag"):
		if key in components:
			result["expected_" + key] = expected[key]
			result["actual_" + key] = actual.get(key)
	return result


def run_prepared(prepared: dict, target_reference: dict | None = None) -> tuple[dict, dict]:
	case_traces = {}
	semantic_failures = []
	target_mismatches = []
	for case in cases():
		outcome = execute_case(prepared, case)
		errors = validate_outcome(case, outcome)
		canonical = canonical_outcome(outcome)
		case_traces[case["name"]] = canonical
		if errors:
			semantic_failures.append({"case": case["name"], "errors": errors})
		if target_reference is not None and canonical != target_reference[case["name"]]:
			target_mismatches.append({
				"case": case["name"],
				**concise_difference(canonical, target_reference[case["name"]]),
			})
	trace_payload = json.dumps(
		case_traces,
		sort_keys=True,
		separators=(",", ":"),
	).encode("utf-8")
	result = {
		"path": str(prepared["path"]),
		"sha256": sha256_path(prepared["path"]),
		"symbol": prepared["symbol"]["name"],
		"size": prepared["info"]["size"],
		"relocation_count": prepared["info"]["relocation_count"],
		"normalized_sha256": prepared["info"]["normalized_sha256"],
		"storage": prepared["symbol"]["storage"],
		"symbol_type": prepared["symbol"]["type"],
		"selection": prepared["selection"],
		"relocation_fingerprint": prepared["relocation_fingerprint"],
		"undefined_dependencies": prepared["undefined_dependencies"],
		"case_count": len(case_traces),
		"seeded_actor_memory_envelope_bytes_checked_per_case": ACTOR_SIZE,
		"trace_sha256": sha256_bytes(trace_payload),
		"semantic_failure_count": len(semantic_failures),
		"target_mismatch_count": len(target_mismatches),
		"first_semantic_failures": semantic_failures[:5],
		"first_target_mismatches": target_mismatches[:5],
		"pass": not semantic_failures and not target_mismatches,
	}
	return result, case_traces


MUTANTS = {
	"locked_priority_5": {"offset": 0x27, "from": 0x04, "to": 0x05},
	"lock_branch_inverted": {"offset": 0x24, "from": 0x74, "to": 0x75},
	"vector_word0_wrong_register": {"offset": 0x48, "from": 0x02, "to": 0x0A},
	"idle_look_type_5": {"offset": 0xA9, "from": 0x04, "to": 0x05},
}


def run_negative_controls(target: dict, target_reference: dict) -> dict:
	results = {}
	all_detected = True
	for name, mutation in MUTANTS.items():
		raw = bytearray(target["raw"])
		if raw[mutation["offset"]] != mutation["from"]:
			raise AssertionError((name, "mutation preimage", raw[mutation["offset"]]))
		raw[mutation["offset"]] = mutation["to"]
		mutant = dict(target)
		mutant["role"] = "negative_control:" + name
		mutant["raw"] = bytes(raw)
		mutant_result, unused = run_prepared(mutant, target_reference)
		detected = (
			mutant_result["semantic_failure_count"] > 0
			and mutant_result["target_mismatch_count"] > 0
		)
		results[name] = {
			"mutation": mutation,
			"semantic_failure_count": mutant_result["semantic_failure_count"],
			"target_mismatch_count": mutant_result["target_mismatch_count"],
			"first_semantic_failures": mutant_result["first_semantic_failures"][:2],
			"first_target_mismatches": mutant_result["first_target_mismatches"][:2],
			"detected": detected,
		}
		all_detected = all_detected and detected
	return {"pass": all_detected, "mutants": results}


def verify_hash(path: Path, expected: str, role: str) -> dict:
	actual = sha256_path(path)
	if actual.lower() != expected.lower():
		raise AssertionError((role, "SHA-256", str(path), actual, expected))
	return {"path": str(path), "sha256": actual, "bytes": path.stat().st_size}


def verify_owner_sources(source: Path, owner_header: Path, owner_consumer: Path) -> dict:
	source_text = source.read_text(encoding="utf-8")
	header_text = owner_header.read_text(encoding="utf-8")
	consumer_text = owner_consumer.read_text(encoding="utf-8")
	definition = "void action_vehicle_control(\n\tlong actor_index)\n{"
	prototype = "void action_vehicle_control(\n\tlong actor_index);"
	checks = {
		"one_source_definition": source_text.count(definition) == 1,
		"one_genuine_actions_h_prototype": header_text.count(prototype) == 1,
		"no_actions_c_surrogate_prototype": consumer_text.count(prototype) == 0,
		"actions_c_callback_owner_reference": consumer_text.count("&action_vehicle_control,") == 1,
		"source_uses_actor_get": source_text.count("actor_get(actor_index)") >= 1,
		"source_calls_actor_path_has_path": source_text.count("actor_path_has_path(actor_index)") == 1,
	}
	failed = [name for name, passed in checks.items() if not passed]
	if failed:
		raise AssertionError(("owner source guard", failed))
	return checks


def resolve_path(path: Path) -> Path:
	return path if path.is_absolute() else (Path.cwd() / path).resolve()


def main() -> int:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--first", type=Path, default=FIRST_DEFAULT)
	parser.add_argument("--actual", type=Path, default=ACTUAL_DEFAULT)
	parser.add_argument("--manifest", type=Path, default=MANIFEST_DEFAULT)
	parser.add_argument("--source", type=Path, default=SOURCE_DEFAULT)
	parser.add_argument("--owner-header", type=Path, default=OWNER_HEADER_DEFAULT)
	parser.add_argument("--owner-consumer", type=Path, default=OWNER_CONSUMER_DEFAULT)
	parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
	args = parser.parse_args()

	paths = {
		"target": resolve_path(args.target),
		"first": resolve_path(args.first),
		"actual": resolve_path(args.actual),
		"manifest": resolve_path(args.manifest),
		"source": resolve_path(args.source),
		"owner_header": resolve_path(args.owner_header),
		"owner_consumer": resolve_path(args.owner_consumer),
	}
	evidence = {
		role: verify_hash(path, DEFAULT_HASHES[role], role)
		for role, path in paths.items()
		if role != "output"
	}
	owner_source_checks = verify_owner_sources(
		paths["source"], paths["owner_header"], paths["owner_consumer"]
	)

	prepared = {
		role: prepare_function(paths[role], role)
		for role in ("target", "first", "actual")
	}
	section_guards = {role: section_guard(value) for role, value in prepared.items()}

	target_result, target_reference = run_prepared(prepared["target"])
	objects = {"target": target_result}
	for role in ("first", "actual"):
		objects[role], unused = run_prepared(prepared[role], target_reference)
	negative_controls = run_negative_controls(prepared["target"], target_reference)

	passed = (
		all(result["pass"] for result in objects.values())
		and negative_controls["pass"]
	)
	report = {
		"pass": passed,
		"scope": ["_action_vehicle_control"],
		"script": {
			"path": str(SCRIPT),
			"sha256": sha256_path(SCRIPT),
		},
		"evidence": evidence,
		"owner_source_checks": owner_source_checks,
		"section_guards": section_guards,
		"objects": objects,
		"negative_controls": negative_controls,
		"coverage": {
			"case_count": len(cases()),
			"lock_facing_bytes": sorted({case["lock_facing"] for case in cases()}),
			"unlocked_path_bytes": sorted({
				case["path_byte"] for case in cases() if not case["lock_facing"]
			}),
			"handles": ["0x%08x" % case["handle"] for case in cases()],
			"raw_vector_patterns": [
				["0x%08x" % word for word in pattern]
				for pattern in VECTOR_PATTERNS
			],
			"seeded_actor_memory_envelope_bytes_checked_per_case": ACTOR_SIZE,
		},
		"claims": [
			"All three real COFF sections have the exact 176-byte/3-relocation target body, public external function storage, function type 0x20, and NODUPLICATES COMDAT selection 1.",
			"The only referenced owners are external _actor_data, _datum_get, and _actor_path_has_path; datum_get and actor_path_has_path are the only executed API stubs.",
			"Every unlocked case calls datum_get then actor_path_has_path with the exact supplied actor-handle bits; every nonzero lock byte calls datum_get only.",
			"Every case validates all bytes in a 0x800 seeded actor-memory envelope, exact write order/value, caller frame, ESP+4 return, clear DF, and preserved EBP/EBX/ESI/EDI.",
			"Target, first, and actual traces compare complete state hashes, external call arguments/order, write traces, caller window, and final register/ABI observations.",
		],
		"limits": [
			"The first object is the canonical first natural reconciled candidate; it is not represented as a donor build.",
			"The 0x800-byte checked actor-memory envelope is a harness coverage boundary, not an actor_datum size claim.",
			"The actor_data value is an opaque pool token passed to genuine datum_get; the stub returns one fully mapped actor datum and makes no datum-generation validity claim.",
			"All actor handles, including boundary and NONE-shaped bit patterns, are arbitrary forwarding-test values; datum_get deliberately returns the modeled owner, so no case claims the handle is valid in the engine.",
			"actor_path_has_path is modeled only at its observed one-handle public contract; AL is varied through canonical and noncanonical boolean bytes while upper EAX and other volatile registers are poisoned.",
			"The function returns void, so volatile register values are differential observations rather than a source-level return-value contract.",
			"No assertion or fatal path exists in the admitted 176-byte function.",
			"The report is scratch output; the oracle never writes production, source, configuration, object, or build inputs.",
		],
	}
	output = resolve_path(args.output)
	output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
	print(json.dumps({
		"pass": passed,
		"output": str(output),
		"script_sha256": report["script"]["sha256"],
		"output_sha256": sha256_path(output),
		"cases": len(cases()),
		"negative_controls": negative_controls["pass"],
	}, indent=2))
	return 0 if passed else 1


if __name__ == "__main__":
	raise SystemExit(main())
