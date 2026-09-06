"""Target-first runtime differential for the eight stronger Bitmaps functions.

This frozen-evidence oracle executes real i386 COFF function bytes under Unicorn.
It compares the frozen first natural candidate and final canonical object to the
frozen current-name January target.  It also checks independent semantic
expectations for constructors, address arithmetic, mipmap dispatch, row pitch,
allocator failures, and representative fatal assertions.

Only public APIs are stubbed.  The stubs record cdecl call order, arguments and
stack depth, and implement the narrow documented behavior required by a case.
No private Bitmaps ABI or whole-game state is invented.
This bounded runtime check does not replace the separate strict COFF,
source-credibility, ownership, or whole-object admission gates.

The default inputs are immutable local reconciliation snapshots under scratch/.
For another checkout, supply the five input paths and their recorded hashes.
The inputs are SHA-256 guarded.  Alternate inputs require
matching explicit ``--*-sha256`` values.  The script writes JSON to stdout and
does not modify source, objects, build products, or reports.
"""

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


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402


TARGET_DEFAULT = (
	ROOT
	/ "scratch/bitmaps-mipmap-owner-final-20260905/files/build/split/source/bitmaps/bitmaps.obj"
)
CANDIDATE_DEFAULT = ROOT / "scratch/bitmaps-stronger-natural-first-20260905.obj"
ACTUAL_DEFAULT = (
	ROOT / "scratch/bitmaps-mipmap-owner-final-20260905/source/bitmaps/bitmaps.obj"
)
SOURCE_DEFAULT = ROOT / "scratch/bitmaps_stronger_natural_candidate_20260905.c"
ACTUAL_SOURCE_DEFAULT = (
	ROOT / "scratch/bitmaps-mipmap-owner-final-20260905/files/source/bitmaps/bitmaps.c"
)

DEFAULT_HASHES = {
	"target": "95c1f3f022bef7a5828ccb64ea526e809ef76cc6f9c2844573279297909eea0a",
	"candidate": "25a902f1bc176bf445454895ecf2b7b17417bfa84722ae7fb0a4cf23ae30918e",
	"actual": "dcbf76839ef0a07669a29d5269efd2dcaf9db728bf290910bde460a859588313",
	"source": "033e7c7b0ff764c0d9d5b580a3b188c2e4f45a44b0ce70ab5b5106e09b91a147",
	"actual_source": "c837251b61b14b6e6620e72cfa6388be039be7508e292beb27d934ce41a3ba9b",
}

FUNCTIONS = {
	"bitmap_2d_new": {
		"symbol": "_bitmap_2d_new",
		"size": 384,
		"relocation_count": 23,
		"normalized_sha256": "68479ae7014792a3394f7ecf09e4bf4d07fda2ef53575f98765c3e74f64723cd",
	},
	"bitmap_3d_new": {
		"symbol": "_bitmap_3d_new",
		"size": 448,
		"relocation_count": 27,
		"normalized_sha256": "c0e0185511ef75dcb7ffd898be4d1b85dc28df147625aa74ea9117f4c77767b7",
	},
	"bitmap_cube_map_new": {
		"symbol": "_bitmap_cube_map_new",
		"size": 368,
		"relocation_count": 23,
		"normalized_sha256": "c5813e5fab4c269cf106bc74a63892fdefa8a96715a08980e669ca0c78edcecd",
	},
	"bitmap_2d_address": {
		"symbol": "_bitmap_2d_address",
		"size": 544,
		"relocation_count": 33,
		"normalized_sha256": "e05dbed80aad3c4fe4f6f38ea9cd93fee9962dac42e6bb59e57b630a6296853e",
	},
	"bitmap_3d_address": {
		"symbol": "_bitmap_3d_address",
		"size": 656,
		"relocation_count": 37,
		"normalized_sha256": "f837a023753386d82cd1b0d4ace9d01fe337e4155945e08e31c4ada3a4452f86",
	},
	"bitmap_cube_map_address": {
		"symbol": "_bitmap_cube_map_address",
		"size": 528,
		"relocation_count": 33,
		"normalized_sha256": "43ae8a218be87e73ca8b99320a75588a3fa17e7431b1dcd5510174180d02ff63",
	},
	"bitmap_mipmap_address": {
		"symbol": "_bitmap_mipmap_address",
		"size": 208,
		"relocation_count": 15,
		"normalized_sha256": "1f23ac477450722c3652ca7881f25452a7a8819f660f516d106b1c6385401d16",
	},
	"bitmap_mipmap_get_row_pitch": {
		"symbol": "_bitmap_mipmap_get_row_pitch",
		"size": 224,
		"relocation_count": 19,
		"normalized_sha256": "2bd2fa900e7f0b7a6680b4aa66345616606e3d22df260c48fb9619ec422c7541",
	},
}

CODE = 0x100000
STOP = 0x10F000
STUBS = 0x200000
STRINGS = 0x300000
BITMAP = 0x400000
ALLOC_BITMAP = 0x410000
PIXELS = 0x500000
STACK_PAGE = 0x700000
ENTRY_SP = STACK_PAGE + 0xC000
BITMAP_SIZE = 0x30

