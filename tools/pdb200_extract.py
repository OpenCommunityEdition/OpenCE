#!/usr/bin/env python3
"""Fail-closed advisory extractor for VC7 PDB 2.00 (MSF/JG) files.

This tool deliberately does not update project configuration.  It emits evidence
that a human or a separate audit can consume.  Malformed or incomplete records
are counted and dropped; addresses are accepted only when a PE section proves
that the complete range belongs to the image.
"""

from __future__ import annotations

import argparse
import json
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


MAGIC = b"Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0"
MISSING_STREAM = 0xFFFFFFFF

S_LABEL_V3 = 0x1105
S_LDATA_V3 = 0x110C
S_GDATA_V3 = 0x110D
S_PUB_V3 = 0x110E
S_LPROC_V3 = 0x110F
S_GPROC_V3 = 0x1110
S_LDATA_V2 = 0x1007
S_GDATA_V2 = 0x1008
S_PUB_V2 = 0x1009
S_LPROC_V2 = 0x100A
S_GPROC_V2 = 0x100B

LF_FIELDLIST_V2 = 0x1203
LF_CLASS_V3 = 0x1504
LF_STRUCTURE_V3 = 0x1505
LF_UNION_V3 = 0x1506
LF_ENUM_V3 = 0x1507
LF_MEMBER_V3 = 0x150D
LF_CLASS_V2 = 0x1004
LF_STRUCTURE_V2 = 0x1005
LF_UNION_V2 = 0x1006
LF_ENUM_V2 = 0x1007
LF_MEMBER_V2 = 0x1405
CV_PROP_FORWARD = 0x0080


class PdbError(ValueError):
    """The input cannot be interpreted without guessing."""


def _u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise PdbError("truncated uint16")
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise PdbError("truncated uint32")
    return struct.unpack_from("<I", data, offset)[0]


def _zname(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if offset < 0 or end < 0:
        raise PdbError("unterminated name")
    raw = data[offset:end]
    if not raw:
        raise PdbError("empty name")
    return raw.decode("latin-1")


def _pname(data: bytes, offset: int) -> str:
    if offset >= len(data):
        raise PdbError("truncated Pascal name")
    size = data[offset]
    if not size or offset + 1 + size > len(data):
        raise PdbError("truncated Pascal name")
    return data[offset + 1 : offset + 1 + size].decode("latin-1")


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    rva: int
    virtual_size: int
    raw_size: int
    raw_offset: int

    @property
    def extent(self) -> int:
        return self.virtual_size if self.virtual_size else self.raw_size


class PeImage:
    def __init__(self, data: bytes):
        if len(data) < 0x40 or data[:2] != b"MZ":
            raise PdbError("not a PE image")
        nt = _u32(data, 0x3C)
        if nt + 24 > len(data) or data[nt : nt + 4] != b"PE\0\0":
            raise PdbError("invalid PE header")
        count = _u16(data, nt + 6)
        optional_size = _u16(data, nt + 20)
        table = nt + 24 + optional_size
        if not count or table + count * 40 > len(data):
            raise PdbError("invalid PE section table")
        self.sections = []
        for number in range(count):
            offset = table + number * 40
            name = data[offset : offset + 8].split(b"\0", 1)[0].decode("latin-1")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, offset + 8)
            section = Section(number + 1, name, rva, virtual_size, raw_size, raw_offset)
            if section.extent:
                if section.rva + section.extent > 0x100000000:
                    raise PdbError("PE section RVA overflows")
                self.sections.append(section)
        ordered = sorted(self.sections, key=lambda item: item.rva)
        if any(left.rva + left.extent > right.rva for left, right in zip(ordered, ordered[1:])):
            raise PdbError("overlapping PE sections")

    def resolve(self, segment: int, offset: int, length: int) -> tuple[Section, int] | None:
        if length <= 0 or offset < 0:
            return None
        section = next((item for item in self.sections if item.index == segment), None)
        if section is None or offset + length > section.extent:
            return None
        return section, section.rva + offset

    def file_offset_to_rva(self, file_offset: int) -> int | None:
        for section in self.sections:
            if section.raw_offset <= file_offset < section.raw_offset + section.raw_size:
                return section.rva + file_offset - section.raw_offset
        return None


