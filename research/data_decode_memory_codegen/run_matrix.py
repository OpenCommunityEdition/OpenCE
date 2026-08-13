"""Bounded legal-C codegen matrix for data_encoding::_data_decode_memory.

Production source is never edited.  Each variant is generated under this
research directory, compiled as the complete translation unit with XDK 3911,
and checked against every January-owned code section.  The production
admission workflow separately checks all owned data sections.
"""

from __future__ import annotations

import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
SOURCE = ROOT / "source" / "memory" / "data_encoding.c"
TARGET = ROOT / "build" / "split" / "source" / "memory" / "data_encoding.obj"
CL = ROOT / "xbox" / "bin" / "vc7" / "CL.Exe"
OUT = HERE / "objects"
SRC = HERE / "sources"
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402


INCLUDES = [
    "xbox/include", "source", "source/main", "source/cseries", "source/sound",
    "source/bink", "source/saved films", "source/saved games", "source/cache",
    "source/units", "source/text", "source/tag_files", "source/structures",
    "source/shell", "source/shaders", "source/scenario", "source/render",
    "source/rasterizer", "source/physics", "source/objects",
    "source/objects/widgets", "source/networking", "source/models",
    "source/memory", "source/memory/zlib", "source/math", "source/tool",
    "source/items", "source/interface", "source/input", "source/hs",
    "source/game", "source/effects", "source/editor", "source/dialogs",
    "source/devices", "source/cutscene", "source/camera", "source/bungie_net",
    "source/bitmaps", "source/ai",
]


def run(args: list[str]) -> str:
    return subprocess.run(
        args, cwd=ROOT, check=True, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    ).stdout


def replace_function(source: str, transform) -> str:
    start = source.index("void *data_decode_memory(\n")
    end = source.index("\nbyte data_decode_byte(\n", start)
    body = source[start:end]
    changed = transform(body)
    if changed == body:
        raise RuntimeError("variant did not change function")
    return source[:start] + changed + source[end:]


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"expected exactly one occurrence of {old!r}")
    return text.replace(old, new, 1)