STUB_NAMES = (
	"_display_assert",
	"_system_exit",
	"_debug_malloc",
	"_csmemset",
	"_bitmap_get_pixel_data_size",
	"_bitmap_verify",
	"_error",
	"_bitmap_format_get_bits_per_pixel",
	"_bitmap_mipmap_get_width",
	"_bitmap_2d_address",
	"_bitmap_3d_address",
	"_bitmap_cube_map_address",
)
STUB_ADDRESSES = {
	name: STUBS + 0x100 * (index + 1)
	for index, name in enumerate(STUB_NAMES)
}
DISPATCH_RETURNS = {
	"_bitmap_2d_address": PIXELS + 0x120,
	"_bitmap_3d_address": PIXELS + 0x230,
	"_bitmap_cube_map_address": PIXELS + 0x340,
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

BITS_PER_PIXEL = {
	0: 8,
	1: 8,
	2: 8,
	3: 16,
	6: 16,
	8: 16,
	9: 16,
	10: 32,
	11: 32,
	14: 4,
	15: 8,
	16: 8,
	17: 8,
}

TYPE_2D = 0
TYPE_3D = 1
TYPE_CUBE = 2
FLAG_POWER_OF_TWO = 1 << 0
FLAG_COMPRESSED = 1 << 1
FLAG_PALETTIZED = 1 << 2
FLAG_SWIZZLED = 1 << 3
FLAG_ALLOCATED = 1 << 6
BITMAP_GROUP_TAG = 0x6269746D
SOURCE_FILE = "c:\\halo\\SOURCE\\bitmaps\\bitmaps.c"


def p16(value):
	return struct.pack("<H", value & 0xFFFF)


def p32(value):
	return struct.pack("<I", value & 0xFFFFFFFF)


def read16(machine, address):
	return struct.unpack("<H", machine.mem_read(address, 2))[0]


def read32(machine, address):
	return struct.unpack("<I", machine.mem_read(address, 4))[0]


def signed32(value):
	return struct.unpack("<i", p32(value))[0]


def sha256_path(path):
	return hashlib.sha256(path.read_bytes()).hexdigest()


def seeded_bytes(size, salt):
	return bytes(((index * 37 + salt * 53 + 11) & 0xFF) for index in range(size))


def differing_ranges(actual, expected):
	indices = [
		index
		for index, pair in enumerate(zip(actual, expected))
		if pair[0] != pair[1]
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


def symbol_cstring(obj, name):
	symbol = cc.symbol(obj, name)
	section = obj["sections"][symbol["section"] - 1]
	raw = cc._section_bytes(obj, section)
	start = symbol["value"]
	end = raw.find(b"\0", start)
	if end < 0:
		raise AssertionError(("unterminated literal", name))
	return raw[start:end].decode("ascii")


def prepare_function(obj, function_name, literal_catalog):
	spec = FUNCTIONS[function_name]
	available = function_sections(obj)
	if spec["symbol"] not in available:
		raise AssertionError((function_name, "missing symbol", spec["symbol"]))
	function = available[spec["symbol"]]
	section = obj["sections"][function.section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))

	literals = {}
	for relocation in function.info["relocations"]:
		target = relocation.get("symbolic_target", relocation["target"])
		if not isinstance(target, (list, tuple)) or target[0] != "symbol":
			raise AssertionError((function_name, "unresolved relocation", target))
		name, unused_addend = target[1:]
		if relocation["type"] == cc.IMAGE_REL_I386_DIR32:
			try:
				literals[name] = symbol_cstring(obj, name)
			except cc.CoffError:
				if name not in literal_catalog:
					raise AssertionError((function_name, "unresolved linked literal", name))
				literals[name] = literal_catalog[name]

	literal_addresses = {
		text: STRINGS + 0x200 * (index + 1)
		for index, text in enumerate(sorted(set(literals.values())))
	}
	string_memory = {
		literal_addresses[text]: text
		for text in literal_addresses
	}
	fingerprint = []
	for relocation in function.info["relocations"]:
		target = relocation.get("symbolic_target", relocation["target"])
		name, addend = target[1:]
		if name in STUB_ADDRESSES:
			identity = "api:" + name
			address = STUB_ADDRESSES[name] + addend
		elif name in literals:
			text = literals[name]
			identity = "literal:" + text
			address = literal_addresses[text] + addend
		else:
			raise AssertionError((function_name, "unsupported owner", name))
		fingerprint.append((relocation["address"], relocation["type"], identity))
		if relocation["type"] == cc.IMAGE_REL_I386_REL32:
			address -= CODE + relocation["address"] + 4
		elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
			raise AssertionError((function_name, "unsupported relocation type", relocation["type"]))
		raw[relocation["address"]:relocation["address"] + 4] = p32(address)
	return {
		"name": function_name,
		"symbol": function.name,
		"info": function.info,
		"raw": bytes(raw),
		"strings": string_memory,
		"relocation_fingerprint": fingerprint,
	}


def verify_section(prepared):
	spec = FUNCTIONS[prepared["name"]]
	info = prepared["info"]
	checks = {
		"symbol": prepared["symbol"] == spec["symbol"],
		"size": info["size"] == spec["size"],
		"relocation_count": info["relocation_count"] == spec["relocation_count"],
		"normalized_sha256": info["normalized_sha256"] == spec["normalized_sha256"],
	}
	failed = [key for key, value in checks.items() if not value]
	if failed:
		raise AssertionError((prepared["name"], "section guard", failed))
	return checks


def put16(data, offset, value):
	data[offset:offset + 2] = p16(value)


def put32(data, offset, value):
	data[offset:offset + 4] = p32(value)


def make_bitmap(case):
	data = bytearray(seeded_bytes(BITMAP_SIZE, case["seed"] + 17))
	bitmap = case.get("bitmap", {})
	put16(data, 4, bitmap.get("width", 8))
	put16(data, 6, bitmap.get("height", 8))
	put16(data, 8, bitmap.get("depth", 1))
	put16(data, 10, bitmap.get("type", TYPE_2D))
	put16(data, 12, bitmap.get("format", 11))
	put16(data, 14, bitmap.get("flags", 0))
	put16(data, 20, bitmap.get("mipmaps", 3))
	put32(data, 44, bitmap.get("base", PIXELS))
	return bytes(data)


def constructor_cases(function_name, bitmap_type):
	if bitmap_type == TYPE_2D:
		ordinary_args = [8, 4, 2, 11]
		compressed_args = [8, 8, 1, 14]
	elif bitmap_type == TYPE_3D:
		ordinary_args = [8, 4, 2, 2, 6]
		compressed_args = [8, 8, 4, 1, 15]
	else:
		ordinary_args = [8, 2, 11]
		compressed_args = [8, 1, 16]
	base = [
		{"name": "uncompressed_success", "args": ordinary_args, "malloc": "success"},
		{"name": "compressed_success", "args": compressed_args, "malloc": "success"},
		{"name": "object_allocation_failure", "args": ordinary_args, "malloc": "object_fail"},
		{"name": "pixel_allocation_failure", "args": ordinary_args, "malloc": "pixel_fail"},
	]
	if bitmap_type == TYPE_2D:
		base.extend([
			{"name": "post_allocation_verify_failure", "args": ordinary_args, "malloc": "success", "verify": 0},
			{
				"name": "invalid_width_assert",
				"args": [0, 4, 2, 11],
				"malloc": "success",
				"assert": (0xB5, "bitmap_format_type_valid_width (format, _bitmap_type_2d, width)"),
			},
		])
	for index, case in enumerate(base):
		case.update({
			"function": function_name,
			"seed": 100 * bitmap_type + index + 1,
			"pixel_size": 0x2340 + 0x20 * bitmap_type + index,
			"constructor_type": bitmap_type,
		})
	return base


def address_case(function_name, name, bitmap, args, expected=None, assertion=None):
	return {
		"function": function_name,
		"name": name,
		"seed": sum(ord(character) for character in function_name + name) & 0xFF,
		"bitmap": bitmap,
		"args": [BITMAP] + args,
		"expected_eax": expected,
		"assert": assertion,
	}


def address_cases():
	cases = []
	cases.extend([
		address_case(
			"bitmap_2d_address", "uncompressed_base",
			{"width": 8, "height": 4, "depth": 1, "type": TYPE_2D, "format": 11, "flags": 0, "mipmaps": 3},
			[3, 2, 0], PIXELS + 76,
		),
		address_case(
			"bitmap_2d_address", "uncompressed_mip2",
			{"width": 8, "height": 4, "depth": 1, "type": TYPE_2D, "format": 11, "flags": 0, "mipmaps": 3},
			[1, 0, 2], PIXELS + 164,
		),
		address_case(
			"bitmap_2d_address", "compressed_mip1",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 14, "flags": FLAG_COMPRESSED, "mipmaps": 2},
			[0, 0, 1], PIXELS + 32,
		),
		address_case(
			"bitmap_2d_address", "compressed_nonzero_x_assert",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 14, "flags": FLAG_COMPRESSED, "mipmaps": 2},
			[1, 0, 1], assertion=(0x1A7, "!TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) || (x==0 && y==0)"),
		),
		address_case(
			"bitmap_3d_address", "uncompressed_base",
			{"width": 8, "height": 4, "depth": 2, "type": TYPE_3D, "format": 6, "flags": 0, "mipmaps": 3},
			[2, 1, 1, 0], PIXELS + 84,
		),
		address_case(
			"bitmap_3d_address", "uncompressed_mip2",
			{"width": 8, "height": 4, "depth": 4, "type": TYPE_3D, "format": 0, "flags": 0, "mipmaps": 3},
			[1, 0, 0, 2], PIXELS + 145,
		),
		address_case(
			"bitmap_3d_address", "compressed_mip1",
			{"width": 8, "height": 8, "depth": 4, "type": TYPE_3D, "format": 15, "flags": FLAG_COMPRESSED, "mipmaps": 2},
			[0, 0, 0, 1], PIXELS + 256,
		),
		address_case(
			"bitmap_3d_address", "z_out_of_range_assert",
			{"width": 8, "height": 4, "depth": 2, "type": TYPE_3D, "format": 6, "flags": 0, "mipmaps": 2},
			[0, 0, 2, 0], assertion=(0x1CC, "z>=0 && z<bitmap->depth"),
		),
		address_case(
			"bitmap_cube_map_address", "uncompressed_face5",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_CUBE, "format": 11, "flags": 0, "mipmaps": 3},
			[2, 3, 5, 0], PIXELS + 1384,
		),
		address_case(
			"bitmap_cube_map_address", "uncompressed_mip1",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_CUBE, "format": 6, "flags": 0, "mipmaps": 3},
			[0, 1, 2, 1], PIXELS + 840,
		),
		address_case(
			"bitmap_cube_map_address", "compressed_mip1_face5",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_CUBE, "format": 14, "flags": FLAG_COMPRESSED, "mipmaps": 2},
			[0, 0, 5, 1], PIXELS + 232,
		),
	])
	return cases