class Msf2:
    """Strict reader for the old JG/MSF stream container."""

    def __init__(self, data: bytes):
        if not data.startswith(MAGIC) or len(data) < 60:
            raise PdbError("not a PDB 2.00 JG file")
        self.data = data
        self.block_size, free, block_count, toc_size, toc_unknown = struct.unpack_from(
            "<IHHII", data, len(MAGIC)
        )
        if free >= block_count or self.block_size < 64 or self.block_size & (self.block_size - 1):
            raise PdbError("invalid MSF block geometry")
        if block_count * self.block_size > len(data) or not toc_size:
            raise PdbError("MSF claims blocks outside file")
        toc_count = (toc_size + self.block_size - 1) // self.block_size
        block_list_offset = len(MAGIC) + 16
        if block_list_offset + toc_count * 2 > self.block_size:
            raise PdbError("MSF root block list overflows header block")
        toc_blocks = struct.unpack_from(f"<{toc_count}H", data, block_list_offset)
        if any(block >= block_count for block in toc_blocks):
            raise PdbError("TOC references invalid block")
        toc = self._read_blocks(toc_blocks, toc_size)
        stream_count = _u32(toc, 0)
        if stream_count > 0x100000 or 4 + stream_count * 8 > len(toc):
            raise PdbError("invalid stream directory")
        entries = [struct.unpack_from("<II", toc, 4 + index * 8) for index in range(stream_count)]
        cursor = 4 + stream_count * 8
        self._streams: list[tuple[int, tuple[int, ...]]] = []
        for size, unknown in entries:
            count = 0 if size in (0, MISSING_STREAM) else (size + self.block_size - 1) // self.block_size
            if cursor + count * 2 > len(toc):
                raise PdbError("truncated stream block list")
            blocks = struct.unpack_from(f"<{count}H", toc, cursor) if count else ()
            cursor += count * 2
            for block in blocks:
                if block >= block_count:
                    raise PdbError("stream references invalid block")
            self._streams.append((size, blocks))
        if cursor != len(toc):
            raise PdbError("unconsumed MSF directory bytes")

    def _read_blocks(self, blocks: tuple[int, ...], size: int) -> bytes:
        result = bytearray()
        for block in blocks:
            start = block * self.block_size
            end = start + self.block_size
            if block < 0 or end > len(self.data):
                raise PdbError("block outside file")
            result.extend(self.data[start:end])
        if len(result) < size:
            raise PdbError("truncated stream")
        return bytes(result[:size])

    def stream(self, index: int) -> bytes | None:
        if index < 0 or index >= len(self._streams):
            raise PdbError("invalid stream index")
        size, blocks = self._streams[index]
        if size == MISSING_STREAM:
            return None
        return self._read_blocks(blocks, size)


def _records(data: bytes, start: int = 0, end: int | None = None):
    end = len(data) if end is None else min(end, len(data))
    cursor = start
    while cursor < end:
        if cursor + 2 > end:
            raise PdbError("truncated CodeView record length")
        size = _u16(data, cursor)
        stop = cursor + 2 + size
        if size < 2 or stop > end:
            raise PdbError("truncated CodeView record")
        yield data[cursor:stop]
        cursor = stop
    if cursor != end:
        raise PdbError("misaligned CodeView record stream")


def _numeric(data: bytes, offset: int) -> tuple[int, int]:
    leaf = _u16(data, offset)
    if leaf < 0x8000:
        return leaf, offset + 2
    formats = {0x8000: ("<b", 1), 0x8001: ("<h", 2), 0x8002: ("<H", 2),
               0x8003: ("<i", 4), 0x8004: ("<I", 4), 0x8009: ("<q", 8), 0x800A: ("<Q", 8)}
    if leaf not in formats:
        raise PdbError("unsupported numeric leaf")
    fmt, size = formats[leaf]
    if offset + 2 + size > len(data):
        raise PdbError("truncated numeric leaf")
    return struct.unpack_from(fmt, data, offset + 2)[0], offset + 2 + size


