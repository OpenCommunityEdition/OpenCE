"""Target-first runtime differential for three public RenderDebug functions.

The real public point, line, and box-outline machine code is executed.  Box
outline also executes the real same-object polygon-edges and public-line code.
Deferred calls execute the real same-object variadic cache helper; only genuine
outside APIs (assert/exit, time/error, and the rasterizer line sink) are modeled.

The accepted domain is deliberately finite: valid non-NULL inputs with exactly
representable finite coordinates/sizes, the documented cache bounds, and seven
NULL assertion exits.  This does not claim exhaustive floating-point proof for
the natural point residual.  Synthetic outcome controls mutate captured results.
One separate execution control changes a verified target instruction byte only
in Unicorn memory; no on-disk object is modified.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys

import unicorn as u
from unicorn.x86_const import *


ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc
from tools.campaign._coff_tools import function_sections
from tools.regression_gate import _section_aux_selection


PATHS = {
    "target": ROOT / "scratch/render-debug-three-before-20260906/build/split/source/render/render_debug.obj",
    "first": ROOT / "scratch/render-debug-three-first-20260906.obj",
    "actual": ROOT / "scratch/render-debug-three-actual-20260906.obj",
    "source": ROOT / "scratch/render-debug-three-source-final-20260906.c",
    "cache_reference": ROOT / "scratch/render_debug_cache_behavior_20260905.py",
}
PINS = {
    "target": "d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0",
    "first": "c17a3543cac88b90690090da553f4e6b3d252354f122cf9b73ead08a3581909b",
    "actual": "62a44f55c5a75662b70c068d6d4390b0dd6abddc1d699bd82e47bd0ae5c5bf90",
    "source": "8a1d5cd0765136789b8c04fa5850acfa20a54f47274c90c0f766fc68cd41a79b",
    "cache_reference": "6705d663d86f5185fcf9b7e71ce9e0549d9716bd3c86399a97bc2cecd8d28964",
}

POINT = "_render_debug_point"
LINE = "_render_debug_line"
BOX = "_render_debug_box_outline"
POLYGON = "_render_debug_polygon_edges"
CACHE_ENTRY = "_render_debug_add_cache_entry"
CACHE_STRING = "_render_debug_add_cache_string"
FUNCTIONS = (POINT, LINE, BOX, POLYGON, CACHE_ENTRY, CACHE_STRING)

CODE = 0x100000
STUB = 0x200000
STOP = 0x20F000
LITERAL = 0x210000
CACHE = 0x300100
INPUT = 0x400100
STACK = 0x500000
SP = 0x508000

CACHE_SIZE = 0x740C
POOL_SIZE = 1024
ENTRY_SIZE = 0x38
ENTRY_LIMIT = 512
TIME_OFFSET = 0x7400
COUNT_OFFSET = 0x7404
POOL_OFFSET = 0x7408
CACHE_WARNING_OFFSET = 0x740A
STRING_WARNING_OFFSET = 0x740B

FPCW_MODES = (0x027F, 0x037F)
SAVED = {
    UC_X86_REG_EBP: 0x11223344,
    UC_X86_REG_EBX: 0x22334455,
    UC_X86_REG_ESI: 0x33445566,
    UC_X86_REG_EDI: 0x44556677,
}

APIS = (
    "_display_assert",
    "_system_exit",
    "_rasterizer_debug_line",
    "_game_time_get",
    "_error",
    "_csstrcmp",
    "_csstrncpy",
    "_csstrlen",
)
STUBS = {name: STUB + index * 0x100 for index, name in enumerate(APIS)}
FUNCTION_ADDRESSES = {name: CODE + index * 0x1000 for index, name in enumerate(FUNCTIONS)}

SOURCE_TEXT = b"c:\\halo\\SOURCE\\render\\render_debug.c\0"
LITERALS = {
    "??_C@_0CF@JKFPHJJE@c?3?2halo?2SOURCE?2render?2render_deb@": SOURCE_TEXT,
    "??_C@_05BBEJFBFA@point?$AA@": b"point\0",
    "??_C@_06LEDMCOFG@point0?$AA@": b"point0\0",
    "??_C@_06KNCHBPBH@point1?$AA@": b"point1\0",
    "??_C@_06GPGIDCJA@points?$AA@": b"points\0",
    "??_C@_05PEENBMOG@color?$AA@": b"color\0",
    "??_C@_06CGBPGGIE@bounds?$AA@": b"bounds\0",
    "??_C@_0BN@JJPKCCPJ@render?5debug?5cache?5overflow?4?$AA@": b"render debug cache overflow.\0",
    "??_C@_0CD@CLODFBGE@render?5debug?5cache?5string?5overfl@": b"render debug cache string overflow\0",
    "__real@3f000000": struct.pack("<f", 0.5),
}
LITERAL_ADDRESSES = {
    name: LITERAL + index * 0x400 for index, name in enumerate(sorted(LITERALS))
}


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def p16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def f32(value: float) -> bytes:
    return struct.pack("<f", value)


def f32_value(value: float) -> float:
    return struct.unpack("<f", f32(value))[0]


def point_bytes(values) -> bytes:
    return struct.pack("<3f", *values)


def color_bytes(values) -> bytes:
    return struct.pack("<4f", *values)


def s16(raw: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<h", raw, offset)[0]


def contains(base: int, length: int, address: int, size: int) -> bool:
    return base <= address and address + size <= base + length


def cases() -> list[dict]:
    return [
        {"name": "line_immediate", "function": LINE, "immediate": True},
        {"name": "line_immediate_aliased_endpoints", "function": LINE, "immediate": True, "alias": True},
        {"name": "line_deferred_same_tick", "function": LINE, "immediate": False, "count": 3},
        {"name": "line_deferred_new_tick", "function": LINE, "immediate": False, "cache_time": 9,
         "game_time": 10, "second_time": 11, "count": 12, "pool": b"old\0"},
        {"name": "point_immediate_positive", "function": POINT, "immediate": True, "size": 4.0},
        {"name": "point_immediate_zero", "function": POINT, "immediate": True, "size": 0.0},
        {"name": "point_immediate_negative", "function": POINT, "immediate": True, "size": -2.0},
        {"name": "point_deferred_same_tick", "function": POINT, "immediate": False,
         "size": 2.25, "count": 7},
        {"name": "point_deferred_new_tick", "function": POINT, "immediate": False,
         "size": -2.0, "cache_time": 9, "game_time": 10, "second_time": 11,
         "count": 511, "pool": b"stale\0"},
        {"name": "box_immediate_ordinary", "function": BOX, "immediate": True},
        {"name": "box_immediate_degenerate", "function": BOX, "immediate": True,
         "bounds": (2.0, 2.0, -1.0, -1.0, 0.0, 0.0)},
        {"name": "box_immediate_inverted", "function": BOX, "immediate": True,
         "bounds": (4.0, -3.0, 6.0, -5.0, 8.0, -7.0)},
        {"name": "box_deferred_same_tick", "function": BOX, "immediate": False, "count": 9},
        {"name": "box_deferred_last_slot", "function": BOX, "immediate": False, "count": 511},
        {"name": "line_deferred_full_cache_warns", "function": LINE, "immediate": False,
         "count": 512, "cache_warning": 0},
        {"name": "assert_point_null", "function": POINT, "immediate": True, "null": "point"},
        {"name": "assert_point_color_null", "function": POINT, "immediate": True, "null": "color"},
        {"name": "assert_line_point0_null", "function": LINE, "immediate": True, "null": "point0"},
        {"name": "assert_line_point1_null", "function": LINE, "immediate": True, "null": "point1"},
        {"name": "assert_line_color_null", "function": LINE, "immediate": True, "null": "color"},
        {"name": "assert_box_bounds_null", "function": BOX, "immediate": True, "null": "bounds"},
        {"name": "assert_box_color_null", "function": BOX, "immediate": True, "null": "color"},
    ]


def input_image(case: dict) -> tuple[bytes, dict[str, bytes], dict[str, int]]:
    raw = bytearray((index * 23 + 11) & 0xFF for index in range(0x10000))
    bounds = case.get("bounds", (-3.0, 4.0, -5.0, 6.0, -7.0, 8.0))
    blocks = {
        "point": point_bytes((1.25, -2.5, 3.75)),
        "point0": point_bytes((1.25, -2.5, 3.75)),
        "point1": point_bytes((-4.5, 5.25, -6.0)),
        "color": color_bytes((0.75, 0.125, 0.5, 1.0)),
        "bounds": struct.pack("<6f", *bounds),
    }
    locations = {name: INPUT + index * 0x100 for index, name in enumerate(blocks)}
    if case.get("alias"):
        locations["point1"] = locations["point0"]
        blocks["point1"] = blocks["point0"]
    for name, data in blocks.items():
        at = locations[name] - INPUT
        raw[at:at + len(data)] = data
    return bytes(raw), blocks, locations


def public_arguments(case: dict, locations: dict[str, int]) -> bytes:
    null = case.get("null")
    address = lambda name: 0 if null == name else locations[name]
    immediate = p32(int(case["immediate"]))
    if case["function"] == POINT:
        return immediate + p32(address("point")) + f32(case.get("size", 4.0)) + p32(address("color"))
    if case["function"] == LINE:
        return immediate + p32(address("point0")) + p32(address("point1")) + p32(address("color"))
    assert case["function"] == BOX
    return immediate + p32(address("bounds")) + p32(address("color"))


def initial_cache(case: dict) -> bytearray:
    raw = bytearray((index * 37 + 19) & 0xFF for index in range(CACHE_SIZE))
    pool = case.get("pool", b"")
    assert len(pool) <= 1023
    raw[:len(pool)] = pool
    raw[len(pool)] = 0
    raw[TIME_OFFSET:TIME_OFFSET + 2] = p16(case.get("cache_time", 10))
    raw[COUNT_OFFSET:COUNT_OFFSET + 2] = p16(case.get("count", 0))
    raw[POOL_OFFSET:POOL_OFFSET + 2] = p16(len(pool))
    raw[CACHE_WARNING_OFFSET] = case.get("cache_warning", 0)
    raw[STRING_WARNING_OFFSET] = case.get("string_warning", 0)
    return raw


def cache_model(before: bytearray, case: dict, blocks: dict[str, bytes]) -> tuple[bytearray, list[dict], int | None]:
    raw = bytearray(before)
    events = [{"api": "game_time_get", "result": case.get("game_time", 10) & 0xFFFFFFFF}]
    now = case.get("game_time", 10)
    if s16(raw, TIME_OFFSET) != struct.unpack("<h", p16(now))[0]:
        second = case.get("second_time", now)
        events.append({"api": "game_time_get", "result": second & 0xFFFFFFFF})
        raw[TIME_OFFSET:TIME_OFFSET + 2] = p16(second)
        raw[POOL_OFFSET:POOL_OFFSET + 2] = p16(0)
        raw[0] = 0
        index = 0
    else:
        index = s16(raw, COUNT_OFFSET)
        if index >= ENTRY_LIMIT:
            if not raw[CACHE_WARNING_OFFSET]:
                events.append({"api": "error", "priority": 2,
                               "message": "render debug cache overflow."})
                raw[CACHE_WARNING_OFFSET] = 1
            return raw, events, None

    raw[COUNT_OFFSET:COUNT_OFFSET + 2] = p16(index + 1)
    start = POOL_SIZE + index * ENTRY_SIZE
    if case["function"] == POINT:
        raw[start:start + 2] = p16(1)
        raw[start + 4:start + 0x10] = blocks["point"]
        raw[start + 0x10:start + 0x14] = f32(case.get("size", 4.0))
        raw[start + 0x14:start + 0x24] = blocks["color"]
    elif case["function"] == LINE:
        raw[start:start + 2] = p16(2)
        raw[start + 4:start + 0x10] = blocks["point0"]
        raw[start + 0x10:start + 0x1C] = blocks["point1"]
        raw[start + 0x1C:start + 0x2C] = blocks["color"]
    else:
        raw[start:start + 2] = p16(7)
        raw[start + 4:start + 0x1C] = blocks["bounds"]
        raw[start + 0x1C:start + 0x2C] = blocks["color"]
    return raw, events, index


def point_axis_pairs(point: bytes, size: float) -> list[tuple[bytes, bytes]]:
    x, y, z = struct.unpack("<3f", point)
    half = f32_value(size) * 0.5
    values = [
        ((x - half, y, z), (x + half, y, z)),
        ((x, y - half, z), (x, y + half, z)),
        ((x, y, z - half), (x, y, z + half)),
    ]
    return [(point_bytes(a), point_bytes(b)) for a, b in values]


def box_points(bounds: bytes) -> list[bytes]:
    x0, x1, y0, y1, z0, z1 = struct.unpack("<6f", bounds)
    return [point_bytes(point) for point in (
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    )]


def raster_event(point0: bytes, point1: bytes, color: bytes) -> dict:
    return {"api": "rasterizer_debug_line", "point0": point0.hex(),
            "point1": point1.hex(), "color": color.hex()}


def expected_success(case: dict, before: bytearray, blocks: dict[str, bytes]) -> tuple[bytearray, list[dict], list[str], int | None]:
    if not case["immediate"]:
        cache, events, index = cache_model(before, case, blocks)
        return cache, events, [case["function"], CACHE_ENTRY], index

    if case["function"] == LINE:
        return before, [raster_event(blocks["point0"], blocks["point1"], blocks["color"])], [LINE], None
    if case["function"] == POINT:
        events = [raster_event(a, b, blocks["color"])
                  for a, b in point_axis_pairs(blocks["point"], case.get("size", 4.0))]
        return before, events, [POINT], None

    points = box_points(blocks["bounds"])
    edges = [(3, 0), (0, 1), (1, 2), (2, 3),
             (7, 4), (4, 5), (5, 6), (6, 7),
             (0, 4), (1, 5), (2, 6), (3, 7)]
    events = [raster_event(points[a], points[b], blocks["color"]) for a, b in edges]
    trace = [BOX, POLYGON] + [LINE] * 4 + [POLYGON] + [LINE] * 8
    return before, events, trace, None


ASSERTIONS = {
    (POINT, "point"): ("point", 327),
    (POINT, "color"): ("color", 328),
    (LINE, "point0"): ("point0", 363),
    (LINE, "point1"): ("point1", 364),
    (LINE, "color"): ("color", 365),
    (BOX, "bounds"): ("bounds", 812),
    (BOX, "color"): ("color", 813),
}


class Loader:
    def __init__(self, label: str, path: Path, code_patches: dict[str, list[tuple[int, int, int]]] | None = None):
        self.label = label
        self.path = path
        self.obj = cc.load(path)
        self.sections = function_sections(self.obj)
        self.code_patches = code_patches or {}
        self.uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for base, size in ((CODE, 0x10000), (STUB, 0x10000), (LITERAL, 0x10000),
                           (CACHE - 0x100, 0x8000), (INPUT - 0x100, 0x11000),
                           (STACK, 0x10000)):
            self.uc.mem_map(base, size)
        self.loaded: set[str] = set()
        self.extents: dict[int, int] = {}
        for address in STUBS.values():
            self.uc.mem_write(address, b"\xC3")
        self.uc.mem_write(STOP, b"\xCC")
        for name, payload in LITERALS.items():
            self.uc.mem_write(LITERAL_ADDRESSES[name], payload)
        for name in (POINT, LINE, BOX):
            self.function(name)
        self.uc.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.uc.mem_protect(STUB, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.uc.mem_protect(LITERAL, 0x10000, u.UC_PROT_READ)
        self.uc.mem_protect(CACHE - 0x100, 0x8000, u.UC_PROT_READ | u.UC_PROT_WRITE)
        self.uc.mem_protect(INPUT - 0x100, 0x11000, u.UC_PROT_READ)
        self.uc.mem_protect(STACK, 0x10000, u.UC_PROT_READ | u.UC_PROT_WRITE)

    def function(self, name: str) -> int:
        assert name in FUNCTION_ADDRESSES, (self.label, "unexpected internal function", name)
        address = FUNCTION_ADDRESSES[name]
        if name in self.loaded:
            return address
        self.loaded.add(name)
        section = self.sections[name]
        raw = bytearray(cc._section_bytes(self.obj, self.obj["sections"][section.section_number - 1]))
        assert len(raw) <= 0x1000
        for offset, expected, replacement in self.code_patches.get(name, []):
            assert raw[offset] == expected, (self.label, name, offset, raw[offset], expected)
            raw[offset] = replacement
        self.extents[address] = len(raw)
        for relocation in section.info["relocations"]:
            target = relocation.get("symbolic_target", relocation["target"])
            if target[0] == "internal":
                destination = address + target[1]
            else:
                assert target[0] in ("symbol", "defined-noncode"), target
                symbol, addend = target[1:] if target[0] == "symbol" else target[2:]
                if symbol == "_render_debug_globals":
                    destination = CACHE
                elif symbol in FUNCTION_ADDRESSES:
                    destination = self.function(symbol)
                elif symbol in STUBS:
                    destination = STUBS[symbol]
                else:
                    destination = self.constant(symbol)
                destination += addend
            if relocation["type"] == cc.IMAGE_REL_I386_REL32:
                destination -= address + relocation["address"] + 4
            else:
                assert relocation["type"] == cc.IMAGE_REL_I386_DIR32, relocation
            raw[relocation["address"]:relocation["address"] + 4] = p32(destination)
        self.uc.mem_write(address, bytes(raw))
        return address

    def constant(self, name: str) -> int:
        assert name in LITERALS, (self.label, "unexpected constant owner", name)
        expected = LITERALS[name]
        try:
            symbol = cc.symbol(self.obj, name)
        except cc.CoffError:
            symbol = None
        if symbol is not None:
            section = self.obj["sections"][symbol["section"] - 1]
            raw = bytes(cc._section_bytes(self.obj, section))[symbol["value"]:]
            assert raw == expected, (self.label, name, raw.hex(), expected.hex())
        return LITERAL_ADDRESSES[name]

    def cstring(self, address: int) -> str:
        if contains(LITERAL, 0x10000, address, 1):
            limit = LITERAL + 0x10000
        else:
            raise AssertionError((self.label, "invalid cstring", hex(address)))
        raw = bytes(self.uc.mem_read(address, limit - address))
        assert b"\0" in raw
        return raw.split(b"\0", 1)[0].decode("ascii")

    @staticmethod
    def pointer_region(address: int) -> list:
        for label, base, size in (("input", INPUT, 0x10000), ("stack", STACK, 0x10000),
                                  ("cache", CACHE, CACHE_SIZE), ("literal", LITERAL, 0x10000)):
            if contains(base, size, address, 1):
                return [label, address - base]
        return ["unknown", address]

    def run(self, case: dict, fpcw: int) -> dict:
        uc = self.uc
        input_raw, blocks, locations = input_image(case)
        cache_before = initial_cache(case)
        expected_cache = bytearray(cache_before)
        expected_events: list[dict]
        expected_trace: list[str]
        expected_index = None
        assertion = case.get("null")
        if assertion:
            information, line = ASSERTIONS[(case["function"], assertion)]
            expected_events = [
                {"api": "display_assert", "information": information,
                 "file": SOURCE_TEXT[:-1].decode("ascii"), "line": line, "fatal": 1},
                {"api": "system_exit", "code": 0xFFFFFFFF},
            ]
            expected_trace = [case["function"]]
        else:
            expected_cache, expected_events, expected_trace, expected_index = expected_success(
                case, cache_before, blocks)

        uc.mem_write(CACHE - 64, b"\xA5" * 64 + bytes(cache_before) + b"\xA5" * 64)
        uc.mem_write(INPUT - 64, b"\xA5" * 64 + input_raw + b"\xA5" * 64)
        payload = public_arguments(case, locations)
        stack = bytearray(b"\x5A" * 0x10000)
        stack_at = SP - STACK
        stack[stack_at:stack_at + 4 + len(payload)] = p32(STOP) + payload
        uc.mem_write(STACK, bytes(stack))

        state = {"events": [], "trace": [], "snapshots": [], "time_calls": 0,
                 "exited": False, "forbidden_access": None}

        def args(count: int) -> tuple[int, ...]:
            esp = uc.reg_read(UC_X86_REG_ESP)
            return struct.unpack("<" + "I" * count, uc.mem_read(esp + 4, count * 4))

        def code_hook(emulator, address, size, unused):
            assert (address == STOP or address in STUBS.values()
                    or any(contains(base, length, address, size)
                           for base, length in self.extents.items())), (
                               self.label, "unexpected executable address", hex(address), size)
            if address in FUNCTION_ADDRESSES.values():
                state["trace"].append(next(name for name, value in FUNCTION_ADDRESSES.items()
                                           if value == address))
            name = next((name for name, value in STUBS.items() if value == address), None)
            if name is None:
                return
            state["snapshots"].append({"api": name,
                                       "cache_sha256": digest(bytes(uc.mem_read(CACHE, CACHE_SIZE)))})
            result = 0x76543210
            if name == "_rasterizer_debug_line":
                p0, p1, color = args(3)
                event = raster_event(bytes(uc.mem_read(p0, 12)), bytes(uc.mem_read(p1, 12)),
                                     bytes(uc.mem_read(color, 16)))
                event.update({"point0_pointer": self.pointer_region(p0),
                              "point1_pointer": self.pointer_region(p1),
                              "color_pointer": self.pointer_region(color)})
            elif name == "_game_time_get":
                key = "game_time" if state["time_calls"] == 0 else "second_time"
                result = case.get(key, case.get("game_time", 10)) & 0xFFFFFFFF
                state["time_calls"] += 1
                event = {"api": "game_time_get", "result": result}
            elif name == "_error":
                priority, message = args(2)
                event = {"api": "error", "priority": priority,
                         "message": self.cstring(message)}
            elif name == "_display_assert":
                information, file_name, line, fatal = args(4)
                event = {"api": "display_assert", "information": self.cstring(information),
                         "file": self.cstring(file_name), "line": line, "fatal": fatal}
            elif name == "_system_exit":
                (code,) = args(1)
                event = {"api": "system_exit", "code": code}
                state["exited"] = True
                state["events"].append(event)
                emulator.emu_stop()
                return
            else:
                raise AssertionError((self.label, "unexpected cache-string API", name))
            state["events"].append(event)
            uc.reg_write(UC_X86_REG_EAX, result & 0xFFFFFFFF)
            uc.reg_write(UC_X86_REG_ECX, 0xA1B2C3D4)
            uc.reg_write(UC_X86_REG_EDX, 0xB2C3D4E5)

        def memory_hook(emulator, access, address, size, value, unused):
            if access == u.UC_MEM_WRITE:
                allowed = (contains(CACHE, CACHE_SIZE, address, size)
                           or contains(STACK + 64, SP - STACK - 64, address, size))
            else:
                allowed = (contains(CACHE, CACHE_SIZE, address, size)
                           or contains(INPUT, 0x10000, address, size)
                           or contains(STACK + 64, 0x10000 - 128, address, size)
                           or contains(LITERAL, 0x10000, address, size)
                           or contains(CODE, 0x10000, address, size))
            if not allowed:
                state["forbidden_access"] = [access, address, size, value]
                raise AssertionError((self.label, "forbidden memory access", access,
                                      hex(address), size, value))

        code_handle = uc.hook_add(u.UC_HOOK_CODE, code_hook)
        memory_handle = uc.hook_add(u.UC_HOOK_MEM_READ | u.UC_HOOK_MEM_WRITE, memory_hook)
        initial_tag = None
        try:
            uc.reg_write(UC_X86_REG_ESP, SP)
            uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
            uc.reg_write(UC_X86_REG_FPCW, fpcw)
            uc.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
            uc.reg_write(UC_X86_REG_FPSW, 0)
            initial_tag = uc.reg_read(UC_X86_REG_FPTAG)
            for register, value in SAVED.items():
                uc.reg_write(register, value)
            uc.emu_start(FUNCTION_ADDRESSES[case["function"]], STOP, count=500000)
        finally:
            uc.hook_del(code_handle)
            uc.hook_del(memory_handle)

        returned = not assertion
        esp_restored = (uc.reg_read(UC_X86_REG_ESP) == SP + 4) if returned else None
        nonvolatiles_restored = (
            all(uc.reg_read(register) == value for register, value in SAVED.items())
            if returned else None)
        fpcw_final = uc.reg_read(UC_X86_REG_FPCW)
        fptag_final = uc.reg_read(UC_X86_REG_FPTAG)
        fpsw_final = uc.reg_read(UC_X86_REG_FPSW)
        assert state["exited"] == bool(assertion)
        if returned:
            assert uc.reg_read(UC_X86_REG_EIP) == STOP
            assert esp_restored and nonvolatiles_restored
        assert fpcw_final == fpcw
        assert fptag_final == initial_tag, "x87 tag word changed"
        assert (fpsw_final & 0x3800) == 0, "x87 TOP not restored"
        assert not (uc.reg_read(UC_X86_REG_EFLAGS) & 0x400), "direction flag set"

        guarded_cache = bytes(uc.mem_read(CACHE - 64, CACHE_SIZE + 128))
        guarded_input = bytes(uc.mem_read(INPUT - 64, 0x10000 + 128))
        after_stack = bytes(uc.mem_read(STACK, 0x10000))
        cache_guard = guarded_cache[:64] == guarded_cache[-64:] == b"\xA5" * 64
        input_guard = guarded_input[:64] == guarded_input[-64:] == b"\xA5" * 64
        input_immutable = guarded_input[64:-64] == input_raw
        stack_guard = (after_stack[:64] == bytes(stack[:64])
                       and after_stack[SP - STACK:] == bytes(stack[SP - STACK:]))
        actual_cache = guarded_cache[64:-64]
        assert cache_guard and input_guard and input_immutable and stack_guard
        assert actual_cache == bytes(expected_cache), (
            self.label, case["name"], "cache mismatch",
            [index for index, pair in enumerate(zip(actual_cache, expected_cache))
             if pair[0] != pair[1]][:20])
        assert state["trace"] == expected_trace, (self.label, case["name"], state["trace"], expected_trace)

        semantic_events = [{key: value for key, value in event.items() if not key.endswith("_pointer")}
                           for event in state["events"]]
        assert semantic_events == expected_events, (
            self.label, case["name"], semantic_events, expected_events)

        entry_hex = None
        if expected_index is not None:
            start = POOL_SIZE + expected_index * ENTRY_SIZE
            entry_hex = actual_cache[start:start + ENTRY_SIZE].hex()
        return {
            "case": case["name"],
            "fpcw_initial": fpcw,
            "events": state["events"],
            "function_trace": state["trace"],
            "api_cache_snapshots": state["snapshots"],
            "cache_sha256": digest(actual_cache),
            "cache_entry_hex": entry_hex,
            "cache_count": s16(actual_cache, COUNT_OFFSET),
            "cache_time": s16(actual_cache, TIME_OFFSET),
            "cache_pool_offset": s16(actual_cache, POOL_OFFSET),
            "cache_flags": list(actual_cache[CACHE_WARNING_OFFSET:STRING_WARNING_OFFSET + 1]),
            "returned": returned,
            "system_exit_nonreturn": state["exited"],
            "abi": {
                "esp_restored": esp_restored,
                "nonvolatiles_restored": nonvolatiles_restored,
                "direction_flag_clear": True,
                "fpcw_final": fpcw_final,
                "fptag_restored": fptag_final == initial_tag,
                "x87_top_restored": (fpsw_final & 0x3800) == 0,
                "cache_guard": cache_guard,
                "input_guard": input_guard,
                "input_immutable": input_immutable,
                "stack_guard": stack_guard,
                "forbidden_access": state["forbidden_access"],
            },
        }


def comparable(outcome: dict) -> dict:
    return {key: value for key, value in outcome.items() if key not in ("case",)}


def captured_outcome_negative_controls(outcomes: dict[tuple[str, int], dict]) -> dict:
    controls = {}

    def changed(name: str, case_name: str, mutate):
        key = (case_name, 0x037F)
        original = outcomes[key]
        modified = copy.deepcopy(original)
        mutate(modified)
        controls[name] = comparable(modified) != comparable(original)
        assert controls[name], name

    def wrong_axis(item):
        events = [event for event in item["events"] if event["api"] == "rasterizer_debug_line"]
        events[0]["point0"] = events[1]["point0"]

    def wrong_sign(item):
        event = next(event for event in item["events"] if event["api"] == "rasterizer_debug_line")
        raw = bytearray.fromhex(event["point0"])
        raw[:4] = bytearray.fromhex(item["events"][0]["point1"][:8])[:4]
        event["point0"] = raw.hex()

    def wrong_tag(item):
        raw = bytearray.fromhex(item["cache_entry_hex"])
        raw[:2] = p16(2)
        item["cache_entry_hex"] = raw.hex()

    changed("wrong_point_axis", "point_immediate_positive", wrong_axis)
    changed("wrong_point_sign", "point_immediate_positive", wrong_sign)
    changed("wrong_cache_tag", "point_deferred_same_tick", wrong_tag)
    changed("wrong_box_call_order", "box_immediate_ordinary",
            lambda item: item["events"].__setitem__(slice(0, 2), list(reversed(item["events"][:2]))))
    changed("wrong_stack_result", "line_immediate",
            lambda item: item["abi"].__setitem__("esp_restored", False))
    changed("wrong_nonvolatile_result", "line_immediate",
            lambda item: item["abi"].__setitem__("nonvolatiles_restored", False))
    changed("wrong_guard_result", "box_immediate_ordinary",
            lambda item: item["abi"].__setitem__("cache_guard", False))
    changed("wrong_fpcw_result", "point_immediate_positive",
            lambda item: item["abi"].__setitem__("fpcw_final", 0x027F))
    return controls


def in_memory_target_byte_negative_control() -> dict:
    # January point +0x77 is `d8 e1` (fsub st(1)); replace only its ModR/M byte
    # with c1 (`d8 c1`, fadd st(1)).  This reverses the first x-axis endpoint
    # while leaving the function runnable.  Loader.run must reject its modeled
    # event.  The pinned target object on disk remains untouched.
    mutation = {POINT: [(0x78, 0xE1, 0xC1)]}
    loader = Loader("target_in_memory_wrong_x_sign", PATHS["target"], mutation)
    case = next(item for item in cases() if item["name"] == "point_immediate_positive")
    try:
        loader.run(case, 0x037F)
    except AssertionError as error:
        return {
            "detected": True,
            "kind": "transient Unicorn code-memory mutation; on-disk object unchanged",
            "function": POINT,
            "offset": 0x78,
            "expected_byte": 0xE1,
            "replacement_byte": 0xC1,
            "instruction_change": "d8 e1 fsub st(1) -> d8 c1 fadd st(1)",
            "case": case["name"],
            "fpcw": 0x037F,
            "rejection": repr(error),
        }
    raise AssertionError("in-memory target instruction mutation was not detected")


def object_metadata(loader: Loader) -> dict:
    metadata = {}
    for name in FUNCTIONS:
        symbol = cc.symbol(loader.obj, name)
        metadata[name] = {
            "storage": symbol["storage"],
            "type": symbol["type"],
            "value": symbol["value"],
            "selection": _section_aux_selection(loader.obj, symbol["section"]),
            **loader.sections[name].info,
        }
    half = next((symbol for symbol in loader.obj["symbols"]
                 if symbol["name"] == "__real@3f000000" and symbol["section"] > 0), None)
    metadata["half_literal"] = None if half is None else {
        "size": loader.obj["sections"][half["section"] - 1]["size"],
        "selection": _section_aux_selection(loader.obj, half["section"]),
        "payload": bytes(cc._section_bytes(loader.obj,
                           loader.obj["sections"][half["section"] - 1])).hex(),
    }
    return metadata


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "scratch/render-debug-three-runtime-report.json",
        help="report path (default preserves the original scratch artifact name)",
    )
    args = parser.parse_args()
    for label, path in PATHS.items():
        actual = digest(path.read_bytes())
        assert actual == PINS[label], (label, actual, PINS[label])

    loaders = {label: Loader(label, PATHS[label]) for label in ("target", "first", "actual")}
    results = []
    failures = []
    target_outcomes: dict[tuple[str, int], dict] = {}
    for case in cases():
        for fpcw in FPCW_MODES:
            try:
                outcomes = {label: loader.run(case, fpcw) for label, loader in loaders.items()}
                target = outcomes["target"]
                target_outcomes[(case["name"], fpcw)] = target
                first_equal = comparable(outcomes["first"]) == comparable(target)
                actual_equal = comparable(outcomes["actual"]) == comparable(target)
                assert first_equal and actual_equal
                results.append({"case": case["name"], "fpcw": fpcw,
                                "target": target, "first_equal": first_equal,
                                "actual_equal": actual_equal})
            except Exception as error:
                failures.append({"case": case["name"], "fpcw": fpcw, "error": repr(error)})

    outcome_controls = captured_outcome_negative_controls(target_outcomes) if not failures else {}
    byte_control = in_memory_target_byte_negative_control() if not failures else {}
    accepted_execution_count = len(cases()) * len(FPCW_MODES) * len(loaders)
    report = {
        "scope": __doc__,
        "harness_sha256": digest(Path(__file__).read_bytes()),
        "unicorn_version": u.__version__,
        "pins": {label: {"path": str(path), "sha256": PINS[label]}
                 for label, path in PATHS.items()},
        "objects": {label: object_metadata(loader) for label, loader in loaders.items()},
        "scenario_count": len(cases()),
        "fpcw_modes": list(FPCW_MODES),
        "accepted_execution_count": accepted_execution_count,
        "in_memory_negative_execution_count": 1 if byte_control else 0,
        "execution_count": accepted_execution_count + (1 if byte_control else 0),
        "failures": failures,
        "all_target_models_pass": not failures,
        "first_differential_pass": not failures and all(item["first_equal"] for item in results),
        "actual_differential_pass": not failures and all(item["actual_equal"] for item in results),
        "captured_outcome_negative_controls": {
            "kind": "synthetic captured-outcome mutations; object bytes are never modified",
            "results": outcome_controls,
            "all_detected": bool(outcome_controls) and all(outcome_controls.values()),
        },
        "in_memory_target_byte_negative_control": byte_control,
        "results": results,
        "limitations": [
            "Finite exact-value scenarios only; no NaN, infinity, denormal, or arbitrary invalid-input claim.",
            "The point residual differs by four independent integer-store schedules around x87 operations; this harness proves the listed outcomes, not exhaustive equivalence.",
            "system_exit is modeled as a genuine nonreturn boundary, so post-exit callee restoration is intentionally not asserted.",
            "Only genuine external APIs are modeled; polygon_edges, public line, and private cache helpers execute their real same-object bytes.",
        ],
    }
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "output": str(output),
        "scenario_count": report["scenario_count"],
        "fpcw_modes": report["fpcw_modes"],
        "execution_count": report["execution_count"],
        "failures": failures,
        "first_differential_pass": report["first_differential_pass"],
        "actual_differential_pass": report["actual_differential_pass"],
        "captured_outcome_negative_controls": report["captured_outcome_negative_controls"],
        "in_memory_target_byte_negative_control": report["in_memory_target_byte_negative_control"],
    }))
    if (failures or not report["captured_outcome_negative_controls"]["all_detected"]
            or not report["in_memory_target_byte_negative_control"].get("detected")):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