def dispatcher_cases():
	cases = []
	for bitmap_type, api in (
		(TYPE_2D, "_bitmap_2d_address"),
		(TYPE_3D, "_bitmap_3d_address"),
		(TYPE_CUBE, "_bitmap_cube_map_address"),
	):
		case = address_case(
			"bitmap_mipmap_address", "type_%d_mip2" % bitmap_type,
			{"width": 8, "height": 8, "depth": 4, "type": bitmap_type, "format": 11, "flags": 0, "mipmaps": 3},
			[2], DISPATCH_RETURNS[api],
		)
		case["dispatch_api"] = api
		cases.append(case)
	cases.append(address_case(
		"bitmap_mipmap_address", "unsupported_type_assert",
		{"width": 8, "height": 8, "depth": 1, "type": 3, "format": 11, "flags": 0, "mipmaps": 3},
		[1], assertion=(0x21C, "### ERROR unsupported bitmap type"),
	))
	return cases


def row_pitch_cases():
	return [
		address_case(
			"bitmap_mipmap_get_row_pitch", "eight_bit_base",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 0, "flags": 0, "mipmaps": 3},
			[0], 8,
		),
		address_case(
			"bitmap_mipmap_get_row_pitch", "thirty_two_bit_mip2",
			{"width": 9, "height": 7, "depth": 1, "type": TYPE_2D, "format": 11, "flags": 0, "mipmaps": 3},
			[2], 8,
		),
		address_case(
			"bitmap_mipmap_get_row_pitch", "verify_failure_assert",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 11, "flags": 0, "mipmaps": 3},
			[0], assertion=(0x3F5, "bitmap_verify(bitmap, FALSE)"),
		),
		address_case(
			"bitmap_mipmap_get_row_pitch", "compressed_assert",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 14, "flags": FLAG_COMPRESSED, "mipmaps": 3},
			[0], assertion=(0x3F7, "!TEST_FLAG(bitmap->flags, _bitmap_compressed_bit)"),
		),
		address_case(
			"bitmap_mipmap_get_row_pitch", "swizzled_assert",
			{"width": 8, "height": 8, "depth": 1, "type": TYPE_2D, "format": 11, "flags": FLAG_SWIZZLED, "mipmaps": 3},
			[0], assertion=(0x3F8, "!TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit)"),
		),
	]