def parse_types(stream: bytes, stats: Counter) -> tuple[list[dict], dict[int, int]]:
    if len(stream) < 16:
        raise PdbError("truncated TPI header")
    version = _u32(stream, 0)
    if version < 19960000:
        first, last, type_size, header_size = _u16(stream, 4), _u16(stream, 6), _u32(stream, 8), 16
    else:
        if len(stream) < 20:
            raise PdbError("truncated modern TPI header")
        header_size, first, last, type_size = _u32(stream, 4), _u32(stream, 8), _u32(stream, 12), _u32(stream, 16)
    if first < 0x1000 or last < first or header_size + type_size > len(stream):
        raise PdbError("invalid TPI range")
    raw_records = list(_records(stream, header_size, header_size + type_size))
    if len(raw_records) != last - first:
        raise PdbError("TPI record count mismatch")
    records = {first + index: record for index, record in enumerate(raw_records)}
    widths: dict[int, int] = {}
    parsed: list[dict] = []
    pending: list[tuple[int, dict, int, int]] = []
    for index, record in records.items():
        leaf = _u16(record, 2)
        try:
            if leaf in (LF_STRUCTURE_V3, LF_CLASS_V3):
                props, field = _u16(record, 6), _u32(record, 8)
                size, name_at = _numeric(record, 20)
                item = {"type_name": _zname(record, name_at), "kind": "STRUCT", "size": size, "fields": []}
            elif leaf in (LF_STRUCTURE_V2, LF_CLASS_V2):
                props, field = _u16(record, 6), _u32(record, 8)
                size, name_at = _numeric(record, 20)
                item = {"type_name": _pname(record, name_at), "kind": "STRUCT", "size": size, "fields": []}
            elif leaf == LF_UNION_V3:
                props, field = _u16(record, 6), _u32(record, 8)
                size, name_at = _numeric(record, 12)
                item = {"type_name": _zname(record, name_at), "kind": "UNION", "size": size, "fields": []}
            elif leaf == LF_UNION_V2:
                props, field = _u16(record, 6), _u32(record, 8)
                size, name_at = _numeric(record, 12)
                item = {"type_name": _pname(record, name_at), "kind": "UNION", "size": size, "fields": []}
            elif leaf == LF_ENUM_V3:
                props, field, size = _u16(record, 6), _u32(record, 12), _basic_width(_u32(record, 8))
                if size is None:
                    raise PdbError("unknown enum width")
                item = {"type_name": _zname(record, 16), "kind": "ENUM", "size": size, "fields": []}
            elif leaf == LF_ENUM_V2:
                props, field, size = _u16(record, 6), _u32(record, 12), _basic_width(_u32(record, 8))
                if size is None:
                    raise PdbError("unknown enum width")
                item = {"type_name": _pname(record, 16), "kind": "ENUM", "size": size, "fields": []}
            else:
                continue
            stats["types_seen"] += 1
            if props & CV_PROP_FORWARD:
                stats["types_forward_dropped"] += 1
                continue
            if size <= 0 or size > 0x10000000:
                raise PdbError("invalid type size")
            widths[index] = size
            pending.append((index, item, field, size))
        except (PdbError, UnicodeError):
            stats["types_partial_dropped"] += 1
    for index, item, field_index, aggregate_size in pending:
        field_record = records.get(field_index)
        if field_record is None or _u16(field_record, 2) != LF_FIELDLIST_V2:
            stats["types_missing_fieldlist"] += 1
            parsed.append(item)
            continue
        cursor = 4
        fields = []
        valid = True
        while cursor < len(field_record):
            if field_record[cursor] >= 0xF0:
                padding = field_record[cursor] & 0x0F
                if not padding or cursor + padding > len(field_record):
                    valid = False
                    break
                expected = bytes(0xF0 | value for value in range(padding, 0, -1))
                if field_record[cursor : cursor + padding] != expected:
                    valid = False
                    break
                cursor += padding
                continue
            if cursor >= len(field_record):
                break
            leaf = _u16(field_record, cursor)
            if leaf not in (LF_MEMBER_V2, LF_MEMBER_V3):
                valid = False
                break
            try:
                type_index = _u32(field_record, cursor + 4)
                offset, name_at = _numeric(field_record, cursor + 8)
                if leaf == LF_MEMBER_V2:
                    name = _pname(field_record, name_at)
                    stop = name_at + 1 + field_record[name_at]
                else:
                    name = _zname(field_record, name_at)
                    stop = field_record.find(b"\0", name_at) + 1
                width = widths.get(type_index, _basic_width(type_index))
                if offset < 0 or offset >= aggregate_size or (width is not None and offset + width > aggregate_size):
                    raise PdbError("field outside aggregate")
                fields.append({"name": name, "offset": offset, "type": f"0x{type_index:x}", "width": width})
                cursor = stop
            except (PdbError, UnicodeError):
                valid = False
                break
        if valid:
            item["fields"] = fields
        else:
            stats["fieldlists_partial_dropped"] += 1
        parsed.append(item)
    return parsed, widths