def variants(base: str):
    yield "v00", "unchanged full-TU control", base

    def byte_result(body: str) -> str:
        return replace_once(body, "\tvoid *memory = NULL;\n", "\tbyte *memory = NULL;\n")
    yield "v01", "byte-pointer result local", replace_function(base, byte_result)

    def char_result(body: str) -> str:
        return replace_once(body, "\tvoid *memory = NULL;\n", "\tchar *memory = NULL;\n")
    yield "v02", "char-pointer result local", replace_function(base, char_result)

    def early_buffer(body: str) -> str:
        body = replace_once(
            body, "\tvoid *memory = NULL;\n\tlong memory_size;\n",
            "\tvoid *memory = NULL;\n\tlong memory_size;\n\tbyte *buffer;\n")
        body = replace_once(
            body, "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\n\tswitch",
            "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\tbuffer = state->buffer;\n\n\tswitch")
        return replace_once(body, "\t\tmemory = state->buffer+state->offset;\n",
                            "\t\tmemory = buffer+state->offset;\n")
    yield "v03", "buffer pointer born after asserts and live across switch", replace_function(base, early_buffer)

    def early_current(body: str) -> str:
        body = replace_once(
            body, "\tvoid *memory = NULL;\n\tlong memory_size;\n",
            "\tvoid *memory = NULL;\n\tlong memory_size;\n\tbyte *current;\n")
        body = replace_once(
            body, "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\n\tswitch",
            "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\tcurrent = state->buffer + state->offset;\n\n\tswitch")
        return replace_once(body, "\t\tmemory = state->buffer+state->offset;\n",
                            "\t\tmemory = current;\n")
    yield "v04", "current pointer born after asserts and live across switch", replace_function(base, early_current)

    def early_offset(body: str) -> str:
        body = replace_once(
            body, "\tvoid *memory = NULL;\n\tlong memory_size;\n",
            "\tvoid *memory = NULL;\n\tlong memory_size;\n\tlong offset;\n")
        body = replace_once(
            body, "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\n\tswitch",
            "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\toffset = state->offset;\n\n\tswitch")
        body = replace_once(body, "\tif (state->offset+memory_size<=state->buffer_size && !state->overflow)\n",
                            "\tif (offset+memory_size<=state->buffer_size && !state->overflow)\n")
        body = replace_once(body, "\t\tmemory = state->buffer+state->offset;\n",
                            "\t\tmemory = state->buffer+offset;\n")
        return replace_once(body, "\t\tstate->offset += memory_size;\n",
                            "\t\tstate->offset = offset + memory_size;\n")
    yield "v05", "offset born after asserts and live across switch", replace_function(base, early_offset)

    def split_pointer(body: str) -> str:
        body = replace_once(body, "\t\tmemory = state->buffer+state->offset;\n",
                            "\t\tmemory = state->buffer;\n\t\tmemory = (byte *)memory + state->offset;\n")
        return body
    yield "v06", "split result base load and offset addition", replace_function(base, split_pointer)

    def failure_first(body: str) -> str:
        old = """\tif (state->offset+memory_size<=state->buffer_size && !state->overflow)\n\t{\n\t\tmemory = state->buffer+state->offset;\n\t\tif (element_size!=1)\n\t\t{\n\t\t\tbyte_swap_memory(memory, count, element_size);\n\t\t}\n\t\tstate->offset += memory_size;\n\t}\n\telse\n\t{\n\t\tstate->overflow = TRUE;\n\t}\n"""
        new = """\tif (state->offset+memory_size>state->buffer_size || state->overflow)\n\t{\n\t\tstate->overflow = TRUE;\n\t}\n\telse\n\t{\n\t\tmemory = state->buffer+state->offset;\n\t\tif (element_size!=1)\n\t\t{\n\t\t\tbyte_swap_memory(memory, count, element_size);\n\t\t}\n\t\tstate->offset += memory_size;\n\t}\n"""
        return replace_once(body, old, new)
    yield "v07", "failure-first spelling of the final gate", replace_function(base, failure_first)

    def named_valid(body: str) -> str:
        body = replace_once(
            body, "\tvoid *memory = NULL;\n\tlong memory_size;\n",
            "\tvoid *memory = NULL;\n\tlong memory_size;\n\tboolean valid;\n")
        return replace_once(
            body, "\tif (state->offset+memory_size<=state->buffer_size && !state->overflow)\n",
            "\tvalid = state->offset+memory_size<=state->buffer_size && !state->overflow;\n\tif (valid)\n")
    yield "v08", "named final-gate boolean born after switch", replace_function(base, named_valid)

    def long_count_alias(body: str) -> str:
        body = replace_once(
            body, "\tvoid *memory = NULL;\n\tlong memory_size;\n",
            "\tvoid *memory = NULL;\n\tlong memory_size;\n\tlong decoded_count;\n")
        body = replace_once(
            body, "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\n\tswitch",
            "\tmatch_assert(\"c:\\\\halo\\\\SOURCE\\\\memory\\\\data_encoding.c\", 257, count>=0);\n\tdecoded_count = count;\n\n\tswitch")
        segment_start = body.index("\tswitch (element_size)")
        segment_end = body.index("\n\tif (state->offset+memory_size", segment_start)
        prefix, segment, suffix = body[:segment_start], body[segment_start:segment_end], body[segment_end:]
        segment = segment.replace("count", "decoded_count")
        suffix = suffix.replace("byte_swap_memory(memory, count, element_size)",
                                "byte_swap_memory(memory, (short)decoded_count, element_size)", 1)
        return prefix + segment + suffix
    yield "v09", "32-bit decoded-count lifetime across switch and swap", replace_function(base, long_count_alias)

    def initialized_zero(body: str) -> str:
        return replace_once(body, "\tlong memory_size;\n", "\tlong memory_size = 0;\n")
    yield "v10", "defined fallback: initialize memory_size to zero", replace_function(base, initialized_zero)

    def initialized_count(body: str) -> str:
        return replace_once(body, "\tlong memory_size;\n", "\tlong memory_size = count;\n")
    yield "v11", "defined fallback: initialize memory_size to count", replace_function(base, initialized_count)

    def default_zero(body: str) -> str:
        return replace_once(
            body,
            "\t\tsystem_exit(-1);\n\t\tbreak;\n",
            "\t\tsystem_exit(-1);\n\t\tmemory_size = 0;\n\t\tbreak;\n")
    yield "v12", "defined fallback: assign zero after system_exit", replace_function(base, default_zero)

    def default_early_return(body: str) -> str:
        return replace_once(
            body,
            "\t\tsystem_exit(-1);\n\t\tbreak;\n",
            "\t\tsystem_exit(-1);\n\t\treturn NULL;\n")
    yield "v13", "defined fallback: return NULL after system_exit", replace_function(base, default_early_return)

    def default_count(body: str) -> str:
        return replace_once(
            body,
            "\t\tsystem_exit(-1);\n\t\tbreak;\n",
            "\t\tsystem_exit(-1);\n\t\tmemory_size = count;\n\t\tbreak;\n")
    yield "v14", "defined fallback: assign count after system_exit", replace_function(base, default_count)