def all_cases():
	cases = []
	cases.extend(constructor_cases("bitmap_2d_new", TYPE_2D))
	cases.extend(constructor_cases("bitmap_3d_new", TYPE_3D))
	cases.extend(constructor_cases("bitmap_cube_map_new", TYPE_CUBE))
	cases.extend(address_cases())
	cases.extend(dispatcher_cases())
	cases.extend(row_pitch_cases())
	for case in cases:
		if case["function"] == "bitmap_mipmap_get_row_pitch" and case["name"] == "verify_failure_assert":
			case["verify"] = 0
	return cases


def cases_by_function(selected):
	result = {name: [] for name in selected}
	for case in all_cases():
		if case["function"] in result:
			result[case["function"]].append(case)
	return result


def pointer_label(pointer):
	if pointer == BITMAP:
		return "bitmap"
	if pointer == ALLOC_BITMAP:
		return "allocated_bitmap"
	if pointer == PIXELS:
		return "pixels"
	if pointer in DISPATCH_RETURNS.values():
		return "pixels+0x%x" % (pointer - PIXELS)
	return "0x%08x" % pointer


def read_cstring(machine, address, limit=512):
	data = bytearray()
	for offset in range(limit):
		value = machine.mem_read(address + offset, 1)[0]
		if value == 0:
			return data.decode("ascii")
		data.append(value)
	raise AssertionError(("runtime string unterminated", address))


