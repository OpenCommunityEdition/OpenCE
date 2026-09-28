#!/usr/bin/env python3
"""Types, typedefs, constants and function prototypes from a VC7 PDB 2.00.

The January build's debug information (cachebeta.pdb) describes every type
its units were compiled with: the game's own and those of the libraries and
system headers it used. This module reads that description; tools/
xdk_headers.py writes the Xbox SDK declarations the native ports need from
it (port/include/xdk/README.md).

The container is read by tools/pdb200_extract.py's Msf2. Records use the
VC6/VC7 CodeView forms: 32-bit type indices and length-prefixed names.
"""

from __future__ import annotations

import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterator, List, Optional, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pdb200_extract import Msf2, PdbError, _module_streams  # noqa: E402

# type records
LF_MODIFIER = 0x1001
LF_POINTER = 0x1002
LF_ARRAY = 0x1003
LF_CLASS = 0x1004
LF_STRUCTURE = 0x1005
LF_UNION = 0x1006
LF_ENUM = 0x1007
LF_PROCEDURE = 0x1008
LF_MFUNCTION = 0x1009
LF_ARGLIST = 0x1201
LF_FIELDLIST = 0x1203
LF_BITFIELD = 0x1205

# field list entries
LF_BCLASS = 0x1400
LF_MEMBER = 0x1405
LF_ENUMERATE = 0x0403

# symbol records
S_CONSTANT = 0x1002
S_UDT = 0x1003
S_LDATA = 0x1007
S_GDATA = 0x1008
S_PUB = 0x1009
S_LPROC = 0x100A
S_GPROC = 0x100B

PROP_FORWARD = 0x0080

# CodeView calling conventions (LF_PROCEDURE)
CALLING_CONVENTIONS = {0x00: "__cdecl", 0x04: "__fastcall", 0x07: "__stdcall"}

# the basic types' C names (CodeView "special" type indices, low byte)
BASIC = {
    0x03: "void", 0x08: "long",  # HRESULT
    0x10: "signed char", 0x11: "short", 0x12: "long", 0x13: "__int64",
    0x20: "unsigned char", 0x21: "unsigned short", 0x22: "unsigned long",
    0x23: "unsigned __int64", 0x30: "unsigned char",
    0x40: "float", 0x41: "double", 0x42: "long double",
    0x68: "signed char", 0x69: "unsigned char", 0x70: "char", 0x71: "wchar_t",
    0x72: "short", 0x73: "unsigned short", 0x74: "int", 0x75: "unsigned int",
    0x76: "__int64", 0x77: "unsigned __int64",
}
BASIC_SIZE = {
    0x03: 0, 0x08: 4, 0x10: 1, 0x11: 2, 0x12: 4, 0x13: 8, 0x20: 1, 0x21: 2, 0x22: 4,
    0x23: 8, 0x30: 1, 0x40: 4, 0x41: 8, 0x42: 8, 0x68: 1, 0x69: 1, 0x70: 1, 0x71: 2,
    0x72: 2, 0x73: 2, 0x74: 4, 0x75: 4, 0x76: 8, 0x77: 8,
}


def u8(data: bytes, offset: int) -> int:
    return data[offset]


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def numeric(data: bytes, offset: int) -> Tuple[int, int]:
    """A CodeView numeric leaf: (value, offset after it)."""
    leaf = u16(data, offset)
    if leaf < 0x8000:
        return leaf, offset + 2
    formats = {0x8000: "<b", 0x8001: "<h", 0x8002: "<H", 0x8003: "<i", 0x8004: "<I",
               0x8009: "<q", 0x800A: "<Q"}
    if leaf not in formats:
        raise PdbError(f"unsupported numeric leaf {leaf:#x}")
    fmt = formats[leaf]
    return struct.unpack_from(fmt, data, offset + 2)[0], offset + 2 + struct.calcsize(fmt)


def pascal(data: bytes, offset: int) -> Tuple[str, int]:
    """A length-prefixed name: (name, offset after it)."""
    size = data[offset]
    return data[offset + 1:offset + 1 + size].decode("latin-1"), offset + 1 + size


def records(data: bytes, start: int, end: int) -> Iterator[bytes]:
    """CodeView records; stops at the first one that does not fit."""
    cursor = start
    while cursor + 4 <= end:
        size = u16(data, cursor)
        if size < 2 or cursor + 2 + size > end:
            return
        yield data[cursor:cursor + 2 + size]
        cursor += 2 + size


@dataclass
class Member:
    name: str
    type: int
    offset: int