def _basic_width(type_index: int) -> int | None:
    if type_index >= 0x1000:
        return None
    base = type_index & 0xFF
    mode = (type_index >> 8) & 0x7
    if mode:
        return 4
    return {0x03: 4, 0x10: 1, 0x11: 2, 0x12: 4, 0x13: 8, 0x20: 1, 0x21: 2,
            0x22: 4, 0x23: 8, 0x30: 1, 0x31: 2, 0x32: 4, 0x40: 4, 0x41: 8,
            0x68: 1, 0x69: 1, 0x72: 2, 0x73: 2, 0x74: 4, 0x75: 4, 0x76: 8,
            0x77: 8}.get(base)


def parse_symbols(data: bytes, pe: PeImage, widths: dict[int, int], stats: Counter,
                  start: int = 0, end: int | None = None, pascal: bool = False) -> list[dict]:
    result = []
    try:
        iterable = _records(data, start, end)
        for record in iterable:
            stats["symbol_records_seen"] += 1
            kind = _u16(record, 2)
            try:
                symbol = None
                if kind in (S_LPROC_V3, S_GPROC_V3):
                    length, type_index, offset, segment = _u32(record, 16), _u32(record, 28), _u32(record, 32), _u16(record, 36)
                    symbol = _make_symbol(record, 39, segment, offset, length, "FUNC", pe, False)
                elif kind in (S_LPROC_V2, S_GPROC_V2):
                    length, type_index, offset, segment = _u32(record, 16), _u32(record, 28), _u32(record, 32), _u16(record, 36)
                    symbol = _make_symbol(record, 39, segment, offset, length, "FUNC", pe, True)
                elif kind in (S_LDATA_V3, S_GDATA_V3):
                    type_index, offset, segment = _u32(record, 4), _u32(record, 8), _u16(record, 12)
                    symbol = _make_symbol(record, 14, segment, offset, widths.get(type_index, _basic_width(type_index)), "DATA", pe, False)
                elif kind in (S_LDATA_V2, S_GDATA_V2):
                    type_index, offset, segment = _u32(record, 4), _u32(record, 8), _u16(record, 12)
                    symbol = _make_symbol(record, 14, segment, offset, widths.get(type_index, _basic_width(type_index)), "DATA", pe, True)
                elif kind == S_LABEL_V3:
                    offset, segment = _u32(record, 4), _u16(record, 8)
                    symbol = _make_symbol(record, 11, segment, offset, 0, "LABEL", pe, False)
                elif kind == S_PUB_V2:
                    offset, segment = _u32(record, 8), _u16(record, 12)
                    symbol = _make_symbol(record, 14, segment, offset, 0, "LABEL", pe, True)
                elif kind == S_PUB_V3:
                    offset, segment = _u32(record, 8), _u16(record, 12)
                    symbol = _make_symbol(record, 14, segment, offset, 0, "LABEL", pe, False)
                if symbol is not None:
                    result.append(symbol)
                    stats["symbols_emitted"] += 1
            except (PdbError, UnicodeError, struct.error):
                stats["symbols_partial_dropped"] += 1
    except PdbError:
        stats["symbol_stream_tail_dropped"] += 1
    return result


def _make_symbol(record: bytes, name_at: int, segment: int, offset: int, length: int | None,
                 kind: str, pe: PeImage, pascal: bool) -> dict:
    if length is None or length < 0 or (length == 0 and kind != "LABEL"):
        raise PdbError("symbol has no authoritative length")
    name = _pname(record, name_at) if pascal else _zname(record, name_at)
    resolved = pe.resolve(segment, offset, max(length, 1))
    if resolved is None:
        raise PdbError("symbol outside image")
    section, rva = resolved
    return {"name": name, "rva": rva, "section": section.name, "length": length,
            "kind": kind, "source": "PDB2.00"}


def _mark_ambiguities(symbols: list[dict], stats: Counter) -> None:
    by_name = defaultdict(list)
    for symbol in symbols:
        by_name[symbol["name"]].append(symbol)
    for group in by_name.values():
        identities = {(item["rva"], item["length"], item["kind"]) for item in group}
        if len(group) > 1 and len(identities) > 1:
            for item in group:
                item["ambiguous"] = True
            stats["ambiguous_duplicate_symbols"] += len(group)
    ranges = [(item["rva"], item["rva"] + item["length"], index, item)
              for index, item in enumerate(symbols) if item["kind"] != "LABEL"]
    ranges.sort(key=lambda entry: (entry[0], entry[1], entry[2]))
    active: list[tuple[int, dict]] = []
    for start, stop, index, item in ranges:
        active = [(end, other) for end, other in active if end > start]
        for end, other in active:
            if item["name"] != other["name"] or item["rva"] != other["rva"] or item["length"] != other["length"]:
                item["ambiguous"] = other["ambiguous"] = True
                stats["overlapping_symbols"] += 2
        active.append((stop, item))


