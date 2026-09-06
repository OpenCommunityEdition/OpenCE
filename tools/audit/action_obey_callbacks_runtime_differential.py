"""Target-first real-COFF oracle for three small ActionObey callbacks.

The frozen January target is the behavioral authority.  Before executing any
code, this program checks the complete target-object hash, the target callback
section facts, and the three retained HCEA donor-source hashes.  It then runs
the real i386 COFF bytes for:

* action_obey_individual_setup (January ``_code_000051e0``),
* action_obey_individual_flush_command_indices (``_code_00005250``), and
* action_obey_individual_advance (``_code_000052e0``).

All callbacks use their real six-argument cdecl callback ABI.  External calls
execute through behavior-recording cdecl stubs, including the nested argument
stack used by January's tag-block lookup.  Every ordinary-return case checks
the complete modeled 0x24 simple state, complete modeled 0x58 complex state,
caller argument frame, stack balance, direction flag, and EBX/ESI/EDI/EBP.

The sole failure path modeled is setup's authentic null-targeting-reference
assertion.  ``display_assert`` returns and ``system_exit(-1)`` is treated as
noreturn; nonvolatile-return guarantees are therefore intentionally not
claimed on that terminal path.  Flush only uses a valid modeled tag block
(eight 0x60 command-list records, indices 0/1/7, nonnegative command counts).
Actor and unit handles are opaque unused callback arguments in this scope.

The default target hash includes the nine audited semantic private-symbol
renames from the 2026-09-05 partial-graph integration. The three guarded
callback sections and ordered relocations are unchanged from the earlier
stripped-name split; either verified symbol spelling is accepted.

Begin/end are deliberately excluded.  Begin adds unit object access; end may
call the private command_end body.  This oracle neither invents nor stubs a
private command_end ABI.

Examples::

    python tools/audit/action_obey_callbacks_runtime_differential.py
    python tools/audit/action_obey_callbacks_runtime_differential.py \
        --actual path/to/action_obey.obj --isolated path/to/isolated.obj

JSON is written to stdout only.  No object, source, build, or report file is
created or modified.
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


TARGET_OBJECT_SHA256 = (
	"13a4e47de2c79a3122033539284d148cae052f5f008f084a3d15862dda844368"
)
TARGET_DEFAULT = ROOT / "build/split/source/ai/action_obey.obj"
EVIDENCE_DEFAULT = (
	ROOT.parents[1]
	/ "research-cache/halocea-full-blobs-20260830/src/blam/ai"
)

DONOR_HASHES = {
	"action_obey_individual_setup.c": (
		"ec424295a1eae4fe5a98263d1bc8b6a23970bd71db973b8a36d9a8d4f55f6f4e"
	),
	"action_obey_individual_flush_command_indices.c": (
		"f0cd9232cc0d39e036ea7fd32be3692a8d60d6e9658530b2063b9796bc70f4b9"
	),
	"action_obey_individual_advance.c": (
		"83395615f36002f2dd8eaa7128b2d6df079c929ad7373421d649f368296c42d1"
	),
}

CALLBACKS = {
	"setup": {
		"target_symbol": "_code_000051e0",
		"source_symbol": "_action_obey_individual_setup",
		"size": 112,
		"relocation_count": 6,
		"normalized_sha256": (
			"3555918b2bbf054bbb749ed4771adaf13bfecb226a83e9495452a92c5c58a339"
		),
	},
	"flush_command_indices": {
		"target_symbol": "_code_00005250",
		"source_symbol": "_action_obey_individual_flush_command_indices",
		"size": 64,
		"relocation_count": 2,
		"normalized_sha256": (
			"7eabfe65fde59e35c5945bc5f1df7be33ab660a3c4656229c1785faf62bb8920"
		),
	},
	"advance": {
		"target_symbol": "_code_000052e0",
		"source_symbol": "_action_obey_individual_advance",
		"size": 32,
		"relocation_count": 0,
		"normalized_sha256": (
			"55b13ab11cc6a46bb8e86d22c5200de5c7d56e2240c45ed1d52b2bf61b631a74"
		),
	},
}

EXPECTED_TARGET_RELOCATIONS = {
	"setup": [
		(14, cc.IMAGE_REL_I386_REL32, "api:_csmemset"),
		(39, cc.IMAGE_REL_I386_DIR32, "literal:c:\\halo\\SOURCE\\ai\\action_obey.c"),
		(44, cc.IMAGE_REL_I386_DIR32, "literal:targeting_reference"),
		(49, cc.IMAGE_REL_I386_REL32, "api:_display_assert"),
		(56, cc.IMAGE_REL_I386_REL32, "api:_system_exit"),
		(93, cc.IMAGE_REL_I386_REL32, "api:_csmemset"),
	],
	"flush_command_indices": [
		(12, cc.IMAGE_REL_I386_REL32, "api:_global_scenario_get"),
		(23, cc.IMAGE_REL_I386_REL32, "api:_tag_block_get_element_with_size"),
	],
	"advance": [],
}

CODE = 0x100000
STOP = 0x10F000
STUBS = 0x200000
STRINGS = 0x300000
ASSERT_FILE = STRINGS + 0x100
ASSERT_EXPRESSION = STRINGS + 0x200
SCENARIO = 0x400000
AI_COMMAND_LISTS_OFFSET = 0x438
COMMAND_LISTS = 0x410000
COMMAND_LIST_COUNT = 8
COMMAND_LIST_SIZE = 0x60
COMMANDS_BLOCK_COUNT_OFFSET = 0x30
SIMPLE = 0x500000
SIMPLE_SIZE = 0x24
COMPLEX = 0x510000
COMPLEX_SIZE = 0x58
USER_DATA = 0x520000
STACK_PAGE = 0x600000
ENTRY_SP = STACK_PAGE + 0x8000

STUB_NAMES = (
	"_csmemset",
	"_display_assert",
	"_system_exit",
	"_global_scenario_get",
	"_tag_block_get_element_with_size",
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


def p32(value):
	return struct.pack("<I", value & 0xFFFFFFFF)


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


def symbol_cstring(obj, name):
	symbol = cc.symbol(obj, name)
	section = obj["sections"][symbol["section"] - 1]
	raw = cc._section_bytes(obj, section)
	start = symbol["value"]
	end = raw.find(b"\0", start)
	if end < 0:
		raise AssertionError(("unterminated literal", name))
	return raw[start:end].decode("ascii")


def relocation_identity(obj, relocation):
	target = relocation.get("symbolic_target", relocation["target"])
	if not isinstance(target, (list, tuple)) or target[0] != "symbol":
		raise AssertionError(("unresolved relocation", target))
	name, addend = target[1:]
	if name in STUB_ADDRESSES:
		return "api:" + name, STUB_ADDRESSES[name] + addend, None
	literal = symbol_cstring(obj, name)
	if literal.lower() == "c:\\halo\\source\\ai\\action_obey.c":
		return "literal:" + literal, ASSERT_FILE + addend, (ASSERT_FILE, literal)
	if literal == "targeting_reference":
		return (
			"literal:" + literal,
			ASSERT_EXPRESSION + addend,
			(ASSERT_EXPRESSION, literal),
		)
	raise AssertionError(("unknown relocation target", name, literal, addend))


def resolve_function(obj, callback_name, role):
	spec = CALLBACKS[callback_name]
	available = function_sections(obj)
	aliases = [spec["target_symbol"], spec["source_symbol"]]
	matches = [name for name in aliases if name in available]
	if len(matches) != 1:
		raise AssertionError((callback_name, role, "function aliases", matches))
	return available[matches[0]]


def prepare_function(obj, callback_name, role):
	function = resolve_function(obj, callback_name, role)
	section = obj["sections"][function.section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))
	strings = {}
	relocation_fingerprint = []
	for relocation in function.info["relocations"]:
		identity, address, literal = relocation_identity(obj, relocation)
		relocation_fingerprint.append(
			(relocation["address"], relocation["type"], identity)
		)
		if literal is not None:
			literal_address, literal_text = literal
			previous = strings.setdefault(literal_address, literal_text)
			if previous != literal_text:
				raise AssertionError(("literal address collision", previous, literal_text))
		if relocation["type"] == cc.IMAGE_REL_I386_REL32:
			address -= CODE + relocation["address"] + 4
		elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
			raise AssertionError(("unsupported relocation type", relocation["type"]))
		raw[relocation["address"]:relocation["address"] + 4] = p32(address)
	return {
		"callback": callback_name,
		"symbol": function.name,
		"info": function.info,
		"raw": bytes(raw),
		"strings": strings,
		"relocation_fingerprint": relocation_fingerprint,
	}


def verify_target_function(prepared):
	callback_name = prepared["callback"]
	spec = CALLBACKS[callback_name]
	info = prepared["info"]
	checks = {
		"symbol": prepared["symbol"] in (spec["target_symbol"], spec["source_symbol"]),
		"size": info["size"] == spec["size"],
		"relocation_count": (
			info["relocation_count"] == spec["relocation_count"]
		),
		"normalized_sha256": (
			info["normalized_sha256"] == spec["normalized_sha256"]
		),
		"relocations": (
			prepared["relocation_fingerprint"]
			== EXPECTED_TARGET_RELOCATIONS[callback_name]
		),
	}
	failed = [name for name, passed in checks.items() if not passed]
	if failed:
		raise AssertionError((callback_name, "target section guard", failed))
	return checks


def setup_cases():
	cases = []
	for complex_present in (False, True):
		for targeting_reference in (0, 1, 0x80, 0xFF):
			cases.append({
				"name": "complex_%d_target_%02x" % (
					complex_present,
					targeting_reference,
				),
				"seed": len(cases) + 1,
				"actor_index": 0x12340007,
				"unit_index": 0x23450009,
				"command_list_index": 0,
				"complex_present": complex_present,
				"targeting_reference": targeting_reference,
			})
	cases.append({
		"name": "null_targeting_reference_assert",
		"seed": len(cases) + 1,
		"actor_index": 0x12340007,
		"unit_index": 0x23450009,
		"command_list_index": 0,
		"complex_present": True,
		"targeting_reference": None,
	})
	return cases


def flush_cases():
	cases = []
	for command_count in (0, 1, 2, 127, 128, 255, 256):
		indices = {
			0,
			1,
			127,
			128,
			254,
			255,
			max(0, min(255, command_count - 1)),
			min(255, command_count),
		}
		for current_command_index in sorted(indices):
			case_number = len(cases)
			cases.append({
				"name": "count_%03d_current_%03d" % (
					command_count,
					current_command_index,
				),
				"seed": case_number + 31,
				"actor_index": 0x3456000B,
				"unit_index": 0x4567000D,
				"command_list_index": (0, 1, 7)[case_number % 3],
				"current_command_index": current_command_index,
				"command_count": command_count,
			})
	return cases


def advance_cases():
	return [
		{
			"name": "metadata_%02x" % metadata_flags,
			"seed": metadata_flags + 101,
			"actor_index": (0x5678000F + metadata_flags) & 0xFFFFFFFF,
			"unit_index": (0x67890011 + metadata_flags) & 0xFFFFFFFF,
			"command_list_index": (0, 1, 7)[metadata_flags % 3],
			"metadata_flags": metadata_flags,
		}
		for metadata_flags in range(256)
	]


CASE_FACTORIES = {
	"setup": setup_cases,
	"flush_command_indices": flush_cases,
	"advance": advance_cases,
}


def pointer_label(pointer):
	if pointer == SIMPLE:
		return "simple"
	if pointer == COMPLEX:
		return "complex"
	if pointer == USER_DATA:
		return "user_data"
	if pointer == SCENARIO + AI_COMMAND_LISTS_OFFSET:
		return "scenario.ai_command_lists"
	return "0x%08x" % pointer


def machine_argument(machine, index):
	return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def read_cstring(machine, address, limit=256):
	data = bytearray()
	for offset in range(limit):
		value = machine.mem_read(address + offset, 1)[0]
		if value == 0:
			return data.decode("ascii")
		data.append(value)
	raise AssertionError(("runtime string is unterminated", address))


def initial_state(callback_name, case):
	simple = bytearray(seeded_bytes(SIMPLE_SIZE, case["seed"]))
	complex_control = bytearray(seeded_bytes(COMPLEX_SIZE, case["seed"] + 1))
	user_data = bytearray(seeded_bytes(0x20, case["seed"] + 2))
	command_lists = bytearray(
		seeded_bytes(COMMAND_LIST_COUNT * COMMAND_LIST_SIZE, case["seed"] + 3)
	)
	if callback_name == "setup" and case["targeting_reference"] is not None:
		user_data[0] = case["targeting_reference"]
	elif callback_name == "flush_command_indices":
		simple[0] = case["current_command_index"]
		list_offset = case["command_list_index"] * COMMAND_LIST_SIZE
		command_lists[
			list_offset + COMMANDS_BLOCK_COUNT_OFFSET:
			list_offset + COMMANDS_BLOCK_COUNT_OFFSET + 4
		] = p32(case["command_count"])
	elif callback_name == "advance":
		simple[4] = case["metadata_flags"]
	return {
		"simple": bytes(simple),
		"complex": bytes(complex_control),
		"user_data": bytes(user_data),
		"command_lists": bytes(command_lists),
	}


def callback_frame(case, callback_name):
	complex_pointer = COMPLEX
	user_pointer = USER_DATA
	if callback_name == "setup":
		if not case["complex_present"]:
			complex_pointer = 0
		if case["targeting_reference"] is None:
			user_pointer = 0
	arguments = (
		case["actor_index"],
		case["unit_index"],
		case["command_list_index"] & 0xFFFF,
		SIMPLE,
		complex_pointer,
		user_pointer,
	)
	return p32(STOP) + b"".join(p32(argument) for argument in arguments)


def expected_state(callback_name, case, original):
	expected = {name: bytearray(value) for name, value in original.items()}
	if callback_name == "setup":
		expected["simple"][:] = b"\0" * SIMPLE_SIZE
		expected["simple"][0] = 0xFF
		if case["targeting_reference"] is not None:
			if case["targeting_reference"]:
				expected["simple"][4] |= 1
			else:
				expected["simple"][4] &= 0xFE
			if case["complex_present"]:
				expected["complex"][:] = b"\0" * COMPLEX_SIZE
				expected["complex"][2:4] = b"\xff\xff"
	elif callback_name == "flush_command_indices":
		if case["current_command_index"] >= case["command_count"]:
			expected["simple"][0] = 0xFF
	elif callback_name == "advance":
		expected["simple"][4] = (case["metadata_flags"] & 0xEF) | 0x08
	return {name: bytes(value) for name, value in expected.items()}


def expected_events(callback_name, case):
	if callback_name == "setup":
		events = [{
			"api": "_csmemset",
			"stack_depth": 28,
			"args": ["simple", 0, SIMPLE_SIZE],
		}]
		if case["targeting_reference"] is None:
			events.extend([
				{
					"api": "_display_assert",
					"stack_depth": 32,
					"args": [
						"targeting_reference",
						"c:\\halo\\SOURCE\\ai\\action_obey.c",
						1360,
						1,
					],
				},
				{
					"api": "_system_exit",
					"stack_depth": 36,
					"args": [-1],
					"retained_assert_args": [
						"targeting_reference",
						"c:\\halo\\SOURCE\\ai\\action_obey.c",
						1360,
						1,
					],
				},
			])
		elif case["complex_present"]:
			events.append({
				"api": "_csmemset",
				"stack_depth": 28,
				"args": ["complex", 0, COMPLEX_SIZE],
			})
		return events
	if callback_name == "flush_command_indices":
		return [
			{
				"api": "_global_scenario_get",
				"stack_depth": 20,
				"args": [],
			},
			{
				"api": "_tag_block_get_element_with_size",
				"stack_depth": 24,
				"args": [
					"scenario.ai_command_lists",
					case["command_list_index"],
					COMMAND_LIST_SIZE,
				],
			},
		]
	return []


def execute_case(prepared, callback_name, case):
	machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
	for address, size in (
		(CODE, 0x10000),
		(STUBS, 0x10000),
		(STRINGS, 0x10000),
		(SCENARIO, 0x1000),
		(COMMAND_LISTS, 0x1000),
		(SIMPLE, 0x1000),
		(COMPLEX, 0x1000),
		(USER_DATA, 0x1000),
		(STACK_PAGE, 0x10000),
	):
		machine.mem_map(address, size)
	machine.mem_write(CODE, prepared["raw"])
	for address in STUB_ADDRESSES.values():
		machine.mem_write(address, b"\xc3")
	for address, literal in prepared["strings"].items():
		machine.mem_write(address, literal.encode("ascii") + b"\0")

	original = initial_state(callback_name, case)
	machine.mem_write(SIMPLE, original["simple"])
	machine.mem_write(COMPLEX, original["complex"])
	machine.mem_write(USER_DATA, original["user_data"])
	machine.mem_write(COMMAND_LISTS, original["command_lists"])
	machine.mem_write(
		SCENARIO + AI_COMMAND_LISTS_OFFSET,
		p32(COMMAND_LIST_COUNT) + p32(COMMAND_LISTS) + p32(0),
	)
	stack_seed = seeded_bytes(0x800, case["seed"] + 4)
	machine.mem_write(ENTRY_SP - 0x400, stack_seed)
	frame = callback_frame(case, callback_name)
	machine.mem_write(ENTRY_SP, frame)
	machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
	machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
	for register, value in SAVED_REGISTERS.items():
		machine.reg_write(register, value)

	events = []
	terminal = {"kind": None}
	hook_errors = []

	def record(api, arguments, extra=None):
		event = {
			"api": api,
			"stack_depth": ENTRY_SP - machine.reg_read(UC_X86_REG_ESP),
			"args": arguments,
		}
		if extra:
			event.update(extra)
		events.append(event)

	def poison_volatile(eax=None):
		if eax is not None:
			machine.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
		machine.reg_write(UC_X86_REG_ECX, 0xC1C2C3C4)
		machine.reg_write(UC_X86_REG_EDX, 0xD1D2D3D4)

	def execute_stub(unused_machine, address, unused_size, unused_data):
		name = next(
			(candidate for candidate, stub in STUB_ADDRESSES.items() if stub == address),
			None,
		)
		if name is None:
			hook_errors.append("execution entered unknown stub address 0x%08x" % address)
			machine.emu_stop()
			return
		try:
			if name == "_csmemset":
				destination = machine_argument(machine, 0)
				value = machine_argument(machine, 1)
				count = machine_argument(machine, 2)
				record(name, [pointer_label(destination), value, count])
				if destination not in (SIMPLE, COMPLEX):
					raise AssertionError(("unexpected memset destination", destination))
				machine.mem_write(destination, bytes([value & 0xFF]) * count)
				poison_volatile(destination)
			elif name == "_display_assert":
				expression = read_cstring(machine, machine_argument(machine, 0))
				filename = read_cstring(machine, machine_argument(machine, 1))
				record(name, [
					expression,
					filename,
					machine_argument(machine, 2),
					machine_argument(machine, 3),
				])
				poison_volatile(0xA1A2A3A4)
			elif name == "_system_exit":
				stack = machine.reg_read(UC_X86_REG_ESP)
				retained = [
					read_cstring(machine, read32(machine, stack + 8)),
					read_cstring(machine, read32(machine, stack + 12)),
					read32(machine, stack + 16),
					read32(machine, stack + 20),
				]
				record(
					name,
					[signed32(machine_argument(machine, 0))],
					{"retained_assert_args": retained},
				)
				terminal["kind"] = "system_exit"
				machine.emu_stop()
			elif name == "_global_scenario_get":
				record(name, [])
				poison_volatile(SCENARIO)
			elif name == "_tag_block_get_element_with_size":
				block = machine_argument(machine, 0)
				index = signed32(machine_argument(machine, 1))
				element_size = machine_argument(machine, 2)
				record(name, [pointer_label(block), index, element_size])
				block_count = read32(machine, block)
				block_address = read32(machine, block + 4)
				if not 0 <= index < block_count:
					raise AssertionError(("invalid modeled tag index", index, block_count))
				if element_size != COMMAND_LIST_SIZE:
					raise AssertionError(("wrong command-list element size", element_size))
				poison_volatile(block_address + index * element_size)
		except (AssertionError, u.UcError) as error:
			hook_errors.append(repr(error))
			machine.emu_stop()

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

	if terminal["kind"] is None and not execution_error and not hook_errors:
		terminal["kind"] = "return" if machine.reg_read(UC_X86_REG_EIP) == STOP else "other"

	state = {
		"simple": bytes(machine.mem_read(SIMPLE, SIMPLE_SIZE)),
		"complex": bytes(machine.mem_read(COMPLEX, COMPLEX_SIZE)),
		"user_data": bytes(machine.mem_read(USER_DATA, 0x20)),
		"command_lists": bytes(machine.mem_read(
			COMMAND_LISTS,
			COMMAND_LIST_COUNT * COMMAND_LIST_SIZE,
		)),
	}
	registers = {
		SAVED_REGISTER_NAMES[register]: machine.reg_read(register)
		for register in SAVED_REGISTERS
	}
	return {
		"terminal": terminal["kind"],
		"execution_error": execution_error,
		"hook_errors": hook_errors,
		"events": events,
		"state": state,
		"eip": machine.reg_read(UC_X86_REG_EIP),
		"esp": machine.reg_read(UC_X86_REG_ESP),
		"eflags": machine.reg_read(UC_X86_REG_EFLAGS),
		"registers": registers,
		"caller_frame": bytes(machine.mem_read(ENTRY_SP, len(frame))),
		"expected_caller_frame": frame,
		"original": original,
	}


def validate_outcome(callback_name, case, outcome):
	errors = list(outcome["hook_errors"])
	if outcome["execution_error"]:
		errors.append("unicorn: " + outcome["execution_error"])
	expected = expected_state(callback_name, case, outcome["original"])
	for state_name, expected_bytes in expected.items():
		actual_bytes = outcome["state"][state_name]
		if actual_bytes != expected_bytes:
			errors.append(
				"%s differs at %s" % (
					state_name,
					differing_ranges(actual_bytes, expected_bytes),
				)
			)
	if outcome["events"] != expected_events(callback_name, case):
		errors.append("external-call trace differs")
	expected_terminal = (
		"system_exit"
		if callback_name == "setup" and case["targeting_reference"] is None
		else "return"
	)
	if outcome["terminal"] != expected_terminal:
		errors.append(
			"terminal %r != %r" % (outcome["terminal"], expected_terminal)
		)
	if outcome["caller_frame"] != outcome["expected_caller_frame"]:
		errors.append("caller return/argument frame was modified")
	if expected_terminal == "return":
		if outcome["eip"] != STOP:
			errors.append("ordinary return did not reach STOP")
		if outcome["esp"] != ENTRY_SP + 4:
			errors.append(
				"cdecl stack imbalance: 0x%08x != 0x%08x" % (
					outcome["esp"],
					ENTRY_SP + 4,
				)
			)
		if outcome["eflags"] & 0x400:
			errors.append("direction flag left set")
		for register, expected_value in SAVED_REGISTERS.items():
			name = SAVED_REGISTER_NAMES[register]
			if outcome["registers"][name] != expected_value:
				errors.append("nonvolatile register %s was not preserved" % name)
	return errors


def canonical_outcome(outcome):
	return {
		"terminal": outcome["terminal"],
		"events": outcome["events"],
		"state": {
			name: value.hex()
			for name, value in sorted(outcome["state"].items())
		},
		"esp": outcome["esp"],
		"direction_flag": bool(outcome["eflags"] & 0x400),
		"registers": outcome["registers"] if outcome["terminal"] == "return" else None,
		"caller_frame": outcome["caller_frame"].hex(),
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


def run_object(path, role, callbacks, target_reference=None):
	obj = cc.load(path)
	result = {
		"path": str(path),
		"sha256": sha256_path(path),
		"callbacks": {},
		"pass": True,
	}
	reference = {}
	for callback_name in callbacks:
		prepared = prepare_function(obj, callback_name, role)
		cases = CASE_FACTORIES[callback_name]()
		case_traces = {}
		semantic_failures = []
		target_mismatches = []
		for case in cases:
			outcome = execute_case(prepared, callback_name, case)
			errors = validate_outcome(callback_name, case, outcome)
			canonical = canonical_outcome(outcome)
			case_traces[case["name"]] = canonical
			if errors:
				semantic_failures.append({"case": case["name"], "errors": errors})
			if target_reference is not None:
				expected = target_reference[callback_name][case["name"]]
				if canonical != expected:
					target_mismatches.append({
						"case": case["name"],
						**concise_difference(canonical, expected),
					})
		trace_payload = json.dumps(
			case_traces,
			sort_keys=True,
			separators=(",", ":"),
		).encode("utf-8")
		callback_pass = not semantic_failures and not target_mismatches
		result["callbacks"][callback_name] = {
			"symbol": prepared["symbol"],
			"size": prepared["info"]["size"],
			"relocation_count": prepared["info"]["relocation_count"],
			"normalized_sha256": prepared["info"]["normalized_sha256"],
			"relocation_fingerprint": prepared["relocation_fingerprint"],
			"case_count": len(cases),
			"trace_sha256": hashlib.sha256(trace_payload).hexdigest(),
			"semantic_failure_count": len(semantic_failures),
			"target_mismatch_count": len(target_mismatches),
			"first_semantic_failures": semantic_failures[:5],
			"first_target_mismatches": target_mismatches[:5],
			"pass": callback_pass,
		}
		result["pass"] = result["pass"] and callback_pass
		reference[callback_name] = case_traces
	return result, reference


def verify_evidence(target_path, expected_target_hash, evidence_root):
	target_hash = sha256_path(target_path)
	if target_hash.lower() != expected_target_hash.lower():
		raise AssertionError((
		"frozen target object hash mismatch",
		str(target_path),
		target_hash,
		expected_target_hash,
	))
	donor_results = {}
	for filename, expected_hash in DONOR_HASHES.items():
		path = evidence_root / filename
		actual_hash = sha256_path(path)
		if actual_hash.lower() != expected_hash:
			raise AssertionError((
				"frozen donor hash mismatch",
				str(path),
				actual_hash,
				expected_hash,
			))
		donor_results[filename] = {
			"path": str(path),
			"sha256": actual_hash,
		}
	return {"target_sha256": target_hash, "donors": donor_results}


def resolve_path(path):
	return path if path.is_absolute() else (Path.cwd() / path).resolve()


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--target", type=Path, default=TARGET_DEFAULT)
	parser.add_argument("--actual", type=Path)
	parser.add_argument("--isolated", type=Path)
	parser.add_argument(
		"--target-sha256",
		default=TARGET_OBJECT_SHA256,
		help="required frozen whole-object SHA-256 (defaults to January)",
	)
	parser.add_argument("--evidence-root", type=Path, default=EVIDENCE_DEFAULT)
	parser.add_argument(
		"--callback",
		action="append",
		choices=tuple(CALLBACKS),
		dest="callbacks",
		help="limit execution; repeat for more than one (default: all three)",
	)
	args = parser.parse_args()
	callbacks = args.callbacks or list(CALLBACKS)
	target_path = resolve_path(args.target)
	evidence_root = resolve_path(args.evidence_root)
	evidence = verify_evidence(
		target_path,
		args.target_sha256,
		evidence_root,
	)

	target_obj = cc.load(target_path)
	target_section_guards = {}
	for callback_name in callbacks:
		prepared = prepare_function(target_obj, callback_name, "target")
		target_section_guards[callback_name] = verify_target_function(prepared)

	target_result, reference = run_object(target_path, "target", callbacks)
	objects = {"target": target_result}
	for role in ("actual", "isolated"):
		path = getattr(args, role)
		if path is not None:
			resolved = resolve_path(path)
			objects[role], unused_reference = run_object(
				resolved,
				role,
				callbacks,
				reference,
			)

	passed = all(result["pass"] for result in objects.values())
	report = {
		"pass": passed,
		"scope": [
			"action_obey_individual_setup",
			"action_obey_individual_flush_command_indices",
			"action_obey_individual_advance",
		],
		"callbacks_executed": callbacks,
		"script_sha256": sha256_path(Path(__file__)),
		"evidence": evidence,
		"target_section_guards": target_section_guards,
		"objects": objects,
		"limits": [
			"Six-argument i386 cdecl callback ABI; callback return type is void.",
			"All ordinary cases require exact full 0x24/0x58 state, external-call arguments and target stack depths, caller-frame preservation, ESP+4 return, clear DF, and preserved EBP/EBX/ESI/EDI.",
			"The only failure case is setup's target-authentic null targeting_reference assertion ending at noreturn system_exit(-1).",
			"Flush tag lookups are restricted to a modeled valid block of eight 0x60 records, indices 0/1/7, and nonnegative command counts 0..256.",
			"Actor/unit handles are opaque 32-bit values and are not dereferenced by these three callbacks; no datum-generation or object-type claim is made.",
			"Begin/end and the private command_end ABI are outside scope.",
		],
	}
	print(json.dumps(report, indent=2))
	return 0 if passed else 1


if __name__ == "__main__":
	raise SystemExit(main())