def machine_argument(machine, index):
	return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def constructor_expected_bitmap(case):
	data = bytearray(BITMAP_SIZE)
	args = case["args"]
	bitmap_type = case["constructor_type"]
	if bitmap_type == TYPE_2D:
		width, height, mipmaps, bitmap_format = args
		depth = 1
	elif bitmap_type == TYPE_3D:
		width, height, depth, mipmaps, bitmap_format = args
	else:
		width, mipmaps, bitmap_format = args
		height = width
		depth = 1
	put32(data, 0, BITMAP_GROUP_TAG)
	put16(data, 4, width)
	put16(data, 6, height)
	put16(data, 8, depth)
	put16(data, 10, bitmap_type)
	put16(data, 12, bitmap_format)
	flags = FLAG_ALLOCATED
	if bitmap_type == TYPE_CUBE or (
		(width & (width - 1)) == 0
		and (height & (height - 1)) == 0
		and (bitmap_type != TYPE_3D or (depth & (depth - 1)) == 0)
	):
		flags |= FLAG_POWER_OF_TWO
	if 14 <= bitmap_format <= 16:
		flags |= FLAG_COMPRESSED
	if bitmap_format == 17:
		flags |= FLAG_PALETTIZED
	put16(data, 14, flags)
	put16(data, 20, mipmaps)
	if case["malloc"] == "success":
		put32(data, 44, PIXELS)
	return bytes(data)


def constructor_lines(case):
	bitmap_type = case["constructor_type"]
	if bitmap_type == TYPE_2D:
		return 0xB8, 0xD5
	if bitmap_type == TYPE_3D:
		return 0xF5, 0x112
	return 0x12F, 0x14D


def expected_event_core(case):
	function_name = case["function"]
	if function_name.endswith("_new"):
		if case.get("assert"):
			line, expression = case["assert"]
			return [
				{"api": "_display_assert", "args": [expression, SOURCE_FILE, line, 1]},
				{"api": "_system_exit", "args": [-1]},
			]
		first_line, second_line = constructor_lines(case)
		events = [{
			"api": "_debug_malloc",
			"args": [BITMAP_SIZE, 0, SOURCE_FILE, first_line],
		}]
		if case["malloc"] == "object_fail":
			events.append({
				"api": "_error",
				"args": [2, "### ERROR failed to allocate bitmap"],
			})
			return events
		events.extend([
			{"api": "_csmemset", "args": ["allocated_bitmap", 0, BITMAP_SIZE]},
			{"api": "_bitmap_get_pixel_data_size", "args": ["allocated_bitmap"]},
			{"api": "_debug_malloc", "args": [case["pixel_size"], 0, SOURCE_FILE, second_line]},
		])
		if case["malloc"] == "pixel_fail":
			events.append({
				"api": "_error",
				"args": [2, "### ERROR failed to allocate bitmap->base_address"],
			})
			return events
		events.append({"api": "_bitmap_verify", "args": ["allocated_bitmap", 0]})
		if case.get("verify", 1) == 0:
			line = {TYPE_2D: 0xD9, TYPE_3D: 0x116, TYPE_CUBE: 0x151}[case["constructor_type"]]
			events.extend([
				{"api": "_display_assert", "args": ["bitmap_verify(bitmap, FALSE)", SOURCE_FILE, line, 1]},
				{"api": "_system_exit", "args": [-1]},
			])
		return events

	if case.get("assert"):
		prefix = []
		if function_name == "bitmap_mipmap_get_row_pitch":
			prefix.append({"api": "_bitmap_verify", "args": ["bitmap", 0]})
		line, expression = case["assert"]
		prefix.extend([
			{"api": "_display_assert", "args": [expression, SOURCE_FILE, line, 1]},
			{"api": "_system_exit", "args": [-1]},
		])
		return prefix

	bitmap_format = case["bitmap"]["format"]
	if function_name in (
		"bitmap_2d_address",
		"bitmap_3d_address",
		"bitmap_cube_map_address",
	):
		return [{"api": "_bitmap_format_get_bits_per_pixel", "args": [bitmap_format]}]
	if function_name == "bitmap_mipmap_address":
		api = case["dispatch_api"]
		mipmap = case["args"][1]
		if api == "_bitmap_2d_address":
			args = ["bitmap", 0, 0, mipmap]
		else:
			args = ["bitmap", 0, 0, 0, mipmap]
		return [{"api": api, "args": args}]
	if function_name == "bitmap_mipmap_get_row_pitch":
		return [
			{"api": "_bitmap_verify", "args": ["bitmap", 0]},
			{"api": "_bitmap_mipmap_get_width", "args": ["bitmap", case["args"][1]]},
			{"api": "_bitmap_format_get_bits_per_pixel", "args": [bitmap_format]},
		]
	raise AssertionError(("unhandled expected events", function_name))


