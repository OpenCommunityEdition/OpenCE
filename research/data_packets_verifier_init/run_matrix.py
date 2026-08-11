"""Compile bounded, fully-defined verifier variants against the January target.

This script never edits production source.  It snapshots data_packets.c into
this research directory, substitutes only the verifier's initialization/control
topology, compiles each copy with the repository XDK 3911 flags, and compares
_code_0010a5f0 with the hardened COFF comparator.
"""

from __future__ import annotations

import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CL = ROOT / "xbox" / "bin" / "vc7" / "CL.Exe"
OBJDUMP = Path(r"C:\Program Files\LLVM\bin\llvm-objdump.exe")
PRODUCTION = ROOT / "source" / "memory" / "data_packets.c"
TARGET = ROOT / "build" / "split" / "source" / "memory" / "data_packets.obj"
OUT = HERE / "objects"
SRC = HERE / "sources"
sys.path.insert(0, str(ROOT))

from tools import coff_compare  # noqa: E402


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


MARKER = "\tfield = fields;\n\ttotal_size = 0;\n"
COMMENT_RE = re.compile(
    r"\t/\* BUG \(original\): field_size is intentionally not initialized here\..*?\*/\n",
    re.S,
)


VARIANTS: list[tuple[str, str, str, str]] = [
    # id, description, declarations replacement (exact old block), pre-loop code
    ("v00", "defined zero after total_size initialization",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = 0;\n"),
    ("v01", "field_size initialized from total_size after total_size=0",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = total_size;\n"),
    ("v02", "declaration initializer zero",
     "\tshort field_size = 0;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n"),
    ("v03", "total_size declaration initializer then field_size copy",
     "\tshort total_size = 0;\n\tshort field_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\tfield_size = total_size;\n"),
    ("v04", "both declaration-initialized, field first",
     "\tshort field_size = 0;\n\tshort total_size = 0;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v05", "both declaration-initialized, total first",
     "\tshort total_size = 0;\n\tshort field_size = total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v06", "chained assignment after field binding",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\tfield_size = total_size = 0;\n"),
    ("v07", "chained assignment before field binding",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield_size = total_size = 0;\n\tfield = fields;\n"),
    ("v08", "total_size from literal, field_size from total in comma expression",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0, field_size = total_size;\n"),
    ("v09", "field pointer comma-bound with defined zero",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields, total_size = 0, field_size = total_size;\n"),
    ("v10", "derive field_size from constant comparison (defined zero)",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = (short)(fields == field ? total_size : 0);\n"),
    ("v11", "derive field_size from packet_size-independent boolean false",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = (short)(field != fields);\n"),
]

# Additional setup-only variants.  Kept separate so the first measured batch
# remains stable in results even as the bounded search is extended.
MORE_VARIANTS: list[tuple[str, str, str, str]] = [
    ("v12", "same declaration statement: total zero, field copy",
     "\tshort total_size = 0, field_size = total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v13", "same declaration statement: field zero, total copy",
     "\tshort field_size = 0, total_size = field_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v14", "field declared first; total initializer assigns field",
     "\tshort field_size;\n\tshort total_size = (field_size = 0);\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v15", "total declared first; field initializer copies total",
     "\tshort total_size = 0;\n\tshort field_size = total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n"),
    ("v16", "initialize only when loop can execute",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tif (field->type != _data_packet_field_end)\n\t\tfield_size = 0;\n"),
    ("v17", "initialize through zero-valued conditional",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = field->type == _data_packet_field_end ? total_size : 0;\n"),
    ("v18", "initialize from arithmetic identity total-total",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\ttotal_size = 0;\n\tfield_size = (short)(total_size - total_size);\n"),
    ("v19", "for-init declaration-equivalent assignments",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields, total_size = 0, field_size = 0;\n"),
    ("v20", "field zero first, then total copies it",
     "\tshort field_size;\n\tshort total_size;\n\tstruct data_packet_field *field;\n",
     "\tfield = fields;\n\tfield_size = 0;\n\ttotal_size = field_size;\n"),
]

# These replace the two scalar locals by a small zero-initialized aggregate.
# A four-byte aggregate initializer could, in principle, share the target's
# existing four-byte zero store and initialize both values without new code.
AGGREGATE_VARIANTS = [
    ("v21", "zeroed aggregate: total then field", "total_size", "field_size"),
    ("v22", "zeroed aggregate: field then total", "field_size", "total_size"),
    ("v23", "zeroed two-short array: total slot 0, field slot 1", "array", "array"),
]

# Control-flow variants make initialization conditional on the loop executing.
CONTROL_VARIANTS = [
    ("v24", "guarded do/while with field_size initialized inside guard"),
]


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def function_disasm(obj: Path) -> str:
    text = run([str(OBJDUMP), "-dr", str(obj)]).stdout
    match = re.search(
        r"(?ms)^_code_0010a5f0:\n(.*?)(?=^_[A-Za-z0-9?@].*?:\n|\Z)", text
    )
    return match.group(1) if match else text


def normalized_bytes(loaded: dict, name: str) -> bytes:
    owner = coff_compare.symbol(loaded, name)
    section = loaded["sections"][owner["section"] - 1]
    raw = coff_compare._section_bytes(loaded, section)  # same bytes section_info hashes
    for index in range(section["reloc_count"]):
        offset = section["reloc"] + index * coff_compare.RELOC_ENTRY_SIZE
        address = struct.unpack_from("<L", loaded["data"], offset)[0]
        raw[address:address + 4] = b"\0\0\0\0"
    return bytes(raw)


def create_source(base: str, declarations: str, setup: str) -> str:
    old_declarations = (
        "\tshort field_size;\n\tshort total_size;\n"
        "\tstruct data_packet_field *field;\n"
    )
    start = base.index("void code_0010a5f0(\n", base.index("/* ---------- private code */"))
    end = base.index("\nvoid code_0010a7c0(\n", start)
    prefix, body, suffix = base[:start], base[start:end], base[end:]
    if body.count(old_declarations) != 1 or body.count(MARKER) != 1:
        raise RuntimeError("verifier source shape changed; refusing ambiguous substitution")
    body = body.replace(old_declarations, declarations, 1)
    body = COMMENT_RE.sub("", body, count=1)
    body = body.replace(MARKER, setup, 1)
    return prefix + body + suffix


def replace_verifier(base: str, transform) -> str:
    start = base.index("void code_0010a5f0(\n", base.index("/* ---------- private code */"))
    end = base.index("\nvoid code_0010a7c0(\n", start)
    return base[:start] + transform(base[start:end]) + base[end:]


def aggregate_source(base: str, ident: str, first: str, second: str) -> str:
    def transform(body: str) -> str:
        body = COMMENT_RE.sub("", body, count=1)
        old = ("\tshort field_size;\n\tshort total_size;\n"
               "\tstruct data_packet_field *field;\n")
        if ident == "v23":
            declaration = ("\tshort sizes[2] = {0};\n"
                           "\tstruct data_packet_field *field;\n")
            body = body.replace(old, "@@DECLARATIONS@@\n", 1)
            body = body.replace("total_size", "sizes[0]")
            body = body.replace("field_size", "sizes[1]")
        else:
            declaration = (
                "\tstruct verifier_sizes\n\t{\n"
                f"\t\tshort {first};\n\t\tshort {second};\n"
                "\t} sizes = {0};\n\tstruct data_packet_field *field;\n"
            )
            body = body.replace(old, "@@DECLARATIONS@@\n", 1)
            body = body.replace("total_size", "sizes.total_size")
            body = body.replace("field_size", "sizes.field_size")
        body = body.replace("@@DECLARATIONS@@\n", declaration, 1)
        body = body.replace("\tfield = fields;\n\t" + ("sizes[0]" if ident == "v23" else "sizes.total_size") + " = 0;\n",
                            "\tfield = fields;\n", 1)
        return body
    return replace_verifier(base, transform)


def guarded_do_while_source(base: str) -> str:
    def transform(body: str) -> str:
        body = COMMENT_RE.sub("", body, count=1)
        body = body.replace(
            "\tfield = fields;\n\ttotal_size = 0;\n"
            "\twhile (field->type != _data_packet_field_end)\n\t{\n",
            "\tfield = fields;\n\ttotal_size = 0;\n"
            "\tif (field->type != _data_packet_field_end)\n\t{\n"
            "\t\tfield_size = 0;\n\t\tdo\n\t\t{\n",
            1,
        )
        needle = ("\t\tfield++;\n\t}\n\tif (field_count)\n")
        replacement = ("\t\tfield++;\n"
                       "\t\t} while (field->type != _data_packet_field_end);\n"
                       "\t}\n\tif (field_count)\n")
        if needle not in body:
            raise RuntimeError("loop tail changed")
        return body.replace(needle, replacement, 1)
    return replace_verifier(base, transform)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    SRC.mkdir(parents=True, exist_ok=True)
    base = PRODUCTION.read_text(encoding="utf-8")
    target_loaded = coff_compare.load(TARGET)
    target = coff_compare.section_info(target_loaded, "_code_0010a5f0")
    target_bytes = normalized_bytes(target_loaded, "_code_0010a5f0")
    results: list[dict[str, object]] = []

    sources: list[tuple[str, str, str, bool]] = [
        ("control_uninitialized", "copied current exact source (undefined control)", base, False)
    ]
    for ident, description, declarations, setup in VARIANTS + MORE_VARIANTS:
        sources.append((ident, description, create_source(base, declarations, setup), True))
    for ident, description, first, second in AGGREGATE_VARIANTS:
        sources.append((ident, description, aggregate_source(base, ident, first, second), True))
    sources.append(("v24", CONTROL_VARIANTS[0][1], guarded_do_while_source(base), True))

    for ident, description, source_text, is_defined in sources:
        source = SRC / f"data_packets_{ident}.c"
        obj = OUT / f"data_packets_{ident}.obj"
        source.write_text(source_text, encoding="utf-8")
        command = [str(CL), "/nologo", "/c", "/O2", "/Oy-", "/DDEBUG", "/Dxbox"]
        command += [f'/I"{ROOT / include}"' for include in INCLUDES]
        command += [f"/Fo{obj}", str(source)]
        compile_result = run(command)
        loaded = coff_compare.load(obj)
        info = coff_compare.section_info(loaded, "_code_0010a5f0")
        exact = coff_compare.section_infos_equal(target, info)
        candidate_bytes = normalized_bytes(loaded, "_code_0010a5f0")
        differences = [index for index, pair in enumerate(zip(target_bytes, candidate_bytes))
                       if pair[0] != pair[1]]
        disasm = function_disasm(obj)
        (OUT / f"data_packets_{ident}.disasm.txt").write_text(disasm, encoding="utf-8")
        results.append({
            "id": ident,
            "description": description,
            "defined": is_defined,
            "exact": exact,
            "size": info["size"],
            "relocation_count": info["relocation_count"],
            "normalized_sha256": info["normalized_sha256"],
            "target_size": target["size"],
            "target_relocation_count": target["relocation_count"],
            "target_normalized_sha256": target["normalized_sha256"],
            "differing_normalized_bytes": len(differences) + abs(len(target_bytes) - len(candidate_bytes)),
            "first_normalized_difference": differences[0] if differences else None,
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "compiler_output": compile_result.stdout.strip(),
        })
        print(ident, "EXACT" if exact else "different", info["size"],
              info["relocation_count"], info["normalized_sha256"][:16])

    (HERE / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
