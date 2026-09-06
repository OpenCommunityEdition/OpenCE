"""Target-first runtime differential for weapon inventory pickup handling.

This bounded oracle executes the real i386 COFF bytes for exactly
``_weapon_handle_potential_inventory_item`` under Unicorn.  It compares the
frozen January target, the first natural inventory candidate, and the final
owner-correct candidate.  Only existing public object/tag/tag-block/sound/
equipment/delete/assert APIs are modeled, as narrow cdecl recording stubs.

The harness deliberately preserves and reports a target behavior that would be
unsafe in a live object store: after deleting a depleted source item inside the
magazine loop, the target can continue into later magazines and access/delete
the same item again.  The stub records that sequence and keeps the mapped datum
available; it does not hide, repair, or reinterpret the target behavior.

Inputs and owner-source evidence are SHA-256 guarded.  The script writes only
its requested scratch JSON result; it does not compile or modify production,
configuration, build, or snapshot inputs.
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


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


TARGET_DEFAULT = (
	ROOT
	/ "scratch/weapons-fable-owner-before-20260905/files/build/split/source/items/weapons.obj"
)
FIRST_DEFAULT = ROOT / "scratch/weapons-fable-inventory-first-20260905.obj"
ACTUAL_DEFAULT = (
	ROOT / "scratch/weapons-fable-owner-inventory-20260905/source/items/weapons.obj"
)
SOURCE_DEFAULT = (
	ROOT / "scratch/weapons-fable-owner-inventory-20260905/files/source/items/weapons.c"
)
SCHEMA_DEFAULT = (
	ROOT
	/ "scratch/weapons-fable-owner-inventory-20260905/files/source/items/weapon_definitions.h"
)
EQUIPMENT_HEADER_DEFAULT = (
	ROOT / "scratch/weapons-fable-owner-inventory-20260905/files/source/items/equipment.h"
)
OUTPUT_DEFAULT = ROOT / "scratch/weapons-inventory-runtime-differential-report.json"

DEFAULT_HASHES = {
	"target": "479772a8aa3ae2518cad52ec71c82faf44382a8808214546e52e2b7cf202ac11",
	"first": "0c824db65f9347bae031aada846eed12bad8edb45f088b65ae2115ef640bcc9a",
	"actual": "015c3ff53cf778015aa666f68298ad011207aa155069658c5483a47d3c59a2a9",
	"source": "391f4f59f91828dfb283a5c7198f4e93d3d59e2481e6f218055e18126d3762fa",
	"schema": "b79d200be38639991ca19162421ae293295285bea02dd6fce7e351269597d9bb",
	"equipment_header": "36e97e2546eb11345b560c1650380eaf6d5a6056cdb4544f369ada1f9ea0bb84",
}

FUNCTION = {
	"symbol": "_weapon_handle_potential_inventory_item",
	"size": 544,
	"relocation_count": 19,
	"normalized_sha256": "afa68283e01df84b8c6dc3feb6f010e45959c139a661b442af1dbc6d432b9ec2",
	"storage": 2,
	"selection": 1,
}

CODE = 0x100000
STOP = 0x10F000
STUBS = 0x200000
STRINGS = 0x300000

WEAPON = 0x400000
ITEM = 0x410000
ITEM_WEAPON = 0x420000
WEAPON_DEFINITION = 0x500000
ALTERNATE_DEFINITION = 0x510000
MAGAZINE_DEFINITIONS = 0x520000
AMMUNITION_OBJECTS = 0x530000
ROUNDS_OUT = 0x600000

STACK_PAGE = 0x700000
ENTRY_SP = STACK_PAGE + 0xC000

WEAPON_HANDLE = 0x13572468
ITEM_HANDLE = 0x24681357
WEAPON_DEFINITION_INDEX = 0x11223344
ITEM_DEFINITION_INDEX = 0x55667766
NONMATCH_DEFINITION_INDEX = 0x66778855
PICKUP_SOUND_INDEX = 0x10203040
WEAPON_GROUP_TAG = 0x77656170
NONE = -1

WEAPON_MAGAZINE_OFFSET = 0x25E
WEAPON_MAGAZINE_STRIDE = 0x0C
WEAPON_DEFINITION_MAGAZINES_OFFSET = 0x4F0
WEAPON_DEFINITION_PICKUP_SOUND_INDEX_OFFSET = 0x49C
MAGAZINE_DEFINITION_SIZE = 0x70
MAGAZINE_DEFINITION_STRIDE = MAGAZINE_DEFINITION_SIZE
MAGAZINE_MAXIMUM_OFFSET = 0x08
MAGAZINE_AMMUNITION_BLOCK_OFFSET = 0x64
AMMUNITION_OBJECT_SIZE = 0x1C
AMMUNITION_OBJECT_STRIDE = 0x1C
AMMUNITION_OBJECT_MAGAZINE_STRIDE = 0x1000
AMMUNITION_OBJECT_INDEX_OFFSET = 0x18

STATE_REGIONS = {
	"weapon": (WEAPON, 0x300),
	"item": (ITEM, 0x40),
	"item_weapon": (ITEM_WEAPON, 0x300),
	"weapon_definition": (WEAPON_DEFINITION, 0x600),
	"alternate_definition": (ALTERNATE_DEFINITION, 0x600),
	"magazine_definitions": (MAGAZINE_DEFINITIONS, 0x200),
	"ammunition_objects": (AMMUNITION_OBJECTS, 0x2000),
	"rounds_out": (ROUNDS_OUT, 0x10),
}

STUB_NAMES = (
	"_object_get_and_verify_type",
	"_tag_get",
	"_tag_block_get_element_with_size",
	"_display_assert",
	"_system_exit",
	"_unspatialized_impulse_sound_new",
	"_equipment_definition_handle_pickup",
	"_object_delete",
)
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


def p16(value: int) -> bytes:
	return struct.pack("<H", value & 0xFFFF)


def p32(value: int) -> bytes:
	return struct.pack("<I", value & 0xFFFFFFFF)


def read16(machine: u.Uc, address: int) -> int:
	return struct.unpack("<H", machine.mem_read(address, 2))[0]


def read32(machine: u.Uc, address: int) -> int:
	return struct.unpack("<I", machine.mem_read(address, 4))[0]


def signed16(value: int) -> int:
	return struct.unpack("<h", p16(value))[0]


def signed32(value: int) -> int:
	return struct.unpack("<i", p32(value))[0]


def put16(data: bytearray, offset: int, value: int) -> None:
	data[offset : offset + 2] = p16(value)


def put32(data: bytearray, offset: int, value: int) -> None:
	data[offset : offset + 4] = p32(value)


def sha256_path(path: Path) -> str:
	return hashlib.sha256(path.read_bytes()).hexdigest()


def seeded_bytes(size: int, salt: int) -> bytes:
	return bytes(((index * 37 + salt * 53 + 11) & 0xFF) for index in range(size))


def read_cstring(machine: u.Uc, address: int, limit: int = 512) -> str:
	data = bytearray()
	for offset in range(limit):
		value = machine.mem_read(address + offset, 1)[0]
		if value == 0:
			return data.decode("ascii")
		data.append(value)
	raise AssertionError(("runtime string unterminated", address))


def symbol_cstring(obj: dict, name: str) -> str:
	symbol = cc.symbol(obj, name)
	section = obj["sections"][symbol["section"] - 1]
	raw = cc._section_bytes(obj, section)
	start = symbol["value"]
	end = raw.find(b"\0", start)
	if end < 0:
		raise AssertionError(("unterminated literal", name))
	return raw[start:end].decode("ascii")


def machine_argument(machine: u.Uc, index: int) -> int:
	return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def handle_label(value: int) -> str:
	if value == WEAPON_HANDLE:
		return "weapon_handle:0x%08x" % value
	if value == ITEM_HANDLE:
		return "item_handle:0x%08x" % value
	return "0x%08x" % value


def definition_label(value: int) -> str:
	if value == WEAPON_DEFINITION_INDEX:
		return "weapon_definition:0x%08x" % value
	if value == ITEM_DEFINITION_INDEX:
		return "item_definition:0x%08x" % value
	if value == NONMATCH_DEFINITION_INDEX:
		return "nonmatch_definition:0x%08x" % value
	return "0x%08x" % value


def pointer_label(value: int) -> str:
	labels = {
		WEAPON: "weapon",
		ITEM: "item",
		ITEM_WEAPON: "item_weapon",
		WEAPON_DEFINITION: "weapon_definition",
		ALTERNATE_DEFINITION: "alternate_definition",
		ROUNDS_OUT: "rounds_out",
	}
	if value in labels:
		return labels[value]
	if value == WEAPON_DEFINITION + WEAPON_DEFINITION_MAGAZINES_OFFSET:
		return "weapon_definition.magazines"
	for magazine_index in range(2):
		magazine_definition = (
			MAGAZINE_DEFINITIONS + magazine_index * MAGAZINE_DEFINITION_STRIDE
		)
		if value == magazine_definition:
			return "magazine_definition[%d]" % magazine_index
		if value == magazine_definition + MAGAZINE_AMMUNITION_BLOCK_OFFSET:
			return "magazine_definition[%d].ammunition_objects" % magazine_index
		ammunition_base = (
			AMMUNITION_OBJECTS
			+ magazine_index * AMMUNITION_OBJECT_MAGAZINE_STRIDE
		)
		if ammunition_base <= value < ammunition_base + 0x1000:
			index = (value - ammunition_base) // AMMUNITION_OBJECT_STRIDE
			return "ammunition_object[%d][%d]" % (magazine_index, index)
	return "0x%08x" % value


def prepare_function(obj: dict) -> dict:
	available = function_sections(obj)
	symbol_name = FUNCTION["symbol"]
	if symbol_name not in available:
		raise AssertionError(("missing function", symbol_name))
	function = available[symbol_name]
	section = obj["sections"][function.section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))

	literals: dict[str, str] = {}
	for relocation in function.info["relocations"]:
		target = relocation.get("symbolic_target", relocation["target"])
		if not isinstance(target, (list, tuple)) or target[0] != "symbol":
			raise AssertionError(("unresolved relocation", target))
		name = target[1]
		if relocation["type"] == cc.IMAGE_REL_I386_DIR32:
			literals[name] = symbol_cstring(obj, name)

	literal_addresses = {
		text: STRINGS + 0x200 * (index + 1)
		for index, text in enumerate(sorted(set(literals.values())))
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
			raise AssertionError(("unsupported relocation owner", name))
		fingerprint.append([relocation["address"], relocation["type"], identity])
		if relocation["type"] == cc.IMAGE_REL_I386_REL32:
			address -= CODE + relocation["address"] + 4
		elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
			raise AssertionError(("unsupported relocation type", relocation["type"]))
		raw[relocation["address"] : relocation["address"] + 4] = p32(address)

	symbol = cc.symbol(obj, symbol_name)
	return {
		"raw": bytes(raw),
		"strings": {literal_addresses[text]: text for text in literal_addresses},
		"info": function.info,
		"symbol_type": symbol["type"],
		"storage": symbol["storage"],
		"selection": _section_aux_selection(obj, symbol["section"]),
		"relocation_fingerprint": fingerprint,
	}


def verify_section(prepared: dict) -> dict:
	info = prepared["info"]
	checks = {
		"size": info["size"] == FUNCTION["size"],
		"relocation_count": info["relocation_count"] == FUNCTION["relocation_count"],
		"normalized_sha256": info["normalized_sha256"] == FUNCTION["normalized_sha256"],
		"function_type_0x20": prepared["symbol_type"] == 0x20,
		"storage_external_2": prepared["storage"] == FUNCTION["storage"],
		"comdat_noduplicates_1": prepared["selection"] == FUNCTION["selection"],
	}
	failed = [key for key, value in checks.items() if not value]
	if failed:
		raise AssertionError(("section guard", failed))
	return checks


def add_case(
	cases: list[dict],
	name: str,
	mode: str,
	target_rounds: list[int],
	maximums: list[int],
	*,
	source_rounds: list[int] | None = None,
	ammunition: list[list[tuple[int, int]]] | None = None,
	local_player: int = 0,
	pickup_sound: int = PICKUP_SOUND_INDEX,
	rounds_out_initial: int | None = None,
	expect_target: list[int] | None = None,
	expect_source: list[int] | None = None,
	expect_rounds_out: int | None = None,
	expect_handled: int = 0,
	expect_sounds: list[int] | None = None,
	expect_equipment: list[int] | None = None,
	expect_deletes: int = 0,
	tag_sequence: list[str] | None = None,
	terminal: str = "return",
) -> None:
	magazine_count = len(target_rounds)
	if len(maximums) != magazine_count or magazine_count > 2:
		raise AssertionError((name, "bad magazine arrays"))
	if source_rounds is None:
		source_rounds = [101 + index for index in range(magazine_count)]
	if len(source_rounds) != magazine_count:
		raise AssertionError((name, "bad source array"))
	if ammunition is None:
		ammunition = [[] for unused in range(magazine_count)]
	if len(ammunition) != magazine_count:
		raise AssertionError((name, "bad ammunition array"))
	if rounds_out_initial is None:
		rounds_out_initial = 0x5A00 + len(cases)
	if expect_target is None:
		expect_target = list(target_rounds)
	if expect_source is None:
		expect_source = list(source_rounds)
	if expect_rounds_out is None:
		expect_rounds_out = rounds_out_initial
	cases.append({
		"name": name,
		"seed": len(cases) + 1,
		"mode": mode,
		"magazine_count": magazine_count,
		"target_rounds": target_rounds,
		"maximums": maximums,
		"source_rounds": source_rounds,
		"ammunition": ammunition,
		"local_player": local_player,
		"pickup_sound": pickup_sound,
		"rounds_out_initial": rounds_out_initial,
		"tag_sequence": tag_sequence or ["main"],
		"expected": {
			"terminal": terminal,
			"handled": expect_handled,
			"target_rounds": expect_target,
			"source_rounds": expect_source,
			"rounds_out": expect_rounds_out,
			"sounds": expect_sounds or [],
			"equipment": expect_equipment or [],
			"deletes": expect_deletes,
		},
	})


def all_cases() -> list[dict]:
	cases: list[dict] = []
	add_case(cases, "zero_magazines_sentinel", "same", [], [])
	add_case(
		cases, "same_partial_depletes_sound_local", "same", [2], [10],
		source_rounds=[7], expect_target=[9], expect_source=[0],
		expect_rounds_out=7, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "same_capped_source_remains", "same", [8], [10],
		source_rounds=[7], expect_target=[10], expect_source=[5],
		expect_rounds_out=2, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX],
	)
	add_case(
		cases, "same_pickup_sound_none", "same", [2], [10],
		source_rounds=[3], pickup_sound=NONE, expect_target=[5],
		expect_source=[0], expect_rounds_out=3, expect_handled=1,
		expect_deletes=1,
	)
	add_case(
		cases, "same_local_none", "same", [0], [4],
		source_rounds=[2], local_player=NONE, expect_target=[2],
		expect_source=[0], expect_rounds_out=2, expect_handled=1,
		expect_deletes=1,
	)
	add_case(
		cases, "same_empty_source_still_handled", "same", [5], [10],
		source_rounds=[0], expect_rounds_out=0, expect_handled=1,
	)
	add_case(
		cases, "same_full_target_preserves_sentinel", "same", [10], [10],
		source_rounds=[7],
	)
	add_case(
		cases, "same_two_magazines_last_write_zero", "same", [0, 0], [5, 5],
		source_rounds=[2, 0], expect_target=[2, 0], expect_source=[0, 0],
		expect_rounds_out=0, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "same_two_magazines_repeated_delete_target_behavior", "same",
		[0, 0], [5, 5], source_rounds=[1, 1], expect_target=[1, 1],
		expect_source=[0, 0], expect_rounds_out=1, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX, PICKUP_SOUND_INDEX],
		expect_deletes=2,
	)
	add_case(
		cases, "same_first_full_second_depletes", "same", [5, 1], [5, 5],
		source_rounds=[4, 3], expect_target=[5, 4], expect_source=[4, 0],
		expect_rounds_out=3, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "equipment_second_object_matches_capped", "equipment", [7], [10],
		ammunition=[[(9, NONMATCH_DEFINITION_INDEX), (5, ITEM_DEFINITION_INDEX)]],
		expect_target=[10], expect_rounds_out=3, expect_handled=1,
		expect_equipment=[ITEM_DEFINITION_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "equipment_match_local_none", "equipment", [0], [5],
		ammunition=[[(2, ITEM_DEFINITION_INDEX)]], local_player=NONE,
		expect_target=[2], expect_rounds_out=2, expect_handled=1,
		expect_deletes=1,
	)
	add_case(
		cases, "equipment_nonmatch", "equipment", [1], [5],
		ammunition=[[(4, NONMATCH_DEFINITION_INDEX)]], expect_rounds_out=0,
	)
	add_case(
		cases, "equipment_matching_zero_rounds", "equipment", [1], [5],
		ammunition=[[(0, ITEM_DEFINITION_INDEX)]], expect_rounds_out=0,
	)
	add_case(
		cases, "equipment_zero_then_positive_match", "equipment", [1], [5],
		ammunition=[[(0, ITEM_DEFINITION_INDEX), (3, ITEM_DEFINITION_INDEX)]],
		expect_target=[4], expect_rounds_out=3, expect_handled=1,
		expect_equipment=[ITEM_DEFINITION_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "equipment_full_target_preserves_sentinel", "equipment", [8], [8],
		ammunition=[[(7, ITEM_DEFINITION_INDEX)]],
	)
	add_case(
		cases, "equipment_two_magazines_last_nonmatch_zero", "equipment",
		[0, 0], [5, 5],
		ammunition=[[(3, ITEM_DEFINITION_INDEX)], [(4, NONMATCH_DEFINITION_INDEX)]],
		expect_target=[3, 0], expect_rounds_out=0, expect_handled=1,
		expect_equipment=[ITEM_DEFINITION_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "equipment_two_magazines_repeated_delete_target_behavior", "equipment",
		[0, 0], [5, 5],
		ammunition=[[(1, ITEM_DEFINITION_INDEX)], [(1, ITEM_DEFINITION_INDEX)]],
		expect_target=[1, 1], expect_rounds_out=1, expect_handled=1,
		expect_equipment=[ITEM_DEFINITION_INDEX, ITEM_DEFINITION_INDEX],
		expect_deletes=2,
	)
	add_case(
		cases, "equipment_rounds_short_max_local_short_max", "equipment",
		[0], [32767], ammunition=[[(32767, ITEM_DEFINITION_INDEX)]],
		local_player=32767, expect_target=[32767], expect_rounds_out=32767,
		expect_handled=1, expect_equipment=[ITEM_DEFINITION_INDEX],
		expect_deletes=1,
	)
	add_case(
		cases, "equipment_local_short_min_is_not_none", "equipment", [0], [2],
		ammunition=[[(1, ITEM_DEFINITION_INDEX)]], local_player=-32768,
		expect_target=[1], expect_rounds_out=1, expect_handled=1,
		expect_equipment=[ITEM_DEFINITION_INDEX], expect_deletes=1,
	)
	add_case(
		cases, "same_local_short_max_sound", "same", [32766], [32767],
		source_rounds=[32767], local_player=32767, expect_target=[32767],
		expect_source=[32766], expect_rounds_out=1, expect_handled=1,
		expect_sounds=[PICKUP_SOUND_INDEX],
	)
	add_case(
		cases, "fatal_weapon_magazine_bounds_after_tag_change", "equipment",
		[0], [5], ammunition=[[]], tag_sequence=["main", "alternate"],
		terminal="system_exit",
	)
	add_case(
		cases, "fatal_item_magazine_bounds_after_tag_change", "same", [0], [5],
		source_rounds=[1], tag_sequence=["main", "main", "alternate"],
		terminal="system_exit",
	)
	return cases


def build_initial_state(case: dict) -> dict[str, bytes]:
	state = {
		name: bytearray(seeded_bytes(size, case["seed"] + index * 17))
		for index, (name, (unused_address, size)) in enumerate(STATE_REGIONS.items())
	}

	put32(state["weapon"], 0, WEAPON_DEFINITION_INDEX)
	put32(
		state["item"], 0,
		WEAPON_DEFINITION_INDEX if case["mode"] == "same" else ITEM_DEFINITION_INDEX,
	)
	put32(state["item_weapon"], 0, WEAPON_DEFINITION_INDEX)
	for index, rounds in enumerate(case["target_rounds"]):
		put16(
			state["weapon"],
			WEAPON_MAGAZINE_OFFSET + index * WEAPON_MAGAZINE_STRIDE,
			rounds,
		)
	for index, rounds in enumerate(case["source_rounds"]):
		put16(
			state["item_weapon"],
			WEAPON_MAGAZINE_OFFSET + index * WEAPON_MAGAZINE_STRIDE,
			rounds,
		)

	put32(
		state["weapon_definition"], WEAPON_DEFINITION_MAGAZINES_OFFSET,
		case["magazine_count"],
	)
	put32(
		state["weapon_definition"], WEAPON_DEFINITION_MAGAZINES_OFFSET + 4,
		MAGAZINE_DEFINITIONS,
	)
	put32(
		state["weapon_definition"], WEAPON_DEFINITION_MAGAZINES_OFFSET + 8,
		0x0BADF00D,
	)
	put32(
		state["weapon_definition"], WEAPON_DEFINITION_PICKUP_SOUND_INDEX_OFFSET,
		case["pickup_sound"],
	)
	put32(state["alternate_definition"], WEAPON_DEFINITION_MAGAZINES_OFFSET, 0)

	for magazine_index in range(case["magazine_count"]):
		definition_offset = magazine_index * MAGAZINE_DEFINITION_STRIDE
		put16(
			state["magazine_definitions"],
			definition_offset + MAGAZINE_MAXIMUM_OFFSET,
			case["maximums"][magazine_index],
		)
		ammunition = case["ammunition"][magazine_index]
		put32(
			state["magazine_definitions"],
			definition_offset + MAGAZINE_AMMUNITION_BLOCK_OFFSET,
			len(ammunition),
		)
		put32(
			state["magazine_definitions"],
			definition_offset + MAGAZINE_AMMUNITION_BLOCK_OFFSET + 4,
			AMMUNITION_OBJECTS
			+ magazine_index * AMMUNITION_OBJECT_MAGAZINE_STRIDE,
		)
		put32(
			state["magazine_definitions"],
			definition_offset + MAGAZINE_AMMUNITION_BLOCK_OFFSET + 8,
			0x0D15EA5E + magazine_index,
		)
		for ammunition_index, (rounds, definition_index) in enumerate(ammunition):
			object_offset = (
				magazine_index * AMMUNITION_OBJECT_MAGAZINE_STRIDE
				+ ammunition_index * AMMUNITION_OBJECT_STRIDE
			)
			put16(state["ammunition_objects"], object_offset, rounds)
			put32(
				state["ammunition_objects"],
				object_offset + AMMUNITION_OBJECT_INDEX_OFFSET,
				definition_index,
			)

	put16(state["rounds_out"], 0, case["rounds_out_initial"])
	return {name: bytes(value) for name, value in state.items()}


def expected_state(case: dict, initial: dict[str, bytes]) -> dict[str, bytes]:
	state = {name: bytearray(value) for name, value in initial.items()}
	for index, rounds in enumerate(case["expected"]["target_rounds"]):
		put16(
			state["weapon"],
			WEAPON_MAGAZINE_OFFSET + index * WEAPON_MAGAZINE_STRIDE,
			rounds,
		)
	for index, rounds in enumerate(case["expected"]["source_rounds"]):
		put16(
			state["item_weapon"],
			WEAPON_MAGAZINE_OFFSET + index * WEAPON_MAGAZINE_STRIDE,
			rounds,
		)
	put16(state["rounds_out"], 0, case["expected"]["rounds_out"])
	return {name: bytes(value) for name, value in state.items()}


def state_rounds(state: dict[str, bytes], region: str, count: int) -> list[int]:
	return [
		signed16(struct.unpack_from(
			"<H", state[region],
			WEAPON_MAGAZINE_OFFSET + index * WEAPON_MAGAZINE_STRIDE,
		)[0])
		for index in range(count)
	]


def execute_case(prepared: dict, case: dict) -> dict:
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x10000),
		(STUBS, 0x10000),
		(STRINGS, 0x20000),
		(0x400000, 0x30000),
		(0x500000, 0x50000),
		(ROUNDS_OUT, 0x10000),
		(STACK_PAGE, 0x10000),
	):
		machine.mem_map(address, size)
	machine.mem_write(CODE, prepared["raw"])
	for address in STUB_ADDRESSES.values():
		machine.mem_write(address, b"\xc3")
	for address, text in prepared["strings"].items():
		machine.mem_write(address, text.encode("ascii") + b"\0")

	initial = build_initial_state(case)
	for name, (address, unused_size) in STATE_REGIONS.items():
		machine.mem_write(address, initial[name])

	frame = (
		p32(STOP)
		+ p32(WEAPON_HANDLE)
		+ p32(ITEM_HANDLE)
		+ p32(case["local_player"])
		+ p32(ROUNDS_OUT)
	)
	machine.mem_write(ENTRY_SP - 0x1000, seeded_bytes(0x1000, case["seed"] + 97))
	machine.mem_write(ENTRY_SP, frame)
	machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
	machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
	machine.reg_write(UC_X86_REG_EAX, 0xA0A1A2A3)
	machine.reg_write(UC_X86_REG_ECX, 0xB0B1B2B3)
	machine.reg_write(UC_X86_REG_EDX, 0xC0C1C2C3)
	for register, value in SAVED_REGISTERS.items():
		machine.reg_write(register, value)

	events: list[dict] = []
	hook_errors: list[str] = []
	terminal = {"kind": None}
	tag_get_index = {"value": 0}

	def record(api: str, arguments: list) -> None:
		events.append({
			"api": api,
			"stack_depth": ENTRY_SP - machine.reg_read(UC_X86_REG_ESP),
			"args": arguments,
		})

	def poison_volatile(eax: int) -> None:
		machine.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
		machine.reg_write(UC_X86_REG_ECX, 0xC1C2C3C4)
		machine.reg_write(UC_X86_REG_EDX, 0xD1D2D3D4)

	def stub_hook(unused_machine, address, unused_size, unused_data) -> None:
		name = next(
			(key for key, value in STUB_ADDRESSES.items() if value == address),
			None,
		)
		if name is None:
			hook_errors.append("unknown stub 0x%08x" % address)
			machine.emu_stop()
			return
		try:
			if name == "_object_get_and_verify_type":
				handle = machine_argument(machine, 0)
				mask = machine_argument(machine, 1)
				record(name, [handle_label(handle), "mask:0x%x" % mask])
				if (handle, mask) == (WEAPON_HANDLE, 0x4):
					result = WEAPON
				elif (handle, mask) == (ITEM_HANDLE, 0x1C):
					# One genuine object handle has one datum address.  A same-weapon
					# item satisfies both masks and therefore returns ITEM_WEAPON for
					# both lookups; an equipment item satisfies only the item mask.
					result = ITEM_WEAPON if case["mode"] == "same" else ITEM
				elif (
					(handle, mask) == (ITEM_HANDLE, 0x4)
					and case["mode"] == "same"
				):
					result = ITEM_WEAPON
				else:
					raise AssertionError(("unexpected object lookup", handle, mask))
				poison_volatile(result)
			elif name == "_tag_get":
				group_tag = machine_argument(machine, 0)
				definition_index = machine_argument(machine, 1)
				record(name, ["weap:0x%08x" % group_tag, definition_label(definition_index)])
				if group_tag != WEAPON_GROUP_TAG:
					raise AssertionError(("unexpected tag group", group_tag))
				if definition_index != WEAPON_DEFINITION_INDEX:
					raise AssertionError(("unexpected tag index", definition_index))
				sequence = case["tag_sequence"]
				sequence_index = tag_get_index["value"]
				tag_get_index["value"] += 1
				choice = sequence[min(sequence_index, len(sequence) - 1)]
				if choice == "main":
					result = WEAPON_DEFINITION
				elif choice == "alternate":
					result = ALTERNATE_DEFINITION
				else:
					raise AssertionError(("bad tag sequence choice", choice))
				poison_volatile(result)
			elif name == "_tag_block_get_element_with_size":
				block = machine_argument(machine, 0)
				index = signed32(machine_argument(machine, 1))
				element_size = machine_argument(machine, 2)
				record(name, [pointer_label(block), index, element_size])
				if block == WEAPON_DEFINITION + WEAPON_DEFINITION_MAGAZINES_OFFSET:
					if element_size != MAGAZINE_DEFINITION_SIZE:
						raise AssertionError(("bad magazine element size", element_size))
					if not 0 <= index < case["magazine_count"]:
						raise AssertionError(("bad magazine index", index))
					result = MAGAZINE_DEFINITIONS + index * MAGAZINE_DEFINITION_STRIDE
				else:
					result = None
					for magazine_index in range(case["magazine_count"]):
						magazine_definition = (
							MAGAZINE_DEFINITIONS
							+ magazine_index * MAGAZINE_DEFINITION_STRIDE
						)
						if block != magazine_definition + MAGAZINE_AMMUNITION_BLOCK_OFFSET:
							continue
						if element_size != AMMUNITION_OBJECT_SIZE:
							raise AssertionError(("bad ammunition element size", element_size))
						if not 0 <= index < len(case["ammunition"][magazine_index]):
							raise AssertionError(("bad ammunition index", magazine_index, index))
						result = (
							AMMUNITION_OBJECTS
							+ magazine_index * AMMUNITION_OBJECT_MAGAZINE_STRIDE
							+ index * AMMUNITION_OBJECT_STRIDE
						)
						break
					if result is None:
						raise AssertionError(("unexpected tag block", block, index))
				poison_volatile(result)
			elif name == "_display_assert":
				record(name, [
					read_cstring(machine, machine_argument(machine, 0)),
					read_cstring(machine, machine_argument(machine, 1)),
					machine_argument(machine, 2),
					machine_argument(machine, 3),
				])
				poison_volatile(0xA1A2A3A4)
			elif name == "_system_exit":
				record(name, [signed32(machine_argument(machine, 0))])
				if signed32(machine_argument(machine, 0)) != -1:
					raise AssertionError(("unexpected exit status", machine_argument(machine, 0)))
				terminal["kind"] = "system_exit"
				machine.emu_stop()
			elif name == "_unspatialized_impulse_sound_new":
				sound_index = machine_argument(machine, 0)
				gain_bits = machine_argument(machine, 1)
				record(name, [definition_label(sound_index), "float_bits:0x%08x" % gain_bits])
				if gain_bits != 0x3F800000:
					raise AssertionError(("unexpected sound gain", gain_bits))
				poison_volatile(0x51525354)
			elif name == "_equipment_definition_handle_pickup":
				definition_index = machine_argument(machine, 0)
				record(name, [definition_label(definition_index)])
				if definition_index != ITEM_DEFINITION_INDEX:
					raise AssertionError(("unexpected equipment index", definition_index))
				poison_volatile(0x61626364)
			elif name == "_object_delete":
				handle = machine_argument(machine, 0)
				record(name, [handle_label(handle)])
				if handle != ITEM_HANDLE:
					raise AssertionError(("unexpected delete handle", handle))
				poison_volatile(0x71727374)
			else:
				raise AssertionError(("unhandled stub", name))
		except (AssertionError, UnicodeDecodeError, u.UcError) as error:
			hook_errors.append(repr(error))
			machine.emu_stop()

	machine.hook_add(u.UC_HOOK_CODE, stub_hook, begin=STUBS, end=STUBS + 0xFFFF)
	execution_error = None
	try:
		machine.emu_start(CODE, STOP, count=20000)
	except u.UcError as error:
		execution_error = str(error)
	if terminal["kind"] is None and execution_error is None and not hook_errors:
		terminal["kind"] = (
			"return" if machine.reg_read(UC_X86_REG_EIP) == STOP else "other"
		)

	state = {
		name: bytes(machine.mem_read(address, size))
		for name, (address, size) in STATE_REGIONS.items()
	}
	return {
		"terminal": terminal["kind"],
		"execution_error": execution_error,
		"hook_errors": hook_errors,
		"events": events,
		"state": state,
		"initial": initial,
		"al": machine.reg_read(UC_X86_REG_EAX) & 0xFF,
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


def external_summary(events: list[dict]) -> dict:
	return {
		"sounds": [
			event["args"][0]
			for event in events
			if event["api"] == "_unspatialized_impulse_sound_new"
		],
		"equipment": [
			event["args"][0]
			for event in events
			if event["api"] == "_equipment_definition_handle_pickup"
		],
		"deletes": [
			event["args"][0]
			for event in events
			if event["api"] == "_object_delete"
		],
	}


def observed_summary(case: dict, outcome: dict) -> dict:
	summary = external_summary(outcome["events"])
	return {
		"target_rounds": state_rounds(
			outcome["state"], "weapon", case["magazine_count"]
		),
		"source_rounds": state_rounds(
			outcome["state"], "item_weapon", case["magazine_count"]
		),
		"rounds_out": signed16(struct.unpack_from("<H", outcome["state"]["rounds_out"])[0]),
		"sounds": summary["sounds"],
		"equipment": summary["equipment"],
		"deletes": summary["deletes"],
	}


def validate_semantics(case: dict, outcome: dict) -> list[str]:
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected = case["expected"]
	if outcome["terminal"] != expected["terminal"]:
		errors.append("terminal %r != %r" % (outcome["terminal"], expected["terminal"]))

	expected_bytes = expected_state(case, outcome["initial"])
	for name in sorted(expected_bytes):
		if outcome["state"][name] != expected_bytes[name]:
			errors.append("state region %s differs from explicit expectation" % name)

	observed = observed_summary(case, outcome)
	if observed["target_rounds"] != expected["target_rounds"]:
		errors.append("target rounds differ")
	if observed["source_rounds"] != expected["source_rounds"]:
		errors.append("source rounds differ")
	if observed["rounds_out"] != expected["rounds_out"]:
		errors.append("rounds_out differs")
	if observed["sounds"] != [definition_label(value) for value in expected["sounds"]]:
		errors.append("sound calls differ")
	if observed["equipment"] != [
		definition_label(value) for value in expected["equipment"]
	]:
		errors.append("equipment calls differ")
	if observed["deletes"] != [handle_label(ITEM_HANDLE)] * expected["deletes"]:
		errors.append("delete calls differ")

	if expected["terminal"] == "return":
		if outcome["al"] != expected["handled"]:
			errors.append("AL %d != handled %d" % (outcome["al"], expected["handled"]))
		if outcome["eip"] != STOP:
			errors.append("ordinary return did not reach STOP")
		if outcome["esp"] != ENTRY_SP + 4:
			errors.append("cdecl stack imbalance")
		if outcome["eflags"] & 0x400:
			errors.append("direction flag left set")
		for register, value in SAVED_REGISTERS.items():
			name = SAVED_REGISTER_NAMES[register]
			if outcome["registers"][name] != value:
				errors.append("nonvolatile register %s changed" % name)
	if outcome["caller_frame"] != outcome["expected_caller_frame"]:
		errors.append("caller frame bytes changed")
	return errors


def canonical_outcome(case: dict, outcome: dict) -> dict:
	return {
		"terminal": outcome["terminal"],
		"events": outcome["events"],
		"state_sha256": {
			name: hashlib.sha256(value).hexdigest()
			for name, value in sorted(outcome["state"].items())
		},
		"observed": observed_summary(case, outcome),
		"al": outcome["al"] if outcome["terminal"] == "return" else None,
		"esp": outcome["esp"] if outcome["terminal"] == "return" else None,
		"direction_flag": (
			bool(outcome["eflags"] & 0x400)
			if outcome["terminal"] == "return"
			else None
		),
		"registers": outcome["registers"] if outcome["terminal"] == "return" else None,
		"caller_frame_sha256": hashlib.sha256(outcome["caller_frame"]).hexdigest(),
	}


def concise_difference(actual: dict, expected: dict) -> dict:
	keys = [key for key in expected if actual.get(key) != expected[key]]
	result = {"differing_components": keys}
	for key in ("terminal", "events", "observed", "al", "esp", "direction_flag"):
		if key in keys:
			result["expected_" + key] = expected[key]
			result["actual_" + key] = actual.get(key)
	return result


def run_object(
	path: Path,
	cases: list[dict],
	target_reference: dict[str, dict] | None = None,
) -> tuple[dict, dict[str, dict]]:
	prepared = prepare_function(cc.load(path))
	section_guards = verify_section(prepared)
	traces: dict[str, dict] = {}
	semantic_failures = []
	target_mismatches = []
	for case in cases:
		outcome = execute_case(prepared, case)
		errors = validate_semantics(case, outcome)
		canonical = canonical_outcome(case, outcome)
		traces[case["name"]] = canonical
		if errors:
			semantic_failures.append({"case": case["name"], "errors": errors})
		if target_reference is not None:
			expected = target_reference[case["name"]]
			if canonical != expected:
				target_mismatches.append({
					"case": case["name"],
					**concise_difference(canonical, expected),
				})
	trace_payload = json.dumps(traces, sort_keys=True, separators=(",", ":")).encode()
	return ({
		"path": str(path),
		"sha256": sha256_path(path),
		"size": path.stat().st_size,
		"section": {
			"symbol": FUNCTION["symbol"],
			"size": prepared["info"]["size"],
			"relocation_count": prepared["info"]["relocation_count"],
			"normalized_sha256": prepared["info"]["normalized_sha256"],
			"storage": prepared["storage"],
			"selection": prepared["selection"],
			"relocation_fingerprint_sha256": hashlib.sha256(json.dumps(
				prepared["relocation_fingerprint"], separators=(",", ":"),
			).encode()).hexdigest(),
			"guards": section_guards,
		},
		"case_count": len(cases),
		"trace_sha256": hashlib.sha256(trace_payload).hexdigest(),
		"semantic_failure_count": len(semantic_failures),
		"target_mismatch_count": len(target_mismatches),
		"semantic_failures": semantic_failures,
		"target_mismatches": target_mismatches,
		"pass": not semantic_failures and not target_mismatches,
	}, traces)


def run_negative_controls(reference: dict[str, dict]) -> dict:
	baseline = reference["same_partial_depletes_sound_local"]
	controls = {}

	def check(name: str, mutate, expected_component: str) -> None:
		mutant = json.loads(json.dumps(baseline))
		mutate(mutant)
		difference = concise_difference(mutant, baseline)
		controls[name] = expected_component in difference["differing_components"]

	check(
		"external_argument",
		lambda value: value["events"][0]["args"].__setitem__(0, "wrong_handle"),
		"events",
	)
	check(
		"event_order",
		lambda value: value["events"].__setitem__(
			slice(0, 2), list(reversed(value["events"][:2]))
		),
		"events",
	)
	check("return_al", lambda value: value.__setitem__("al", value["al"] ^ 1), "al")
	check("stack_pointer", lambda value: value.__setitem__("esp", value["esp"] - 4), "esp")
	check(
		"callee_saved",
		lambda value: value["registers"].__setitem__("esi", 0),
		"registers",
	)
	check(
		"direction_flag",
		lambda value: value.__setitem__("direction_flag", True),
		"direction_flag",
	)
	check(
		"weapon_mutation",
		lambda value: value["state_sha256"].__setitem__("weapon", "0" * 64),
		"state_sha256",
	)
	check(
		"rounds_out_mutation",
		lambda value: value["state_sha256"].__setitem__("rounds_out", "f" * 64),
		"state_sha256",
	)
	check(
		"delete_trace",
		lambda value: value["observed"].__setitem__("deletes", []),
		"observed",
	)
	return {"pass": all(controls.values()), "controls": controls}


def verify_hash(path: Path, expected: str, label: str) -> dict:
	actual = sha256_path(path)
	if actual.lower() != expected.lower():
		raise AssertionError((label + " SHA-256 mismatch", str(path), actual, expected))
	return {"path": str(path), "sha256": actual, "size": path.stat().st_size}


def resolve_path(path: Path) -> Path:
	return path if path.is_absolute() else (Path.cwd() / path).resolve()


def main() -> int:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--first", type=Path, default=FIRST_DEFAULT)
	parser.add_argument("--actual", type=Path, default=ACTUAL_DEFAULT)
	parser.add_argument("--source", type=Path, default=SOURCE_DEFAULT)
	parser.add_argument("--schema", type=Path, default=SCHEMA_DEFAULT)
	parser.add_argument(
		"--equipment-header", type=Path, default=EQUIPMENT_HEADER_DEFAULT
	)
	parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
	for label in DEFAULT_HASHES:
		parser.add_argument(
			"--%s-sha256" % label.replace("_", "-"),
			dest=label + "_sha256",
			default=DEFAULT_HASHES[label],
		)
	args = parser.parse_args()
	paths = {
		"target": resolve_path(args.target),
		"first": resolve_path(args.first),
		"actual": resolve_path(args.actual),
		"source": resolve_path(args.source),
		"schema": resolve_path(args.schema),
		"equipment_header": resolve_path(args.equipment_header),
	}
	evidence = {
		label: verify_hash(paths[label], getattr(args, label + "_sha256"), label)
		for label in paths
	}

	cases = all_cases()
	target_result, reference = run_object(paths["target"], cases)
	objects = {"target": target_result}
	for role in ("first", "actual"):
		objects[role], unused = run_object(paths[role], cases, reference)
	negative_controls = run_negative_controls(reference)
	passed = all(result["pass"] for result in objects.values()) and negative_controls["pass"]

	report = {
		"pass": passed,
		"scope": [FUNCTION["symbol"]],
		"case_count": len(cases),
		"script_sha256": sha256_path(Path(__file__)),
		"evidence": evidence,
		"section_contract": FUNCTION,
		"objects": objects,
		"negative_controls": negative_controls,
		"case_expectations": {
			case["name"]: case["expected"] for case in cases
		},
		"target_reference_traces": reference,
		"coverage": {
			"same_weapon": "capped, partial, empty, full, source depletion, pickup sound local/NONE, one/two magazines, last-write behavior, and repeated deletion",
			"equipment": "first/second match, nonmatch, zero then positive, capped/full, local/NONE, one/two magazines, repeated deletion",
			"boundaries": "0/1/2 magazine counts; rounds 0 and 32767; local-player short -32768, -1, and 32767; salted object handles and definition indices",
			"fatal": "weapon and source-weapon magazine-bound assertions under a modeled tag-definition change, ending at display_assert then system_exit(-1)",
			"state": "complete seeded weapon/item/source-weapon/definition/magazine-definition/ammunition-object/rounds-out regions; exact target-relative hashes plus explicit rounds and mutation-event summaries",
			"abi": "AL boolean result, cdecl ESP and caller frame, DF, EBP/EBX/ESI/EDI, external call order, stack depth, and arguments",
			"schema": "weapon_ammunition_object is 0x1C bytes: short rounds at +0x00, word unused[5] through +0x0B, tag_reference object at +0x0C, object.index at +0x18",
		},
		"target_behavior_disclosures": [
			"The January target deletes a depleted same-weapon source item inside the magazine loop and then can request/access it again for a later magazine.",
			"The January target can call object_delete more than once for one item when multiple magazines independently consume/deplete or match it.",
			"rounds_picked_up is untouched when every target magazine is full or the definition has zero magazines; otherwise each non-full magazine overwrites it, so the last eligible magazine wins even when it contributes zero rounds.",
			"The harness leaves deleted fake objects mapped solely to expose and compare the target's subsequent behavior; this is not a claim that such access is safe in the game runtime.",
		],
		"limits": [
			"Only the real COFF function body executes. Public callees are deterministic cdecl recording stubs with argument/domain validation and volatile-register poisoning.",
			"For same-weapon cases, both valid masks on ITEM_HANDLE resolve to the same ITEM_WEAPON address; for equipment cases only the item mask resolves. The separate seeded ITEM region remains an intentional guard region and is unused in same-weapon execution.",
			"Magazine-definition tag elements are contiguous at their authentic 0x70-byte element size; tag-block returns are base + index * 0x70 with no artificial gaps.",
			"The two fatal cases require tag_get to return a changed definition between calls. They exercise authentic target assertion paths but do not claim ordinary tags mutate during this function.",
			"Identical runtime traces do not replace source ownership, whole-object COFF, COMMON/data/BSS, caller-link, or full-build regression gates.",
			"Compiler-local branch labels are internal instruction offsets, not external runtime owners, and are intentionally absent from the stub surface.",
		],
	}

	output = resolve_path(args.output)
	if output in paths.values():
		raise ValueError("output must not overwrite a pinned input")
	output.parent.mkdir(parents=True, exist_ok=True)
	output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
	print(json.dumps({
		"pass": passed,
		"case_count": len(cases),
		"output": str(output),
		"output_sha256": sha256_path(output),
		"script_sha256": report["script_sha256"],
	}, indent=2, sort_keys=True))
	return 0 if passed else 1


if __name__ == "__main__":
	raise SystemExit(main())