def execute_case(prepared, case):
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x10000),
		(STUBS, 0x10000),
		(STRINGS, 0x20000),
		(BITMAP, 0x20000),
		(PIXELS, 0x40000),
		(STACK_PAGE, 0x10000),
	):
		machine.mem_map(address, size)
	machine.mem_write(CODE, prepared["raw"])
	for address in STUB_ADDRESSES.values():
		machine.mem_write(address, b"\xc3")
	for address, text in prepared["strings"].items():
		machine.mem_write(address, text.encode("ascii") + b"\0")

	input_bitmap = make_bitmap(case)
	allocated_seed = seeded_bytes(BITMAP_SIZE, case["seed"] + 31)
	pixel_seed = seeded_bytes(0x100, case["seed"] + 43)
	machine.mem_write(BITMAP, input_bitmap)
	machine.mem_write(ALLOC_BITMAP, allocated_seed)
	machine.mem_write(PIXELS, pixel_seed)

	frame = p32(STOP) + b"".join(p32(argument) for argument in case["args"])
	machine.mem_write(ENTRY_SP - 0x800, seeded_bytes(0x800, case["seed"] + 59))
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
	malloc_index = {"value": 0}

	def record(api, arguments):
		events.append({
			"api": api,
			"stack_depth": ENTRY_SP - machine.reg_read(UC_X86_REG_ESP),
			"args": arguments,
		})

	def poison_volatile(eax=None):
		if eax is not None:
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
				poison_volatile(0xA1A2A3A4)
			elif name == "_system_exit":
				record(name, [signed32(machine_argument(machine, 0))])
				terminal["kind"] = "system_exit"
				machine.emu_stop()
			elif name == "_debug_malloc":
				size = machine_argument(machine, 0)
				clear = machine_argument(machine, 1)
				filename = read_cstring(machine, machine_argument(machine, 2))
				line = machine_argument(machine, 3)
				record(name, [size, clear, filename, line])
				index = malloc_index["value"]
				malloc_index["value"] += 1
				mode = case.get("malloc", "success")
				if mode == "object_fail" and index == 0:
					result = 0
				elif mode == "pixel_fail" and index == 1:
					result = 0
				else:
					result = ALLOC_BITMAP if index == 0 else PIXELS
				poison_volatile(result)
			elif name == "_csmemset":
				destination = machine_argument(machine, 0)
				value = machine_argument(machine, 1)
				count = machine_argument(machine, 2)
				record(name, [pointer_label(destination), value, count])
				if destination != ALLOC_BITMAP or count != BITMAP_SIZE:
					raise AssertionError(("unexpected csmemset", destination, count))
				machine.mem_write(destination, bytes([value & 0xFF]) * count)
				poison_volatile(destination)
			elif name == "_bitmap_get_pixel_data_size":
				bitmap_pointer = machine_argument(machine, 0)
				record(name, [pointer_label(bitmap_pointer)])
				if bitmap_pointer != ALLOC_BITMAP:
					raise AssertionError(("unexpected size bitmap", bitmap_pointer))
				poison_volatile(case["pixel_size"])
			elif name == "_bitmap_verify":
				bitmap_pointer = machine_argument(machine, 0)
				verbose = machine_argument(machine, 1)
				record(name, [pointer_label(bitmap_pointer), verbose])
				poison_volatile(case.get("verify", 1))
			elif name == "_error":
				priority = signed32(machine_argument(machine, 0))
				message = read_cstring(machine, machine_argument(machine, 1))
				record(name, [priority, message])
				poison_volatile(0xE1E2E3E4)
			elif name == "_bitmap_format_get_bits_per_pixel":
				bitmap_format = signed32(machine_argument(machine, 0))
				record(name, [bitmap_format])
				if bitmap_format not in BITS_PER_PIXEL:
					raise AssertionError(("unmodeled bitmap format", bitmap_format))
				poison_volatile(BITS_PER_PIXEL[bitmap_format])
			elif name == "_bitmap_mipmap_get_width":
				bitmap_pointer = machine_argument(machine, 0)
				mipmap = signed32(machine_argument(machine, 1))
				record(name, [pointer_label(bitmap_pointer), mipmap])
				width = read16(machine, bitmap_pointer + 4)
				poison_volatile(max(1, width >> mipmap))
			elif name in DISPATCH_RETURNS:
				argument_count = 4 if name == "_bitmap_2d_address" else 5
				arguments = [pointer_label(machine_argument(machine, 0))]
				arguments.extend(
					signed32(machine_argument(machine, index))
					for index in range(1, argument_count)
				)
				record(name, arguments)
				poison_volatile(DISPATCH_RETURNS[name])
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
		"state": {
			"input_bitmap": bytes(machine.mem_read(BITMAP, BITMAP_SIZE)),
			"allocated_bitmap": bytes(machine.mem_read(ALLOC_BITMAP, BITMAP_SIZE)),
			"pixel_prefix": bytes(machine.mem_read(PIXELS, 0x100)),
		},
		"initial": {
			"input_bitmap": input_bitmap,
			"allocated_bitmap": allocated_seed,
			"pixel_prefix": pixel_seed,
		},
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


def event_core(events):
	return [{"api": event["api"], "args": event["args"]} for event in events]


