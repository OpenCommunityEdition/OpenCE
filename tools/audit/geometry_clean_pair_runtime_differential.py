"""Frozen target-first differential for the clean Fable Geometry pair.

The script executes the January and independently compiled candidate/actual COFF
sections in separate Unicorn instances.  It covers only convex_hull3d and
convex_hull3d_test_point.  Public dependencies of the wrapper are narrow cdecl
recording stubs; no private ABI or whole-game state is modeled.

It also verifies the frozen owner packet: the schema-only build differs from the
before build only by COFF timestamps in all six consumers, the five non-owner
pair consumers remain timestamp-normalized byte-identical, and geometry.obj adds
exactly the two intended function owners and four assertion-string COMDATs.

Every default input is SHA-256 guarded.  The script writes JSON to stdout only.
"""

import argparse
import copy
import difflib
import hashlib
import json
import math
from pathlib import Path
import re
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


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


DEFAULT_PATHS = {
	"target": ROOT / "scratch/geometry-fable-owner-pair-20260906/files/build/split/source/math/geometry.obj",
	"candidate": ROOT / "scratch/geometry-fable-clean-pair-first-20260906.obj",
	"actual": ROOT / "scratch/geometry-fable-owner-pair-20260906/source/math/geometry.obj",
	"source": ROOT / "scratch/geometry-fable-owner-pair-20260906/files/source/math/geometry.c",
	"header": ROOT / "scratch/geometry-fable-owner-pair-20260906/files/source/math/geometry.h",
	"config": ROOT / "scratch/geometry-fable-owner-pair-20260906/files/config/symbols.json",
	"before_manifest": ROOT / "scratch/geometry-fable-owner-before-20260906.json",
	"schema_manifest": ROOT / "scratch/geometry-fable-owner-schema-20260906.json",
	"pair_manifest": ROOT / "scratch/geometry-fable-owner-pair-20260906.json",
}

DEFAULT_HASHES = {
	"target": "9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c",
	"candidate": "3b4b1a60c4536730da1374725d5905046621dc79fd8c3315a3e85046b463af94",
	"actual": "c3ab4ebdedeb476cc64bc4a0e8db747c05616fe4d7e121e75ee5b8863725171c",
	"source": "20c7baa58e7e1e4eaa93be5f7ddbf752c76684b2e8d61a8f5b3ccde0c9db0990",
	"header": "04f7b7b7d027c0621d93410abd856c9cb42b6268164ad8db4a78647d91c5411f",
	"config": "c2224cc0088411edbe1501013e7f4536c2e0c45db7bbe5a545f8e137e740ed5a",
	"before_manifest": "5a236fcd242d14b2bad7d3b7359e1eb9b9e4800c3242700c9b2d94ad94ceb63f",
	"schema_manifest": "d774719f7fe36d0426d5878fc787ea455d36b09fb810c323946e04edb5ac62eb",
	"pair_manifest": "b5aa381d9dc3c1dc56bec46816d73a164f58c5b142144b83c8cf39bbc6360d78",
}

FUNCTIONS = {
	"hull": {
		"symbol": "_convex_hull3d",
		"size": 272,
		"relocation_count": 18,
		"normalized_sha256": "d9c64e0f0e98d708c3746dfb07260bc5d359100baa561935645f7d9779dc114b",
	},
	"point": {
		"symbol": "_convex_hull3d_test_point",
		"size": 112,
		"relocation_count": 1,
		"normalized_sha256": "bebd453d4592cbd7e4a7e64228c863bd6049caba61f8c5bd76a6542cf79b7568",
	},
}

CODE = 0x100000
STOP = 0x10F000
STUBS = 0x200000
STRINGS = 0x300000
GLOBALS = 0x400000
DATA = 0x500000
STACK = 0x700000
ENTRY_SP = STACK + 0xC000

POINTS = DATA + 0x0000
VERTICES = DATA + 0x2000
EDGES = DATA + 0x4000
SURFACES = DATA + 0x6000
TEST_POINT = DATA + 0x8000
DATA_SIZE = 0x10000

STUB_NAMES = (
	"_display_assert",
	"_system_exit",
	"_convex_hull3d_begin",
	"_convex_hull3d_expand",
)
STUB_ADDRESSES = {
	name: STUBS + 0x100 * (index + 1)
	for index, name in enumerate(STUB_NAMES)
}

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
EPSILON_BITS = struct.pack("<f", 0.001)

EXPECTED_UNITS = (
	"source/game/game_engine_king",
	"source/math/geometry",
	"source/physics/breakable_surfaces",
	"source/structures/leaf_map",
	"source/structures/structure_lens_flares",
	"source/structures/structure_visibility",
)

EXPECTED_HEADER_OUTPUTS = (
	"build/base/source/game/game_engine_king.obj",
	"build/base/source/math/geometry.obj",
	"build/base/source/physics/breakable_surfaces.obj",
	"build/base/source/structures/leaf_map.obj",
	"build/base/source/structures/structure_lens_flares.obj",
	"build/base/source/structures/structure_visibility.obj",
)

EXPECTED_COFF_COUNTS = {
	"build/base": 572,
	"build/split": 833,
}

LITERALS = {
	"??_C@_06GPGIDCJA@points?$AA@": b"points\0",
	"??_C@_08BOLPBPCL@vertices?$AA@": b"vertices\0",
	"??_C@_05JGEJPNHM@edges?$AA@": b"edges\0",
	"??_C@_08FGPNIDBO@surfaces?$AA@": b"surfaces\0",
}

EXPECTED_LITERAL_OWNER_HASHES = {
	"build/base/source/math/geometry.obj": "c3ab4ebdedeb476cc64bc4a0e8db747c05616fe4d7e121e75ee5b8863725171c",
	"build/base/source/objects/widgets/lightning.obj": "7c2f484593c300726707a8a468a234eb8b0f4558c959f37e3bade70f922e2740",
	"build/base/source/render/render_debug.obj": "94cd1794b3e0696ecb3e9bef66b308ddd07c4995a498e87ac217ba256cb8908e",
	"build/base/source/render/triangle_strips.obj": "6c097264ac976ae35b7a0a986e417a6065bd771c0dcab753b07e3ee0d2479f26",
	"build/base/source/tool/error_geometry.obj": "45570b2b36f931bd0cf0e9f685f5295c98a4269e7adeb810bf09481f4c36892a",
	"build/split/source/math/geometry.obj": "9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c",
	"build/split/source/tool/error_geometry.obj": "fe872618f054d9641d5974279d2fb9c4a68a55eb11915662853194903371c0c6",
}