def _module_streams(dbi: bytes) -> tuple[int, int, list[tuple[int, int]]]:
    if len(dbi) < 64 or _u32(dbi, 0) != 0xFFFFFFFF:
        raise PdbError("unsupported DBI header")
    gsym = _u16(dbi, 20)
    module_size = _u32(dbi, 24)
    limit = 64 + module_size
    if limit > len(dbi):
        raise PdbError("truncated DBI module table")
    cursor, modules = 64, []
    while cursor < limit:
        if cursor + 64 > limit:
            raise PdbError("truncated DBI module record")
        stream = _u16(dbi, cursor + 34)
        symbol_size = _u32(dbi, cursor + 36)
        first_end = dbi.find(b"\0", cursor + 64, limit)
        second_end = dbi.find(b"\0", first_end + 1, limit) if first_end >= 0 else -1
        if first_end < 0 or second_end < 0:
            raise PdbError("truncated DBI module name")
        if stream != 0xFFFF and symbol_size >= 4:
            modules.append((stream, symbol_size))
        cursor = (second_end + 4) & ~3
    if cursor != limit:
        raise PdbError("misaligned DBI module table")
    return gsym, module_size, modules


def _delta(symbols: list[dict], config_path: Path, pe: PeImage) -> dict:
    existing = json.loads(config_path.read_text(encoding="utf-8"))
    known = defaultdict(list)
    for item in existing:
        rva = pe.file_offset_to_rva(item.get("file_offset", -1))
        if rva is not None:
            known[item.get("name", "")].append(rva)
    recovered = defaultdict(list)
    for item in symbols:
        recovered[item["name"]].append(item)
    confirmations, conflicts, additions = [], [], []
    for name, items in recovered.items():
        rvas = sorted(set(item["rva"] for item in items))
        if name not in known:
            additions.extend(items)
        else:
            matching = [rva for rva in rvas if rva in known[name]]
            nonmatching = [rva for rva in rvas if rva not in known[name]]
            if matching:
                confirmations.append({"name": name, "rvas": matching})
            if nonmatching:
                conflicts.append({"name": name, "pdb_rvas": nonmatching,
                                  "config_rvas": sorted(set(known[name])), "ambiguous": bool(matching)})
    return {"advisory_only": True, "confirmations": confirmations, "conflicts": conflicts,
            "additions": additions}


def extract(pdb_path: Path, exe_path: Path, config_path: Path, output: Path) -> dict:
    msf = Msf2(pdb_path.read_bytes())
    pe = PeImage(exe_path.read_bytes())
    stats = Counter()
    types_stream = msf.stream(2)
    dbi = msf.stream(3)
    if types_stream is None or dbi is None:
        raise PdbError("required PDB stream is missing")
    types, widths = parse_types(types_stream, stats)
    gsym, module_size, modules = _module_streams(dbi)
    symbols = []
    global_stream = msf.stream(gsym)
    if global_stream is not None:
        symbols.extend(parse_symbols(global_stream, pe, widths, stats))
    for stream_index, symbol_size in modules:
        stream = msf.stream(stream_index)
        if stream is None or len(stream) < symbol_size:
            stats["module_streams_partial_dropped"] += 1
            continue
        symbols.extend(parse_symbols(stream, pe, widths, stats, 4, symbol_size, pascal=True))
    _mark_ambiguities(symbols, stats)
    symbols.sort(key=lambda item: (item["rva"], item["name"], item["length"]))
    types.sort(key=lambda item: (item["type_name"], item["kind"], item["size"]))
    output.mkdir(parents=True, exist_ok=True)
    (output / "pdb_symbols.json").write_text(json.dumps(symbols, indent=2) + "\n", encoding="utf-8")
    (output / "pdb_types.json").write_text(json.dumps(types, indent=2) + "\n", encoding="utf-8")
    delta = _delta(symbols, config_path, pe)
    delta["record_coverage"] = dict(sorted(stats.items()))
    (output / "pdb_delta.json").write_text(json.dumps(delta, indent=2) + "\n", encoding="utf-8")
    return {"symbols": len(symbols), "types": len(types), "modules": len(modules), "stats": dict(stats)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pdb", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        report = extract(args.pdb, args.exe, args.config, args.output)
    except (OSError, PdbError, json.JSONDecodeError) as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
