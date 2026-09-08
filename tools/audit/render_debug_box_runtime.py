"""Pinned actual-x86 replay of RenderDebug box, its faces, and its cache caller.

Each role executes its own box, quadrilateral, triangle, and private cache code.
Only genuine external assert/exit, clock/error, and rasterizer triangle sinks are
modeled. The finite corpus checks ordinary, inverted, degenerate, signed-zero and
overlapping read-only inputs, cache boundaries and assertion order. It is not an
exhaustive floating-point, GPU, final-link, or whole-object admission proof.

The existing 3D oracle supplies the reviewed COFF relocation loader and cache
model through a PRIVATE module instance. This file never changes that module on
disk or the instance imported by its original tests. Input objects are read-only;
negative controls alter host copies of target instructions before Unicorn mapping.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import sys

ROOT = next(p for p in Path(__file__).resolve().parents if (p / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

BASE_PATH = ROOT / "tools/audit/render_debug_three_runtime.py"
BASE_PIN = "fd66d5fd9df8df4f79d0ff98c526470c42d4c286f023545a8c0cdd6eda084a08"
spec = importlib.util.spec_from_file_location("_render_debug_box_private_base", BASE_PATH)
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)

BOX = "_render_debug_box"
QUAD = "_render_debug_quadrilateral"
TRIANGLE = "_render_debug_triangle"
FUNCTIONS = (BOX, QUAD, TRIANGLE, b.CACHE_ENTRY, b.CACHE_STRING)
TARGET_PIN = "d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0"
TARGET_FUNCTIONS = {
    BOX: (384, 15, "84117ae7cea9313a9233634e8b3951384145c74cb6559d1a6757a7ded2fdfc7d"),
    QUAD: (288, 26, "a4a08ca2262639129ff65d24d4797763a36f897ba439e56241851aa2a44addc4"),
    TRIANGLE: (224, 21, "a8fa95bfe2c5b27979c1ee7689b084a28b9fd2a9a7128331007b208946bd7c2d"),
}

b.FUNCTIONS = FUNCTIONS
b.FUNCTION_ADDRESSES = {name: b.CODE + index * 0x1000 for index, name in enumerate(FUNCTIONS)}
b.APIS = ("_display_assert", "_system_exit", "_rasterizer_debug_triangle", "_game_time_get",
          "_error", "_csstrcmp", "_csstrncpy", "_csstrlen")
b.STUBS = {name: b.STUB + index * 0x100 for index, name in enumerate(b.APIS)}
b.LITERALS.update({
    "??_C@_09MBGEJPJA@immediate?$AA@": b"immediate\0",
    "??_C@_06IGAKEMNE@point2?$AA@": b"point2\0",
    "??_C@_06JPBBHNJF@point3?$AA@": b"point3\0",
})
b.LITERAL_ADDRESSES = {name: b.LITERAL + index * 0x400 for index, name in enumerate(sorted(b.LITERALS))}

# January face order, independently decoded from the six call argument lists.
FACES = ((0, 2, 4, 6), (1, 3, 5, 7), (0, 1, 3, 2),
         (4, 5, 7, 6), (0, 1, 7, 6), (2, 3, 5, 4))


def cases():
    result = []
    geometries = {
        "ordinary": (-3., 4., -5., 6., -7., 8.),
        "asymmetric": (0.125, 32., -0.25, 0.5, -64., 2.),
        "inverted": (4., -3., 6., -5., 8., -7.),
        "plane": (2., 2., -5., 6., -7., 8.),
        "line": (2., 2., -1., -1., -7., 8.),
        "point": (2., 2., -1., -1., 0., 0.),
        "signed_zero": (-0., 0., 0., -0., -0., 0.),
    }
    for name, bounds in geometries.items():
        result.append({"name": "immediate_" + name, "immediate": True, "bounds": bounds})
    result += [
        {"name": "immediate_overlapping_inputs", "immediate": True, "overlap": True},
        {"name": "deferred_overlapping_inputs", "immediate": False, "overlap": True},
        {"name": "deferred_empty", "immediate": False},
        {"name": "deferred_same_tick", "immediate": False, "count": 9},
        {"name": "deferred_last_slot", "immediate": False, "count": 511},
        {"name": "deferred_full_warn", "immediate": False, "count": 512},
        {"name": "deferred_full_already_warned", "immediate": False, "count": 512, "cache_warning": 1},
        {"name": "deferred_new_tick", "immediate": False, "count": 511,
         "cache_time": 9, "game_time": 10, "second_time": 11, "pool": b"stale\0",
         "cache_warning": 1, "string_warning": 1},
        {"name": "deferred_signed_tick_wrap", "immediate": False, "count": 4,
         "cache_time": 0x7fff, "game_time": 0x8000, "second_time": 0x8001},
        {"name": "deferred_same_low_word_tick", "immediate": False, "count": 5,
         "cache_time": 0x8000, "game_time": 0x18000},
    ]
    for immediate in (False, True):
        for missing in ("bounds", "color", "both"):
            result.append({"name": f"null_{missing}_{int(immediate)}", "immediate": immediate, "null": missing})
    return [dict(case, function=BOX) for case in result]


def input_image(case):
    raw, blocks, locations = b.input_image(case)
    if case.get("overlap"):
        raw = bytearray(raw)
        locations["color"] = locations["bounds"] + 4
        offset = locations["color"] - b.INPUT
        raw[offset:offset + 16] = blocks["color"]
        start = locations["bounds"] - b.INPUT
        blocks["bounds"] = bytes(raw[start:start + 24])
        raw = bytes(raw)
    return raw, blocks, locations


def arguments(case, locations):
    null = case.get("null")
    bounds = 0 if null in ("bounds", "both") else locations["bounds"]
    color = 0 if null in ("color", "both") else locations["color"]
    return b.p32(int(case["immediate"])) + b.p32(bounds) + b.p32(color)


def box_points(raw):
    # Copy component bytes, including signed zero, without host FP arithmetic.
    x0, x1, y0, y1, z0, z1 = (raw[index:index + 4] for index in range(0, 24, 4))
    return tuple(a + c + d for a, c, d in (
        (x0, y0, z0), (x1, y0, z0), (x0, y0, z1), (x1, y0, z1),
        (x0, y1, z1), (x1, y1, z1), (x0, y1, z0), (x1, y1, z0)))


def triangle_event(points, color):
    return {"api": "rasterizer_debug_triangle", "points": [point.hex() for point in points], "color": color.hex()}


def expected(case, before, blocks):
    if case.get("null"):
        name = "bounds" if case["null"] in ("bounds", "both") else "color"
        return bytes(before), [
            {"api": "display_assert", "information": name, "file": b.SOURCE_TEXT[:-1].decode("ascii"),
             "line": 776 if name == "bounds" else 777, "fatal": 1},
            {"api": "system_exit", "code": 0xffffffff},
        ], [BOX], None
    if not case["immediate"]:
        after, events, index = b.cache_model(before, dict(case, function=b.BOX), blocks)
        if index is not None:
            start = b.POOL_SIZE + index * b.ENTRY_SIZE
            after[start:start + 2] = b.p16(6)  # Solid box, not box-outline tag7.
        return bytes(after), events, [BOX, b.CACHE_ENTRY], index
    points = box_points(blocks["bounds"])
    events = []
    trace = [BOX]
    for a, c, d, e in FACES:
        events.append(triangle_event((points[a], points[c], points[d]), blocks["color"]))
        events.append(triangle_event((points[a], points[d], points[e]), blocks["color"]))
        trace += [QUAD, TRIANGLE, TRIANGLE]
    return bytes(before), events, trace, None


class Loader(b.Loader):
    def __init__(self, label, path, code_patches=None):
        # Same reviewed mapper, but load only the actual box's reachable graph.
        self.label, self.path = label, Path(path)
        self.obj = cc.load(self.path)
        self.sections = b.function_sections(self.obj)
        self.code_patches = code_patches or {}
        self.uc = b.u.Uc(b.u.UC_ARCH_X86, b.u.UC_MODE_32)
        for base, size in ((b.CODE, 0x10000), (b.STUB, 0x10000), (b.LITERAL, 0x10000),
                           (b.CACHE - 0x100, 0x8000), (b.INPUT - 0x100, 0x11000), (b.STACK, 0x10000)):
            self.uc.mem_map(base, size)
        self.loaded, self.extents = set(), {}
        for address in b.STUBS.values():
            self.uc.mem_write(address, b"\xc3")
        self.uc.mem_write(b.STOP, b"\xcc")
        for name, payload in b.LITERALS.items():
            self.uc.mem_write(b.LITERAL_ADDRESSES[name], payload)
        self.function(BOX)
        for base in (b.CODE, b.STUB):
            self.uc.mem_protect(base, 0x10000, b.u.UC_PROT_READ | b.u.UC_PROT_EXEC)
        self.uc.mem_protect(b.LITERAL, 0x10000, b.u.UC_PROT_READ)
        self.uc.mem_protect(b.INPUT - 0x100, 0x11000, b.u.UC_PROT_READ)
        for base, size in ((b.CACHE - 0x100, 0x8000), (b.STACK, 0x10000)):
            self.uc.mem_protect(base, size, b.u.UC_PROT_READ | b.u.UC_PROT_WRITE)

    def run(self, case, control):
        uc = self.uc
        raw_input, blocks, locations = input_image(case)
        before = b.initial_cache(case)
        wanted_cache, wanted_events, wanted_trace, index = expected(case, before, blocks)
        uc.mem_write(b.CACHE - 64, b"\xa5" * 64 + bytes(before) + b"\xa5" * 64)
        uc.mem_write(b.INPUT - 64, b"\xa5" * 64 + raw_input + b"\xa5" * 64)
        stack = bytearray(b"\x5a" * 0x10000)
        at = b.SP - b.STACK
        payload = b.p32(b.STOP) + arguments(case, locations)
        stack[at:at + len(payload)] = payload
        uc.mem_write(b.STACK, bytes(stack))
        events, trace, snapshots = [], [], []
        state = {"exited": False, "time_calls": 0}

        def args(count):
            return struct.unpack("<" + "I" * count, uc.mem_read(uc.reg_read(b.UC_X86_REG_ESP) + 4, count * 4))

        def code_hook(emulator, address, size, unused):
            assert (address == b.STOP or address in b.STUBS.values() or
                    any(b.contains(start, length, address, size) for start, length in self.extents.items())), (
                        self.label, "unexpected executable address", address)
            for name, start in b.FUNCTION_ADDRESSES.items():
                if address == start:
                    trace.append(name)
            name = next((name for name, start in b.STUBS.items() if address == start), None)
            if name is None:
                return
            snapshots.append({"api": name, "cache_sha256": b.digest(bytes(uc.mem_read(b.CACHE, b.CACHE_SIZE)))})
            result = 0x76543210
            if name == "_rasterizer_debug_triangle":
                p0, p1, p2, color = args(4)
                assert all(b.contains(b.STACK + 64, b.SP - b.STACK - 64, point, 12) for point in (p0, p1, p2))
                assert b.contains(b.INPUT, 0x10000, color, 16)
                event = triangle_event([bytes(uc.mem_read(point, 12)) for point in (p0, p1, p2)],
                                       bytes(uc.mem_read(color, 16)))
                event["point_pointers"] = [self.pointer_region(point) for point in (p0, p1, p2)]
                event["color_pointer"] = self.pointer_region(color)
            elif name == "_game_time_get":
                key = "game_time" if not state["time_calls"] else "second_time"
                result = case.get(key, case.get("game_time", 10)) & 0xffffffff
                state["time_calls"] += 1
                event = {"api": "game_time_get", "result": result}
            elif name == "_error":
                priority, message = args(2)
                event = {"api": "error", "priority": priority, "message": self.cstring(message)}
            elif name == "_display_assert":
                information, file_name, line, fatal = args(4)
                event = {"api": "display_assert", "information": self.cstring(information),
                         "file": self.cstring(file_name), "line": line, "fatal": fatal}
            elif name == "_system_exit":
                event = {"api": "system_exit", "code": args(1)[0]}
                events.append(event)
                state["exited"] = True
                emulator.emu_stop()
                return
            else:
                raise AssertionError((self.label, "unexpected string-cache API", name))
            events.append(event)
            uc.reg_write(b.UC_X86_REG_EAX, result)
            uc.reg_write(b.UC_X86_REG_ECX, 0xa1b2c3d4)
            uc.reg_write(b.UC_X86_REG_EDX, 0xb2c3d4e5)

        def memory_hook(emulator, access, address, size, value, unused):
            if access == b.u.UC_MEM_WRITE:
                allowed = b.contains(b.CACHE, b.CACHE_SIZE, address, size) or b.contains(b.STACK + 64, b.SP - b.STACK - 64, address, size)
            else:
                allowed = any(b.contains(start, length, address, size) for start, length in (
                    (b.CACHE, b.CACHE_SIZE), (b.INPUT, 0x10000), (b.STACK + 64, 0x10000 - 128),
                    (b.LITERAL, 0x10000), (b.CODE, 0x10000)))
            assert allowed, (self.label, "forbidden memory access", access, address, size, value)

        code_handle = uc.hook_add(b.u.UC_HOOK_CODE, code_hook)
        memory_handle = uc.hook_add(b.u.UC_HOOK_MEM_READ | b.u.UC_HOOK_MEM_WRITE, memory_hook)
        try:
            uc.reg_write(b.UC_X86_REG_ESP, b.SP)
            uc.reg_write(b.UC_X86_REG_EFLAGS, 0x202)
            uc.reg_write(b.UC_X86_REG_FPCW, control)
            uc.reg_write(b.UC_X86_REG_FPTAG, 0xffff)
            uc.reg_write(b.UC_X86_REG_FPSW, 0)
            initial_tag = uc.reg_read(b.UC_X86_REG_FPTAG)
            for register, value in b.SAVED.items():
                uc.reg_write(register, value)
            uc.emu_start(b.FUNCTION_ADDRESSES[BOX], b.STOP, count=500000)
        finally:
            uc.hook_del(code_handle)
            uc.hook_del(memory_handle)

        returned = not bool(case.get("null"))
        assert state["exited"] == (not returned)
        if returned:
            assert uc.reg_read(b.UC_X86_REG_EIP) == b.STOP
            assert uc.reg_read(b.UC_X86_REG_ESP) == b.SP + 4
            assert all(uc.reg_read(register) == value for register, value in b.SAVED.items())
        assert uc.reg_read(b.UC_X86_REG_FPCW) == control
        assert uc.reg_read(b.UC_X86_REG_FPTAG) == initial_tag
        assert uc.reg_read(b.UC_X86_REG_FPSW) & 0x3800 == 0
        assert not uc.reg_read(b.UC_X86_REG_EFLAGS) & 0x400
        cache = bytes(uc.mem_read(b.CACHE - 64, b.CACHE_SIZE + 128))
        inputs = bytes(uc.mem_read(b.INPUT - 64, 0x10000 + 128))
        final_stack = bytes(uc.mem_read(b.STACK, 0x10000))
        assert cache[:64] == cache[-64:] == inputs[:64] == inputs[-64:] == b"\xa5" * 64
        assert inputs[64:-64] == raw_input
        assert final_stack[:64] == stack[:64] and final_stack[at:] == stack[at:]
        assert cache[64:-64] == wanted_cache, (self.label, case["name"], "cache mismatch")
        assert trace == wanted_trace, (self.label, case["name"], "trace mismatch", trace, wanted_trace)
        semantic_events = [{key: value for key, value in event.items() if key not in ("point_pointers", "color_pointer")}
                           for event in events]
        assert semantic_events == wanted_events, (self.label, case["name"], "event mismatch", semantic_events, wanted_events)
        entry = None if index is None else cache[64 + b.POOL_SIZE + index * b.ENTRY_SIZE:64 + b.POOL_SIZE + (index + 1) * b.ENTRY_SIZE].hex()
        return {"case": case["name"], "events": events, "trace": trace, "api_cache_snapshots": snapshots,
                "cache_sha256": b.digest(cache[64:-64]), "cache_entry_hex": entry,
                "returned": returned, "system_exit_nonreturn": state["exited"],
                "abi": {"esp_restored": True if returned else None,
                        "nonvolatiles_restored": True if returned else None,
                        "direction_flag_clear": True, "fpcw": control,
                        "x87_top_restored": True, "x87_tags_restored": True,
                        "input_immutable": True, "guards_unchanged": True}}


def validate_objects(paths):
    objects = {role: cc.load(path) for role, path in paths.items()}
    assert b.digest(objects["target"]["data"]) == TARGET_PIN
    target = objects["target"]
    metadata = {}
    for role, obj in objects.items():
        metadata[role] = {}
        storage = cc.symbol(obj, "_render_debug_globals")
        section = obj["sections"][storage["section"] - 1]
        assert storage["storage"] == 3 and storage["value"] == 0
        assert section["name"] == ".bss" and section["size"] == b.CACHE_SIZE
        assert section["reloc_count"] == 0 and b._section_aux_selection(obj, storage["section"]) == 0
        assert not any(cc._section_bytes(obj, section)), (role, "nonzero cache initializer")
        metadata[role]["_render_debug_globals"] = {
            "storage": storage["storage"], "value": storage["value"], "type": storage["type"],
            "size": section["size"], "flags": section["flags"], "selection": 0,
            "normalized_sha256": b.digest(bytes(cc._section_bytes(obj, section))),
        }
        for name in FUNCTIONS:
            symbol = cc.symbol(obj, name)
            assert symbol["value"] == 0 and symbol["type"] == 32
            assert symbol["storage"] == (3 if name in (b.CACHE_ENTRY, b.CACHE_STRING) else 2)
            info = cc.section_info(obj, name)
            if name in TARGET_FUNCTIONS:
                size, relocations, digest = TARGET_FUNCTIONS[name]
                assert (info["size"], info["relocation_count"], info["normalized_sha256"]) == (size, relocations, digest)
                assert cc.section_infos_equal(info, cc.section_info(target, name)), (role, name, "strict mismatch")
            metadata[role][name] = {"storage": symbol["storage"], "type": symbol["type"],
                                    "value": symbol["value"], "selection": b._section_aux_selection(obj, symbol["section"]), **info}
    return metadata


def negative_controls(path):
    controls = []
    # +0x6c: mov eax,[esi+4] loads x1. Loading y0 is a runnable wrong corner.
    # +0x16a: push6 sends the box cache tag. push7 would silently select outline.
    for name, offset, old, new, case_name, reason in (
        ("wrong_corner", 0x6e, 4, 8, "immediate_ordinary", "event mismatch"),
        ("wrong_cache_tag", 0x16b, 6, 7, "deferred_same_tick", "cache mismatch"),
    ):
        case = next(case for case in cases() if case["name"] == case_name)
        loader = Loader(name, path, {BOX: [(offset, old, new)]})
        try:
            loader.run(case, 0x037f)
        except AssertionError as error:
            detail = error.args[0]
            assert isinstance(detail, tuple) and detail[:3] == (name, case_name, reason), repr(error)
            controls.append({"name": name, "detected": True, "offset": offset, "old": old, "new": new, "reason": reason})
        else:
            raise AssertionError((name, "negative not detected"))
    return controls


def read_manifest(path):
    manifest = json.loads(Path(path).read_text(encoding="utf-8"))
    assert manifest["version"] == 1
    assert set(manifest["objects"]) == {"target", "first", "actual"}
    paths = {}
    for role, item in manifest["objects"].items():
        file = Path(item["path"])
        assert file.is_absolute() and re.fullmatch(r"[0-9a-f]{64}", item["sha256"])
        assert b.digest(file.read_bytes()) == item["sha256"], (role, "file pin mismatch")
        paths[role] = file
    return manifest, paths


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), "preserve prior report; use a new path"
    assert b.digest(BASE_PATH.read_bytes()) == BASE_PIN
    manifest, paths = read_manifest(args.manifest)
    metadata = validate_objects(paths)
    loaders = {role: Loader(role, path) for role, path in paths.items()}
    results = []
    for case in cases():
        for control in b.FPCW_MODES:
            outcomes = {role: loader.run(case, control) for role, loader in loaders.items()}
            assert outcomes["target"] == outcomes["first"] == outcomes["actual"], (case["name"], "differential mismatch")
            results.append({"case": case["name"], "fpcw": control, "target": outcomes["target"],
                            "first_equal": True, "actual_equal": True})
    controls = negative_controls(paths["target"])
    report = {"scope": __doc__, "harness_sha256": b.digest(Path(__file__).read_bytes()), "base_sha256": BASE_PIN,
              "manifest": manifest, "objects": metadata, "scenario_count": len(cases()),
              "accepted_execution_count": len(results) * len(loaders), "negative_execution_count": len(controls),
              "all_pass": True, "negative_controls": controls, "results": results}
    # Generated evidence output, never an input/source/object write.
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("scenario_count", "accepted_execution_count", "negative_execution_count", "all_pass")}))


if __name__ == "__main__":
    main()
