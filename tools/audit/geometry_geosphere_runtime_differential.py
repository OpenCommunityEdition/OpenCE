"""Target-first runtime differential for the integrated geosphere graph.

Only the public ``geosphere_new(short)`` entry is invoked.  Its four private
callees are relocated to, and executed from, the same input COFF object; this
tool never invents a callable ABI for a private function.  The genuine
``normalize3d`` COMDAT executes as machine code as well.  Only the public
allocator, free, assertion, and fatal-exit APIs are modeled as cdecl stubs.

The valid domain covers segments 2, 3, and the real production caller's 16.
The failure domain covers the outer allocation, each of the three child
allocations, two informative per-face failures, and the original segment-1
capacity assertion.  Child/per-face failure behavior is intentionally modeled
as the January program implements it, including returned dangling child
pointers and the segment-2 incomplete-strip success; it is not represented as
safe behavior.

All default inputs are SHA-256 guarded.  JSON is written only to stdout.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
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
	UC_X86_REG_FPCW,
	UC_X86_REG_FPSW,
)


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402


DEFAULT_PATHS = {
	"target": ROOT / "scratch/geometry-fable-geosphere-before-20260906/files/build/split/source/math/geometry.obj",
	"candidate": ROOT / "scratch/geometry-fable-geosphere-typed-first-20260906.obj",
	"actual": ROOT / "scratch/geometry-fable-geosphere-final-20260906/source/math/geometry.obj",
	"target_math": ROOT / "build/split/source/ai/action_charge.obj",
	"target_squared": ROOT / "build/split/source/ai/action_alert.obj",
	"caller": ROOT / "build/split/source/math/random_math.obj",
	"source": ROOT / "scratch/geometry-fable-geosphere-final-20260906/files/source/math/geometry.c",
	"header": ROOT / "scratch/geometry-fable-geosphere-final-20260906/files/source/math/geometry.h",
	"config": ROOT / "scratch/geometry-fable-geosphere-final-20260906/files/config/symbols.json",
	"before_manifest": ROOT / "scratch/geometry-fable-geosphere-before-20260906.json",
	"final_manifest": ROOT / "scratch/geometry-fable-geosphere-final-20260906.json",
}

DEFAULT_HASHES = {
	"target": "9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c",
	"candidate": "f4815540d55abc982dadb422631c2c20df11df10dab0f357150d98fbd455b55d",
	"actual": "9370a04cd686f339a687f408df0452774bcc5843d7b5b577f3f4a875502be606",
	"target_math": "76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea",
	"target_squared": "351ff0de9ddfea9b3c03a9038abab75b0c4b48290443b33e09c33105b41ab61a",
	"caller": "1ce32f14ed504b4e14cf80f712f5e9cf3810425c72982da9d5cd5eb744395d57",
	"source": "f17d26efb18b796ceef7f9cf58d4241561be612d85770be28bfadeb38d3be45f",
	"header": "04f7b7b7d027c0621d93410abd856c9cb42b6268164ad8db4a78647d91c5411f",
	"config": "c2224cc0088411edbe1501013e7f4536c2e0c45db7bbe5a545f8e137e740ed5a",
	"before_manifest": "d89de4c54a6ca0471b68e1e44debb3f2106cc942ac4d75be62f44ff6ecc8aecb",
	"final_manifest": "8b22a33fbc3f51a78f44dc7b63de02aa0f3e0568df74dfd36236664928d7c5fa",
}

GRAPH_FUNCTIONS = (
	"_geosphere_new",
	"_subdivide_triangle",
	"_get_face_vertex",
	"_get_edge_vertex",
	"_calculate_vertex",
)

MATH_HELPERS = (
	"_normalize3d",
	"_square_root",
	"_scale_vector3d",
	"_magnitude_squared3d",
	"_magnitude3d",
)

FUNCTION_SPECS = {
	"target": {
		"_geosphere_new": (496, 25, "89b49ad03b434d6258ac264dc7b71bb625984c51016d55a82ad146c86d474911"),
		"_subdivide_triangle": (944, 52, "f2e96f710a5b63988446081b1b3fff165fb4db2779f4b38606e273508fcad334"),
		"_get_face_vertex": (400, 18, "6b9b37ec87ba359ac32e9e5bc04938b01a69de5b6783ee0b7d766818299f1b87"),
		"_get_edge_vertex": (416, 17, "ebafcbd056870e28f2d96ad7e08a64d9a2e1b2513e911e075aed0fca4b9dc880"),
		"_calculate_vertex": (336, 18, "5bc4eb4df60182b53941a5231a7f0f984ca4d4eb37fe4dafb09662a2cf8916af"),
	},
	"compiled": {
		"_geosphere_new": (496, 25, "56760ccae276f6ff7f6f188448a6f19fbffa288c993cd71d4dd9cad5c9b1c978"),
		"_subdivide_triangle": (944, 52, "0ea8faff29e129b55a134432a0e602b69d5f7ad1beeee4b15f02a5e1403599ed"),
		"_get_face_vertex": (400, 18, "10460c1e30f697342132688a0b347c3cf155248e73da04199d5d482d22c26c63"),
		"_get_edge_vertex": (400, 17, "240b22a49f13a88b486b99416a7f162513a3c76a9e4de22ee5445d83f3cc7ded"),
		"_calculate_vertex": (336, 18, "26b9afec42c4b82dbb45631840c95109ba1e2ee7bcbe5597afc9d45570e89783"),
	},
}

HELPER_SPECS = {
	"_normalize3d": (96, 3, "67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b"),
	"_square_root": (16, 0, "0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7"),
	"_scale_vector3d": (48, 0, "6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b"),
	"_magnitude_squared3d": (48, 0, "0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725"),
	"_magnitude3d": (48, 0, "114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c"),
}

EXPECTED_DIRECT_RELOCATIONS = {
	"_geosphere_new": {
		"_debug_malloc": 4,
		"_subdivide_triangle": 1,
		"_display_assert": 2,
		"_system_exit": 2,
		"_debug_free": 3,
	},
	"_subdivide_triangle": {
		"_debug_malloc": 1,
		"_get_face_vertex": 4,
		"_display_assert": 11,
		"_system_exit": 11,
		"_debug_free": 1,
	},
	"_get_face_vertex": {
		"_get_edge_vertex": 5,
		"_calculate_vertex": 1,
		"_display_assert": 3,
		"_system_exit": 3,
	},
	"_get_edge_vertex": {
		"_calculate_vertex": 1,
		"_display_assert": 4,
		"_system_exit": 4,
	},
	"_calculate_vertex": {
		"_normalize3d": 1,
		"_display_assert": 4,
		"_system_exit": 4,
	},
}

CODE = 0x100000
STOP = 0x11F000
STUB = 0x200000
CONSTANT = 0x300000
CONSTANT_SIZE = 0x100000
HEAP = 0x500000
HEAP_SIZE = 0x100000
STACK = 0x800000
STACK_SIZE = 0x40000
ENTRY_SP = STACK + 0x3C000

FUNCTION_ADDRESSES = {
	name: CODE + index * 0x2000
	for index, name in enumerate(GRAPH_FUNCTIONS + MATH_HELPERS)
}

STUB_NAMES = (
	"_debug_malloc",
	"_debug_free",
	"_display_assert",
	"_system_exit",
)
STUB_ADDRESSES = {
	name: STUB + 0x100 * (index + 1)
	for index, name in enumerate(STUB_NAMES)
}
STUB_BY_ADDRESS = {address: name for name, address in STUB_ADDRESSES.items()}

SAVED_REGISTERS = {
	UC_X86_REG_EBP: 0x11223344,
	UC_X86_REG_EBX: 0x22334455,
	UC_X86_REG_ESI: 0x33445566,
	UC_X86_REG_EDI: 0x44556677,
}
SAVED_REGISTER_NAMES = {
	UC_X86_REG_EBP: "ebp",
	UC_X86_REG_EBX: "ebx",
	UC_X86_REG_ESI: "esi",
	UC_X86_REG_EDI: "edi",
}

SOURCE_FILE = "c:\\halo\\SOURCE\\math\\geometry.c"
GEOSPHERE_SIZE = 20
ALLOCATION_STRIDE = 0x10000
HEAP_SEED = bytes(((index * 37 + 19) & 0xFF) for index in range(HEAP_SIZE))
STACK_SEED = bytes(((index * 17 + 7) & 0xFF) for index in range(STACK_SIZE))


def sha256_bytes(raw: bytes) -> str:
	return hashlib.sha256(raw).hexdigest()


def sha256_path(path: Path) -> str:
	return sha256_bytes(path.read_bytes())


def p32(value: int) -> bytes:
	return struct.pack("<I", value & 0xFFFFFFFF)


def s32(value: int) -> int:
	return struct.unpack("<i", p32(value))[0]


def resolve_path(value: str | None, default: Path) -> Path:
	if value is None:
		return default.resolve()
	path = Path(value)
	if not path.is_absolute():
		path = ROOT / path
	return path.resolve()


def verify_hash(path: Path, expected: str, label: str) -> dict:
	actual = sha256_path(path)
	if actual.lower() != expected.lower():
		raise AssertionError((label + " SHA-256 mismatch", str(path), actual, expected))
	return {"path": str(path), "sha256": actual, "size": path.stat().st_size}


def relocation_name_addend(relocation: dict) -> tuple[str, int] | None:
	target = relocation.get("symbolic_target", relocation["target"])
	if target[0] == "internal":
		return None
	if target[0] == "symbol":
		return target[1], target[2]
	if target[0] == "defined-noncode":
		return target[2], target[3]
	raise AssertionError(("unsupported symbolic relocation", target))


def direct_relocation_counts(info: dict) -> dict[str, int]:
	result: dict[str, int] = {}
	for relocation in info["relocations"]:
		parsed = relocation_name_addend(relocation)
		if parsed is None:
			continue
		name, unused_addend = parsed
		if name in GRAPH_FUNCTIONS or name in MATH_HELPERS or name in STUB_NAMES:
			result[name] = result.get(name, 0) + 1
	return result


def section_guard(info: dict, expected: tuple[int, int, str], label: str) -> dict:
	size, relocation_count, normalized_sha256 = expected
	checks = {
		"size": info["size"] == size,
		"relocation_count": info["relocation_count"] == relocation_count,
		"normalized_sha256": info["normalized_sha256"] == normalized_sha256,
	}
	failed = [key for key, value in checks.items() if not value]
	if failed:
		raise AssertionError((label, "section guard failed", failed, info))
	return checks


def geosphere_counts(segment_count: int) -> tuple[int, int]:
	triangle_count = 8 * segment_count * segment_count
	vertex_count = 4 * (segment_count - 2) * (segment_count - 1) + 12 * (segment_count - 1) + 6
	return vertex_count, triangle_count


def allocation_address(call_index: int) -> int:
	return HEAP + (call_index + 1) * ALLOCATION_STRIDE


def allocation_label(call_index: int) -> str:
	return "allocation_%d" % (call_index + 1)


def case(name: str, segment_count: int, failure_at: int | None = None, **values) -> dict:
	result = {
		"name": name,
		"segment_count": segment_count,
		"failure_at": failure_at,
	}
	result.update(values)
	return result


def all_cases() -> list[dict]:
	base_cases = [
		case("valid_segments_2", 2),
		case("valid_segments_3", 3),
		case("production_segments_16", 16, production_caller=True),
		case("outer_allocation_failure", 2, failure_at=0),
		case("vertices_allocation_failure", 2, failure_at=1, unsafe_original_failure=True),
		case("strips_allocation_failure", 2, failure_at=2, unsafe_original_failure=True),
		case("subdivision_allocation_failure", 2, failure_at=3, unsafe_original_failure=True),
		case("first_face_failure_segments_2", 2, failure_at=4,
			unsafe_original_failure=True, incomplete_success=True),
		case("first_face_failure_segments_3", 3, failure_at=4,
			unsafe_original_failure=True, expected_assertion_line=99),
		case("capacity_assertion_segments_1", 1, expected_assertion_line=98,
			unsafe_original_failure=True),
	]
	return [
		{
			**case_spec,
			"name": "%s_fpcw_%04x" % (case_spec["name"], fpcw),
			"fpcw": fpcw,
		}
		for fpcw in (0x027F, 0x037F)
		for case_spec in base_cases
	]


class CoffOwner:
	def __init__(self, label: str, path: Path):
		self.label = label
		self.path = path
		self.obj = cc.load(path)
		self.functions = function_sections(self.obj)


class ConstantSpace:
	def __init__(self, machine: u.Uc, owners: list[CoffOwner]):
		self.machine = machine
		self.owners = owners
		self.section_addresses: dict[tuple[str, int], int] = {}
		self.next_slot = 0

	def _defined_symbol(self, name: str) -> tuple[CoffOwner, dict]:
		for owner in self.owners:
			matches = [
				symbol for symbol in owner.obj["symbols"]
				if symbol["name"] == name and symbol["section"] > 0
			]
			if len(matches) == 1:
				section = owner.obj["sections"][matches[0]["section"] - 1]
				if section["flags"] & cc.IMAGE_SCN_CNT_CODE:
					raise AssertionError((name, "constant resolved to code", owner.label))
				return owner, matches[0]
			if len(matches) > 1:
				raise AssertionError((name, "ambiguous definitions", owner.label, len(matches)))
		raise AssertionError((name, "no authenticated non-code owner"))

	def address(self, name: str, addend: int) -> int:
		owner, symbol = self._defined_symbol(name)
		section_number = symbol["section"]
		key = (owner.label, section_number)
		if key not in self.section_addresses:
			section = owner.obj["sections"][section_number - 1]
			raw = bytes(cc._section_bytes(owner.obj, section))
			if len(raw) > 0x1000:
				raise AssertionError((owner.label, name, "non-code section too large", len(raw)))
			if self.next_slot * 0x1000 + len(raw) > CONSTANT_SIZE:
				raise AssertionError("constant arena exhausted")
			base = CONSTANT + self.next_slot * 0x1000
			self.next_slot += 1
			self.machine.mem_write(base, raw)
			self.section_addresses[key] = base
		return self.section_addresses[key] + symbol["value"] + addend


class GeosphereImage:
	def __init__(
		self,
		role: str,
		primary_path: Path,
		target_math_path: Path,
		target_squared_path: Path,
		fallback_path: Path,
	):
		self.role = role
		self.primary = CoffOwner(role + ":primary", primary_path)
		self.target_math = CoffOwner(role + ":target-math", target_math_path)
		self.target_squared = CoffOwner(role + ":target-squared", target_squared_path)
		self.fallback = CoffOwner(role + ":fallback", fallback_path)
		self.machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
		for base, size in (
			(CODE, 0x20000),
			(STUB, 0x10000),
			(CONSTANT, CONSTANT_SIZE),
			(HEAP, HEAP_SIZE),
			(STACK, STACK_SIZE),
		):
			self.machine.mem_map(base, size)
		for address in STUB_ADDRESSES.values():
			self.machine.mem_write(address, b"\xC3")
		self.machine.mem_write(STOP, b"\xCC")

		if role == "target":
			helper_owners = {
				name: self.target_math for name in MATH_HELPERS
			}
			helper_owners["_magnitude_squared3d"] = self.target_squared
			constant_owners = [self.primary, self.target_math, self.target_squared, self.fallback]
		else:
			helper_owners = {name: self.primary for name in MATH_HELPERS}
			constant_owners = [self.primary, self.fallback, self.target_math, self.target_squared]
		self.helper_owners = helper_owners
		self.constant_space = ConstantSpace(self.machine, constant_owners)
		self.function_infos: dict[str, dict] = {}
		self.function_owner_labels: dict[str, str] = {}
		for name in GRAPH_FUNCTIONS:
			self._materialize_function(name, self.primary)
		for name in MATH_HELPERS:
			self._materialize_function(name, helper_owners[name])
		self.machine.mem_protect(CODE, 0x20000, u.UC_PROT_READ | u.UC_PROT_EXEC)
		self.machine.mem_protect(STUB, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
		self.machine.mem_protect(CONSTANT, CONSTANT_SIZE, u.UC_PROT_READ)

	def _materialize_function(self, name: str, owner: CoffOwner) -> None:
		if name not in owner.functions:
			raise AssertionError((self.role, owner.label, "missing function", name))
		function = owner.functions[name]
		section = owner.obj["sections"][function.section_number - 1]
		raw = bytearray(cc._section_bytes(owner.obj, section))
		if len(raw) > 0x1000:
			raise AssertionError((name, "code section too large", len(raw)))
		base = FUNCTION_ADDRESSES[name]
		for relocation in function.info["relocations"]:
			target = relocation.get("symbolic_target", relocation["target"])
			if target[0] == "internal":
				destination = base + target[1]
			elif target[0] in ("symbol", "defined-noncode"):
				if target[0] == "symbol":
					symbol_name, addend = target[1], target[2]
				else:
					symbol_name, addend = target[2], target[3]
				if symbol_name in FUNCTION_ADDRESSES:
					destination = FUNCTION_ADDRESSES[symbol_name] + addend
				elif symbol_name in STUB_ADDRESSES:
					destination = STUB_ADDRESSES[symbol_name] + addend
				else:
					destination = self.constant_space.address(symbol_name, addend)
			else:
				raise AssertionError((self.role, name, "unsupported relocation", target))
			if relocation["type"] == cc.IMAGE_REL_I386_REL32:
				destination -= base + relocation["address"] + 4
			elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
				raise AssertionError((name, "unsupported relocation type", relocation["type"]))
			raw[relocation["address"]:relocation["address"] + 4] = p32(destination)
		self.machine.mem_write(base, bytes(raw))
		self.function_infos[name] = function.info
		self.function_owner_labels[name] = owner.label

	def cstring(self, address: int, limit: int = 4096) -> str:
		regions = (
			(CONSTANT, CONSTANT_SIZE),
			(HEAP, HEAP_SIZE),
			(STACK, STACK_SIZE),
		)
		for base, size in regions:
			if base <= address < base + size:
				raw = bytes(self.machine.mem_read(address, min(limit, base + size - address)))
				if b"\0" not in raw:
					raise AssertionError(("unterminated cstring", hex(address)))
				return raw.split(b"\0", 1)[0].decode("ascii")
		raise AssertionError(("cstring outside modeled regions", hex(address)))

	def guards(self) -> dict:
		kind = "target" if self.role == "target" else "compiled"
		sections = {}
		for name in GRAPH_FUNCTIONS:
			sections[name] = section_guard(
				self.function_infos[name], FUNCTION_SPECS[kind][name], self.role + ":" + name,
			)
			actual_direct = direct_relocation_counts(self.function_infos[name])
			if actual_direct != EXPECTED_DIRECT_RELOCATIONS[name]:
				raise AssertionError((self.role, name, "direct relocation graph", actual_direct,
					EXPECTED_DIRECT_RELOCATIONS[name]))
		for name in MATH_HELPERS:
			sections[name] = section_guard(
				self.function_infos[name], HELPER_SPECS[name], self.role + ":" + name,
			)
		if any(self.function_owner_labels[name] != self.primary.label for name in GRAPH_FUNCTIONS):
			raise AssertionError((self.role, "private graph escaped primary object"))
		return {
			"sections": sections,
			"function_owners": self.function_owner_labels,
			"same_object_private_graph": True,
			"normalize_executes_real_coff": True,
		}

	def run(self, case_spec: dict) -> dict:
		machine = self.machine
		machine.mem_write(HEAP, HEAP_SEED)
		machine.mem_write(STACK, STACK_SEED)
		frame = p32(STOP) + p32(case_spec["segment_count"])
		machine.mem_write(ENTRY_SP, frame)
		machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
		machine.reg_write(UC_X86_REG_EAX, 0xA1A2A3A4)
		machine.reg_write(UC_X86_REG_ECX, 0xB1B2B3B4)
		machine.reg_write(UC_X86_REG_EDX, 0xC1C2C3C4)
		machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
		machine.reg_write(UC_X86_REG_FPCW, case_spec["fpcw"])
		for register, value in SAVED_REGISTERS.items():
			machine.reg_write(register, value)

		state = {
			"terminal": None,
			"events": [],
			"hook_errors": [],
			"allocation_calls": [],
			"graph_counts": {name: 0 for name in GRAPH_FUNCTIONS},
			"helper_counts": {name: 0 for name in MATH_HELPERS},
			"normalize_inputs": [],
		}

		def stack_args(count: int) -> tuple[int, ...]:
			sp = machine.reg_read(UC_X86_REG_ESP)
			return struct.unpack("<" + "I" * count, machine.mem_read(sp + 4, count * 4))

		def return_from_stub(eax: int) -> None:
			sp = machine.reg_read(UC_X86_REG_ESP)
			return_address = struct.unpack("<I", machine.mem_read(sp, 4))[0]
			machine.reg_write(UC_X86_REG_EAX, eax)
			machine.reg_write(UC_X86_REG_ECX, 0xCCCCCCCC)
			machine.reg_write(UC_X86_REG_EDX, 0xDDDDDDDD)
			machine.reg_write(UC_X86_REG_ESP, sp + 4)
			machine.reg_write(UC_X86_REG_EIP, return_address)

		def pointer_label(pointer: int) -> str:
			if pointer == 0:
				return "NULL"
			for record in state["allocation_calls"]:
				if record["address"] == pointer:
					return record["label"]
			return "0x%08x" % pointer

		def stub_hook(unused_machine, address, unused_size, unused_data) -> None:
			try:
				name = STUB_BY_ADDRESS[address]
				if name == "_debug_malloc":
					size, clear, path_pointer, line = stack_args(4)
					if clear != 0:
						raise AssertionError(("clear-allocation mode is outside this oracle", clear))
					call_index = len(state["allocation_calls"])
					failed = call_index == case_spec.get("failure_at")
					result = 0 if failed else allocation_address(call_index)
					if size > ALLOCATION_STRIDE:
						raise AssertionError(("allocation exceeds deterministic slot", call_index, size))
					record = {
						"index": call_index,
						"label": allocation_label(call_index),
						"address": result,
						"size": size,
					}
					state["allocation_calls"].append(record)
					state["events"].append({
						"api": "_debug_malloc",
						"args": [size, clear, self.cstring(path_pointer), line],
						"result": "NULL" if failed else record["label"],
					})
					return_from_stub(result)
				elif name == "_debug_free":
					pointer, path_pointer, line = stack_args(3)
					state["events"].append({
						"api": "_debug_free",
						"args": [pointer_label(pointer), self.cstring(path_pointer), line],
					})
					return_from_stub(0xFEEEFEEE)
				elif name == "_display_assert":
					expression, path_pointer, line, fatal = stack_args(4)
					state["events"].append({
						"api": "_display_assert",
						"args": [self.cstring(expression), self.cstring(path_pointer), line, fatal],
					})
					return_from_stub(0xA55EA55E)
				elif name == "_system_exit":
					(exit_code,) = stack_args(1)
					state["events"].append({"api": "_system_exit", "args": [s32(exit_code)]})
					state["terminal"] = "system_exit"
					machine.emu_stop()
			except Exception as error:  # Unicorn callback exceptions do not propagate reliably.
				state["hook_errors"].append(type(error).__name__ + ": " + str(error))
				state["terminal"] = "hook_error"
				machine.emu_stop()

		def stop_hook(unused_machine, unused_address, unused_size, unused_data) -> None:
			state["terminal"] = "return"
			machine.emu_stop()

		def graph_hook(name: str):
			def hook(unused_machine, unused_address, unused_size, unused_data) -> None:
				state["graph_counts"][name] += 1
			return hook

		def helper_hook(name: str):
			def hook(unused_machine, unused_address, unused_size, unused_data) -> None:
				state["helper_counts"][name] += 1
				if name == "_normalize3d":
					try:
						(pointer,) = stack_args(1)
						raw = bytes(machine.mem_read(pointer, 12))
						state["normalize_inputs"].append(raw)
					except Exception as error:
						state["hook_errors"].append("normalize input: " + str(error))
						state["terminal"] = "hook_error"
						machine.emu_stop()
			return hook

		machine.hook_add(u.UC_HOOK_CODE, stub_hook, begin=STUB, end=STUB + 0x10000 - 1)
		machine.hook_add(u.UC_HOOK_CODE, stop_hook, begin=STOP, end=STOP)
		for name in GRAPH_FUNCTIONS:
			address = FUNCTION_ADDRESSES[name]
			machine.hook_add(u.UC_HOOK_CODE, graph_hook(name), begin=address, end=address)
		for name in MATH_HELPERS:
			address = FUNCTION_ADDRESSES[name]
			machine.hook_add(u.UC_HOOK_CODE, helper_hook(name), begin=address, end=address)

		execution_error = None
		try:
			machine.emu_start(FUNCTION_ADDRESSES["_geosphere_new"], 0, count=50_000_000)
		except u.UcError as error:
			execution_error = str(error)
		if state["terminal"] is None:
			state["terminal"] = "instruction_limit_or_fault"

		eax = machine.reg_read(UC_X86_REG_EAX)
		heap_after = bytes(machine.mem_read(HEAP, HEAP_SIZE))
		allocations = {}
		for record in state["allocation_calls"]:
			if record["address"]:
				raw = bytes(machine.mem_read(record["address"], record["size"]))
				allocations[record["label"]] = {
					"size": record["size"],
					"sha256": sha256_bytes(raw),
				}
		result = {
			"terminal": state["terminal"],
			"events": state["events"],
			"hook_errors": state["hook_errors"],
			"execution_error": execution_error,
			"allocation_calls": state["allocation_calls"],
			"allocations": allocations,
			"graph_counts": state["graph_counts"],
			"helper_counts": state["helper_counts"],
			"normalize_inputs": state["normalize_inputs"],
			"heap": heap_after,
			"heap_sha256": sha256_bytes(heap_after),
			"eax": eax,
			"eip": machine.reg_read(UC_X86_REG_EIP),
			"esp": machine.reg_read(UC_X86_REG_ESP),
			"eflags": machine.reg_read(UC_X86_REG_EFLAGS),
			"fpcw": machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF,
			"fpsw": machine.reg_read(UC_X86_REG_FPSW) & 0xFFFF,
			"registers": {
				SAVED_REGISTER_NAMES[register]: machine.reg_read(register)
				for register in SAVED_REGISTERS
			},
			"caller_frame": bytes(machine.mem_read(ENTRY_SP, len(frame))),
			"expected_caller_frame": frame,
		}
		augment_geometry(result)
		return result


def augment_geometry(outcome: dict) -> None:
	# On fatal assertion the public function has not returned and EAX is not a
	# result.  The first successful allocator record is still the authentic
	# outer object whose partial state must be inspected.
	if outcome["allocation_calls"] and outcome["allocation_calls"][0]["address"]:
		result_pointer = outcome["allocation_calls"][0]["address"]
	else:
		result_pointer = outcome["eax"]
	if not (HEAP <= result_pointer <= HEAP + HEAP_SIZE - GEOSPHERE_SIZE):
		outcome["sphere"] = None
		outcome["vertices"] = b""
		outcome["strip_used"] = b""
		outcome["strip_parse"] = None
		return
	raw = bytes(outcome["heap"][result_pointer - HEAP:result_pointer - HEAP + GEOSPHERE_SIZE])
	segment_count, vertices_pointer, strips_pointer, vertex_count, triangle_count, strip_count = struct.unpack(
		"<h2xIIhhh2x", raw,
	)
	outcome["sphere"] = {
		"segment_count": segment_count,
		"vertices_pointer": vertices_pointer,
		"strips_pointer": strips_pointer,
		"vertex_count": vertex_count,
		"triangle_count": triangle_count,
		"strip_count": strip_count,
		"raw_sha256": sha256_bytes(raw),
	}
	if HEAP <= vertices_pointer < HEAP + HEAP_SIZE and 0 <= vertex_count <= 4096:
		outcome["vertices"] = bytes(outcome["heap"][
			vertices_pointer - HEAP:vertices_pointer - HEAP + 12 * vertex_count
		])
	else:
		outcome["vertices"] = b""
	strip_values = []
	parse_error = None
	used_shorts = 0
	if HEAP <= strips_pointer < HEAP + HEAP_SIZE and 0 <= strip_count <= 4096:
		try:
			cursor = strips_pointer - HEAP
			for unused_index in range(strip_count):
				header = struct.unpack_from("<h", outcome["heap"], cursor)[0]
				if header < 0 or header > 8192:
					raise AssertionError(("invalid strip header", header))
				cursor += 2
				values = list(struct.unpack_from("<" + "h" * header, outcome["heap"], cursor))
				cursor += 2 * header
				strip_values.append([header, values])
			used_shorts = (cursor - (strips_pointer - HEAP)) // 2
		except Exception as error:
			parse_error = type(error).__name__ + ": " + str(error)
			strip_values = []
			used_shorts = 0
	outcome["strip_parse"] = {
		"error": parse_error,
		"strip_count": len(strip_values),
		"used_shorts": used_shorts,
		"headers": [item[0] for item in strip_values],
		"all_indices": [value for item in strip_values for value in item[1]],
	}
	if strips_pointer and used_shorts:
		start = strips_pointer - HEAP
		outcome["strip_used"] = bytes(outcome["heap"][start:start + 2 * used_shorts])
	else:
		outcome["strip_used"] = b""


def malloc_event(index: int, size: int, line: int, failed_at: int | None) -> dict:
	return {
		"api": "_debug_malloc",
		"args": [size, 0, SOURCE_FILE, line],
		"result": "NULL" if index == failed_at else allocation_label(index),
	}


def free_event(index: int, line: int) -> dict:
	return {"api": "_debug_free", "args": [allocation_label(index), SOURCE_FILE, line]}


def assertion_events(line: int) -> list[dict]:
	expressions = {
		98: "triangle_strip_vertex_indices_index < (NUMBER_OF_VERTICES_PER_TRIANGLE + 1) * result->triangle_count",
		99: "vertex_index == result->vertex_count",
	}
	return [
		{"api": "_display_assert", "args": [expressions[line], SOURCE_FILE, line, 1]},
		{"api": "_system_exit", "args": [-1]},
	]


def expected_events(case_spec: dict) -> list[dict]:
	s = case_spec["segment_count"]
	vertex_count, triangle_count = geosphere_counts(s)
	failure_at = case_spec.get("failure_at")
	events = [
		malloc_event(0, 20, 58, failure_at),
	]
	if failure_at == 0:
		return events
	events.extend([
		malloc_event(1, 12 * vertex_count, 66, failure_at),
		malloc_event(2, 8 * triangle_count, 67, failure_at),
		malloc_event(3, 128, 69, failure_at),
	])
	if failure_at in (1, 2, 3):
		if failure_at != 1:
			events.append(free_event(1, 103))
		if failure_at != 2:
			events.append(free_event(2, 104))
		if failure_at != 3:
			events.append(free_event(3, 107))
		return events
	for face in range(8):
		index = 4 + face
		events.append(malloc_event(index, 2 * (s + 1) * (s + 1), 154, failure_at))
		if index != failure_at:
			events.append(free_event(index, 200))
	if "expected_assertion_line" in case_spec:
		events.extend(assertion_events(case_spec["expected_assertion_line"]))
	else:
		events.append(free_event(3, 107))
	return events


def expected_graph_counts(case_spec: dict) -> dict[str, int]:
	s = case_spec["segment_count"]
	failure_at = case_spec.get("failure_at")
	if failure_at == 0:
		return {name: (1 if name == "_geosphere_new" else 0) for name in GRAPH_FUNCTIONS}
	if failure_at in (1, 2, 3):
		return {name: (1 if name == "_geosphere_new" else 0) for name in GRAPH_FUNCTIONS}
	successful_faces = 7 if failure_at == 4 else 8
	calculate_count = 12 * (s - 1) + successful_faces * (s - 1) * (s - 2) // 2
	return {
		"_geosphere_new": 1,
		"_subdivide_triangle": 8,
		"_get_face_vertex": successful_faces * (2 * s * s + s),
		"_get_edge_vertex": successful_faces * (s * s + 2),
		"_calculate_vertex": calculate_count,
	}


def expected_helper_counts(case_spec: dict) -> dict[str, int]:
	calculate_count = expected_graph_counts(case_spec)["_calculate_vertex"]
	return {
		"_normalize3d": calculate_count,
		"_square_root": 0,
		"_scale_vector3d": 0,
		"_magnitude_squared3d": 0,
		"_magnitude3d": 0,
	}


def validate_geometry(case_spec: dict, outcome: dict) -> list[str]:
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected_terminal = "system_exit" if "expected_assertion_line" in case_spec else "return"
	if outcome["terminal"] != expected_terminal:
		errors.append("terminal %r != %r" % (outcome["terminal"], expected_terminal))
	if outcome["events"] != expected_events(case_spec):
		errors.append("external call order/arguments/results differ from source model")
	if outcome["graph_counts"] != expected_graph_counts(case_spec):
		errors.append("same-object public/private graph entry counts differ")
	if outcome["helper_counts"] != expected_helper_counts(case_spec):
		errors.append("real math helper execution counts differ")
	if outcome["caller_frame"] != outcome["expected_caller_frame"]:
		errors.append("public caller frame changed")
	if outcome["fpcw"] != case_spec["fpcw"]:
		errors.append("x87 control word changed")
	if outcome["fpsw"] & 0x3800:
		errors.append("x87 TOP is not restored to zero at terminal boundary")
	if expected_terminal == "return":
		if outcome["eip"] != STOP:
			errors.append("ordinary return did not reach STOP")
		if outcome["esp"] != ENTRY_SP + 4:
			errors.append("cdecl stack imbalance")
		if outcome["eflags"] & 0x400:
			errors.append("direction flag left set")
		for register, expected in SAVED_REGISTERS.items():
			name = SAVED_REGISTER_NAMES[register]
			if outcome["registers"][name] != expected:
				errors.append("nonvolatile register %s changed" % name)

	failure_at = case_spec.get("failure_at")
	if failure_at == 0:
		if outcome["eax"] != 0 or outcome["sphere"] is not None:
			errors.append("outer allocation failure did not return NULL")
		return errors
	if outcome["terminal"] == "return" and outcome["eax"] != allocation_address(0):
		errors.append("non-outer-failure result is not the allocated outer object")
	if outcome["sphere"] is None:
		errors.append("allocated outer geosphere is not inspectable")
		return errors
	sphere = outcome["sphere"]
	s = case_spec["segment_count"]
	vertex_count, triangle_count = geosphere_counts(s)
	expected_vertices = 0 if failure_at == 1 else allocation_address(1)
	expected_strips = 0 if failure_at == 2 else allocation_address(2)
	if sphere["segment_count"] != s:
		errors.append("segment_count field differs")
	if sphere["vertex_count"] != vertex_count or sphere["triangle_count"] != triangle_count:
		errors.append("vertex/triangle formula fields differ")
	if sphere["vertices_pointer"] != expected_vertices or sphere["strips_pointer"] != expected_strips:
		errors.append("owned child pointer fields differ")
	if failure_at in (1, 2, 3):
		if sphere["strip_count"] != 0:
			errors.append("child failure changed triangle_strip_count")
		# These pointers are deliberately left non-null after their storage is freed.
		return errors

	expected_strip_count = (7 if failure_at == 4 else 8) * s
	if sphere["strip_count"] != expected_strip_count:
		errors.append("triangle_strip_count differs")
	strip_parse = outcome["strip_parse"]
	if strip_parse is None or strip_parse["error"]:
		errors.append("triangle-strip stream did not parse")
	else:
		expected_headers = [2 * row + 1 for unused_face in range(7 if failure_at == 4 else 8)
			for row in range(1, s + 1)]
		if strip_parse["headers"] != expected_headers:
			errors.append("triangle-strip header sequence differs")
		if not all(0 <= index < vertex_count for index in strip_parse["all_indices"]):
			errors.append("triangle-strip vertex index outside returned vertex domain")

	if outcome["terminal"] == "return" and failure_at is None:
		if len(outcome["vertices"]) != 12 * vertex_count:
			errors.append("returned vertex array size is not modeled")
		else:
			for index in range(vertex_count):
				x, y, z = struct.unpack_from("<3f", outcome["vertices"], 12 * index)
				magnitude = math.sqrt(x * x + y * y + z * z)
				if not math.isfinite(magnitude) or abs(magnitude - 1.0) > 2e-6:
					errors.append("vertex %d is not a finite unit vector: %.9g" % (index, magnitude))
					break
	return errors


def canonical_outcome(outcome: dict) -> dict:
	return {
		"terminal": outcome["terminal"],
		"events": outcome["events"],
		"graph_counts": outcome["graph_counts"],
		"helper_counts": outcome["helper_counts"],
		"normalize_inputs_sha256": sha256_bytes(b"".join(outcome["normalize_inputs"])),
		"heap_sha256": outcome["heap_sha256"],
		"allocation_hashes": outcome["allocations"],
		"sphere": outcome["sphere"],
		"vertices_sha256": sha256_bytes(outcome["vertices"]),
		"strip_used_sha256": sha256_bytes(outcome["strip_used"]),
		"eax": outcome["eax"] if outcome["terminal"] == "return" else None,
		"esp": outcome["esp"] if outcome["terminal"] == "return" else None,
		"direction_flag": bool(outcome["eflags"] & 0x400) if outcome["terminal"] == "return" else None,
		"fpcw": outcome["fpcw"],
		"fpu_top": (outcome["fpsw"] >> 11) & 7,
		"registers": outcome["registers"] if outcome["terminal"] == "return" else None,
		"caller_frame_sha256": sha256_bytes(outcome["caller_frame"]),
	}


def difference(actual: dict, expected: dict) -> list[str]:
	return [key for key in expected if actual.get(key) != expected[key]]


def ordered_float_int(bits: int) -> int:
	if bits & 0x80000000:
		return 0x80000000 - (bits & 0x7FFFFFFF)
	return 0x80000000 + bits


def numeric_vertex_delta(left: bytes, right: bytes) -> dict | None:
	if len(left) != len(right) or len(left) % 4:
		return None
	max_abs = 0.0
	max_ulp = 0
	differing = 0
	for offset in range(0, len(left), 4):
		left_bits = struct.unpack_from("<I", left, offset)[0]
		right_bits = struct.unpack_from("<I", right, offset)[0]
		if left_bits == right_bits:
			continue
		differing += 1
		left_float = struct.unpack("<f", p32(left_bits))[0]
		right_float = struct.unpack("<f", p32(right_bits))[0]
		if math.isfinite(left_float) and math.isfinite(right_float):
			max_abs = max(max_abs, abs(left_float - right_float))
			max_ulp = max(max_ulp, abs(ordered_float_int(left_bits) - ordered_float_int(right_bits)))
		else:
			max_abs = float("inf")
			max_ulp = 0xFFFFFFFF
	return {
		"float_count": len(left) // 4,
		"differing_float_count": differing,
		"max_absolute_delta": max_abs,
		"max_ulp_delta": max_ulp,
	}


def summarize_trace(outcome: dict) -> dict:
	canonical = canonical_outcome(outcome)
	return {
		"terminal": canonical["terminal"],
		"external_call_count": len(canonical["events"]),
		"allocation_call_count": len(outcome["allocation_calls"]),
		"graph_counts": canonical["graph_counts"],
		"helper_counts": canonical["helper_counts"],
		"heap_sha256": canonical["heap_sha256"],
		"vertices_sha256": canonical["vertices_sha256"],
		"strip_used_sha256": canonical["strip_used_sha256"],
		"normalize_inputs_sha256": canonical["normalize_inputs_sha256"],
		"sphere": canonical["sphere"],
	}


def run_role(
	role: str,
	path: Path,
	paths: dict[str, Path],
	cases: list[dict],
	target_reference: dict[str, dict] | None = None,
) -> tuple[dict, dict[str, dict], dict[str, dict]]:
	guard_image = GeosphereImage(
		role,
		path,
		paths["target_math"],
		paths["target_squared"],
		paths["actual"],
	)
	guards = guard_image.guards()
	traces = {}
	internal = {}
	semantic_failures = []
	target_mismatches = []
	for case_spec in cases:
		# A fresh Unicorn instance makes FPU/control state and hook lifetime part
		# of the deterministic case boundary, not an accidental prior-case input.
		image = GeosphereImage(
			role,
			path,
			paths["target_math"],
			paths["target_squared"],
			paths["actual"],
		)
		outcome = image.run(case_spec)
		errors = validate_geometry(case_spec, outcome)
		canonical = canonical_outcome(outcome)
		traces[case_spec["name"]] = canonical
		internal[case_spec["name"]] = outcome
		if errors:
			semantic_failures.append({"case": case_spec["name"], "errors": errors})
		if target_reference is not None:
			expected = target_reference[case_spec["name"]]
			components = difference(canonical, expected)
			if components:
				delta = numeric_vertex_delta(
					outcome["vertices"],
					target_reference["__internal__"][case_spec["name"]]["vertices"],
				)
				target_mismatches.append({
					"case": case_spec["name"],
					"components": components,
					"vertex_numeric_delta": delta,
				})
	payload = json.dumps(traces, sort_keys=True, separators=(",", ":")).encode("utf-8")
	passed = not semantic_failures and not target_mismatches
	return ({
		"path": str(path),
		"sha256": sha256_path(path),
		"guards": guards,
		"case_count": len(cases),
		"trace_sha256": sha256_bytes(payload),
		"traces": {name: summarize_trace(internal[name]) for name in internal},
		"semantic_failure_count": len(semantic_failures),
		"target_mismatch_count": len(target_mismatches),
		"semantic_failures": semantic_failures,
		"target_mismatches": target_mismatches,
		"pass": passed,
	}, traces, internal)


def verify_manifests(paths: dict[str, Path]) -> dict:
	before = json.loads(paths["before_manifest"].read_text(encoding="utf-8"))
	final = json.loads(paths["final_manifest"].read_text(encoding="utf-8"))
	checks = {
		"before_phase": before.get("phase") == "before",
		"final_phase": final.get("phase") == "final",
		"target_frozen_hash": before["frozen_files"]["build/split/source/math/geometry.obj"]["sha256"] == DEFAULT_HASHES["target"],
		"final_actual_hash": final["objects"]["source/math/geometry"]["sha256"] == DEFAULT_HASHES["actual"],
		"final_source_hash": final["frozen_files"]["source/math/geometry.c"]["sha256"] == DEFAULT_HASHES["source"],
		"final_header_hash": final["frozen_files"]["source/math/geometry.h"]["sha256"] == DEFAULT_HASHES["header"],
		"final_config_hash": final["frozen_files"]["config/symbols.json"]["sha256"] == DEFAULT_HASHES["config"],
		"same_six_consumers": before.get("units") == final.get("units") and len(final.get("units", [])) == 6,
	}
	failed = [name for name, value in checks.items() if not value]
	if failed:
		raise AssertionError(("manifest provenance guard", failed))
	return checks


def verify_typed_source(path: Path) -> dict:
	text = path.read_text(encoding="utf-8")
	start = text.rindex("static void calculate_vertex(")
	end = text.index("/* ---------- private code */", start + 1)
	body = text[start:end]
	checks = {
		"typed_local": "real_vector3d vertex;" in body,
		"semantic_set_vector": "set_real_vector3d(&vertex," in body,
		"real_normalize_call": "normalize3d(&vertex);" in body,
		"semantic_point_copyout": "set_real_point3d(&sphere->vertices[new_vertex], vertex.i, vertex.j, vertex.k);" in body,
		"no_point_vector_pointer_cast": "(real_vector3d *)" not in body and "(real_point3d *)" not in body,
	}
	failed = [name for name, value in checks.items() if not value]
	if failed:
		raise AssertionError(("typed calculate_vertex source guard", failed))
	return checks


def verify_production_caller(path: Path) -> dict:
	obj = cc.load(path)
	functions = function_sections(obj)
	function = functions["_random_math_initialize"]
	info = function.info
	section = obj["sections"][function.section_number - 1]
	raw = bytes(cc._section_bytes(obj, section))
	geosphere_relocations = []
	for relocation in info["relocations"]:
		parsed = relocation_name_addend(relocation)
		if parsed is not None and parsed[0] == "_geosphere_new":
			geosphere_relocations.append(relocation)
	if len(geosphere_relocations) != 1:
		raise AssertionError(("random_math_initialize geosphere_new relocation count", len(geosphere_relocations)))
	relocation = geosphere_relocations[0]
	call_opcode_offset = relocation["address"] - 1
	# January sequence: push 0x10; store the return target global; call geosphere_new.
	window = raw[call_opcode_offset - 8:call_opcode_offset + 1]
	checks = {
		"function_size_192": info["size"] == 192,
		"function_hash": info["normalized_sha256"] == "7fed98c1211ef94f1e12e3a31801d56d5aedd21b9bc84603d8a4be42e0ce08c2",
		"rel32_call": relocation["type"] == cc.IMAGE_REL_I386_REL32 and raw[call_opcode_offset] == 0xE8,
		"literal_16_push_before_call": len(window) == 9 and window[0:2] == b"\x6A\x10" and window[2:4] == b"\x89\x35",
	}
	failed = [name for name, value in checks.items() if not value]
	if failed:
		raise AssertionError(("production caller guard", failed, window.hex()))
	return checks


def run_negative_controls(reference: dict[str, dict]) -> dict:
	base = reference["valid_segments_2_fpcw_027f"]
	mutations = {
		"allocator_argument": ("events", lambda value: value["events"][0]["args"].__setitem__(0, 24)),
		"external_call_order": ("events", lambda value: value["events"].reverse()),
		"same_object_private_count": ("graph_counts", lambda value: value["graph_counts"].__setitem__("_subdivide_triangle", 7)),
		"real_math_call_count": ("helper_counts", lambda value: value["helper_counts"].__setitem__("_normalize3d", 0)),
		"normalize_input": ("normalize_inputs_sha256", lambda value: value.__setitem__("normalize_inputs_sha256", "0" * 64)),
		"heap_state": ("heap_sha256", lambda value: value.__setitem__("heap_sha256", "1" * 64)),
		"owned_vertex_state": ("vertices_sha256", lambda value: value.__setitem__("vertices_sha256", "2" * 64)),
		"eax": ("eax", lambda value: value.__setitem__("eax", value["eax"] + 4)),
		"esp": ("esp", lambda value: value.__setitem__("esp", value["esp"] - 4)),
		"callee_saved": ("registers", lambda value: value["registers"].__setitem__("esi", 0)),
		"caller_frame": ("caller_frame_sha256", lambda value: value.__setitem__("caller_frame_sha256", "f" * 64)),
	}
	controls = {}
	for name, (expected_component, mutate) in mutations.items():
		mutant = copy.deepcopy(base)
		mutate(mutant)
		controls[name] = expected_component in difference(mutant, base)
	guard_bytes = bytearray(b"frozen-input")
	expected_hash = sha256_bytes(guard_bytes)
	guard_bytes[0] ^= 1
	controls["sha256_guard"] = sha256_bytes(guard_bytes) != expected_hash
	return {"controls": controls, "pass": all(controls.values())}


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description=__doc__)
	for key, default in DEFAULT_PATHS.items():
		parser.add_argument("--" + key.replace("_", "-"), default=None,
			help="override %s (default: %s)" % (key, default))
	return parser.parse_args()


def main() -> int:
	args = parse_args()
	paths = {
		key: resolve_path(getattr(args, key), default)
		for key, default in DEFAULT_PATHS.items()
	}
	input_guards = {
		key: verify_hash(path, DEFAULT_HASHES[key], key)
		for key, path in paths.items()
	}
	manifest_guards = verify_manifests(paths)
	source_guards = verify_typed_source(paths["source"])
	caller_guards = verify_production_caller(paths["caller"])
	cases = all_cases()

	target_report, target_reference, target_internal = run_role(
		"target", paths["target"], paths, cases,
	)
	target_reference["__internal__"] = target_internal
	candidate_report, unused_candidate_reference, unused_candidate_internal = run_role(
		"candidate", paths["candidate"], paths, cases, target_reference,
	)
	actual_report, unused_actual_reference, unused_actual_internal = run_role(
		"actual", paths["actual"], paths, cases, target_reference,
	)
	negative_controls = run_negative_controls(target_reference)

	report = {
		"schema": 1,
		"tool": "geometry_fable_geosphere_runtime_differential_20260906",
		"scope": {
			"public_entry": "_geosphere_new",
			"same_object_private_graph": list(GRAPH_FUNCTIONS[1:]),
			"real_math_entry": "_normalize3d",
			"emitted_math_helpers_authenticated": list(MATH_HELPERS),
			"stubbed_public_apis": list(STUB_NAMES),
			"case_count": len(cases),
			"valid_segments": [2, 3, 16],
			"explicit_x87_control_words": ["0x027f", "0x037f"],
			"production_segment_evidence": "January random_math_initialize pushes literal 16 before its geosphere_new REL32 call",
			"failure_behavior_warning": "January child/per-face allocation failures are modeled faithfully, including dangling pointers, incomplete success, and fatal assertions; this is not a safety claim.",
		},
		"inputs": input_guards,
		"manifest_guards": manifest_guards,
		"typed_source_guards": source_guards,
		"production_caller_guards": caller_guards,
		"cases": cases,
		"target": target_report,
		"candidate": candidate_report,
		"actual": actual_report,
		"synthetic_outcome_negative_controls": negative_controls,
		"limitations": [
			"Only geosphere_new is entered; private function signatures are never exposed to the harness.",
			"debug_malloc/debug_free/display_assert/system_exit are deterministic cdecl stubs; allocator metadata and process teardown are out of scope. A nonzero debug_malloc clear argument is rejected rather than silently treated as an ordinary uninitialized allocation.",
			"The real normalize3d COFF executes. Other authenticated math COMDATs are materialized but correctly remain uncalled by this graph.",
			"Exact heap, vertex, strip, normalize-input, call-trace, public ABI, and register comparisons are required for PASS. Numeric vertex deltas are reported only as diagnostics after an exact mismatch; they never relax PASS.",
			"Original allocation-failure bugs are evidence, not endorsed contracts or proof of memory safety.",
			"Negative controls are synthetic mutations of captured canonical outcomes and one synthetic hash payload; they are not mutations of any input binary.",
		],
	}
	report["pass"] = all((
		target_report["pass"],
		candidate_report["pass"],
		actual_report["pass"],
		negative_controls["pass"],
	))
	print(json.dumps(report, indent=2, sort_keys=True))
	return 0 if report["pass"] else 1


if __name__ == "__main__":
	raise SystemExit(main())