@dataclass
class Aggregate:
    """A structure, union or enumeration."""
    index: int
    kind: str  # "struct", "union", "enum"
    name: str
    size: int
    forward: bool
    members: List[Member] = field(default_factory=list)
    enumerators: List[Tuple[str, int]] = field(default_factory=list)
    underlying: int = 0
    cplusplus: bool = False  # base classes, methods: a C++ view of the type
    bases: List[Tuple[int, int]] = field(default_factory=list)  # (type, offset)
    complete: bool = True  # False when the field list has entries this reader cannot skip


@dataclass
class Procedure:
    name: str
    type: int
    local: bool


class Pdb:
    def __init__(self, path: Path):
        msf = Msf2(path.read_bytes())
        self.types: Dict[int, bytes] = {}
        self._read_types(msf.stream(2))
        self.aggregates: Dict[int, Aggregate] = {}
        for index, record in self.types.items():
            if u16(record, 2) in (LF_STRUCTURE, LF_CLASS, LF_UNION, LF_ENUM):
                self.aggregates[index] = self._aggregate(index, record)
        self.typedefs: Dict[str, List[int]] = {}
        self.constants: Dict[str, List[Tuple[int, int]]] = {}
        self.data: Dict[str, List[int]] = {}
        self.procedures: Dict[str, List[Procedure]] = {}
        self.publics: Dict[str, Tuple[int, int]] = {}
        gsym, _, modules = _module_streams(msf.stream(3))
        stream = msf.stream(gsym)
        self._read_symbols(stream, 0, len(stream))
        for index, size in modules:
            stream = msf.stream(index)
            if stream is not None:
                self._read_symbols(stream, 4, min(size, len(stream)))

    # ---------- reading

    def _read_types(self, stream: bytes) -> None:
        header, first, last, size = u32(stream, 4), u32(stream, 8), u32(stream, 12), u32(stream, 16)
        for number, record in enumerate(records(stream, header, header + size)):
            self.types[first + number] = record
        if len(self.types) != last - first:
            raise PdbError("type record count mismatch")

    def _aggregate(self, index: int, record: bytes) -> Aggregate:
        leaf = u16(record, 2)
        props = u16(record, 6)
        if leaf == LF_ENUM:
            underlying, fields = u32(record, 8), u32(record, 12)
            name, _ = pascal(record, 16)
            item = Aggregate(index, "enum", name, BASIC_SIZE.get(underlying & 0xFF, 4), bool(props & PROP_FORWARD),
                             underlying=underlying)
        else:
            fields = u32(record, 8)
            if leaf == LF_UNION:
                size, at = numeric(record, 12)
            else:
                size, at = numeric(record, 20)
            name, _ = pascal(record, at)
            item = Aggregate(index, "union" if leaf == LF_UNION else "struct", name, size,
                             bool(props & PROP_FORWARD))
        if not item.forward and fields:
            self._fields(item, self.types.get(fields, b""))
        return item

    def _fields(self, item: Aggregate, record: bytes) -> None:
        if not record or u16(record, 2) != LF_FIELDLIST:
            return
        cursor = 4
        while cursor < len(record):
            if record[cursor] >= 0xF0:  # alignment padding
                cursor += record[cursor] & 0x0F
                continue
            leaf = u16(record, cursor)
            if leaf == LF_MEMBER:
                kind = u32(record, cursor + 4)
                offset, at = numeric(record, cursor + 8)
                name, cursor = pascal(record, at)
                item.members.append(Member(name, kind, offset))
            elif leaf == LF_ENUMERATE:
                value, at = numeric(record, cursor + 4)
                name, cursor = pascal(record, at)
                item.enumerators.append((name, value))
            elif leaf == LF_BCLASS:
                # a C++ view: the base class's members come first
                item.cplusplus = True
                base = u32(record, cursor + 4)
                offset, cursor = numeric(record, cursor + 8)
                item.bases.append((base, offset))
            else:
                item.cplusplus = True
                cursor = self._skip_cplusplus(record, cursor, leaf)
                if cursor is None:
                    item.complete = False
                    return

    def _skip_cplusplus(self, record: bytes, cursor: int, leaf: int) -> Optional[int]:
        """The offset after a C++-only field list entry (methods, nested
        types, virtual tables), or None for one this reader does not know."""
        if leaf in (0x1406, 0x1408):  # static member, nested type: attr, type, name
            return pascal(record, cursor + 8)[1]
        if leaf == 0x1407:  # overloaded method: count, method list, name
            return pascal(record, cursor + 8)[1]
        if leaf in (0x1409, 0x140A):  # virtual table pointer, friend class
            return cursor + 8
        if leaf == 0x140B:  # method: attr, type, [virtual table offset], name
            attr = u16(record, cursor + 2)
            at = cursor + 8 + (4 if (attr >> 2) & 7 in (4, 6) else 0)
            return pascal(record, at)[1]
        return None

    def _read_symbols(self, data: bytes, start: int, end: int) -> None:
        for record in records(data, start, end):
            kind = u16(record, 2)
            try:
                if kind == S_UDT:
                    name, _ = pascal(record, 8)
                    self.typedefs.setdefault(name, [])
                    if u32(record, 4) not in self.typedefs[name]:
                        self.typedefs[name].append(u32(record, 4))
                elif kind == S_CONSTANT:
                    value, at = numeric(record, 8)
                    name, _ = pascal(record, at)
                    entry = (u32(record, 4), value)
                    if entry not in self.constants.setdefault(name, []):
                        self.constants[name].append(entry)
                elif kind in (S_GDATA, S_LDATA):
                    name, _ = pascal(record, 14)
                    if u32(record, 4) not in self.data.setdefault(name, []):
                        self.data[name].append(u32(record, 4))
                elif kind in (S_GPROC, S_LPROC):
                    name, _ = pascal(record, 39)
                    procedure = Procedure(name, u32(record, 28), kind == S_LPROC)
                    known = self.procedures.setdefault(name, [])
                    if all(p.type != procedure.type for p in known):
                        known.append(procedure)
                elif kind == S_PUB:
                    name, _ = pascal(record, 14)
                    self.publics[name] = (u16(record, 12), u32(record, 8))
            except (IndexError, struct.error, PdbError, UnicodeError):
                continue

    # ---------- types

    def record(self, index: int) -> Optional[bytes]:
        return self.types.get(index)

    def leaf(self, index: int) -> int:
        record = self.types.get(index)
        return u16(record, 2) if record else 0

    def size(self, index: int) -> int:
        if index < 0x1000:
            if (index >> 8) & 7:
                return 4
            return BASIC_SIZE.get(index & 0xFF, 0)
        record = self.types[index]
        leaf = u16(record, 2)
        if leaf == LF_MODIFIER:
            return self.size(u32(record, 4))
        if leaf == LF_POINTER:
            return 4
        if leaf == LF_ARRAY:
            return numeric(record, 12)[0]
        if leaf in (LF_STRUCTURE, LF_CLASS, LF_UNION, LF_ENUM):
            item = self.aggregates[index]
            if item.forward:
                definition = self.definition(item.kind, item.name)
                return definition.size if definition else 0
            return item.size
        if leaf == LF_BITFIELD:
            return self.size(u32(record, 4))
        return 0

    def definition(self, kind: str, name: str) -> Optional[Aggregate]:
        """A tag's definition: a C unit's if there is one, else a C++
        unit's whose members are all known (see members())."""
        candidates = self.definitions(kind, name)
        plain = [a for a in candidates if not a.cplusplus]
        if plain:
            return plain[0]
        complete = [a for a in candidates if a.complete]
        return complete[0] if complete else None

    def definitions(self, kind: str, name: str) -> List[Aggregate]:
        return [a for a in self.aggregates.values() if a.name == name and a.kind == kind and not a.forward]

    def resolve(self, index: int) -> Optional[Aggregate]:
        """The definition behind an aggregate type index (forward
        references name their definition)."""
        item = self.aggregates.get(index)
        if item is None:
            return None
        return self.definition(item.kind, item.name) if item.forward or item.cplusplus else item

    def members(self, item: Aggregate) -> List[Member]:
        """Data members in declaration order, base classes' first (at their
        offsets): the layout a C declaration of the type needs."""
        result: List[Member] = []
        for base, offset in item.bases:
            definition = self.resolve(base)
            if definition is None:
                raise PdbError(f"base class {base:#x} of {item.name} has no definition")
            result.extend(Member(m.name, m.type, m.offset + offset) for m in self.members(definition))
        result.extend(item.members)
        return result

    def procedure(self, index: int) -> Tuple[int, str, List[int], bool]:
        """(return type, calling convention, argument types, variadic)"""
        record = self.types[index]
        if u16(record, 2) != LF_PROCEDURE:
            raise PdbError(f"type {index:#x} is not a procedure")
        returns, call, arglist = u32(record, 4), u8(record, 8), u32(record, 12)
        args_record = self.types[arglist]
        count = u32(args_record, 4)
        args = [u32(args_record, 8 + 4 * n) for n in range(count)]
        variadic = bool(args) and args[-1] == 0
        if variadic:
            args = args[:-1]
        if call not in CALLING_CONVENTIONS:
            raise PdbError(f"calling convention {call:#x}")
        return returns, CALLING_CONVENTIONS[call], args, variadic