def validate_semantics(case, outcome):
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected_terminal = "system_exit" if case.get("assert") or case.get("verify", 1) == 0 else "return"
	if outcome["terminal"] != expected_terminal:
		errors.append("terminal %r != %r" % (outcome["terminal"], expected_terminal))
	if event_core(outcome["events"]) != expected_event_core(case):
		errors.append("external-call order/arguments differ")
	if outcome["state"]["input_bitmap"] != outcome["initial"]["input_bitmap"]:
		errors.append("input bitmap changed")
	if outcome["state"]["pixel_prefix"] != outcome["initial"]["pixel_prefix"]:
		errors.append("pixel data changed")

	function_name = case["function"]
	if function_name.endswith("_new"):
		if case.get("assert"):
			expected_allocated = outcome["initial"]["allocated_bitmap"]
		elif case["malloc"] == "object_fail":
			expected_allocated = outcome["initial"]["allocated_bitmap"]
		else:
			expected_allocated = constructor_expected_bitmap(case)
		if outcome["state"]["allocated_bitmap"] != expected_allocated:
			errors.append("allocated bitmap fields differ at %s" % differing_ranges(
				outcome["state"]["allocated_bitmap"], expected_allocated,
			))
		if expected_terminal == "return":
			expected_eax = 0 if case["malloc"] == "object_fail" else ALLOC_BITMAP
			if outcome["eax"] != expected_eax:
				errors.append("EAX 0x%08x != 0x%08x" % (outcome["eax"], expected_eax))
	elif expected_terminal == "return" and outcome["eax"] != case["expected_eax"]:
		errors.append("EAX 0x%08x != 0x%08x" % (outcome["eax"], case["expected_eax"]))

	if outcome["caller_frame"][:4] != outcome["expected_caller_frame"][:4]:
		errors.append("caller return address changed")
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
	return errors


def canonical_outcome(outcome):
	return {
		"terminal": outcome["terminal"],
		"events": outcome["events"],
		"state_sha256": {
			name: hashlib.sha256(value).hexdigest()
			for name, value in sorted(outcome["state"].items())
		},
		"eax": outcome["eax"],
		"esp": outcome["esp"],
		"direction_flag": bool(outcome["eflags"] & 0x400),
		"registers": outcome["registers"] if outcome["terminal"] == "return" else None,
		"caller_frame_sha256": hashlib.sha256(outcome["caller_frame"]).hexdigest(),
	}


def concise_difference(actual, expected):
	keys = [key for key in expected if actual.get(key) != expected[key]]
	result = {"differing_components": keys}
	if "events" in keys:
		result["expected_events"] = expected["events"]
		result["actual_events"] = actual.get("events")
	if "terminal" in keys:
		result["expected_terminal"] = expected["terminal"]
		result["actual_terminal"] = actual.get("terminal")
	return result


def run_negative_controls(reference):
	"""Prove that each reported comparison dimension rejects a mutation."""
	first_function = next(iter(reference))
	first_case = next(iter(reference[first_function]))
	baseline = reference[first_function][first_case]
	controls = {}
	mutations = {
		"external_argument": ("events", lambda value: value["events"][0]["args"].__setitem__(0, 10)),
		"eax": ("eax", lambda value: value.__setitem__("eax", value["eax"] ^ 4)),
		"esp": ("esp", lambda value: value.__setitem__("esp", value["esp"] - 4)),
		"callee_saved": ("registers", lambda value: value["registers"].__setitem__("esi", 0)),
		"bitmap_state": ("state_sha256", lambda value: value["state_sha256"].__setitem__("input_bitmap", "0" * 64)),
	}
	for name, pair in mutations.items():
		expected_component, mutate = pair
		mutant = json.loads(json.dumps(baseline))
		mutate(mutant)
		difference = concise_difference(mutant, baseline)
		controls[name] = expected_component in difference["differing_components"]
	return {"pass": all(controls.values()), "controls": controls}


def run_object(path, role, selected, grouped_cases, literal_catalog, target_reference=None):
	obj = cc.load(path)
	result = {"path": str(path), "sha256": sha256_path(path), "functions": {}, "pass": True}
	reference = {}
	for function_name in selected:
		prepared = prepare_function(obj, function_name, literal_catalog)
		section_guards = verify_section(prepared)
		traces = {}
		semantic_failures = []
		target_mismatches = []
		for case in grouped_cases[function_name]:
			outcome = execute_case(prepared, case)
			errors = validate_semantics(case, outcome)
			canonical = canonical_outcome(outcome)
			traces[case["name"]] = canonical
			if errors:
				semantic_failures.append({"case": case["name"], "errors": errors})
			if target_reference is not None:
				expected = target_reference[function_name][case["name"]]
				if canonical != expected:
					target_mismatches.append({
						"case": case["name"],
						**concise_difference(canonical, expected),
					})
		trace_payload = json.dumps(traces, sort_keys=True, separators=(",", ":")).encode("utf-8")
		passed = not semantic_failures and not target_mismatches
		result["functions"][function_name] = {
			"symbol": prepared["symbol"],
			"size": prepared["info"]["size"],
			"relocation_count": prepared["info"]["relocation_count"],
			"normalized_sha256": prepared["info"]["normalized_sha256"],
			"relocation_fingerprint_sha256": hashlib.sha256(json.dumps(
				prepared["relocation_fingerprint"], separators=(",", ":"),
			).encode("utf-8")).hexdigest(),
			"section_guards": section_guards,
			"case_count": len(grouped_cases[function_name]),
			"trace_sha256": hashlib.sha256(trace_payload).hexdigest(),
			"semantic_failure_count": len(semantic_failures),
			"target_mismatch_count": len(target_mismatches),
			"first_semantic_failures": semantic_failures[:5],
			"first_target_mismatches": target_mismatches[:5],
			"pass": passed,
		}
		result["pass"] = result["pass"] and passed
		reference[function_name] = traces
	return result, reference