EXPECTED_LITERAL_OWNERS = {
	("build/base/source/math/geometry.obj", "??_C@_08FGPNIDBO@surfaces?$AA@"),
	("build/base/source/math/geometry.obj", "??_C@_05JGEJPNHM@edges?$AA@"),
	("build/base/source/math/geometry.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
	("build/base/source/math/geometry.obj", "??_C@_06GPGIDCJA@points?$AA@"),
	("build/base/source/objects/widgets/lightning.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
	("build/base/source/render/render_debug.obj", "??_C@_06GPGIDCJA@points?$AA@"),
	("build/base/source/render/triangle_strips.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
	("build/base/source/tool/error_geometry.obj", "??_C@_06GPGIDCJA@points?$AA@"),
	("build/split/source/math/geometry.obj", "??_C@_08FGPNIDBO@surfaces?$AA@"),
	("build/split/source/math/geometry.obj", "??_C@_05JGEJPNHM@edges?$AA@"),
	("build/split/source/math/geometry.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
	("build/split/source/tool/error_geometry.obj", "??_C@_06GPGIDCJA@points?$AA@"),
}

EXPECTED_LITERAL_REFERENCES = {
	("build/split/source/math/geometry.obj", "??_C@_06GPGIDCJA@points?$AA@"),
	("build/split/source/objects/widgets/lightning.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
	("build/split/source/render/render_debug.obj", "??_C@_06GPGIDCJA@points?$AA@"),
	("build/split/source/render/triangle_strips.obj", "??_C@_08BOLPBPCL@vertices?$AA@"),
}


def sha256_bytes(raw):
	return hashlib.sha256(raw).hexdigest()


def sha256_path(path):
	return sha256_bytes(path.read_bytes())


def p32(value):
	return struct.pack("<I", value & 0xFFFFFFFF)


def signed32(value):
	return struct.unpack("<i", p32(value))[0]


def seeded_bytes(size, salt):
	return bytes(((index * 37 + salt * 53 + 11) & 0xFF) for index in range(size))


def resolve_path(path):
	return path.resolve() if path.is_absolute() else (Path.cwd() / path).resolve()


def verify_hash(path, expected, label):
	actual = sha256_path(path)
	if actual.lower() != expected.lower():
		raise AssertionError((label + " SHA-256 mismatch", str(path), actual, expected))
	return {"path": str(path), "sha256": actual, "size": path.stat().st_size}


def timestamp_normalized_sha256(path):
	raw = bytearray(path.read_bytes())
	if len(raw) < 20:
		raise AssertionError(("not a COFF object", str(path), len(raw)))
	raw[4:8] = b"\0\0\0\0"
	return sha256_bytes(raw)


def symbol_blob(obj, name):
	symbol = cc.symbol(obj, name)
	section = obj["sections"][symbol["section"] - 1]
	raw = bytes(cc._section_bytes(obj, section))
	return raw[symbol["value"]:]


def symbol_cstring(obj, name):
	raw = symbol_blob(obj, name)
	if b"\0" not in raw:
		raise AssertionError(("unterminated COFF literal", name))
	return raw.split(b"\0", 1)[0].decode("ascii")


def symbolic_relocation_target(relocation):
	target = relocation.get("symbolic_target", relocation["target"])
	if target[0] == "symbol":
		return target[1], target[2]
	if target[0] == "defined-noncode":
		return target[2], target[3]
	raise AssertionError(("unsupported relocation owner", target))


def logical_relocation_fingerprint(info):
	result = []
	for relocation in info["relocations"]:
		name, addend = symbolic_relocation_target(relocation)
		result.append([
			relocation["address"],
			relocation["type"],
			name,
			addend,
		])
	return result


def literal_catalog(obj):
	result = {}
	for symbol in obj["symbols"]:
		if symbol["section"] <= 0 or not symbol["name"].startswith("??_C@"):
			continue
		try:
			result[symbol["name"]] = symbol_cstring(obj, symbol["name"])
		except (cc.CoffError, UnicodeDecodeError, AssertionError):
			pass
	return result


def prepare_function(path, function_key, fallback_literals):
	obj = cc.load(path)
	spec = FUNCTIONS[function_key]
	available = function_sections(obj)
	if spec["symbol"] not in available:
		raise AssertionError((str(path), "missing function", spec["symbol"]))
	function = available[spec["symbol"]]
	section = obj["sections"][function.section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))
	if len(raw) > 0x1000:
		raise AssertionError((spec["symbol"], "section too large", len(raw)))

	strings_by_symbol = {}
	for relocation in function.info["relocations"]:
		name, unused_addend = symbolic_relocation_target(relocation)
		if name.startswith("??_C@"):
			try:
				strings_by_symbol[name] = symbol_cstring(obj, name)
			except cc.CoffError:
				if name not in fallback_literals:
					raise AssertionError((str(path), "unresolved literal", name))
				strings_by_symbol[name] = fallback_literals[name]

	string_addresses = {
		text: STRINGS + 0x200 * (index + 1)
		for index, text in enumerate(sorted(set(strings_by_symbol.values())))
	}
	for relocation in function.info["relocations"]:
		name, addend = symbolic_relocation_target(relocation)
		if name in STUB_ADDRESSES:
			destination = STUB_ADDRESSES[name] + addend
		elif name == "_global_convex_hull3d_epsilon":
			destination = GLOBALS + addend
		elif name in strings_by_symbol:
			destination = string_addresses[strings_by_symbol[name]] + addend
		else:
			raise AssertionError((function_key, "unsupported dependency", name, addend))
		if relocation["type"] == cc.IMAGE_REL_I386_REL32:
			destination -= CODE + relocation["address"] + 4
		elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
			raise AssertionError((function_key, "unsupported relocation type", relocation["type"]))
		raw[relocation["address"]:relocation["address"] + 4] = p32(destination)

	try:
		epsilon = symbol_blob(obj, "_global_convex_hull3d_epsilon")[:4]
	except cc.CoffError:
		epsilon = EPSILON_BITS
	return {
		"path": str(path),
		"function_key": function_key,
		"symbol": function.name,
		"info": function.info,
		"raw": bytes(raw),
		"strings": {address: text for text, address in string_addresses.items()},
		"epsilon": epsilon,
		"relocation_fingerprint": logical_relocation_fingerprint(function.info),
	}


def verify_section(prepared):
	spec = FUNCTIONS[prepared["function_key"]]
	info = prepared["info"]
	checks = {
		"symbol": prepared["symbol"] == spec["symbol"],
		"size": info["size"] == spec["size"],
		"relocation_count": info["relocation_count"] == spec["relocation_count"],
		"normalized_sha256": info["normalized_sha256"] == spec["normalized_sha256"],
		"epsilon_0_001f": prepared["epsilon"] == EPSILON_BITS,
	}
	failed = [name for name, passed in checks.items() if not passed]
	if failed:
		raise AssertionError((prepared["symbol"], "section guard", failed))
	return checks


def wrapper_case(name, **values):
	case = {
		"function": "hull",
		"name": name,
		"point_count": 3,
		"vertex_count": 4,
		"edge_count": 5,
		"surface_count": 6,
		"begin_result": 1,
		"expand_results": [1, 1, 1],
		"seed": sum(ord(character) for character in name) & 0xFF,
	}
	case.update(values)
	return case


def point_case(name, surfaces, point, expected):
	return {
		"function": "point",
		"name": name,
		"surfaces": surfaces,
		"point": point,
		"expected_eax_low": expected,
		"seed": sum(ord(character) for character in name) & 0xFF,
	}


def all_cases():
	epsilon = struct.unpack("<f", EPSILON_BITS)[0]
	return [
		wrapper_case("begin_failure", begin_result=0),
		wrapper_case("expand_failure", expand_results=[1, 0, 1]),
		wrapper_case("success"),
		wrapper_case("zero_point_count", point_count=0, expand_results=[]),
		wrapper_case("null_points", null_argument=1, assertion=("points", 2284)),
		wrapper_case("null_vertices", null_argument=3, assertion=("vertices", 2285)),
		wrapper_case("null_edges", null_argument=5, assertion=("edges", 2286)),
		wrapper_case("null_surfaces", null_argument=7, assertion=("surfaces", 2287)),
		point_case("zero_surface_count", [], (7.0, 8.0, 9.0), 1),
		point_case("nonextant_ignored", [(0, 1.0, 0.0, 0.0, -100.0)], (10.0, 0.0, 0.0), 1),
		point_case("inside", [(1, 1.0, 0.0, 0.0, 1.0)], (0.25, 4.0, -3.0), 1),
		point_case("outside", [(1, 1.0, 0.0, 0.0, 0.0)], (0.002, 0.0, 0.0), 0),
		point_case("epsilon_boundary", [(1, 1.0, 0.0, 0.0, 0.0)], (epsilon, 0.0, 0.0), 1),
		point_case("nan_is_not_greater", [(1, 1.0, 0.0, 0.0, 0.0)], (float("nan"), 0.0, 0.0), 1),
		point_case("positive_infinity", [(1, 1.0, 0.0, 0.0, 0.0)], (float("inf"), 0.0, 0.0), 0),
		point_case("mixed_extant_break", [
			(0, 1.0, 0.0, 0.0, -50.0),
			(1, 0.0, 1.0, 0.0, 5.0),
			(1, 0.0, 0.0, 1.0, -1.0),
		], (0.0, 1.0, 2.0), 0),
	]


def pointer_label(value):
	labels = {
		POINTS: "points",
		VERTICES: "vertices",
		EDGES: "edges",
		SURFACES: "surfaces",
		TEST_POINT: "point",
		0: "NULL",
	}
	return labels.get(value, "0x%08x" % value)


def make_surface_bytes(records):
	raw = bytearray(28 * len(records))
	for index, record in enumerate(records):
		extant, normal_i, normal_j, normal_k, distance = record
		offset = index * 28
		raw[offset] = extant & 0xFF
		struct.pack_into("<4f", raw, offset + 4, normal_i, normal_j, normal_k, distance)
		struct.pack_into("<ii", raw, offset + 20, 1000 + index, 2000 + index)
	return bytes(raw)


def prepare_case_memory(case):
	raw = bytearray(seeded_bytes(DATA_SIZE, case["seed"] + 17))
	if case["function"] == "hull":
		arguments = [
			case["point_count"], POINTS,
			case["vertex_count"], VERTICES,
			case["edge_count"], EDGES,
			case["surface_count"], SURFACES,
		]
		if "null_argument" in case:
			arguments[case["null_argument"]] = 0
	else:
		surfaces = make_surface_bytes(case["surfaces"])
		raw[SURFACES - DATA:SURFACES - DATA + len(surfaces)] = surfaces
		struct.pack_into("<3f", raw, TEST_POINT - DATA, *case["point"])
		arguments = [
			3, POINTS,
			4, VERTICES,
			5, EDGES,
			len(case["surfaces"]), SURFACES,
			TEST_POINT,
		]
	return bytes(raw), arguments


def read32(machine, address):
	return struct.unpack("<I", machine.mem_read(address, 4))[0]


def machine_argument(machine, index):
	return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + index * 4)


def read_cstring(machine, address, limit=512):
	raw = bytearray()
	for offset in range(limit):
		value = machine.mem_read(address + offset, 1)[0]
		if value == 0:
			return raw.decode("ascii")
		raw.append(value)
	raise AssertionError(("runtime string unterminated", hex(address)))


def hull_api_arguments(machine, count):
	values = [machine_argument(machine, index) for index in range(count)]
	result = []
	for index, value in enumerate(values):
		if index in (1, 3, 5, 7):
			result.append(pointer_label(value))
		else:
			result.append(signed32(value))
	return result


def execute_case(prepared, case):
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x10000),
		(STUBS, 0x10000),
		(STRINGS, 0x10000),
		(GLOBALS, 0x10000),
		(DATA, DATA_SIZE),
		(STACK, 0x10000),
	):
		machine.mem_map(address, size)
	machine.mem_write(CODE, prepared["raw"])
	for address in STUB_ADDRESSES.values():
		machine.mem_write(address, b"\xC3")
	for address, text in prepared["strings"].items():
		machine.mem_write(address, text.encode("ascii") + b"\0")
	machine.mem_write(GLOBALS, prepared["epsilon"])

	initial_data, arguments = prepare_case_memory(case)
	machine.mem_write(DATA, initial_data)
	frame = p32(STOP) + b"".join(p32(argument) for argument in arguments)
	machine.mem_write(ENTRY_SP - 0x800, seeded_bytes(0x800, case["seed"] + 41))
	machine.mem_write(ENTRY_SP, frame)
	machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
	machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
	machine.reg_write(UC_X86_REG_EAX, 0xA0A1A2A3)
	machine.reg_write(UC_X86_REG_ECX, 0xB0B1B2B3)
	machine.reg_write(UC_X86_REG_EDX, 0xC0C1C2C3)
	for register, value in SAVED_REGISTERS.items():
		machine.reg_write(register, value)

	events = []
	hook_errors = []
	terminal = {"kind": None}
	expand_call_index = {"value": 0}

	def record(api, api_arguments):
		events.append({
			"api": api,
			"stack_depth": ENTRY_SP - machine.reg_read(UC_X86_REG_ESP),
			"args": api_arguments,
		})

	def poison_volatile(eax):
		machine.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
		machine.reg_write(UC_X86_REG_ECX, 0xC1C2C3C4)
		machine.reg_write(UC_X86_REG_EDX, 0xD1D2D3D4)

	def stub_hook(unused_machine, address, unused_size, unused_data):
		name = next((key for key, value in STUB_ADDRESSES.items() if value == address), None)
		if name is None:
			hook_errors.append("unknown stub 0x%08x" % address)
			machine.emu_stop()
			return
		try:
			if name == "_display_assert":
				record(name, [
					read_cstring(machine, machine_argument(machine, 0)),
					read_cstring(machine, machine_argument(machine, 1)),
					machine_argument(machine, 2),
					machine_argument(machine, 3),
				])
				poison_volatile(0xD1D2D3D4)
			elif name == "_system_exit":
				record(name, [signed32(machine_argument(machine, 0))])
				terminal["kind"] = "system_exit"
				machine.emu_stop()
			elif name == "_convex_hull3d_begin":
				record(name, hull_api_arguments(machine, 8))
				poison_volatile(0x5A5A5A00 | (case["begin_result"] & 0xFF))
			elif name == "_convex_hull3d_expand":
				record(name, hull_api_arguments(machine, 9))
				index = expand_call_index["value"]
				expand_call_index["value"] += 1
				results = case["expand_results"]
				result = results[index] if index < len(results) else 1
				poison_volatile(0x6B6B6B00 | (result & 0xFF))
			else:
				raise AssertionError(("unhandled stub", name))
		except (AssertionError, u.UcError) as error:
			hook_errors.append(repr(error))
			machine.emu_stop()

	machine.hook_add(u.UC_HOOK_CODE, stub_hook, begin=STUBS, end=STUBS + 0xFFFF)
	execution_error = None
	try:
		machine.emu_start(CODE, STOP, count=10000)
	except u.UcError as error:
		execution_error = str(error)
	if terminal["kind"] is None and execution_error is None and not hook_errors:
		terminal["kind"] = "return" if machine.reg_read(UC_X86_REG_EIP) == STOP else "other"
	return {
		"terminal": terminal["kind"],
		"execution_error": execution_error,
		"hook_errors": hook_errors,
		"events": events,
		"data": bytes(machine.mem_read(DATA, DATA_SIZE)),
		"initial_data": initial_data,
		"eax": machine.reg_read(UC_X86_REG_EAX),
		"eip": machine.reg_read(UC_X86_REG_EIP),
		"esp": machine.reg_read(UC_X86_REG_ESP),
		"eflags": machine.reg_read(UC_X86_REG_EFLAGS),
		"registers": {
			SAVED_REGISTER_NAMES[register]: machine.reg_read(register)
			for register in SAVED_REGISTERS
		},
		"caller_frame": bytes(machine.mem_read(ENTRY_SP, len(frame))),
		"expected_caller_frame": frame,
	}


def wrapper_arguments(case):
	values = [
		case["point_count"], "points",
		case["vertex_count"], "vertices",
		case["edge_count"], "edges",
		case["surface_count"], "surfaces",
	]
	if "null_argument" in case:
		values[case["null_argument"]] = "NULL"
	return values


def expected_events(case):
	if case["function"] == "point":
		return []
	if "assertion" in case:
		expression, line = case["assertion"]
		pre_saved = expression == "points"
		return [
			{
				"api": "_display_assert",
				"stack_depth": 24 if pre_saved else 36,
				"args": [expression, SOURCE_FILE, line, 1],
			},
			{
				"api": "_system_exit",
				"stack_depth": 28 if pre_saved else 40,
				"args": [-1],
			},
		]
	arguments = wrapper_arguments(case)
	result = [{
		"api": "_convex_hull3d_begin",
		"stack_depth": 52,
		"args": arguments,
	}]
	if not case["begin_result"]:
		return result
	for point_index in range(case["point_count"]):
		result.append({
			"api": "_convex_hull3d_expand",
			"stack_depth": 56,
			"args": arguments + [point_index],
		})
		if point_index < len(case["expand_results"]) and not case["expand_results"][point_index]:
			break
	return result


def expected_return_low(case):
	if case["function"] == "point":
		return case["expected_eax_low"]
	if "assertion" in case:
		return None
	if not case["begin_result"]:
		return 0
	for index in range(case["point_count"]):
		if index < len(case["expand_results"]) and not case["expand_results"][index]:
			return 0
	return 1


def validate_semantics(case, outcome):
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected_terminal = "system_exit" if "assertion" in case else "return"
	if outcome["terminal"] != expected_terminal:
		errors.append("terminal %r != %r" % (outcome["terminal"], expected_terminal))
	if outcome["events"] != expected_events(case):
		errors.append("external-call order, arguments, or stack depth differ")
	if outcome["data"] != outcome["initial_data"]:
		errors.append("modeled point/vertex/edge/surface state changed")
	if outcome["caller_frame"] != outcome["expected_caller_frame"]:
		errors.append("caller argument frame changed")
	if expected_terminal == "return":
		expected_low = expected_return_low(case)
		if outcome["eax"] & 0xFF != expected_low:
			errors.append("AL %d != %d" % (outcome["eax"] & 0xFF, expected_low))
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
	return errors


def canonical_outcome(outcome):
	return {
		"terminal": outcome["terminal"],
		"events": outcome["events"],
		"data_sha256": sha256_bytes(outcome["data"]),
		"eax": outcome["eax"],
		"esp": outcome["esp"],
		"direction_flag": bool(outcome["eflags"] & 0x400),
		"registers": outcome["registers"] if outcome["terminal"] == "return" else None,
		"caller_frame_sha256": sha256_bytes(outcome["caller_frame"]),
	}


def difference(actual, expected):
	return [key for key in expected if actual.get(key) != expected[key]]


def run_object(path, role, grouped_cases, fallback_literals, target_reference=None):
	result = {"path": str(path), "sha256": sha256_path(path), "functions": {}, "pass": True}
	reference = {}
	for function_key in FUNCTIONS:
		prepared = prepare_function(path, function_key, fallback_literals)
		section_guards = verify_section(prepared)
		traces = {}
		semantic_failures = []
		target_mismatches = []
		for case in grouped_cases[function_key]:
			outcome = execute_case(prepared, case)
			errors = validate_semantics(case, outcome)
			canonical = canonical_outcome(outcome)
			traces[case["name"]] = canonical
			if errors:
				semantic_failures.append({"case": case["name"], "errors": errors})
			if target_reference is not None:
				expected = target_reference[function_key][case["name"]]
				components = difference(canonical, expected)
				if components:
					target_mismatches.append({"case": case["name"], "components": components})
		payload = json.dumps(traces, sort_keys=True, separators=(",", ":")).encode("utf-8")
		passed = not semantic_failures and not target_mismatches
		result["functions"][function_key] = {
			"symbol": prepared["symbol"],
			"size": prepared["info"]["size"],
			"relocation_count": prepared["info"]["relocation_count"],
			"normalized_sha256": prepared["info"]["normalized_sha256"],
			"relocation_fingerprint_sha256": sha256_bytes(json.dumps(
				prepared["relocation_fingerprint"], separators=(",", ":"),
			).encode("utf-8")),
			"section_guards": section_guards,
			"case_count": len(grouped_cases[function_key]),
			"trace_sha256": sha256_bytes(payload),
			"semantic_failure_count": len(semantic_failures),
			"target_mismatch_count": len(target_mismatches),
			"first_semantic_failures": semantic_failures[:3],
			"first_target_mismatches": target_mismatches[:3],
			"pass": passed,
		}
		result["pass"] = result["pass"] and passed
		reference[function_key] = traces
	return result, reference


def run_negative_controls(reference):
	controls = {}
	base_wrapper = reference["hull"]["success"]
	base_point = reference["point"]["outside"]
	mutations = {
		"external_argument": (base_wrapper, "events", lambda value: value["events"][0]["args"].__setitem__(0, 4)),
		"external_call_order": (base_wrapper, "events", lambda value: value["events"].reverse()),
		"eax_low_byte": (base_point, "eax", lambda value: value.__setitem__("eax", value["eax"] ^ 1)),
		"esp": (base_point, "esp", lambda value: value.__setitem__("esp", value["esp"] - 4)),
		"callee_saved": (base_point, "registers", lambda value: value["registers"].__setitem__("esi", 0)),
		"modeled_state": (base_point, "data_sha256", lambda value: value.__setitem__("data_sha256", "0" * 64)),
		"caller_frame": (base_point, "caller_frame_sha256", lambda value: value.__setitem__("caller_frame_sha256", "f" * 64)),
	}
	for name, (baseline, expected_component, mutate) in mutations.items():
		mutant = copy.deepcopy(baseline)
		mutate(mutant)
		controls[name] = expected_component in difference(mutant, baseline)
	return {"pass": all(controls.values()), "controls": controls}


def function_delta(left_obj, right_obj):
	left = function_sections(left_obj)
	right = function_sections(right_obj)
	added = sorted(set(right) - set(left))
	lost = sorted(set(left) - set(right))
	changed = sorted(
		name for name in set(left) & set(right)
		if not cc.section_infos_equal(left[name].info, right[name].info)
	)
	return {"added": added, "lost": lost, "changed": changed}


def common_symbols(obj):
	return sorted(
		(symbol["name"], symbol["value"], symbol["type"], symbol["storage"])
		for symbol in obj["symbols"]
		if symbol["section"] == 0 and symbol["value"] != 0
	)


def load_manifest(path):
	return json.loads(path.read_text(encoding="utf-8"))


def manifest_object(manifest, unit):
	path = Path(manifest["objects"][unit]["path"])
	if not path.is_absolute():
		path = ROOT / path
	path = path.resolve()
	expected = manifest["objects"][unit]["sha256"]
	actual = sha256_path(path)
	if actual != expected:
		raise AssertionError(("frozen manifest object changed", unit, actual, expected))
	return path


def manifest_frozen_file(manifest, relative):
	record = manifest["frozen_files"][relative]
	path = Path(record["path"])
	if not path.is_absolute():
		path = ROOT / path
	path = path.resolve()
	actual = sha256_path(path)
	if actual != record["sha256"]:
		raise AssertionError((
			"frozen manifest file changed",
			manifest["phase"],
			relative,
			actual,
			record["sha256"],
		))
	return path


def exact_insertions(before_text, after_text):
	before_lines = before_text.splitlines(keepends=True)
	after_lines = after_text.splitlines(keepends=True)
	result = []
	for tag, left_start, left_end, right_start, right_end in difflib.SequenceMatcher(
		None, before_lines, after_lines, autojunk=False,
	).get_opcodes():
		if tag != "equal":
			result.append({
				"tag": tag,
				"before": [left_start, left_end],
				"after": [right_start, right_end],
				"text": "".join(after_lines[right_start:right_end]),
			})
	return result


def defined_owner_names(obj):
	return {
		symbol["name"]
		for symbol in obj["symbols"]
		if symbol["section"] > 0
		and symbol["value"] == 0
		and symbol["storage"] in (2, 3)
		and symbol["name"] != obj["sections"][symbol["section"] - 1]["name"]
	}


def scan_link_literal_owners():
	"""Inventory every exact four-literal owner/reference in base and January trees."""
	owners = []
	references = []
	hash_cache = {}
	parsed_counts = {}
	parse_errors = []
	for tree_name, expected_count in EXPECTED_COFF_COUNTS.items():
		tree = ROOT / tree_name
		paths = sorted(tree.rglob("*.obj"))
		if len(paths) != expected_count:
			raise AssertionError(("COFF census size differs", tree_name, len(paths), expected_count))
		parsed_count = 0
		for path in paths:
			try:
				obj = cc.load(path)
			except cc.CoffError as error:
				parse_errors.append({"path": str(path), "error": str(error)})
				continue
			parsed_count += 1
			relative = path.relative_to(ROOT).as_posix()
			for symbol in obj["symbols"]:
				name = symbol["name"]
				if name not in LITERALS:
					continue
				if symbol["section"] > 0:
					section = obj["sections"][symbol["section"] - 1]
					payload = symbol_blob(obj, name)
					selection = _section_aux_selection(obj, symbol["section"])
					if payload != LITERALS[name] or section["size"] != len(LITERALS[name]):
						raise AssertionError(("competing literal payload differs", relative, name))
					if selection != 2:
						raise AssertionError(("competing literal is not select-any", relative, name, selection))
					if relative not in hash_cache:
						hash_cache[relative] = sha256_path(path)
					owners.append({
						"path": relative,
						"sha256": hash_cache[relative],
						"symbol": name,
						"payload": payload.decode("ascii", errors="replace"),
						"size": section["size"],
						"selection": selection,
					})
				elif symbol["section"] == 0:
					references.append({"path": relative, "symbol": name})
		parsed_counts[tree_name] = parsed_count
	if parse_errors:
		raise AssertionError(("COFF census parse failures", parse_errors[:10], len(parse_errors)))
	if parsed_counts != EXPECTED_COFF_COUNTS:
		raise AssertionError(("COFF census parsed counts differ", parsed_counts, EXPECTED_COFF_COUNTS))

	owner_keys = {(item["path"], item["symbol"]) for item in owners}
	reference_keys = {(item["path"], item["symbol"]) for item in references}
	if owner_keys != EXPECTED_LITERAL_OWNERS:
		raise AssertionError((
			"complete literal owner inventory differs",
			sorted(owner_keys - EXPECTED_LITERAL_OWNERS),
			sorted(EXPECTED_LITERAL_OWNERS - owner_keys),
		))
	if reference_keys != EXPECTED_LITERAL_REFERENCES:
		raise AssertionError((
			"complete literal reference inventory differs",
			sorted(reference_keys - EXPECTED_LITERAL_REFERENCES),
			sorted(EXPECTED_LITERAL_REFERENCES - reference_keys),
		))
	for relative, expected_hash in EXPECTED_LITERAL_OWNER_HASHES.items():
		if hash_cache.get(relative) != expected_hash:
			raise AssertionError((
				"competing literal owner SHA-256 differs",
				relative,
				hash_cache.get(relative),
				expected_hash,
			))
	owner_names = {item["symbol"] for item in owners}
	unresolved = sorted(
		item for item in reference_keys
		if item[1] not in owner_names
	)
	if unresolved:
		raise AssertionError(("literal references lack a select-any owner", unresolved))
	return {
		"parsed_object_counts": parsed_counts,
		"owner_count": len(owners),
		"reference_count": len(references),
		"owners": sorted(owners, key=lambda item: (item["path"], item["symbol"])),
		"references": sorted(references, key=lambda item: (item["path"], item["symbol"])),
		"all_payloads_identical": True,
		"all_selections_select_any_2": True,
		"unresolved_references": unresolved,
		"noduplicates_link_veto": False,
	}


def owner_packet_audit(paths):
	before = load_manifest(paths["before_manifest"])
	schema = load_manifest(paths["schema_manifest"])
	pair = load_manifest(paths["pair_manifest"])
	if tuple(before["units"]) != EXPECTED_UNITS or before["units"] != schema["units"] or before["units"] != pair["units"]:
		raise AssertionError("frozen consumer closure differs")
	for manifest in (before, schema, pair):
		if tuple(manifest["header_outputs"]["source/math/geometry.h"]) != EXPECTED_HEADER_OUTPUTS:
			raise AssertionError((manifest["phase"], "header consumer closure differs"))

	objects = {phase: {} for phase in ("before", "schema", "pair")}
	for phase, manifest in (("before", before), ("schema", schema), ("pair", pair)):
		for unit in EXPECTED_UNITS:
			objects[phase][unit] = manifest_object(manifest, unit)
	frozen_files = {}
	for phase, manifest in (("before", before), ("schema", schema), ("pair", pair)):
		frozen_files[phase] = {}
		for relative in ("source/math/geometry.c", "source/math/geometry.h"):
			path = manifest_frozen_file(manifest, relative)
			frozen_files[phase][relative] = {
				"path": str(path),
				"sha256": sha256_path(path),
			}
	if Path(frozen_files["pair"]["source/math/geometry.c"]["path"]) != paths["source"]:
		raise AssertionError("pair source argument is not the manifest-frozen source")
	if Path(frozen_files["pair"]["source/math/geometry.h"]["path"]) != paths["header"]:
		raise AssertionError("pair header argument is not the manifest-frozen header")

	timestamp_checks = {}
	function_checks = {}
	for unit in EXPECTED_UNITS:
		before_path = objects["before"][unit]
		schema_path = objects["schema"][unit]
		pair_path = objects["pair"][unit]
		before_schema_same = timestamp_normalized_sha256(before_path) == timestamp_normalized_sha256(schema_path)
		pair_same = timestamp_normalized_sha256(schema_path) == timestamp_normalized_sha256(pair_path)
		timestamp_checks[unit] = {
			"before_to_schema_identical_except_timestamp": before_schema_same,
			"schema_to_pair_identical_except_timestamp": pair_same,
		}
		before_obj = cc.load(before_path)
		schema_obj = cc.load(schema_path)
		pair_obj = cc.load(pair_path)
		function_checks[unit] = {
			"before_to_schema": function_delta(before_obj, schema_obj),
			"schema_to_pair": function_delta(schema_obj, pair_obj),
		}
		if not before_schema_same:
			raise AssertionError((unit, "schema build changed more than COFF timestamp"))
		if unit != "source/math/geometry" and not pair_same:
			raise AssertionError((unit, "pair changed a non-owner consumer"))
		if function_checks[unit]["before_to_schema"] != {"added": [], "lost": [], "changed": []}:
			raise AssertionError((unit, "schema function delta", function_checks[unit]["before_to_schema"]))

	geometry_before = cc.load(objects["before"]["source/math/geometry"])
	geometry_pair = cc.load(objects["pair"]["source/math/geometry"])
	expected_added = sorted(spec["symbol"] for spec in FUNCTIONS.values())
	geometry_delta = function_checks["source/math/geometry"]["schema_to_pair"]
	if geometry_delta != {"added": expected_added, "lost": [], "changed": []}:
		raise AssertionError(("geometry function delta", geometry_delta))
	if common_symbols(geometry_before) != common_symbols(geometry_pair):
		raise AssertionError("geometry COMMON inventory changed")

	new_owner_names = defined_owner_names(geometry_pair) - defined_owner_names(geometry_before)
	expected_new_owners = set(expected_added) | set(LITERALS)
	if new_owner_names != expected_new_owners:
		raise AssertionError(("unexpected new owner", sorted(new_owner_names), sorted(expected_new_owners)))
	literal_records = {}
	for name, expected_payload in LITERALS.items():
		symbol = cc.symbol(geometry_pair, name)
		section = geometry_pair["sections"][symbol["section"] - 1]
		payload = symbol_blob(geometry_pair, name)
		record = {
			"payload": payload.decode("ascii", errors="replace"),
			"size": section["size"],
			"selection": _section_aux_selection(geometry_pair, symbol["section"]),
			"section": section["name"],
		}
		literal_records[name] = record
		if payload != expected_payload or record["size"] != len(expected_payload) or record["selection"] != 2 or record["section"] != ".rdata":
			raise AssertionError(("literal COMDAT mismatch", name, record))

	target_obj = cc.load(paths["target"])
	target_literal_status = {}
	for name in LITERALS:
		symbols = [symbol for symbol in target_obj["symbols"] if symbol["name"] == name]
		if len(symbols) != 1:
			raise AssertionError(("target literal symbol ambiguity", name, len(symbols)))
		target_literal_status[name] = "defined" if symbols[0]["section"] > 0 else "undefined"
	if target_literal_status != {
		"??_C@_06GPGIDCJA@points?$AA@": "undefined",
		"??_C@_08BOLPBPCL@vertices?$AA@": "defined",
		"??_C@_05JGEJPNHM@edges?$AA@": "defined",
		"??_C@_08FGPNIDBO@surfaces?$AA@": "defined",
	}:
		raise AssertionError(("target literal ownership differs", target_literal_status))

	for symbol_name in ("_convex_hull3d_begin", "_convex_hull3d_expand"):
		symbols = [symbol for symbol in geometry_pair["symbols"] if symbol["name"] == symbol_name]
		if len(symbols) != 1:
			raise AssertionError(("public dependency symbol ambiguity", symbol_name, len(symbols)))
		symbol = symbols[0]
		if symbol["section"] != 0 or symbol["storage"] != 2 or symbol["type"] != 32:
			raise AssertionError(("public dependency linkage differs", symbol_name, symbol))
		target_symbol = cc.symbol(target_obj, symbol_name)
		if target_symbol["storage"] != 2 or target_symbol["type"] != 32 or target_symbol["value"] != 0:
			raise AssertionError(("January public dependency definition differs", symbol_name, target_symbol))

	config_entries = json.loads(paths["config"].read_text(encoding="utf-8"))
	config_by_name = {entry["name"]: entry for entry in config_entries}
	expected_config = {
		"_convex_hull3d_begin": 0xF6770,
		"_convex_hull3d_expand": 0xF6D40,
		"_convex_hull3d": 0xF7450,
		"_convex_hull3d_test_point": 0xF7560,
	}
	for name, file_offset in expected_config.items():
		entry = config_by_name[name]
		if entry["file_offset"] != file_offset or entry.get("static", False):
			raise AssertionError(("config linkage mismatch", name, entry))

	before_source = Path(frozen_files["before"]["source/math/geometry.c"]["path"]).read_text(encoding="utf-8")
	pair_source = paths["source"].read_text(encoding="utf-8")
	source_changes = exact_insertions(before_source, pair_source)
	if len(source_changes) != 1 or source_changes[0]["tag"] != "insert" or source_changes[0]["before"] != [507, 507]:
		raise AssertionError(("source scope differs", source_changes))
	inserted_source = source_changes[0]["text"]
	definition_names = re.findall(r"^boolean (convex_hull3d(?:_test_point)?)\($", inserted_source, re.MULTILINE)
	if definition_names != ["convex_hull3d", "convex_hull3d_test_point"]:
		raise AssertionError(("inserted source definitions differ", definition_names))
	for forbidden in (
		"convex_hull2d_perimeter",
		"convex_hull3d_test_vector",
		"plane3d_distance_to_point",
		"dot_product3d",
		"geosphere_new",
	):
		if forbidden in inserted_source:
			raise AssertionError(("held/filler source entered pair", forbidden))
	if "surface->plane.n.i*point->x" not in inserted_source:
		raise AssertionError("test-point implementation is not the authenticated explicit plane expression")

	before_header = Path(frozen_files["before"]["source/math/geometry.h"]["path"]).read_text(encoding="utf-8")
	schema_header_path = Path(frozen_files["schema"]["source/math/geometry.h"]["path"])
	schema_header = schema_header_path.read_text(encoding="utf-8")
	pair_header = paths["header"].read_text(encoding="utf-8")
	if schema_header != pair_header:
		raise AssertionError("pair header differs from frozen schema-only header")
	header_changes = exact_insertions(before_header, schema_header)
	if [(item["tag"], item["before"], item["after"]) for item in header_changes] != [
		("insert", [26, 26], [26, 51]),
		("insert", [91, 91], [116, 155]),
	]:
		raise AssertionError(("header scope differs", header_changes))
	declaration_patterns = (
		r"^struct vertex3d\r?\n\{$",
		r"^struct edge3d\r?\n\{$",
		r"^struct surface3d\r?\n\{$",
		r"^boolean convex_hull3d_begin\($",
		r"^boolean convex_hull3d_expand\($",
		r"^boolean convex_hull3d\($",
		r"^boolean convex_hull3d_test_point\($",
	)
	for pattern in declaration_patterns:
		count = len(re.findall(pattern, schema_header, re.MULTILINE))
		if count != 1:
			raise AssertionError(("missing/duplicate owning declaration", pattern, count))

	link_literal_owners = scan_link_literal_owners()

	return {
		"pass": True,
		"consumer_count": len(EXPECTED_UNITS),
		"consumer_units": list(EXPECTED_UNITS),
		"frozen_source_header_evidence": frozen_files,
		"schema_timestamp_only_count": sum(
			1 for record in timestamp_checks.values()
			if record["before_to_schema_identical_except_timestamp"]
		),
		"pair_non_owner_timestamp_only_count": sum(
			1 for unit, record in timestamp_checks.items()
			if unit != "source/math/geometry" and record["schema_to_pair_identical_except_timestamp"]
		),
		"function_checks": function_checks,
		"geometry_new_owner_names": sorted(new_owner_names),
		"common_unchanged": True,
		"literal_comdats": literal_records,
		"target_literal_status": target_literal_status,
		"complete_link_literal_inventory": link_literal_owners,
		"public_dependencies": {
			"actual_undefined": ["_convex_hull3d_begin", "_convex_hull3d_expand"],
			"january_defined": ["_convex_hull3d_begin", "_convex_hull3d_expand"],
			"status": "authentic public API dependencies; canonical implementations remain unwritten",
			"canonical_implementations_pending": True,
			"full_canonical_link_proof_claimed": False,
		},
		"config_public_offsets": expected_config,
		"source_change": {
			"kind": "one insertion",
			"before_line_index": 507,
			"inserted_line_count": source_changes[0]["after"][1] - source_changes[0]["after"][0],
			"definitions": definition_names,
		},
		"header_change": {
			"kind": "two insertions",
			"struct_layouts_from_release_pdb": {
				"vertex3d": {"size": 12, "extant": 0, "point_index": 2, "edge_index": 4, "user_data": 8},
				"edge3d": {"size": 32, "extant": 0, "vertex_indices": 4, "edge_indices": 12, "surface_indices": 20, "user_data": 28},
				"surface3d": {"size": 28, "extant": 0, "plane": 4, "edge_index": 20, "user_data": 24},
			},
		},
	}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	for label, default in DEFAULT_PATHS.items():
		parser.add_argument("--" + label.replace("_", "-"), type=Path, default=default)
		parser.add_argument(
			"--" + label.replace("_", "-") + "-sha256",
			dest=label + "_sha256",
			default=DEFAULT_HASHES[label],
		)
	args = parser.parse_args()
	paths = {
		label: resolve_path(getattr(args, label))
		for label in DEFAULT_PATHS
	}
	evidence = {
		label: verify_hash(paths[label], getattr(args, label + "_sha256"), label)
		for label in DEFAULT_PATHS
	}

	owner_packet = owner_packet_audit(paths)
	actual_obj = cc.load(paths["actual"])
	fallback_literals = literal_catalog(actual_obj)
	grouped_cases = {key: [] for key in FUNCTIONS}
	for case in all_cases():
		grouped_cases[case["function"]].append(case)

	target_result, reference = run_object(
		paths["target"], "target", grouped_cases, fallback_literals,
	)
	objects = {"target": target_result}
	for role in ("candidate", "actual"):
		objects[role], unused_reference = run_object(
			paths[role], role, grouped_cases, fallback_literals, reference,
		)

	for function_key in FUNCTIONS:
		fingerprints = {
			role: objects[role]["functions"][function_key]["relocation_fingerprint_sha256"]
			for role in objects
		}
		if len(set(fingerprints.values())) != 1:
			raise AssertionError((function_key, "relocation identities differ", fingerprints))

	negative_controls = run_negative_controls(reference)
	passed = owner_packet["pass"] and all(item["pass"] for item in objects.values()) and negative_controls["pass"]
	report = {
		"pass": passed,
		"script_sha256": sha256_path(Path(__file__)),
		"evidence": evidence,
		"owner_packet": owner_packet,
		"objects": objects,
		"case_count": sum(len(items) for items in grouped_cases.values()),
		"cases_by_function": {key: len(value) for key, value in grouped_cases.items()},
		"negative_controls": negative_controls,
		"coverage": {
			"convex_hull3d": "begin failure, expand failure, success, zero point count, and each of four pointer assertions; exact cdecl call order/arguments/stack depth",
			"convex_hull3d_test_point": "zero surfaces, nonextant skip, inside, outside, exact epsilon boundary, NaN, positive infinity, and mixed records",
			"state_and_abi": "full 64 KiB modeled geometry state, caller frame, AL/EAX target trace, ESP, return address, DF, and EBP/EBX/ESI/EDI",
			"owner_schema": "six frozen Ninja consumers, timestamp-normalized whole-object closure, inherited owner functions, COMMON, public linkage, config offsets, source/header insertion scope, and assertion literal COMDAT selection",
		},
		"limits": [
			"convex_hull3d_begin and convex_hull3d_expand are genuine public cdecl recording stubs with deterministic boolean returns; their canonical implementations remain unwritten and this is partial-credit runtime evidence, not a full canonical-link proof.",
			"Fatal assertion paths stop at the authentic display_assert call followed by modeled noreturn system_exit(-1); no post-exit register guarantee is claimed.",
			"The point predicate does not access points, vertices, or edges; those arguments are valid distinct buffers, but this oracle makes no claim about their contents beyond nonmutation.",
			"The January split leaves the points assertion literal undefined; its exact text is supplied by the separately hash-guarded actual object under the same COFF symbol name.",
			"Release-PDB layout offsets are recorded as intake evidence; runtime code independently confirms the 28-byte surface stride and extant/plane offsets used by this pair.",
		],
	}
	json.dump(report, sys.stdout, indent=2, sort_keys=True)
	sys.stdout.write("\n")
	return 0 if passed else 1


if __name__ == "__main__":
	raise SystemExit(main())
