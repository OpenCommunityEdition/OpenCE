#!/usr/bin/env python3
"""Writes port/linux/game/mcc_fingerprint_fields.h, the fields of the tag
schemas' definitions that the MCC import's fingerprint
(port/linux/game/mcc_import_fingerprint.c) treats apart:

- string_fields: every char[N] field, whose bytes after the terminator are
  not compared;
- ignored_fields: the fields the game never reads (named unused*, pad*,
  padding* or garbage*: the disc's maps keep leftover bytes there and MCC
  fills some), and the words of a block, data or reference that are
  addresses, not values.

Each is {definition name, definition size, offset, size}: definition names
repeat across the schema files, and the size tells them apart.

    python tools/mcc_fingerprint_fields.py           # writes the header
    python tools/mcc_fingerprint_fields.py --check   # fails if it is not what it writes

The fields come from clang's record layouts (-fdump-record-layouts) of the
game's port/linux/game/tag_schema_*.c, compiled as the build compiles them:
it needs a configured build (python configure.py) whose game sources build
with clang for the game's 32-bit layout (the Windows or Linux build), and
their generated headers (ninja once).
"""

import glob
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCHEMAS = ROOT / "port" / "linux" / "game"
HEADER = SCHEMAS / "mcc_fingerprint_fields.h"
# (the game's 32-bit layout: the web build's wasm32 one is another)
BUILDS = ("build/windows/", "build/linux/")

LINE = re.compile(r"^\s*(\d+)(?::\d+-\d+)? \|( *)(.*)$")
SIZE = re.compile(r"\[sizeof=(\d+)")
CHAR = re.compile(r"^char\[(\d+)\]")
UNUSED = re.compile(r"^(unused|pad|padding|garbage)\w*$")
ENTRY = "static struct { char const *definition; long definition_size; long offset; long size; } const"


def layouts() -> str:
    """clang's record layouts of every tag_schema_*.c, as the build compiles them"""
    try:
        database = json.loads(subprocess.run(["ninja", "-t", "compdb"], cwd=ROOT, capture_output=True,
                                             text=True, check=True).stdout)
    except (OSError, subprocess.CalledProcessError, ValueError) as failure:
        sys.exit(f"mcc_fingerprint_fields.py: no compile commands from ninja -t compdb ({failure}): run configure.py")
    text = []
    for path in sorted(glob.glob(str(SCHEMAS / "tag_schema_*.c"))):
        name = Path(path).name
        entries = [entry for entry in database if Path(entry["file"]).name == name and
                   any(entry["output"].replace("\\", "/").startswith(build) for build in BUILDS)]
        for entry in entries:
            command = re.sub(r"\s-MMD\s+-MF\s+\S+", "", entry["command"])
            command = re.sub(r"\s-o\s+\S+", "", command)
            command += " -fsyntax-only -Xclang -fdump-record-layouts"
            result = subprocess.run(command, cwd=entry.get("directory", ROOT), shell=True, capture_output=True,
                                    text=True, encoding="latin-1")
            if result.returncode == 0:
                text.append(result.stdout)
                break
        else:
            sys.exit(f"mcc_fingerprint_fields.py: {name} does not compile with the Windows or Linux build's command "
                     "(configure.py, then ninja windows or ninja linux once for its generated headers)")
    return "".join(text)


def records(dump: str):
    """each record's fields (offset, dotted path, declaration) and its size,
    from the first layout of it the dump has"""
    fields, sizes = {}, {}
    current = None
    stack = []
    for line in dump.splitlines():
        if "Dumping AST Record Layout" in line:
            current = None
            continue
        match = SIZE.search(line)
        if match and current and current not in sizes:
            sizes[current] = int(match.group(1))
            continue
        match = LINE.match(line)
        if not match:
            continue
        offset, indent, text = int(match.group(1)), len(match.group(2)), match.group(3).strip()
        if current is None:
            current = text
            if current in fields:
                current = "#skip"
            fields.setdefault(current, [])
            stack = []
            continue
        if current == "#skip":
            continue
        depth = indent // 2
        del stack[depth - 1:]
        stack.append(text.split(" ")[-1])
        fields[current].append((offset, ".".join(stack), text))
    return fields, sizes


