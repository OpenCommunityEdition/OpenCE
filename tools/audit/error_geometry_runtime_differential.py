"""Bounded frozen-COFF differential for the reviewed ErrorGeometry subset.

The January split object and the selected candidate object are copied into separate
Unicorn instances.  ErrorGeometry-to-ErrorGeometry calls execute real code.
Only the genuine CRT/string/matrix APIs are stubbed, with their arguments and
state-at-call recorded. Modeled CRT calls never touch host files; the harness
reads its inputs and writes only its requested JSON report.

The matrix stub is deliberately limited to the authenticated initial identity
transform.  Valid non-NULL geometry inputs and nonnegative counts are used; the
assert/exit failure domain and CRT formatting implementation are out of scope.
For fprintf/vfprintf, the harness compares the exact format string and every promoted
vararg (double bits, signed int, or pointed-to string), which is the observable
contract at this object boundary.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import traceback

import unicorn as u
from unicorn.x86_const import *


def find_repo_root(script: Path) -> Path:
	"""Find the repository whether installed in scratch/ or tools/audit/."""
	for candidate in (script.parent, *script.parents):
		if ((candidate / "config/symbols.json").is_file()
				and (candidate / "tools/coff_compare.py").is_file()):
			return candidate
	raise RuntimeError(f"repository root not found above {script}")


ROOT = find_repo_root(Path(__file__).resolve())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from tools.campaign._coff_tools import function_sections


DEFAULT_TARGET = ROOT / "build/split/source/tool/error_geometry.obj"
DEFAULT_CANDIDATE = ROOT / "build/base/source/tool/error_geometry.obj"

FUNCTIONS = (
	"_error_geometry_file_is_open",
	"_error_geometry_initialize",
	"_error_geometry_dispose",
	"_error_geometry_set_name",
	"_error_geometry_point",
	"_error_geometry_line",
	"_error_geometry_triangle",
	"_error_geometry_polygon",
	"_error_geometry_rectangle3d",
	"_error_geometry_bounded_point",
	"_error_geometry_bounded_line",
	"_error_geometry_bounded_polygon",
	"_error_geometry_comment",
)

APIS = (
	"_fopen",
	"_fprintf",
	"_vfprintf",
	"_fflush",
	"_fclose",
	"_remove",
	"_csstrncmp",
	"_csstrncpy",
	"_csstrcat",
	"_matrix4x3_transform_point",
	"_display_assert",
	"_system_exit",
)

CODE = 0x100000
STUB = 0x200000
LITERAL = 0x300000
GLOBALS = 0x401000
FILE_OWNER = 0x402000
INPUT = 0x500000
STACK = 0x600000
SP = STACK + 0x18000
STOP = STUB + 0xF000
FILE_HANDLE = 0x0BADF00D
FUNCTION_ADDRESSES = {name: CODE + index * 0x1000 for index, name in enumerate(FUNCTIONS)}
STUB_ADDRESSES = {name: STUB + index * 0x100 for index, name in enumerate(APIS)}
SAVED = {
	UC_X86_REG_EBP: 0x11223344,
	UC_X86_REG_EBX: 0x22334455,
	UC_X86_REG_ESI: 0x33445566,
	UC_X86_REG_EDI: 0x44556677,
}

POINTS = (
	(1.25, -2.5, 3.75),
	(-4.5, 5.25, -6.125),
	(0.1, -0.2, 0.3),
	(1234.5, -0.0009765625, 7.7),
	(-8.25, 9.5, -10.75),
)
COLOR = (0.75, 0.125, 0.5, 1.0)  # alpha, red, green, blue
BOUNDS = (-3.25, 4.5, -5.75, 6.125, -7.5, 8.875)
RADIUS = 0.375


def digest(raw: bytes) -> str:
	return hashlib.sha256(raw).hexdigest()


def canonical_digest(value: object) -> str:
	return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8"))


def p32(value: int) -> bytes:
	return struct.pack("<I", value & 0xFFFFFFFF)


def f32(value: float) -> bytes:
	return struct.pack("<f", value)


def contains(base: int, length: int, address: int, size: int) -> bool:
	return base <= address and address + size <= base + length


def format_conversions(format_string: str) -> list[str]:
	"""Return modeled conversions, rejecting every unmodeled percent form."""
	result = []
	position = 0
	while True:
		position = format_string.find("%", position)
		if position < 0:
			return result
		if position + 1 < len(format_string) and format_string[position + 1] == "%":
			position += 2
			continue
		match = re.match(
			r"%(?:[-+ #0]*)(?:\d+)?(?:\.\d+)?([fds])",
			format_string[position:],
		)
		if match is None:
			raise AssertionError((
				"unsupported format conversion",
				format_string,
				format_string[position:position + 16],
			))
		result.append(match.group(1))
		position += match.end()


def owner_bytes(obj: dict, name: str) -> bytes:
	symbol = cc.symbol(obj, name)
	section = obj["sections"][symbol["section"] - 1]
	raw = bytes(cc._section_bytes(obj, section))
	return raw[symbol["value"]:]


def initial_globals(filename: str = "debug.wrl") -> bytes:
	encoded = filename.encode("ascii")
	assert len(encoded) <= 63
	raw = bytearray(0x74)
	raw[:len(encoded)] = encoded
	raw[len(encoded)] = 0
	# real_matrix4x3: scale, forward, left, up, position.
	struct.pack_into(
		"<13f",
		raw,
		0x40,
		1.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 0.0,
	)
	return bytes(raw)


def cases() -> list[dict]:
	def item(name: str, operation: str, *, initial_open: bool = False,
			fopen_success: bool = False, expected_apis: dict | None = None,
			expected_functions: dict | None = None, expected_filename: str = "debug.wrl",
			expected_open: bool | None = None, expected_return: int | None = None,
			**extra) -> dict:
		return {
			"name": name,
			"operation": operation,
			"initial_open": initial_open,
			"fopen_success": fopen_success,
			"expected_apis": expected_apis or {},
			"expected_functions": expected_functions or {operation: 1},
			"expected_filename": expected_filename,
			"expected_open": initial_open if expected_open is None else expected_open,
			"expected_return": expected_return,
			**extra,
		}

	return [
		item("helper_open_failure", FUNCTIONS[0], fopen_success=False,
			expected_apis={"_fopen": 1}, expected_return=0, expected_open=False),
		item("helper_open_success", FUNCTIONS[0], fopen_success=True,
			expected_apis={"_fopen": 1, "_fprintf": 1, "_fflush": 1},
			expected_return=1, expected_open=True),
		item("helper_already_open", FUNCTIONS[0], initial_open=True,
			expected_return=1, expected_open=True),
		item("initialize_closed", "_error_geometry_initialize",
			expected_apis={"_remove": 1}, expected_open=False),
		item("dispose_closed", "_error_geometry_dispose", expected_open=False),
		item("dispose_open", "_error_geometry_dispose", initial_open=True,
			expected_apis={"_fclose": 1}, expected_open=False),
		item("set_name_unchanged", "_error_geometry_set_name", name_value="debug.wrl",
			expected_apis={"_csstrncmp": 1}, expected_open=False),
		item("set_name_changed_closed", "_error_geometry_set_name", name_value="renamed",
			expected_apis={"_csstrncmp": 1, "_csstrncpy": 1, "_csstrcat": 1, "_remove": 1},
			expected_filename="renamed.wrl", expected_open=False),
		item("set_name_changed_open", "_error_geometry_set_name", initial_open=True,
			name_value="opened_then_renamed",
			expected_apis={"_csstrncmp": 1, "_fclose": 1, "_csstrncpy": 1,
				"_csstrcat": 1, "_remove": 1},
			expected_filename="opened_then_renamed.wrl", expected_open=False),
		item("set_name_59_byte_truncation", "_error_geometry_set_name", name_value="N" * 80,
			expected_apis={"_csstrncmp": 1, "_csstrncpy": 1, "_csstrcat": 1, "_remove": 1},
			expected_filename="N" * 59 + ".wrl", expected_open=False),
		item("point_open_failure", "_error_geometry_point", fopen_success=False,
			expected_apis={"_fopen": 1},
			expected_functions={"_error_geometry_point": 1, FUNCTIONS[0]: 1}, expected_open=False),
		item("point_nested_open_success", "_error_geometry_point", fopen_success=True,
			expected_apis={"_fopen": 1, "_fprintf": 91, "_fflush": 7,
				"_matrix4x3_transform_point": 24},
			expected_functions={"_error_geometry_point": 1, FUNCTIONS[0]: 8,
				"_error_geometry_rectangle3d": 1, "_error_geometry_polygon": 6},
			expected_open=True),
		item("line_already_open", "_error_geometry_line", initial_open=True,
			expected_apis={"_fprintf": 6, "_fflush": 1, "_matrix4x3_transform_point": 2},
			expected_functions={"_error_geometry_line": 1, FUNCTIONS[0]: 1}, expected_open=True),
		item("triangle_already_open", "_error_geometry_triangle", initial_open=True,
			expected_apis={"_fprintf": 6, "_fflush": 1, "_matrix4x3_transform_point": 3},
			expected_functions={"_error_geometry_triangle": 1, FUNCTIONS[0]: 1}, expected_open=True),
		item("polygon_below_minimum", "_error_geometry_polygon", point_count=2,
			expected_open=False),
		item("polygon_triangle_open_success", "_error_geometry_polygon", point_count=3,
			fopen_success=True,
			expected_apis={"_fopen": 1, "_fprintf": 14, "_fflush": 2,
				"_matrix4x3_transform_point": 3},
			expected_functions={"_error_geometry_polygon": 1, FUNCTIONS[0]: 1}, expected_open=True),
		item("polygon_five_already_open", "_error_geometry_polygon", point_count=5,
			initial_open=True,
			expected_apis={"_fprintf": 17, "_fflush": 1, "_matrix4x3_transform_point": 5},
			expected_functions={"_error_geometry_polygon": 1, FUNCTIONS[0]: 1}, expected_open=True),
		item("rectangle_already_open", "_error_geometry_rectangle3d", initial_open=True,
			expected_apis={"_fprintf": 90, "_fflush": 6, "_matrix4x3_transform_point": 24},
			expected_functions={"_error_geometry_rectangle3d": 1,
				"_error_geometry_polygon": 6, FUNCTIONS[0]: 7}, expected_open=True),
		item("bounded_point_already_open", "_error_geometry_bounded_point", initial_open=True,
			expected_apis={"_fprintf": 180, "_fflush": 12, "_matrix4x3_transform_point": 48},
			expected_functions={"_error_geometry_bounded_point": 1,
				"_error_geometry_rectangle3d": 2, "_error_geometry_polygon": 12,
				"_error_geometry_point": 1, FUNCTIONS[0]: 16}, expected_open=True),
		item("bounded_line_already_open", "_error_geometry_bounded_line", initial_open=True,
			expected_apis={"_fprintf": 96, "_fflush": 7, "_matrix4x3_transform_point": 26},
			expected_functions={"_error_geometry_bounded_line": 1,
				"_error_geometry_rectangle3d": 1, "_error_geometry_polygon": 6,
				"_error_geometry_line": 1, FUNCTIONS[0]: 9}, expected_open=True),
		item("bounded_polygon_below_minimum", "_error_geometry_bounded_polygon", point_count=2,
			expected_open=False),
		item("bounded_polygon_four_already_open", "_error_geometry_bounded_polygon",
			point_count=4, initial_open=True,
			expected_apis={"_fprintf": 105, "_fflush": 7, "_matrix4x3_transform_point": 28},
			expected_functions={"_error_geometry_bounded_polygon": 1,
				"_error_geometry_rectangle3d": 1, "_error_geometry_polygon": 7,
				FUNCTIONS[0]: 9}, expected_open=True),
		item("comment_open_failure", "_error_geometry_comment", fopen_success=False,
			format_value="unwritten %d", comment_int=-17, comment_real=1.25,
			comment_text="not emitted", expected_apis={"_fopen": 1},
			expected_functions={"_error_geometry_comment": 1, FUNCTIONS[0]: 1},
			expected_open=False),
		item("comment_mixed_varargs_already_open", "_error_geometry_comment",
			initial_open=True,
			format_value="mixed signed=%d real=%f text=%s %% done",
			comment_int=-17, comment_real=1.25, comment_text="payload",
			expected_apis={"_fprintf": 2, "_vfprintf": 1, "_fflush": 1},
			expected_functions={"_error_geometry_comment": 1, FUNCTIONS[0]: 1},
			expected_open=True),
	]


class Loader:
	def __init__(self, path: Path):
		self.path = path.resolve()
		self.obj = cc.load(self.path)
		self.sections = function_sections(self.obj)
		self.symbol_offsets = {
			symbol["name"]: symbol["file_offset"]
			for symbol in json.loads((ROOT / "config/symbols.json").read_text(encoding="utf-8"))
		}
		self.image = (ROOT / "cachebeta.exe").read_bytes()
		self.uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
		for base, size in (
			(CODE, 0x10000),
			(STUB, 0x10000),
			(LITERAL, 0x10000),
			(0x400000, 0x10000),
			(INPUT, 0x10000),
			(STACK, 0x20000),
		):
			self.uc.mem_map(base, size)
		self.constants: dict[str, int] = {}
		self.extents: dict[int, int] = {}
		for address in STUB_ADDRESSES.values():
			self.uc.mem_write(address, b"\xC3")
		self.uc.mem_write(STOP, b"\xCC")
		for name in FUNCTIONS:
			self._materialize_function(name)
		self.uc.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
		self.uc.mem_protect(STUB, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
		self.uc.mem_protect(LITERAL, 0x10000, u.UC_PROT_READ)
		self.uc.mem_protect(INPUT, 0x10000, u.UC_PROT_READ)

	def _constant(self, name: str) -> int:
		if name not in self.constants:
			try:
				symbol = cc.symbol(self.obj, name)
				section = self.obj["sections"][symbol["section"] - 1]
				payload = bytes(cc._section_bytes(self.obj, section))[symbol["value"]:]
			except cc.CoffError:
				# csplit keeps some selected January literals undefined.  Read them
				# from their authenticated cachebeta offset, never from candidate C.
				offset = self.symbol_offsets[name]
				payload = self.image[offset:offset + 0x400]
			assert len(payload) <= 0x400, (name, len(payload))
			address = LITERAL + len(self.constants) * 0x400
			self.uc.mem_write(address, payload)
			self.constants[name] = address
		return self.constants[name]

	def _materialize_function(self, name: str) -> None:
		fn = self.sections[name]
		address = FUNCTION_ADDRESSES[name]
		raw = bytearray(cc._section_bytes(
			self.obj,
			self.obj["sections"][fn.section_number - 1],
		))
		assert len(raw) <= 0x1000
		self.extents[address] = len(raw)
		for relocation in fn.info["relocations"]:
			target = relocation.get("symbolic_target", relocation["target"])
			if target[0] == "internal":
				destination = address + target[1]
			elif target[0] in ("symbol", "defined-noncode"):
				symbol, addend = target[1:] if target[0] == "symbol" else target[2:]
				if symbol in FUNCTION_ADDRESSES:
					destination = FUNCTION_ADDRESSES[symbol]
				elif symbol in STUB_ADDRESSES:
					destination = STUB_ADDRESSES[symbol]
				elif symbol == "_error_geometry_globals":
					destination = GLOBALS
				elif symbol == "_error_geometry_file":
					destination = FILE_OWNER
				else:
					destination = self._constant(symbol)
				destination += addend
			else:
				raise AssertionError((name, "unsupported relocation", target))
			if relocation["type"] == cc.IMAGE_REL_I386_REL32:
				destination -= address + relocation["address"] + 4
			else:
				assert relocation["type"] == cc.IMAGE_REL_I386_DIR32, relocation
			raw[relocation["address"]:relocation["address"] + 4] = p32(destination)
		self.uc.mem_write(address, bytes(raw))

	def _cstring(self, address: int, limit: int = 8192) -> bytes:
		regions = (
			(LITERAL, 0x10000),
			(GLOBALS, 0x74),
			(INPUT, 0x10000),
			(STACK, 0x20000),
		)
		for base, size in regions:
			if contains(base, size, address, 1):
				length = min(limit, base + size - address)
				raw = bytes(self.uc.mem_read(address, length))
				assert b"\0" in raw, ("unterminated string", hex(address), length)
				return raw.split(b"\0", 1)[0]
		raise AssertionError(("string outside modeled regions", hex(address)))

	def _prepare_input(self, case: dict) -> tuple[bytes, dict[str, int]]:
		raw = bytearray((index * 29 + 7) & 0xFF for index in range(0x10000))
		locations = {
			"points": INPUT + 0x100,
			"bounds": INPUT + 0x600,
			"color": INPUT + 0x700,
			"name": INPUT + 0x1000,
			"format": INPUT + 0x1800,
			"comment_text": INPUT + 0x1900,
		}
		points = b"".join(struct.pack("<3f", *point) for point in POINTS)
		raw[0x100:0x100 + len(points)] = points
		raw[0x600:0x618] = struct.pack("<6f", *BOUNDS)
		raw[0x700:0x710] = struct.pack("<4f", *COLOR)
		name = case.get("name_value", "unused").encode("ascii") + b"\0"
		raw[0x1000:0x1000 + len(name)] = name
		format_value = case.get("format_value", "unused").encode("ascii") + b"\0"
		comment_text = case.get("comment_text", "unused").encode("ascii") + b"\0"
		raw[0x1800:0x1800 + len(format_value)] = format_value
		raw[0x1900:0x1900 + len(comment_text)] = comment_text
		return bytes(raw), locations

	def _arguments(self, case: dict, at: dict[str, int]) -> bytes:
		op = case["operation"]
		p0, p1, p2 = at["points"], at["points"] + 12, at["points"] + 24
		color = at["color"]
		if op in (FUNCTIONS[0], "_error_geometry_initialize", "_error_geometry_dispose"):
			return b""
		if op == "_error_geometry_set_name":
			return p32(at["name"])
		if op == "_error_geometry_point":
			return p32(p0) + p32(color)
		if op == "_error_geometry_line":
			return p32(p0) + p32(p1) + p32(color)
		if op == "_error_geometry_triangle":
			return p32(p0) + p32(p1) + p32(p2) + p32(color)
		if op == "_error_geometry_polygon":
			return p32(case["point_count"]) + p32(p0) + p32(color)
		if op == "_error_geometry_rectangle3d":
			return p32(at["bounds"]) + p32(color)
		if op == "_error_geometry_bounded_point":
			return p32(p0) + f32(RADIUS) + p32(color)
		if op == "_error_geometry_bounded_line":
			return p32(p0) + p32(p1) + f32(RADIUS) + p32(color)
		if op == "_error_geometry_bounded_polygon":
			return p32(case["point_count"]) + p32(p0) + f32(RADIUS) + p32(color)
		if op == "_error_geometry_comment":
			return (
				p32(at["format"])
				+ p32(case["comment_int"])
				+ struct.pack("<d", case["comment_real"])
				+ p32(at["comment_text"])
			)
		raise AssertionError(op)

	def run(self, case: dict) -> dict:
		uc = self.uc
		globals_before = initial_globals()
		uc.mem_write(GLOBALS, globals_before)
		uc.mem_write(FILE_OWNER, p32(FILE_HANDLE if case["initial_open"] else 0))
		input_before, locations = self._prepare_input(case)
		uc.mem_write(INPUT, input_before)
		stack_before = bytes((index * 17 + 3) & 0xFF for index in range(0x20000))
		arguments = self._arguments(case, locations)
		stack_image = bytearray(stack_before)
		stack_offset = SP - STACK
		stack_image[stack_offset:stack_offset + 4 + len(arguments)] = p32(STOP) + arguments
		uc.mem_write(STACK, bytes(stack_image))

		state = {"events": [], "functions": [], "api_snapshots": []}

		def args(count: int) -> tuple[int, ...]:
			sp = uc.reg_read(UC_X86_REG_ESP)
			return struct.unpack("<" + "I" * count, uc.mem_read(sp + 4, count * 4))

		def signed32(value: int) -> int:
			return struct.unpack("<i", p32(value))[0]

		def cstrncmp(left: bytes, right: bytes, count: int) -> int:
			for index in range(count):
				a = left[index] if index < len(left) else 0
				b = right[index] if index < len(right) else 0
				if a != b:
					return -1 if a < b else 1
				if a == 0:
					return 0
			return 0

		def parse_varargs(cursor: int, format_string: str) -> list[dict]:
			parsed = []
			for kind in format_conversions(format_string):
				if kind == "f":
					raw = bytes(uc.mem_read(cursor, 8))
					parsed.append({"kind": "double", "little_endian_hex": raw.hex()})
					cursor += 8
				elif kind == "d":
					value = struct.unpack("<i", uc.mem_read(cursor, 4))[0]
					parsed.append({"kind": "int", "value": value})
					cursor += 4
				else:
					pointer = struct.unpack("<I", uc.mem_read(cursor, 4))[0]
					parsed.append({"kind": "string", "value": self._cstring(pointer).decode("ascii")})
					cursor += 4
			return parsed

		def record(name: str, payload: list) -> None:
			state["events"].append([name, *payload])

		def hook(emulator, address, size, unused):
			if address in FUNCTION_ADDRESSES.values():
				state["functions"].append(next(name for name, value in FUNCTION_ADDRESSES.items()
					if value == address))
				return
			name = next((name for name, value in STUB_ADDRESSES.items() if value == address), None)
			if name is None:
				assert address == STOP or any(contains(base, length, address, size)
					for base, length in self.extents.items()), ("unexpected execution", hex(address), size)
				return
			# Always snapshot at API entry, before a modeled API mutates any owner.
			# This is significant for csstrncpy/csstrcat in set_name.
			state["api_snapshots"].append({
				"api": name,
				"phase": "entry_before_stub_effects",
				"file": struct.unpack("<I", uc.mem_read(FILE_OWNER, 4))[0],
				"globals_sha256": digest(bytes(uc.mem_read(GLOBALS, 0x74))),
			})
			result = 0
			if name == "_fopen":
				filename, mode = args(2)
				mode_string = self._cstring(mode).decode("ascii")
				assert mode_string == "w"
				result = FILE_HANDLE if case["fopen_success"] else 0
				record(name, [self._cstring(filename).decode("ascii"), mode_string, result])
			elif name == "_fprintf":
				file_pointer, format_pointer = args(2)
				assert file_pointer == FILE_HANDLE
				format_string = self._cstring(format_pointer).decode("ascii")
				record(name, [format_string, parse_varargs(
					uc.reg_read(UC_X86_REG_ESP) + 12, format_string)])
				result = 1
			elif name == "_vfprintf":
				file_pointer, format_pointer, argument_list = args(3)
				assert file_pointer == FILE_HANDLE
				assert contains(STACK, 0x20000, argument_list, 1)
				format_string = self._cstring(format_pointer).decode("ascii")
				record(name, [format_string, parse_varargs(argument_list, format_string)])
				result = 1
			elif name == "_fflush":
				(file_pointer,) = args(1)
				assert file_pointer == FILE_HANDLE
				record(name, [file_pointer])
			elif name == "_fclose":
				(file_pointer,) = args(1)
				assert file_pointer == FILE_HANDLE
				record(name, [file_pointer])
			elif name == "_remove":
				(filename,) = args(1)
				record(name, [self._cstring(filename).decode("ascii")])
			elif name == "_csstrncmp":
				left, right, count = args(3)
				left_text, right_text = self._cstring(left), self._cstring(right)
				result = cstrncmp(left_text, right_text, count)
				record(name, [left_text.decode("ascii"), right_text.decode("ascii"), count, result])
			elif name == "_csstrncpy":
				destination, source, count = args(3)
				assert contains(GLOBALS, 64, destination, count)
				text = self._cstring(source)
				copy = text[:count]
				if len(copy) < count:
					copy += b"\0" * (count - len(copy))
				uc.mem_write(destination, copy)
				result = destination
				record(name, [text.decode("ascii"), count])
			elif name == "_csstrcat":
				destination, source = args(2)
				assert contains(GLOBALS, 64, destination, 1)
				before, suffix = self._cstring(destination), self._cstring(source)
				combined = before + suffix + b"\0"
				assert contains(GLOBALS, 64, destination, len(combined))
				uc.mem_write(destination, combined)
				result = destination
				record(name, [before.decode("ascii"), suffix.decode("ascii")])
			elif name == "_matrix4x3_transform_point":
				matrix, point, output = args(3)
				assert matrix == GLOBALS + 0x40
				assert bytes(uc.mem_read(matrix, 0x34)) == initial_globals()[0x40:]
				assert contains(STACK, 0x20000, output, 12)
				raw_point = bytes(uc.mem_read(point, 12))
				uc.mem_write(output, raw_point)
				result = output
				record(name, [raw_point.hex()])
			elif name in ("_display_assert", "_system_exit"):
				raise AssertionError(("unexpected assertion/exit", name))
			else:
				raise AssertionError(name)
			uc.reg_write(UC_X86_REG_EAX, result & 0xFFFFFFFF)
			uc.reg_write(UC_X86_REG_ECX, 0xA1B2C3D4)
			uc.reg_write(UC_X86_REG_EDX, 0xB2C3D4E5)

		def memory(emulator, access, address, size, value, unused):
			if access == u.UC_MEM_WRITE:
				assert (
					contains(GLOBALS, 0x74, address, size)
					or contains(FILE_OWNER, 4, address, size)
					or contains(STACK, 0x20000, address, size)
				), ("write outside owned state/stack", hex(address), size)
			else:
				assert (
					contains(CODE, 0x10000, address, size)
					or contains(LITERAL, 0x10000, address, size)
					or contains(GLOBALS, 0x74, address, size)
					or contains(FILE_OWNER, 4, address, size)
					or contains(INPUT, 0x10000, address, size)
					or contains(STACK, 0x20000, address, size)
				), ("read outside modeled state", hex(address), size)

		code_hook = uc.hook_add(u.UC_HOOK_CODE, hook)
		memory_hook = uc.hook_add(u.UC_HOOK_MEM_READ | u.UC_HOOK_MEM_WRITE, memory)
		try:
			uc.reg_write(UC_X86_REG_ESP, SP)
			uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
			uc.reg_write(UC_X86_REG_FPCW, 0x037F)
			uc.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
			uc.reg_write(UC_X86_REG_FPSW, 0)
			initial_fpu_tag = uc.reg_read(UC_X86_REG_FPTAG)
			for register, value in SAVED.items():
				uc.reg_write(register, value)
			uc.reg_write(UC_X86_REG_ECX, 0x55667788)
			uc.reg_write(UC_X86_REG_EDX, 0x66778899)
			uc.emu_start(FUNCTION_ADDRESSES[case["operation"]], STOP, count=2_000_000)
			assert uc.reg_read(UC_X86_REG_EIP) == STOP
			assert uc.reg_read(UC_X86_REG_ESP) == SP + 4
			assert all(uc.reg_read(register) == value for register, value in SAVED.items())
			assert not uc.reg_read(UC_X86_REG_EFLAGS) & 0x400
			assert uc.reg_read(UC_X86_REG_FPTAG) == initial_fpu_tag
			assert not uc.reg_read(UC_X86_REG_FPSW) & 0x3800
		finally:
			uc.hook_del(code_hook)
			uc.hook_del(memory_hook)

		input_after = bytes(uc.mem_read(INPUT, 0x10000))
		assert input_after == input_before
		stack_after = bytes(uc.mem_read(STACK, 0x20000))
		assert stack_after[stack_offset:stack_offset + 4] == p32(STOP)
		# MSVC is permitted to reuse incoming argument slots (polygon does so for
		# its short loop index).  Capture those bytes for the differential rather
		# than imposing a non-ABI preservation rule.
		argument_slots_after = stack_after[
			stack_offset + 4:stack_offset + 4 + len(arguments)]
		globals_after = bytes(uc.mem_read(GLOBALS, 0x74))
		file_after = struct.unpack("<I", uc.mem_read(FILE_OWNER, 4))[0]
		filename_after = globals_after[:64].split(b"\0", 1)[0].decode("ascii")
		return {
			"events": state["events"],
			"functions": state["functions"],
			"api_snapshots": state["api_snapshots"],
			"globals_hex": globals_after.hex(),
			"filename": filename_after,
			"file_pointer": file_after,
			"helper_return": (uc.reg_read(UC_X86_REG_EAX) & 0xFF)
				if case["operation"] == FUNCTIONS[0] else None,
			"argument_slots_after_hex": argument_slots_after.hex(),
			"abi_and_memory_guards_pass": True,
		}


def validate_target(case: dict, result: dict) -> None:
	api_counts = dict(Counter(event[0] for event in result["events"]))
	function_counts = dict(Counter(result["functions"]))
	assert api_counts == case["expected_apis"], (case["name"], "API counts", api_counts)
	assert function_counts == case["expected_functions"], (
		case["name"], "function counts", function_counts)
	assert result["filename"] == case["expected_filename"], (
		case["name"], "filename", result["filename"])
	expected_file = FILE_HANDLE if case["expected_open"] else 0
	assert result["file_pointer"] == expected_file, (
		case["name"], "file pointer", hex(result["file_pointer"]))
	if case["expected_return"] is not None:
		assert result["helper_return"] == case["expected_return"], (
			case["name"], "helper return", result["helper_return"])
	if case["name"] == "helper_open_success":
		assert result["events"] == [
			["_fopen", "debug.wrl", "w", FILE_HANDLE],
			["_fprintf", "#VRML V1.0 ascii\n\n", []],
			["_fflush", FILE_HANDLE],
		]
	if case["name"] == "set_name_59_byte_truncation":
		assert len(result["filename"]) == 63
	if case["name"] in ("polygon_below_minimum", "bounded_polygon_below_minimum"):
		assert not result["events"]
	if case["name"] == "comment_mixed_varargs_already_open":
		assert result["events"] == [
			["_fprintf", "#", []],
			["_vfprintf", "mixed signed=%d real=%f text=%s %% done", [
				{"kind": "int", "value": -17},
				{"kind": "double", "little_endian_hex": struct.pack("<d", 1.25).hex()},
				{"kind": "string", "value": "payload"},
			]],
			["_fprintf", "\n", []],
			["_fflush", FILE_HANDLE],
		]


def comparable(result: dict) -> dict:
	return result


def summary(result: dict) -> dict:
	api_counts = dict(sorted(Counter(event[0] for event in result["events"]).items()))
	function_counts = dict(sorted(Counter(result["functions"]).items()))
	return {
		"api_counts": api_counts,
		"function_counts": function_counts,
		"event_trace_sha256": canonical_digest(result["events"]),
		"function_path_sha256": canonical_digest(result["functions"]),
		"api_state_trace_sha256": canonical_digest(result["api_snapshots"]),
		"string_mutation_api_entry_snapshots": [
			snapshot for snapshot in result["api_snapshots"]
			if snapshot["api"] in ("_csstrncpy", "_csstrcat")
		],
		"first_events": result["events"][:3],
		"last_events": result["events"][-3:],
		"globals_sha256": digest(bytes.fromhex(result["globals_hex"])),
		"filename": result["filename"],
		"file_pointer": result["file_pointer"],
		"helper_return": result["helper_return"],
		"argument_slots_after_hex": result["argument_slots_after_hex"],
		"abi_and_memory_guards_pass": result["abi_and_memory_guards_pass"],
	}


def object_metadata(path: Path, loader: Loader) -> dict:
	return {
		"path": str(path),
		"sha256": digest(path.read_bytes()),
		"globals_sha256": digest(owner_bytes(loader.obj, "_error_geometry_globals")[:0x74]),
		"file_owner_sha256": digest(owner_bytes(loader.obj, "_error_geometry_file")[:4]),
		"functions": {
			name: {
				"size": loader.sections[name].info["size"],
				"relocation_count": loader.sections[name].info["relocation_count"],
				"normalized_sha256": loader.sections[name].info["normalized_sha256"],
			}
			for name in FUNCTIONS
		},
	}


def main() -> None:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--target", type=Path, default=DEFAULT_TARGET)
	parser.add_argument("--candidate", type=Path, default=DEFAULT_CANDIDATE)
	parser.add_argument("--output", type=Path, required=True)
	parser.add_argument("--expected-target-sha256")
	parser.add_argument("--expected-candidate-sha256")
	args = parser.parse_args()
	if not __debug__:
		raise RuntimeError("verification requires assertions; do not use python -O")
	protected_inputs = {
		args.target.resolve(), args.candidate.resolve(), Path(__file__).resolve(),
		ROOT / "cachebeta.exe", ROOT / "config/symbols.json",
	}
	if args.output.resolve() in protected_inputs:
		raise ValueError("output must not overwrite a verification input")
	target_path, candidate_path = args.target.resolve(), args.candidate.resolve()
	target_sha256, candidate_sha256 = digest(target_path.read_bytes()), digest(candidate_path.read_bytes())
	if args.expected_target_sha256:
		assert target_sha256 == args.expected_target_sha256.lower(), (
			"target SHA-256 mismatch", target_sha256, args.expected_target_sha256.lower())
	if args.expected_candidate_sha256:
		assert candidate_sha256 == args.expected_candidate_sha256.lower(), (
			"candidate SHA-256 mismatch", candidate_sha256, args.expected_candidate_sha256.lower())
	target, candidate = Loader(target_path), Loader(candidate_path)
	authenticated_globals = initial_globals()
	assert owner_bytes(target.obj, "_error_geometry_globals")[:0x74] == authenticated_globals
	assert owner_bytes(candidate.obj, "_error_geometry_globals")[:0x74] == authenticated_globals
	assert owner_bytes(target.obj, "_error_geometry_file")[:4] == bytes(4)
	assert owner_bytes(candidate.obj, "_error_geometry_file")[:4] == bytes(4)
	# Keep the parser fail-closed property executable and visible in the report.
	assert format_conversions("mixed %d %f %s %%") == ["d", "f", "s"]
	try:
		format_conversions("unsupported %x")
	except AssertionError:
		format_parser_fail_closed = True
	else:
		raise AssertionError("unsupported format conversion was silently accepted")

	results, failures = [], []
	for case in cases():
		try:
			target_result = target.run(case)
			validate_target(case, target_result)
			candidate_result = candidate.run(case)
			differential_pass = comparable(target_result) == comparable(candidate_result)
			assert differential_pass, (case["name"], "candidate differs from January")
			results.append({
				"name": case["name"],
				"operation": case["operation"],
				"target_model_pass": True,
				"differential_pass": True,
				"target": summary(target_result),
				"candidate": summary(candidate_result),
			})
		except Exception as error:
			failure = {
				"name": case["name"],
				"error": repr(error),
				"traceback": traceback.format_exc(),
			}
			failures.append(failure)
			results.append(failure)

	report = {
		"scope": __doc__,
		"harness_path": str(Path(__file__).resolve()),
		"harness_sha256": digest(Path(__file__).read_bytes()),
		"unicorn_version": u.__version__,
		"target": object_metadata(target_path, target),
		"candidate": object_metadata(candidate_path, candidate),
		"expected_sha256": {
			"target": args.expected_target_sha256.lower() if args.expected_target_sha256 else None,
			"candidate": args.expected_candidate_sha256.lower() if args.expected_candidate_sha256 else None,
		},
		"format_parser_fail_closed": format_parser_fail_closed,
		"api_state_snapshot_phase": "entry_before_stub_effects",
		"case_count": len(results),
		"top_level_invocation_count_per_object": len(results),
		"failure_count": len(failures),
		"failures": failures,
		"pass_result": not failures,
		"cases": results,
		"limits": [
			"Object code copies only; no compilation or host CRT effects; only the requested JSON report is written.",
			"Identity real_matrix4x3 only; matrix API stub copies the point after validating the owner and bytes.",
			"Valid non-NULL pointers and nonnegative counts only; assertion/exit behavior is excluded.",
			"fprintf/vfprintf output bytes and locale are not modeled; exact formats and every supported promoted vararg are compared.",
			"Only %d, %f, %s, and literal %% are modeled; every other percent conversion fails closed.",
			"Counts sampled are 2, 3, 4, and 5; no hostile huge-count/out-of-bounds input.",
		],
	}
	args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
	print(json.dumps({
		"case_count": report["case_count"],
		"failure_count": report["failure_count"],
		"pass_result": report["pass_result"],
		"output": str(args.output),
	}))
	if failures:
		raise SystemExit(1)


if __name__ == "__main__":
	main()
