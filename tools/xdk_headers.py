#!/usr/bin/env python3
"""Writes port/include/xdk/xdk_pdb.h from the January build's PDB.

The game was compiled against the Xbox SDK, and its debug information
(cachebeta.pdb, read by tools/pdb200_types.py) records every type, typedef
and enumeration it was compiled with, and the prototype of every library
function it links. This writes the ones the game and the native ports'
platform layer use (the names in port/include/xdk/pdb_names.txt), and what
they depend on, as C declarations:

    python tools/xdk_headers.py --pdb path/to/cachebeta.pdb

Every structure layout written is then checked, for each port's target,
against the sizes and offsets the PDB records; where it needs structure
packing to agree, the declaration gets it. Macros, inline functions and
anything else the PDB cannot describe are written by hand in the other
headers of port/include/xdk (see its README).
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pdb200_types import (  # noqa: E402
    BASIC, LF_ARRAY, LF_BITFIELD, LF_CLASS, LF_ENUM, LF_MODIFIER,
    LF_POINTER, LF_PROCEDURE, LF_STRUCTURE, LF_UNION, Aggregate, Member, Pdb, PdbError, numeric,
    u8, u16, u32,
)

ROOT = Path(__file__).resolve().parent.parent
XDK = ROOT / "port" / "include" / "xdk"
NAMES = XDK / "pdb_names.txt"
OUTPUT = XDK / "xdk_pdb.h"

# the targets whose layouts must agree with the PDB's (each port's ABI flags)
TARGETS = {
    "linux": ["--target=i686-linux-gnu", "-m32", "-malign-double", "-fshort-wchar"],
    "windows": ["--target=i686-pc-windows-msvc"],
    "android": ["--target=arm64_32-apple-watchos", "-fshort-wchar"],
}

AGGREGATE_LEAVES = (LF_STRUCTURE, LF_CLASS, LF_UNION, LF_ENUM)

# arrays whose bound a unit may choose (a Winsock fd_set holds FD_SETSIZE
# sockets, 64 unless the unit defines more, as the ports' prefixes do): the
# member's bound is written as the macro, whose default the layout check uses
MACRO_BOUNDS = {("fd_set", "fd_array"): ("FD_SETSIZE", 64)}


def anonymous(name: str) -> bool:
    return "__unnamed" in name or name.startswith("<")


class Writer:
    def __init__(self, pdb: Pdb):
        self.pdb = pdb
        self.definitions: Dict[Tuple[str, str], Aggregate] = {}  # (kind, name) -> definition
        self.forwards: Set[Tuple[str, str]] = set()
        self.enums: Dict[str, Aggregate] = {}
        self.typedefs: Dict[str, int] = {}
        self.data: Dict[str, int] = {}
        self.prototypes: Dict[str, int] = {}
        self.inline_in_sdk: Set[str] = set()
        self.packing: Dict[Tuple[str, str], int] = {}
        self.empty_classes: Set[Tuple[str, str]] = set()
        self.warnings: List[str] = []

    # ---------- what is needed

    def want(self, name: str) -> bool:
        pdb = self.pdb
        if name in pdb.typedefs:
            self.typedefs[name] = self._typedef_type(name)
            self._need_type(self.typedefs[name], by_value=True)
            return True
        for kind in ("struct", "union"):
            definition = pdb.definition(kind, name)
            if definition is not None:
                self._need_aggregate(definition)
                return True
        enum = self._enum_of(name)
        if enum is not None:
            self._need_aggregate(enum)
            return True
        if name in pdb.procedures:
            self.prototypes[name] = self._procedure_type(name)
            self._need_type(self.prototypes[name], by_value=False)
            return True
        if name in pdb.data:
            self.data[name] = pdb.data[name][0]
            self._need_type(self.data[name], by_value=True)
            return True
        return False

    def _typedef_choices(self, name: str) -> List[int]:
        """A typedef's types, best first: where units disagree, a C unit's
        (not a C++ class's, nor an unrelated structure that took the same
        name)."""
        def rank(index: int) -> Tuple[bool, bool]:
            item = self.pdb.aggregates.get(index)
            return (bool(item and item.cplusplus), bool(item and item.name == name))
        return sorted(self.pdb.typedefs[name], key=rank)

    def _typedef_type(self, name: str) -> int:
        choices = self._typedef_choices(name)
        rendered = {self.declaration(t, name) for t in choices}
        if len(rendered) > 1:
            self.warnings.append(f"typedef {name} has {len(rendered)} definitions; using "
                                 f"{self.declaration(choices[0], name)}")
        return choices[0]

    def _procedure_type(self, name: str) -> int:
        procedures = self.pdb.procedures[name]
        chosen = [p for p in procedures if not p.local] or procedures
        if all(p.local for p in procedures):
            # a static function in every unit that has it: the SDK's inline
            # functions, which the ports must implement themselves
            self.inline_in_sdk.add(name)
        by_prototype: Dict[str, int] = {}
        for p in chosen:
            if self.pdb.leaf(p.type) == LF_PROCEDURE:
                by_prototype.setdefault(self.prototype(name, p.type), p.type)
        if not by_prototype:
            raise PdbError(f"{name} has no procedure type in the PDB")
        if len(by_prototype) > 1:
            # a static copy's convention can differ where a unit took the
            # function's address; the most common (then stdcall) wins
            conventions = [self.pdb.procedure(p.type)[1] for p in chosen]
            best = max(by_prototype.values(),
                       key=lambda t: (conventions.count(self.pdb.procedure(t)[1]),
                                      self.pdb.procedure(t)[1] == "__stdcall"))
            self.warnings.append(f"{name} has {len(by_prototype)} prototypes; using {self.prototype(name, best)}")
            return best
        return next(iter(by_prototype.values()))

    def _enum_of(self, enumerator: str) -> Optional[Aggregate]:
        for item in self.pdb.aggregates.values():
            if item.kind == "enum" and not item.forward and any(n == enumerator for n, _ in item.enumerators):
                return item
        return None

    def _need_aggregate(self, item: Aggregate) -> None:
        if item.kind == "enum":
            if item.name not in self.enums:
                self.enums[item.name] = item
            return
        key = (item.kind, item.name)
        if anonymous(item.name):
            for member in self.pdb.members(item):
                self._need_type(member.type, by_value=True)
            return
        if key in self.definitions:
            return
        self.forwards.add(key)
        if not self.pdb.members(item):
            # a C++ class with no data (D3DDevice): opaque to C, an empty
            # class to C++ (whose units may derive from it)
            self.empty_classes.add(key)
            return
        self.definitions[key] = item
        for member in self.pdb.members(item):
            self._need_type(member.type, by_value=True)

    def _need_type(self, index: int, by_value: bool) -> None:
        pdb = self.pdb
        if index < 0x1000:
            return
        record = pdb.record(index)
        leaf = u16(record, 2)
        if leaf == LF_MODIFIER:
            self._need_type(u32(record, 4), by_value)
        elif leaf == LF_POINTER:
            self._need_type(u32(record, 4), by_value=False)
        elif leaf == LF_ARRAY:
            self._need_type(u32(record, 4), by_value)
        elif leaf == LF_BITFIELD:
            self._need_type(u32(record, 4), by_value)
        elif leaf == LF_PROCEDURE:
            returns, _, args, _ = pdb.procedure(index)
            for t in [returns, *args]:
                self._need_type(t, by_value=False)
        elif leaf in AGGREGATE_LEAVES:
            item = pdb.aggregates[index]
            definition = pdb.resolve(index)
            if self.c_alias(item):
                return
            if item.kind == "enum":
                if definition is None:
                    raise PdbError(f"enum {item.name} has no definition")
                self._need_aggregate(definition)
            elif anonymous(item.name):
                self._need_aggregate(definition or item)
            elif by_value and definition is not None:
                self._need_aggregate(definition)
            else:
                self.forwards.add((item.kind, item.name))

    # ---------- declarations

    def declaration(self, index: int, inner: str, indent: str = "") -> str:
        """C declaration of `inner` (a declarator) with type `index`."""
        pdb = self.pdb
        if index < 0x1000:
            base = BASIC.get(index & 0xFF)
            if base is None:
                raise PdbError(f"basic type {index:#x}")
            if base == "wchar_t":
                base = "unsigned short"  # the Xbox's 16-bit wchar_t, which C spells so
            return join(base, "*" + inner if (index >> 8) & 7 else inner)
        record = pdb.record(index)
        leaf = u16(record, 2)
        if leaf == LF_MODIFIER:
            target, attr = u32(record, 4), u16(record, 8)
            qualifiers = " ".join(q for bit, q in ((1, "const"), (2, "volatile")) if attr & bit)
            if pdb.leaf(target) == LF_POINTER:
                return self.declaration(target, join(qualifiers, inner), indent)
            return join(qualifiers, self.declaration(target, inner, indent))
        if leaf == LF_POINTER:
            target, attr = u32(record, 4), u32(record, 8)
            qualifiers = " ".join(q for bit, q in ((0x400, "const"), (0x200, "volatile")) if attr & bit)
            pointer = "*" + join(qualifiers, inner) if qualifiers else "*" + inner
            if pdb.leaf(target) == LF_PROCEDURE:
                returns, convention, args, variadic = pdb.procedure(target)
                convention = "" if convention == "__cdecl" else convention + " "
                return self.declaration(returns, f"({convention}{pointer})({self.arguments(args, variadic)})", indent)
            if pdb.leaf(target) == LF_ARRAY:
                pointer = f"({pointer})"
            return self.declaration(target, pointer, indent)
        if leaf == LF_ARRAY:
            element = u32(record, 4)
            size, _ = numeric(record, 12)
            element_size = pdb.size(element)
            count = size // element_size if element_size else 0
            return self.declaration(element, f"{inner}[{count}]", indent)
        if leaf == LF_PROCEDURE:
            returns, convention, args, variadic = pdb.procedure(index)
            convention = "" if convention == "__cdecl" else convention + " "
            return self.declaration(returns, f"{convention}{inner}({self.arguments(args, variadic)})", indent)
        if leaf in AGGREGATE_LEAVES:
            item = pdb.aggregates[index]
            alias = self.c_alias(item)
            if alias:
                return join(alias, inner)
            if anonymous(item.name):
                definition = pdb.resolve(index) or item
                body = self.body(definition, indent + "    ")
                return join(f"{item.kind} {{\n" + "\n".join(body) + f"\n{indent}}}", inner)
            return join(f"{item.kind} {item.name}", inner)
        raise PdbError(f"type {index:#x} (leaf {leaf:#x}) has no C declaration")

    def c_alias(self, item: Aggregate) -> Optional[str]:
        """For a C++ library's class that C units know by a typedef of
        another structure (D3DX's D3DXMATRIX, a _D3DMATRIX to C): the
        typedef's name, which prototypes use instead."""
        if item.kind == "enum" or self.pdb.definition(item.kind, item.name) is None:
            return None
        if not self.pdb.definition(item.kind, item.name).cplusplus or item.name not in self.pdb.typedefs:
            return None
        target = self.pdb.aggregates.get(self._typedef_choices(item.name)[0])
        if target is None or target.name == item.name:
            return None
        if item.name in self.typedefs:
            return item.name
        self.typedefs[item.name] = self._typedef_choices(item.name)[0]
        self._need_type(self.typedefs[item.name], by_value=True)
        return item.name

    def arguments(self, args: List[int], variadic: bool) -> str:
        if not args:
            return "..." if variadic else "void"
        text = ", ".join(self.declaration(a, "") for a in args)
        return text + ", ..." if variadic else text

    def prototype(self, name: str, index: int) -> str:
        return self.declaration(index, name)

    # ---------- aggregate bodies

    def member_size(self, member: Member) -> int:
        return self.pdb.size(member.type)

    def bitfield(self, member: Member) -> Optional[Tuple[int, int, int]]:
        """(underlying type, width, position) of a bit field member"""
        if self.pdb.leaf(member.type) != LF_BITFIELD:
            return None
        record = self.pdb.record(member.type)
        return u32(record, 4), u8(record, 8), u8(record, 9)

    def continues(self, previous: Member, member: Member) -> bool:
        """Whether a bit field shares its predecessor's storage unit."""
        a, b = self.bitfield(previous), self.bitfield(member)
        return bool(a and b and previous.offset == member.offset and b[2] > a[2])

    def body(self, item: Aggregate, indent: str) -> List[str]:
        if item.kind == "enum":
            lines = []
            for name, value in item.enumerators:
                lines.append(f"{indent}{name} = {literal(value)},")
            return lines
        members = self.pdb.members(item)
        return self.render(self.group(members, item.kind == "union"), indent, item.name)

    def group(self, members: List[Member], union: bool) -> list:
        """Members as the C declaration needs them: the PDB lists an
        anonymous structure's or union's members in their container, at
        their offsets, so overlapping runs are regrouped."""
        if union:
            runs: List[List[Member]] = []
            for member in members:
                if runs and (member.offset > runs[-1][-1].offset or self.continues(runs[-1][-1], member)):
                    runs[-1].append(member)
                else:
                    runs.append([member])
            return [("member", r[0]) if len(r) == 1 else ("struct", self.group(r, False)) for r in runs]
        items: list = []
        i = 0
        while i < len(members):
            member = members[i]
            if items and not self.continues(self._last(items), member) and member.offset < self._end(items):
                # an anonymous union: its first alternative is what the
                # output already holds from the member at this offset on
                start = next(n for n, it in enumerate(items) if self._first(it).offset == member.offset)
                alternatives = [items[start:]]
                del items[start:]
                union_start = member.offset
                union_end = max(self._end(alternatives[0]), member.offset + self.member_size(member))
                current = [("member", member)]
                i += 1
                while i < len(members):
                    other = members[i]
                    if other.offset == union_start and not self.continues(self._last(current), other):
                        alternatives.append(current)
                        current = [("member", other)]
                    elif other.offset >= union_end and not self.continues(self._last(current), other):
                        break
                    else:
                        current.append(("member", other))
                    union_end = max(union_end, other.offset + self.member_size(other))
                    i += 1
                alternatives.append(current)
                grouped = []
                for alternative in alternatives:
                    flat = [m for m in self._flatten(alternative)]
                    grouped.append(alternative[0] if len(alternative) == 1 else ("struct", self.group(flat, False)))
                items.append(("union", grouped))
                continue
            items.append(("member", member))
            i += 1
        return items

    def _flatten(self, items: list) -> List[Member]:
        result = []
        for item in items:
            if item[0] == "member":
                result.append(item[1])
            else:
                result.extend(self._flatten(item[1]))
        return result

    def _first(self, item) -> Member:
        return item[1] if item[0] == "member" else self._first(item[1][0])

    def _last(self, items) -> Member:
        last = items[-1]
        return last[1] if last[0] == "member" else self._last(last[1])

    def _end(self, items) -> int:
        return max(m.offset + self.member_size(m) for m in self._flatten(items))

    def render(self, items: list, indent: str, tag: str) -> List[str]:
        """Grouped members as lines; tag is the structure or union they
        belong to (for MACRO_BOUNDS)."""
        lines = []
        for item in items:
            if item[0] == "member":
                member = item[1]
                bits = self.bitfield(member)
                if bits:
                    lines.append(f"{indent}{self.declaration(bits[0], member.name, indent)} : {bits[1]};")
                else:
                    line = self.declaration(member.type, member.name, indent)
                    bound = MACRO_BOUNDS.get((tag, member.name))
                    if bound:
                        macro, default = bound
                        line, count = re.subn(rf"\[{default}\]$", f"[{macro}]", line)
                        if count != 1:
                            raise PdbError(f"{tag}.{member.name} is not an array of {default}")
                    lines.append(f"{indent}{line};")
            else:
                lines.append(f"{indent}{item[0]} {{")
                lines.extend(self.render(item[1], indent + "    ", tag))
                lines.append(f"{indent}}};")
        return lines

    # ---------- output

    def definition_order(self) -> List[Aggregate]:
        """Definitions, each after those it contains by value."""
        order: List[Aggregate] = []
        placed: Set[Tuple[str, str]] = set()

        def contained(item: Aggregate) -> List[Tuple[str, str]]:
            result = []
            for member in self.pdb.members(item):
                result.extend(self._by_value(member.type))
            return result

        def place(key: Tuple[str, str], stack: Set[Tuple[str, str]]) -> None:
            if key in placed or key not in self.definitions:
                return
            if key in stack:
                raise PdbError(f"{key} contains itself")
            for dependency in contained(self.definitions[key]):
                place(dependency, stack | {key})
            placed.add(key)
            order.append(self.definitions[key])

        for key in sorted(self.definitions, key=lambda k: k[1]):
            place(key, set())
        return order

    def _by_value(self, index: int) -> List[Tuple[str, str]]:
        pdb = self.pdb
        if index < 0x1000:
            return []
        leaf = pdb.leaf(index)
        record = pdb.record(index)
        if leaf in (LF_MODIFIER, LF_ARRAY, LF_BITFIELD):
            return self._by_value(u32(record, 4))
        if leaf in (LF_STRUCTURE, LF_CLASS, LF_UNION):
            item = pdb.aggregates[index]
            if anonymous(item.name):
                definition = pdb.resolve(index) or item
                return [k for m in pdb.members(definition) for k in self._by_value(m.type)]
            return [(item.kind, item.name)]
        return []

    def write(self) -> str:
        out = [
            "/*",
            "XDK_PDB.H",
            "",
            "The Xbox SDK types, enumerations and function prototypes the game and the",
            "platform layer use, as the January build's debug information",
            "(cachebeta.pdb) records them. Generated by tools/xdk_headers.py from the",
            "names in pdb_names.txt: do not edit, regenerate (see README.md).",
            "*/",
            "",
            "#ifndef HALO_XDK_PDB_H",
            "#define HALO_XDK_PDB_H",
            "",
            "/* ---------- enumerations */",
            "",
        ]
        for name in sorted(self.enums):
            item = self.enums[name]
            tag = "" if anonymous(name) else f" {name}"
            out += [f"enum{tag} {{", *self.body(item, "    "), "};", ""]
        out += ["/* ---------- structures and unions */", ""]
        for kind, name in sorted(self.forwards, key=lambda k: (k[1], k[0])):
            out.append(f"{kind} {name};")
        out.append("")
        if self.empty_classes:
            out.append("#ifdef __cplusplus")
            for kind, name in sorted(self.empty_classes, key=lambda k: k[1]):
                out.append(f"{kind} {name} {{}};")
            out += ["#endif", ""]
        for item in self.definition_order():
            pack = self.packing.get((item.kind, item.name))
            if pack:
                out.append(f"#pragma pack(push, {pack})")
            out += [f"{item.kind} {item.name} {{", *self.body(item, "    "), "};"]
            if pack:
                out.append("#pragma pack(pop)")
            out.append("")
        out += ["/* ---------- types */", ""]
        for name in sorted(self.typedefs):
            out.append(f"typedef {self.declaration(self.typedefs[name], name)};")
        out += ["", "/* ---------- data */", ""]
        for name in sorted(self.data):
            out.append(f"extern {self.declaration(self.data[name], name)};")
        out += ["", "/* ---------- functions */", ""]
        for name in sorted(n for n in self.prototypes if n not in self.inline_in_sdk):
            out.append(f"{self.prototype(name, self.prototypes[name])};")
        out += ["", "/* functions the SDK defined inline (see xdk_d3d8.h) */", ""]
        for name in sorted(n for n in self.prototypes if n in self.inline_in_sdk):
            out.append(f"{self.prototype(name, self.prototypes[name])};")
        out += ["", "#endif", ""]
        return "\n".join(out)

    # ---------- checking layouts against the PDB

    def layout_checks(self) -> Dict[Tuple[str, str], List[str]]:
        checks: Dict[Tuple[str, str], List[str]] = {}
        for key, item in self.definitions.items():
            kind, name = key
            lines = [f"_Static_assert(sizeof({kind} {name}) == {item.size}, \"{name} size\");"]
            for member in self.pdb.members(item):
                if not member.name or self.bitfield(member):
                    continue
                lines.append(f"_Static_assert(__builtin_offsetof({kind} {name}, {member.name}) == {member.offset}, "
                             f"\"{name} {member.name}\");")
            checks[key] = lines
        return checks

    def failing(self, header: str, target: List[str]) -> Set[Tuple[str, str]]:
        checks = self.layout_checks()
        with tempfile.TemporaryDirectory() as work:
            source = Path(work) / "layout.c"
            defaults = "".join(f"#define {macro} {default}\n" for macro, default in MACRO_BOUNDS.values())
            source.write_text(defaults + header + "\n" + "\n".join(line for lines in checks.values() for line in lines) + "\n")
            result = subprocess.run(["clang", *target, "-fms-extensions", "-std=gnu11", "-w", "-fsyntax-only",
                                     "-ferror-limit=0", str(source)], capture_output=True, text=True)
        if result.returncode and "static assertion failed" not in result.stderr and "static_assert" not in result.stderr:
            raise PdbError("the generated declarations do not compile:\n" + result.stderr[:4000])
        names = set(re.findall(r'static assertion failed[^"]*"([^" ]+)', result.stderr))
        return {key for key in checks if key[1] in names}

    def fix_layouts(self) -> None:
        """Packs each structure that needs it, until every layout agrees
        with the PDB on every target."""
        for _ in range(4):
            failures = set()
            for target in TARGETS.values():
                failures |= self.failing(self.write(), target)
            if not failures:
                return
            for key in failures:
                smaller = {8: 4, 4: 2, 2: 1}.get(self.packing.get(key, 8))
                if smaller is None:
                    raise PdbError(f"{key[1]} does not lay out as the PDB records")
                self.packing[key] = smaller
        raise PdbError("layouts still differ: " + " ".join(sorted(k[1] for k in failures)))


def join(left: str, right: str) -> str:
    if not left:
        return right
    if not right:
        return left
    return f"{left} {right}"


def literal(value: int) -> str:
    return f"{value:#x}" if value > 0xFFFF else str(value)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--pdb", type=Path, required=True, help="cachebeta.pdb of the January build")
    parser.add_argument("--names", type=Path, default=NAMES)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    writer = Writer(Pdb(args.pdb))
    missing = []
    for line in args.names.read_text(encoding="utf-8").splitlines():
        name = line.split("#", 1)[0].strip()
        if name and not writer.want(name):
            missing.append(name)
    if missing:
        sys.exit("not in the PDB: " + " ".join(missing))
    writer.fix_layouts()
    args.output.write_text(writer.write(), encoding="utf-8")
    for warning in writer.warnings:
        print("note:", warning, file=sys.stderr)
    print(f"{args.output}: {len(writer.enums)} enumerations, {len(writer.definitions)} structures and unions, "
          f"{len(writer.typedefs)} types, {len(writer.prototypes)} functions "
          f"({len(writer.inline_in_sdk)} the SDK defined inline), {len(writer.data)} data")
    return 0


if __name__ == "__main__":
    sys.exit(main())