def normalized_bytes(obj: dict, name: str) -> bytes:
    sym = cc.symbol(obj, name)
    sec = obj["sections"][sym["section"] - 1]
    raw = cc._section_bytes(obj, sec)
    for index in range(sec["reloc_count"]):
        offset = sec["reloc"] + index * cc.RELOC_ENTRY_SIZE
        address = struct.unpack_from("<L", obj["data"], offset)[0]
        raw[address:address + 4] = b"\0\0\0\0"
    return bytes(raw)


def code_owners(obj: dict) -> list[str]:
    result = []
    seen = set()
    for sym in obj["symbols"]:
        if sym["section"] <= 0 or sym["section"] in seen:
            continue
        sec = obj["sections"][sym["section"] - 1]
        if not sec["flags"] & cc.IMAGE_SCN_CNT_CODE:
            continue
        owners = [item for item in obj["symbols"]
                  if item["section"] == sym["section"]
                  and item["storage"] == cc.IMAGE_SYM_CLASS_EXTERNAL]
        if owners:
            seen.add(sym["section"])
            result.append(owners[0]["name"])
    return result


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    SRC.mkdir(parents=True, exist_ok=True)
    base = SOURCE.read_text(encoding="utf-8")
    target = cc.load(TARGET)
    target_info = cc.section_info(target, "_data_decode_memory")
    target_bytes = normalized_bytes(target, "_data_decode_memory")
    functions = code_owners(target)
    results = []

    for ident, description, text in variants(base):
        source = SRC / f"data_encoding_{ident}.c"
        obj_path = OUT / f"data_encoding_{ident}.obj"
        source.write_text(text, encoding="utf-8")
        command = [str(CL), "/nologo", "/c", "/O2", "/Oy-", "/DDEBUG", "/Dxbox"]
        command.extend(f'/I"{ROOT / include}"' for include in INCLUDES)
        command.extend([f"/Fo{obj_path}", str(source)])
        compiler_output = run(command)
        obj = cc.load(obj_path)
        info = cc.section_info(obj, "_data_decode_memory")
        data = normalized_bytes(obj, "_data_decode_memory")
        differences = [i for i, pair in enumerate(zip(target_bytes, data)) if pair[0] != pair[1]]
        exact_functions = []
        nonexact_functions = []
        for name in functions:
            equal = cc.section_infos_equal(cc.section_info(target, name), cc.section_info(obj, name))
            (exact_functions if equal else nonexact_functions).append(name)
        results.append({
            "id": ident,
            "description": description,
            "size": info["size"],
            "relocations": info["relocation_count"],
            "normalized_sha256": info["normalized_sha256"],
            "exact": cc.section_infos_equal(target_info, info),
            "differing_normalized_bytes": len(differences) + abs(len(target_bytes) - len(data)),
            "first_normalized_difference": differences[0] if differences else None,
            "exact_siblings": len([name for name in exact_functions if name != "_data_decode_memory"]),
            "nonexact_functions": nonexact_functions,
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "compiler_output": compiler_output.strip(),
        })
        print(ident, info["normalized_sha256"][:12], info["size"],
              info["relocation_count"], results[-1]["differing_normalized_bytes"],
              results[-1]["first_normalized_difference"], nonexact_functions)

    (HERE / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
