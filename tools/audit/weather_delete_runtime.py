"""Pinned actual-x86 replay of Weather delete and three genuine private helpers.

This finite oracle executes each role's own public/private COFF bodies. Only the
real tag_get, datum_get, datum_delete, display_assert and system_exit boundaries
are modeled. It is not a full tag system, allocator, final-link or engine proof.
The stable tag view and supplied valid handle map are explicit fixture contracts.
datum_delete poisons the freed record, exposing a late next-pointer read.

Objects are read-only. A manifest pins January, baseline, first and ordinary
outputs. A target-only preparatory mode requires January and baseline. Candidate
private ABIs are accepted only after complete byte/ordered-relocation equality.
Any nonexact body fails closed and needs a separately reviewed extension of this
oracle; the current admitted packet does not require an ABI override mechanism.
Negative controls mutate host copies of actual January instructions only.
"""
from __future__ import annotations

if not __debug__:
    raise RuntimeError("Weather oracle cannot run with Python optimization: assertions are required")

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

import unicorn as u
from unicorn import x86_const as x

ROOT = next(p for p in Path(__file__).resolve().parents if (p / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

TARGET_PIN = "3ebaff7040870aff906a53c36fd84c8dccaa33b6c4be1b15101b42e1b1354c89"
BASELINE_PIN = "41a0975a157a4a4c1894fa90ba1fd622638dd4ec048c3bf9978aa439bc6d0ece"
KEYS = ("delete", "get", "get_type", "delete_particle")
NAMES = {
    "delete": ("_weather_particle_system_delete",),
    "get": ("_code_000924d0", "_weather_particle_system_get"),
    "get_type": ("_code_00092510", "_weather_particle_system_get_type"),
    "delete_particle": ("_code_00092640", "_weather_particle_system_type_delete_particle"),
}
GLOBAL_NAMES = ("_bss_0043d590", "_weather_particle_system_globals")
RENAMES = {names[0]: names[-1] for names in NAMES.values()}
RENAMES[GLOBAL_NAMES[0]] = GLOBAL_NAMES[1]
TARGET_PACKETS = {
    "delete": (272, 16, "21b3a12284f9603d5f4e76018be68af7de7989de3aae36614d0bf40f718a503b"),
    "get": (64, 5, "c950314e89e576896257b76f570e2c86407ffa644a731baed92ce799156e2e11"),
    "get_type": (80, 5, "3de80b33190fc00e0e0559f77fd73b49fba878d3250a69a8f54f15419f58bb52"),
    "delete_particle": (48, 4, "ad6524655fa3783ff857dc30bf46fec9d0da3f1432d0ca2b9117b7608202d85a"),
}
BASELINE_FUNCTIONS = (
    "_weather_particle_systems_initialize", "_weather_particle_systems_initialize_for_new_map",
    "_weather_particle_systems_dispose_from_old_map", "_weather_particle_systems_dispose",
)
PRESERVED = ("ebp", "ebx", "esi", "edi")
REGISTERS = {name: getattr(x, "UC_X86_REG_" + name.upper()) for name in
             ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")}
TARGET_ABI = {
    "delete": {"registers": {}, "stack": ["local_index"], "preserved": list(PRESERVED),
               "return": "void", "argument_write": "final_type_index"},
    "get": {"registers": {"esi": "local_index"}, "stack": [], "preserved": list(PRESERVED),
            "return": "system", "argument_write": "unchanged"},
    "get_type": {"registers": {"edi": "system", "esi": "type_index"}, "stack": [],
                 "preserved": list(PRESERVED), "return": "type", "argument_write": "unchanged"},
    "delete_particle": {"registers": {"esi": "type"}, "stack": [],
                        "preserved": list(PRESERVED), "return": "void", "argument_write": "unchanged"},
}

CODE, STUB, LITERAL = 0x100000, 0x200000, 0x280000
GLOBAL_PAGE, POINTER_PAGE, DATA_PAGE, TAG_PAGE, PARTICLE_PAGE = (
    0x300000, 0x310000, 0x320000, 0x330000, 0x340000)
GLOBALS, DATA_POINTER, DATA_ARRAY, DEFINITION, PARTICLES = (
    GLOBAL_PAGE + 0x100, POINTER_PAGE + 0x100, DATA_PAGE + 0x100,
    TAG_PAGE + 0x100, PARTICLE_PAGE + 0x100)
STACK, STACK_SIZE, SP, STOP = 0x400000, 0x10000, 0x40f000, STUB + 0xf000
GLOBAL_SIZE, SYSTEM_SIZE, TYPE_SIZE, PARTICLE_SIZE = 0x274, 0x9c, 0x10, 0x54
PAGE_SIZE = 0x1000
RAIN = 0x7261696e
NONE = 0xffffffff
APIS = ("_tag_get", "_datum_get", "_datum_delete", "_display_assert", "_system_exit")
STUBS = {name: STUB + index * 0x100 for index, name in enumerate(APIS)}
CODE_ADDRESSES = {key: CODE + index * 0x1000 for index, key in enumerate(KEYS)}
SOURCE_TEXT = b"c:\\halo\\SOURCE\\effects\\weather_particle_systems.c\0"
LOCAL_TEXT = b"local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS\0"
TYPE_TEXT = b"type_index>=0 && type_index<definition->particle_types.count\0"
LITERALS = {
    "??_C@_0DC@LLJNAPDG@c?3?2halo?2SOURCE?2effects?2weather_p@": SOURCE_TEXT,
    "??_C@_0EM@JCKEKCNI@local_player_index?$DO?$DN0?5?$CG?$CG?5local_p@": LOCAL_TEXT,
    "??_C@_0DN@CIHCKGAO@type_index?$DO?$DN0?5?$CG?$CG?5type_index?$DMdefi@": TYPE_TEXT,
}
LITERAL_ADDRESSES = {name: LITERAL + index * 0x200 for index, name in enumerate(LITERALS)}
FPCW_MODES = (0x027f, 0x037f)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def p32(value):
    return struct.pack("<I", value & 0xffffffff)


def p16(value):
    return struct.pack("<H", value & 0xffff)


def get32(raw, at):
    return struct.unpack_from("<I", raw, at)[0]


def get16(raw, at):
    return struct.unpack_from("<H", raw, at)[0]


def signed16(value):
    return (value & 0x7fff) - (value & 0x8000)


def pattern(size, salt):
    return bytearray(((index * 37 + salt) & 255) for index in range(size))


def cases():
    result = []
    for player in range(4):
        for label, chains in (("no_types", []), ("empty", [0]), ("one", [1]),
                              ("mixed", [0, 3, 1]), ("eight", [1, 0, 2, 0, 1, 0, 2, 1])):
            result.append({"name": f"delete_p{player}_{label}", "function": "delete",
                           "local_index": player, "chains": chains})
        result.append({"name": f"get_p{player}", "function": "get", "local_index": player})
    result += [
        {"name": "delete_salted_short_argument", "function": "delete", "local_index": 0xabcd0003,
         "chains": [2, 0, 1]},
        {"name": "get_salted_short_register", "function": "get", "local_index": 0xabcd0002},
        {"name": "delete_active_count_wrap", "function": "delete", "active_count": 0x8000,
         "chains": [1]},
        {"name": "delete_type_count_wrap", "function": "delete", "particle_count": 0x8000,
         "chains": [1]},
        {"name": "delete_negative_tag_count", "function": "delete", "chains": [], "tag_count": -1},
    ]
    for key in ("delete", "get"):
        for index in (-32768, -1, 4, 32767):
            result.append({"name": f"{key}_invalid_{index}", "function": key, "local_index": index})
    for index in range(8):
        result.append({"name": f"get_type_{index}", "function": "get_type", "local_index": index % 4,
                       "type_index": index, "tag_count": 8})
    for index, count in ((-32768, 8), (-1, 8), (0, 0), (3, 3), (32767, 8)):
        result.append({"name": f"get_type_invalid_{index}_{count}", "function": "get_type",
                       "type_index": index, "tag_count": count})
    result.append({"name": "get_type_salted_short", "function": "get_type",
                   "type_index": 0x89ab0007, "tag_count": 8})
    for player in range(4):
        for count in (1, 0x8000):
            result.append({"name": f"delete_head_p{player}_{count}", "function": "delete_particle",
                           "local_index": player, "type_index": 2, "chains": [0, 0, 2],
                           "particle_count": count})
    return [dict({"local_index": 0, "type_index": 0, "chains": [1], "active_count": 4}, **case)
            for case in result]


def prepare(case):
    """Seed complete arenas, including unrelated slots, gaps and record guards."""
    pages = {base: pattern(PAGE_SIZE, salt) for base, salt in
             ((GLOBAL_PAGE, 17), (POINTER_PAGE, 31), (DATA_PAGE, 49), (TAG_PAGE, 71), (PARTICLE_PAGE, 97))}
    globals_raw = pages[GLOBAL_PAGE]
    global_at = GLOBALS - GLOBAL_PAGE
    globals_raw[global_at:global_at + 2] = p16(case["active_count"])
    for player in range(4):
        system = global_at + 4 + player * SYSTEM_SIZE
        globals_raw[system:system + 4] = p32(0xa1230080 + player)
        for index in range(8):
            at = system + 0x1c + index * TYPE_SIZE
            globals_raw[at + 8:at + 10] = p16(0)
            globals_raw[at + 12:at + 16] = p32(NONE)
    player = signed16(case["local_index"])
    selected = player if 0 <= player < 4 else 0
    system_pointer = GLOBALS + 4 + selected * SYSTEM_SIZE
    type_index = signed16(case["type_index"])
    selected_type = type_index if 0 <= type_index < 8 else 0
    records, chains = {}, []
    serial = 0
    for index, length in enumerate(case["chains"]):
        handles = []
        for _ in range(length):
            # Nonzero high words must survive both engine-boundary calls and list links.
            salt = (0x89ab, 0x1234, 0xffff, 0x8000)[serial % 4]
            handle = (salt << 16) | (7 + serial * 3)
            address = PARTICLES + serial * 0x80
            records[handle] = address
            handles.append(handle)
            serial += 1
        chains.append(handles)
        at = system_pointer - GLOBAL_PAGE + 0x1c + index * TYPE_SIZE
        globals_raw[at + 8:at + 10] = p16(case.get("particle_count", length))
        globals_raw[at + 12:at + 16] = p32(handles[0] if handles else NONE)
        for ordinal, handle in enumerate(handles):
            at = records[handle] - PARTICLE_PAGE
            pages[PARTICLE_PAGE][at:at + 2] = p16(handle >> 16)
            pages[PARTICLE_PAGE][at + 0x50:at + 0x54] = p32(
                handles[ordinal + 1] if ordinal + 1 < len(handles) else NONE)
    assert serial <= 24 and len(case["chains"]) <= 8
    pages[POINTER_PAGE][DATA_POINTER - POINTER_PAGE:DATA_POINTER - POINTER_PAGE + 4] = p32(DATA_ARRAY)
    count = case.get("tag_count", len(chains))
    pages[TAG_PAGE][DEFINITION - TAG_PAGE + 0x24:DEFINITION - TAG_PAGE + 0x28] = p32(count)
    return {"pages": {base: bytes(raw) for base, raw in pages.items()}, "records": records,
            "chains": chains, "system": system_pointer,
            "type": system_pointer + 0x1c + selected_type * TYPE_SIZE,
            "definition_index": 0xa1230080 + selected, "tag_count": count}


def assertion_events(line):
    return [{"api": "display_assert", "information": (LOCAL_TEXT if line == 91 else TYPE_TEXT)[:-1].decode(),
             "file": SOURCE_TEXT[:-1].decode(), "line": line, "fatal": 1},
            {"api": "system_exit", "code": NONE}]


def expected(case, prepared):
    pages = {base: bytearray(raw) for base, raw in prepared["pages"].items()}
    events, result = [], None
    key, player = case["function"], signed16(case["local_index"])
    if key in ("delete", "get") and not 0 <= player < 4:
        return pages, assertion_events(91), result, True, None
    if key == "get":
        return pages, events, prepared["system"], False, None

    def tag():
        events.append({"api": "tag_get", "tag": RAIN, "index": prepared["definition_index"], "result": DEFINITION})

    def delete_head(type_pointer):
        at = type_pointer - GLOBAL_PAGE
        handle = get32(pages[GLOBAL_PAGE], at + 12)
        address = prepared["records"][handle]
        next_handle = get32(pages[PARTICLE_PAGE], address - PARTICLE_PAGE + 0x50)
        events.append({"api": "datum_get", "data": DATA_ARRAY, "index": handle, "result": address})
        events.append({"api": "datum_delete", "data": DATA_ARRAY, "index": handle})
        start = address - PARTICLE_PAGE
        pages[PARTICLE_PAGE][start:start + PARTICLE_SIZE] = b"\xd5" * PARTICLE_SIZE
        pages[GLOBAL_PAGE][at + 8:at + 10] = p16(get16(pages[GLOBAL_PAGE], at + 8) - 1)
        pages[GLOBAL_PAGE][at + 12:at + 16] = p32(next_handle)

    if key == "get_type":
        tag()
        index = signed16(case["type_index"])
        if not 0 <= index < prepared["tag_count"]:
            return pages, events + assertion_events(102), None, True, None
        return pages, events, prepared["system"] + 0x1c + index * TYPE_SIZE, False, None
    if key == "delete_particle":
        delete_head(prepared["type"])
        return pages, events, None, False, None
    assert key == "delete"
    tag()
    count = max(0, prepared["tag_count"])
    assert count <= 8
    for index in range(count):
        tag()
        pointer = prepared["system"] + 0x1c + index * TYPE_SIZE
        while get32(pages[GLOBAL_PAGE], pointer - GLOBAL_PAGE + 12) != NONE:
            delete_head(pointer)
    at = GLOBALS - GLOBAL_PAGE
    pages[GLOBAL_PAGE][at:at + 2] = p16(get16(pages[GLOBAL_PAGE], at) - 1)
    at = prepared["system"] - GLOBAL_PAGE
    pages[GLOBAL_PAGE][at:at + 4] = p32(NONE)
    return pages, events, result, False, count


def canonical_packet(packet):
    """Only the explicit reviewed owner spellings differ; no byte rewriting."""
    if isinstance(packet, str):
        return RENAMES.get(packet, packet)
    if isinstance(packet, list):
        return [canonical_packet(item) for item in packet]
    if isinstance(packet, dict):
        return {key: canonical_packet(value) for key, value in packet.items()}
    return packet


def unique_symbol(obj, names):
    found = [s for s in obj["symbols"] if s["name"] in names and s["section"] > 0]
    assert len(found) == 1, ("missing/ambiguous owner", names, found)
    return found[0]


def validate_abi(key, abi):
    assert set(abi) == {"registers", "stack", "preserved", "return", "argument_write"}, "ABI fields"
    wanted = set(TARGET_ABI[key]["registers"].values()) | set(TARGET_ABI[key]["stack"])
    supplied = list(abi["registers"].values()) + abi["stack"]
    assert set(supplied) == wanted and len(supplied) == len(wanted), "ABI argument mapping"
    assert set(abi["registers"]) <= set(REGISTERS) - {"esp", "ebp"}, "ABI register"
    assert set(abi["preserved"]) <= set(PRESERVED), "ABI preservation"
    assert abi["return"] == TARGET_ABI[key]["return"], "ABI result"
    assert abi["argument_write"] in ("unchanged", "final_type_index"), "ABI stack write"
    if abi["argument_write"] == "final_type_index":
        assert key == "delete" and abi["stack"] == ["local_index"], "ABI argument overwrite"


def read_manifest(path, target_only=False):
    manifest = json.loads(Path(path).read_text())
    assert manifest.get("version") == 1
    roles = ("target", "baseline") if target_only else ("target", "baseline", "first", "ordinary")
    entries = manifest["objects"]
    assert set(entries) == set(roles), ("manifest roles", roles)
    for role in roles:
        entry = entries[role]
        assert Path(entry["path"]).is_absolute(), "absolute object path required"
        assert re.fullmatch("[0-9a-f]{64}", entry["sha256"]), "complete raw object pin required"
        assert digest(Path(entry["path"]).read_bytes()) == entry["sha256"], ("file pin mismatch", role)
    assert entries["target"]["sha256"] == TARGET_PIN, "January pin mismatch"
    assert entries["baseline"]["sha256"] == BASELINE_PIN, "baseline pin mismatch"
    return entries


def validate_objects(entries):
    objects = {role: cc.load(Path(entry["path"])) for role, entry in entries.items()}
    target, baseline = objects["target"], objects["baseline"]
    names, packets, abis = {}, {}, {}
    for role, obj in objects.items():
        assert not any(s["name"] == "_point_from_line3d" for s in obj["symbols"]), "forbidden point helper"
        global_symbol = unique_symbol(obj, GLOBAL_NAMES)
        global_section = obj["sections"][global_symbol["section"] - 1]
        assert global_section["name"] == ".bss" and global_section["size"] == GLOBAL_SIZE
        assert global_symbol["value"] == 0 and global_section["reloc_count"] == 0
        if role != "target":
            assert global_symbol["storage"] == 3, "globals must retain static ownership"
        if role == "baseline":
            continue
        names[role], packets[role], abis[role] = {}, {}, {}
        for key in KEYS:
            symbol = unique_symbol(obj, NAMES[key])
            name = symbol["name"]
            assert symbol["value"] == 0 and symbol["type"] == 32
            if role != "target":
                assert symbol["storage"] == (2 if key == "delete" else 3), ("function ownership", role, key)
            info = cc.section_info(obj, name)
            target_info = cc.section_info(target, unique_symbol(target, NAMES[key])["name"])
            assert (target_info["size"], target_info["relocation_count"], target_info["normalized_sha256"]) == TARGET_PACKETS[key]
            equal = cc.section_infos_equal(canonical_packet(info), canonical_packet(target_info))
            assert equal, ("nonexact body requires a new independently reviewed ABI/oracle", role, key)
            abi = copy.deepcopy(TARGET_ABI[key])
            source = "complete January bytes and ordered relocations"
            validate_abi(key, abi)
            names[role][key], abis[role][key] = name, abi
            packets[role][key] = {"symbol": name, "size": info["size"], "relocations": info["relocation_count"],
                                  "normalized_sha256": info["normalized_sha256"], "strict_target_equal": equal,
                                  "abi": abi, "abi_evidence": source}
        for name in BASELINE_FUNCTIONS:
            assert cc.section_infos_equal(canonical_packet(cc.section_info(baseline, name)),
                                          canonical_packet(cc.section_info(obj, name))), ("inherited function changed", role, name)
    # Literal providers are actual pinned objects, not guessed engine memory.
    for name, expected_raw in LITERALS.items():
        providers = []
        for role, obj in objects.items():
            for symbol in obj["symbols"]:
                if symbol["name"] == name and symbol["section"] > 0:
                    section = obj["sections"][symbol["section"] - 1]
                    raw = cc._section_bytes(obj, section)[symbol["value"]:]
                    assert raw[:len(expected_raw)] == expected_raw, ("literal payload", role, name)
                    providers.append(role)
        assert providers, ("no pinned literal provider", name)
    return objects, names, packets, abis


def validate_output(path, manifest, entries):
    path = Path(path)
    inputs = [Path(manifest)] + [Path(entry["path"]) for entry in entries.values()]
    assert str(path.resolve()).casefold() not in {str(item.resolve()).casefold() for item in inputs}, (
        "output must not overwrite a pinned input", str(path))
    assert not path.exists() and not path.is_symlink(), ("output evidence already exists", str(path))


class Loader:
    def __init__(self, role, obj, names, abis, patches=None):
        self.role, self.obj, self.names, self.abis = role, obj, names, abis
        self.uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        self.extents = {}
        for base, size in ((CODE, 0x10000), (STUB, 0x10000), (LITERAL, PAGE_SIZE),
                           (GLOBAL_PAGE, PAGE_SIZE), (POINTER_PAGE, PAGE_SIZE), (DATA_PAGE, PAGE_SIZE),
                           (TAG_PAGE, PAGE_SIZE), (PARTICLE_PAGE, PAGE_SIZE), (STACK, STACK_SIZE)):
            self.uc.mem_map(base, size)
        for name, address in STUBS.items():
            self.uc.mem_write(address, b"\xc3")
        self.uc.mem_write(STOP, b"\xcc")
        for name, raw in LITERALS.items():
            self.uc.mem_write(LITERAL_ADDRESSES[name], raw)
        for key in KEYS:
            info = cc.section_info(obj, names[key])
            symbol = cc.symbol(obj, names[key])
            raw = cc._section_bytes(obj, obj["sections"][symbol["section"] - 1])
            for offset, old, new in (patches or {}).get(key, []):
                assert bytes(raw[offset:offset + len(old)]) == old, "mutation site mismatch"
                assert len(old) == len(new)
                raw[offset:offset + len(old)] = new
            assert len(raw) < 0x1000
            start = CODE_ADDRESSES[key]
            for relocation in info["relocations"]:
                destination = relocation.get("symbolic_target", relocation["target"])
                kind = destination[0]
                if kind == "internal":
                    address = start + destination[1]
                else:
                    assert kind == "symbol", ("unhandled destination", destination)
                    _, name, addend = destination
                    if name in STUBS:
                        address = STUBS[name] + addend
                    elif name in GLOBAL_NAMES:
                        address = GLOBALS + addend
                    elif name == "_weather_particle_data":
                        address = DATA_POINTER + addend
                    elif name in LITERAL_ADDRESSES:
                        assert addend == 0
                        address = LITERAL_ADDRESSES[name]
                    else:
                        called = [k for k, n in names.items() if n == name]
                        assert len(called) == 1 and addend == 0, ("unmodeled external/callee", name)
                        address = CODE_ADDRESSES[called[0]]
                at, typ = relocation["address"], relocation["type"]
                assert typ in (6, 20), ("unhandled relocation", typ)
                raw[at:at + 4] = p32(address - (start + at + 4) if typ == 20 else address)
            self.uc.mem_write(start, bytes(raw))
            self.extents[start] = len(raw)
        for base in (CODE, STUB):
            self.uc.mem_protect(base, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        for base in (LITERAL, POINTER_PAGE, DATA_PAGE, TAG_PAGE, PARTICLE_PAGE):
            self.uc.mem_protect(base, PAGE_SIZE, u.UC_PROT_READ)
        self.uc.mem_protect(GLOBAL_PAGE, PAGE_SIZE, u.UC_PROT_READ | u.UC_PROT_WRITE)
        self.uc.mem_protect(STACK, STACK_SIZE, u.UC_PROT_READ | u.UC_PROT_WRITE)

    def run(self, case, control):
        uc, key = self.uc, case["function"]
        prepared, abi = prepare(case), self.abis[key]
        wanted_pages, wanted_events, wanted_result, fatal, final_index = expected(case, prepared)
        for base, raw in prepared["pages"].items():
            uc.mem_write(base, raw)
        values = {"local_index": case["local_index"] & 0xffffffff,
                  "type_index": case["type_index"] & 0xffffffff,
                  "system": prepared["system"], "type": prepared["type"]}
        arguments = b"".join(p32(values[name]) for name in abi["stack"])
        stack = pattern(STACK_SIZE, 0x53)
        at = SP - STACK
        stack[at:at + 4 + len(arguments)] = p32(STOP) + arguments
        uc.mem_write(STACK, bytes(stack))
        initial = {name: 0x51510000 + index * 0x101 for index, name in enumerate(REGISTERS)}
        initial.update({name: values[value] for name, value in abi["registers"].items()})
        initial["esp"] = SP
        for name, value in initial.items():
            uc.reg_write(REGISTERS[name], value)
        uc.reg_write(x.UC_X86_REG_EFLAGS, 0x202)
        uc.reg_write(x.UC_X86_REG_FPCW, control)
        uc.reg_write(x.UC_X86_REG_FPSW, 0)
        uc.reg_write(x.UC_X86_REG_FPTAG, 0xffff)
        events, trace, deleted = [], [], set()
        state = {"exited": False, "returned": False}
        minimum_sp = [SP]

        def args(count):
            return struct.unpack("<" + "I" * count, uc.mem_read(uc.reg_read(x.UC_X86_REG_ESP) + 4, count * 4))

        def cstring(address):
            out = bytearray()
            for _ in range(256):
                char = bytes(uc.mem_read(address + len(out), 1))
                if char == b"\0":
                    return out.decode("ascii")
                out += char
            raise AssertionError("unterminated diagnostic string")

        def hook(emulator, address, size, unused):
            current_sp = uc.reg_read(x.UC_X86_REG_ESP)
            assert STACK + 64 <= current_sp <= SP + 4, "stack outside mapped caller frame"
            minimum_sp[0] = min(minimum_sp[0], current_sp)
            assert address == STOP or address in STUBS.values() or any(
                start <= address and address + size <= start + length for start, length in self.extents.items()), (
                    "unexpected executable address", self.role, hex(address))
            if address == STOP:
                state["returned"] = True
                uc.emu_stop()
                return
            for function, start in CODE_ADDRESSES.items():
                if address == start:
                    trace.append(function)
            api = next((name for name, start in STUBS.items() if start == address), None)
            if not api:
                return
            sp = uc.reg_read(x.UC_X86_REG_ESP)
            assert STACK + 64 <= sp < SP, "external call stack outside active frame"
            result = 0xcacacaca
            if api == "_tag_get":
                tag, handle = args(2)
                assert (tag, handle) == (RAIN, prepared["definition_index"]), ("tag contract", tag, handle)
                result = DEFINITION
                events.append({"api": "tag_get", "tag": tag, "index": handle, "result": result})
            elif api in ("_datum_get", "_datum_delete"):
                data, handle = args(2)
                assert data == DATA_ARRAY and handle in prepared["records"] and handle not in deleted, (
                    "datum contract", data, handle, sorted(deleted))
                pointer = prepared["records"][handle]
                event = {"api": api[1:], "data": data, "index": handle}
                if api == "_datum_get":
                    result = pointer
                    event["result"] = pointer
                else:
                    deleted.add(handle)
                    uc.mem_write(pointer, b"\xd5" * PARTICLE_SIZE)
                events.append(event)
            elif api == "_display_assert":
                information, filename, line, is_fatal = args(4)
                events.append({"api": "display_assert", "information": cstring(information),
                               "file": cstring(filename), "line": line, "fatal": is_fatal})
            else:
                code, = args(1)
                events.append({"api": "system_exit", "code": code})
                state["exited"] = True
                uc.emu_stop()
                return
            # Genuine cdecl volatile state; caller-saved values cannot accidentally survive a stub.
            uc.reg_write(x.UC_X86_REG_EAX, result)
            uc.reg_write(x.UC_X86_REG_ECX, 0xc1c1c1c1)
            uc.reg_write(x.UC_X86_REG_EDX, 0xd2d2d2d2)

        handle = uc.hook_add(u.UC_HOOK_CODE, hook)
        try:
            uc.emu_start(CODE_ADDRESSES[key], STOP + 1, count=100000)
        finally:
            uc.hook_del(handle)
        assert state == {"exited": fatal, "returned": not fatal}, ("termination", case["name"], state)
        assert events == wanted_events, ("external event mismatch", case["name"], events, wanted_events)
        snapshots = {}
        for base, wanted in wanted_pages.items():
            actual = bytes(uc.mem_read(base, PAGE_SIZE))
            assert actual == bytes(wanted), ("arena mismatch", case["name"], hex(base),
                [index for index, (a, b) in enumerate(zip(actual, wanted)) if a != b][:16])
            snapshots[hex(base)] = digest(actual)
        unused_stack_size = minimum_sp[0] - STACK
        assert bytes(uc.mem_read(STACK, unused_stack_size)) == bytes(stack[:unused_stack_size]), "unused lower stack guard"
        actual_stack = bytes(uc.mem_read(SP, STACK + STACK_SIZE - SP))
        expected_stack = bytearray(stack[at:])
        if not fatal and abi["argument_write"] == "final_type_index":
            expected_stack[4:8] = p32(final_index)
        assert actual_stack == bytes(expected_stack), ("caller stack/argument mismatch", case["name"])
        if not fatal:
            assert uc.reg_read(x.UC_X86_REG_ESP) == SP + 4, "return stack"
            for name in abi["preserved"]:
                assert uc.reg_read(REGISTERS[name]) == initial[name], ("nonvolatile register", name)
            if abi["return"] != "void":
                assert uc.reg_read(x.UC_X86_REG_EAX) == wanted_result, "pointer result"
        assert not uc.reg_read(x.UC_X86_REG_EFLAGS) & 0x400, "direction flag"
        assert uc.reg_read(x.UC_X86_REG_FPCW) == control, "x87 control"
        assert uc.reg_read(x.UC_X86_REG_FPTAG) == 0xffff, "x87 stack tags"
        assert uc.reg_read(x.UC_X86_REG_FPSW) & 0x3800 == 0, "x87 TOP"
        assert trace and trace[0] == key and set(trace) <= set(KEYS), "real body trace"
        return {"case": case["name"], "function": key, "control": control, "events": events,
                "arenas": snapshots, "result": wanted_result, "fatal": fatal, "body_trace": trace,
                "argument_slot": actual_stack[4:4 + len(arguments)].hex(), "status": "pass"}


def negative_controls(obj, names, abis):
    controls = (
        ("public_wrong_next_field", "delete_p0_one", "delete", 0xc8, b"\x8b\x58\x50", b"\x8b\x58\x4c", "datum contract"),
        ("private_wrong_count_field", "delete_head_p0_1", "delete_particle", 0x26,
         b"\x66\xff\x4e\x08", b"\x66\xff\x4e\x0a", "arena mismatch"),
    )
    results = []
    by_name = {case["name"]: case for case in cases()}
    for label, case_name, key, offset, old, new, wanted_failure in controls:
        loader = Loader("target-mutant", obj, names, abis, {key: [(offset, old, new)]})
        try:
            loader.run(by_name[case_name], FPCW_MODES[0])
        except (AssertionError, u.UcError) as exc:
            assert wanted_failure in str(exc), ("unexpected mutant failure", label, str(exc))
            results.append({"name": label, "function": key, "offset": offset,
                            "before": old.hex(), "after": new.hex(), "rejected": True, "reason": str(exc)})
        else:
            raise AssertionError(("actual instruction mutation escaped", label))
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--target-only", action="store_true", help="preparatory January/baseline replay only")
    args = parser.parse_args()
    entries = read_manifest(args.manifest, args.target_only)
    validate_output(args.output, args.manifest, entries)
    objects, names, packets, abis = validate_objects(entries)
    outcomes = {}
    for role in names:
        loader = Loader(role, objects[role], names[role], abis[role])
        outcomes[role] = [loader.run(case, control) for case in cases() for control in FPCW_MODES]
    # The admitted packet is strict in all roles, so compare internal body traces
    # and the real overwritten caller argument home as well as semantic effects.
    reference = outcomes["target"]
    for role, rows in outcomes.items():
        assert rows == reference, ("target differential mismatch", role)
    negatives = negative_controls(objects["target"], names["target"], abis["target"])
    for role, entry in entries.items():
        assert digest(Path(entry["path"]).read_bytes()) == entry["sha256"], ("object changed during read-only replay", role)
    report = {"status": "pass", "scope": "finite actual-x86 Weather delete/helper graph; external APIs modeled",
              "target_only": args.target_only, "manifest_sha256": digest(args.manifest.read_bytes()),
              "objects": entries, "packets": packets, "cases_per_role": len(cases()),
              "executions": sum(len(rows) for rows in outcomes.values()), "outcomes": outcomes,
              "actual_instruction_negative_controls": negatives}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": "pass", "executions": report["executions"], "negative_controls": len(negatives),
                      "report": str(args.output), "sha256": digest(args.output.read_bytes())}))


if __name__ == "__main__":
    main()