def resolve_path(path):
	return path if path.is_absolute() else (Path.cwd() / path).resolve()


def verify_hash(path, expected, label):
	actual = sha256_path(path)
	if actual.lower() != expected.lower():
		raise AssertionError((label + " SHA-256 mismatch", str(path), actual, expected))
	return {"path": str(path), "sha256": actual, "size": path.stat().st_size}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--candidate", type=Path, default=CANDIDATE_DEFAULT)
	parser.add_argument("--actual", type=Path, default=ACTUAL_DEFAULT)
	parser.add_argument("--source", type=Path, default=SOURCE_DEFAULT)
	parser.add_argument("--actual-source", type=Path, default=ACTUAL_SOURCE_DEFAULT)
	for label in ("target", "candidate", "actual", "source", "actual_source"):
		parser.add_argument(
			"--%s-sha256" % label.replace("_", "-"),
			dest=label + "_sha256",
			default=DEFAULT_HASHES[label],
		)
	parser.add_argument(
		"--function", action="append", choices=tuple(FUNCTIONS), dest="selected",
		help="limit execution; repeat for multiple functions (default: all eight)",
	)
	args = parser.parse_args()
	selected = args.selected or list(FUNCTIONS)
	paths = {
		"target": resolve_path(args.target),
		"candidate": resolve_path(args.candidate),
		"actual": resolve_path(args.actual),
		"source": resolve_path(args.source),
		"actual_source": resolve_path(args.actual_source),
	}
	evidence = {
		label: verify_hash(paths[label], getattr(args, label + "_sha256"), label)
		for label in paths
	}
	grouped_cases = cases_by_function(selected)
	actual_obj = cc.load(paths["actual"])
	literal_catalog = {}
	for symbol in actual_obj["symbols"]:
		if symbol["section"] <= 0 or not symbol["name"].startswith("??_C@"):
			continue
		try:
			literal_catalog[symbol["name"]] = symbol_cstring(actual_obj, symbol["name"])
		except (cc.CoffError, UnicodeDecodeError, AssertionError):
			pass
	target_result, reference = run_object(
		paths["target"], "target", selected, grouped_cases, literal_catalog,
	)
	objects = {"target": target_result}
	for role in ("candidate", "actual"):
		objects[role], unused_reference = run_object(
			paths[role], role, selected, grouped_cases, literal_catalog, reference,
		)
	negative_controls = run_negative_controls(reference)
	passed = all(result["pass"] for result in objects.values()) and negative_controls["pass"]
	report = {
		"pass": passed,
		"scope": selected,
		"case_count": sum(len(cases) for cases in grouped_cases.values()),
		"script_sha256": sha256_path(Path(__file__)),
		"evidence": evidence,
		"objects": objects,
		"negative_controls": negative_controls,
		"coverage": {
			"constructors": "2D/3D/cube; uncompressed and compressed success; object and pixel allocator failure for each; 2D verify failure and invalid-width fatal assertion",
			"addresses": "2D/3D/cube; base and nonzero mip levels; compressed and uncompressed; cube faces bounded to 0..5; representative invalid-coordinate assertion",
			"mipmap_dispatch": "all three public dispatch arms with exact cdecl arguments and a fatal unsupported-type default",
			"row_pitch": "8/32-bit formats, base/nonzero mip, verify failure, compressed rejection, and swizzled rejection",
			"abi_state": "full 0x30 input/allocated bitmap records, pixel prefix, call order/arguments/stack depths, EAX, ESP, return address, exact target-relative post-call argument-frame footprint, DF, and EBP/EBX/ESI/EDI",
		},
		"limits": [
			"External public APIs are cdecl recording stubs; allocator addresses, pixel-data size, bitmap_verify, bits-per-pixel, and mip-width returns are deterministic case inputs.",
			"Constructor tests validate fields and control flow around bitmap_get_pixel_data_size, but deliberately do not emulate that public function's internal pixel-count algorithm.",
			"Mipmap dispatch stubs the three already-tested public address APIs and validates their exact argument lists; it does not recursively execute a second COFF section.",
			"Fatal paths end at the authentic display_assert then modeled noreturn system_exit(-1); ordinary return-register guarantees are not claimed after noreturn.",
			"Cube face inputs are restricted to the documented 0..5 domain; the implementation has no local face-index assertion, and this oracle does not claim behavior outside that domain.",
			"MSVC reuses some incoming short-argument stack slots as scratch in the address functions. Those slots are not ABI-preserved; their complete post-call bytes are still compared exactly to the January target.",
			"The split January target leaves string COMDATs undefined. Literal bytes are supplied by the separately hash-guarded final canonical object, keyed by the target's exact COFF symbol names.",
		],
	}
	json.dump(report, sys.stdout, indent=2, sort_keys=True)
	sys.stdout.write("\n")
	return 0 if passed else 1


if __name__ == "__main__":
	raise SystemExit(main())