def definitions():
    """(name, structure) of each TAG_SCHEMA_DEFINITION in the schema files"""
    found = set()
    for path in glob.glob(str(SCHEMAS / "tag_schema_*.c")):
        text = Path(path).read_text(encoding="latin-1")
        for match in re.finditer(r"TAG_SCHEMA_DEFINITION\(\s*(\w+)\s*,\s*([^,]+?)\s*,", text):
            found.add((match.group(1), match.group(2).strip()))
    return sorted(found)


def each_definition(fields, sizes):
    for name, structure in definitions():
        key = structure if structure.startswith("struct ") else "struct " + structure
        rows = fields.get(key) or fields.get(structure)
        size = sizes.get(key) or sizes.get(structure)
        if rows and size:
            yield name, size, rows


def string_fields(fields, sizes):
    lines = []
    for name, size, rows in each_definition(fields, sizes):
        seen = set()
        for offset, path, text in rows:
            match = CHAR.match(text)
            if match and offset not in seen:
                seen.add(offset)
                lines.append(f'\t{{ "{name}", {size}, {offset}, {match.group(1)} }}, /* {path} */')
    return lines


def ignored_fields(fields, sizes):
    lines = []
    for name, size, rows in each_definition(fields, sizes):
        offsets = sorted(set(row[0] for row in rows)) + [size]
        seen = set()
        for offset, path, text in rows:
            leaf = path.split(".")[-1]
            if text.startswith("struct tag_data ") and (offset + 8, path) not in seen:
                # a data's file offset and address (its size stays)
                seen.add((offset + 8, path))
                lines.append(f'\t{{ "{name}", {size}, {offset + 8}, 12 }}, /* {path}.file_offset, address, definition */')
                continue
            if text.startswith("struct tag_block ") and (offset + 4, path) not in seen:
                # a block's address and definition (its count stays)
                seen.add((offset + 4, path))
                lines.append(f'\t{{ "{name}", {size}, {offset + 4}, 8 }}, /* {path}.address, definition */')
                continue
            if text.startswith("struct tag_reference ") and (offset + 4, path) not in seen:
                # a reference's name pointer: an address, not a value (the
                # tag it names is compared through its index)
                seen.add((offset + 4, path))
                lines.append(f'\t{{ "{name}", {size}, {offset + 4}, 4 }}, /* {path}.name */')
                continue
            if not UNUSED.match(leaf) or (offset, path) in seen:
                continue
            # a leaf: its size is up to the next field's offset
            following = [other for other in offsets if other > offset]
            length = (following[0] if following else size) - offset
            if length <= 0:
                continue
            seen.add((offset, path))
            lines.append(f'\t{{ "{name}", {size}, {offset}, {length} }}, /* {path}: {text} */')
    return lines


def header() -> str:
    fields, sizes = records(layouts())
    strings = string_fields(fields, sizes)
    ignored = ignored_fields(fields, sizes)
    return "\n".join([
        "/*",
        "MCC_FINGERPRINT_FIELDS.H",
        "",
        "The fields of the tag schemas' definitions the fingerprint (mcc_import_fingerprint.c)",
        "treats as strings, and those the game never reads, which it zeroes: made",
        "from the game's structures' layouts by tools/mcc_fingerprint_fields.py, which",
        "writes it again. Do not edit.",
        "*/",
        "",
        "/* char[N] fields of the schema definitions */",
        f"{ENTRY} string_fields[] = {{",
        *strings,
        "\t{ NULL, 0, 0, 0 }",
        "};",
        f"/* {len(strings)} fields */",
        "",
        "/* fields the game never reads, in the schema definitions */",
        f"{ENTRY} ignored_fields[] = {{",
        *ignored,
        "\t{ NULL, 0, 0, 0 }",
        "};",
        f"/* {len(ignored)} fields */",
        "",
    ])


def main() -> None:
    text = header()
    if sys.argv[1:] == ["--check"]:
        current = HEADER.read_bytes().replace(b"\r\n", b"\n").decode("utf-8")
        if current != text:
            sys.exit(f"{HEADER.relative_to(ROOT)} is not what tools/mcc_fingerprint_fields.py writes: run it")
        print(f"{HEADER.relative_to(ROOT)} is what it writes")
        return
    if sys.argv[1:]:
        sys.exit("usage: mcc_fingerprint_fields.py [--check]")
    HEADER.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {HEADER.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
