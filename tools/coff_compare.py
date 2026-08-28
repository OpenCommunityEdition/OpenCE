"""Strict semantic comparator for COFF-i386 function COMDAT sections.

Vendored from the independently reviewed glm-coff-tooling repository at
commit 117ad3c. Ambiguous relocation identity is deliberately non-equal.
"""

import hashlib
import json
import struct
from pathlib import Path


# â”€â”€ COFF constants â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_REL32 = 0x0014
IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SYM_CLASS_EXTERNAL = 0x02
IMAGE_SYM_ABSOLUTE   = -1
IMAGE_SYM_UNDEFINED  = 0
COFF_HEADER_SIZE     = 20
SYMBOL_ENTRY_SIZE    = 18
RELOC_ENTRY_SIZE     = 10
SECTION_HEADER_SIZE  = 40

# Undefined externals that the VC7 linker resolves to absolute zero.  Their
# relocation records vanish from the linked image, so a csplit reconstruction
# cannot carry them even when the linked bytes are identical.
LINK_ABSOLUTE_ZERO_SYMBOLS = frozenset({"__except_list"})

# â”€â”€ helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def _c_string(data, strtab_off, strtab_len, name_off):
    if name_off < 4 or name_off >= strtab_len:
        raise CoffError("invalid string table offset for long name")
    abs_off = strtab_off + name_off
    if abs_off >= len(data):
        raise CoffError("invalid string table offset for long name")
    end = data.find(b"\0", abs_off)
    strtab_end = strtab_off + strtab_len
    if end == -1 or end >= strtab_end:
        raise CoffError("string table entry missing NUL terminator")
    return data[abs_off:end].decode("ascii", "replace")


class CoffError(ValueError):
    pass


def load(data):
    if isinstance(data, (str, Path)):
        data = Path(data).read_bytes()
    if len(data) < COFF_HEADER_SIZE:
        raise CoffError("truncated COFF header")
    machine, section_count, _, symbol_offset, symbol_count, optional_size, _ = \
        struct.unpack_from("<HHLLLHH", data, 0)
    if machine != 0x14C:
        raise CoffError(f"unsupported machine type 0x{machine:04X} (expected 0x014C i386)")
    if optional_size != 0:
        raise CoffError(f"optional header present (size={optional_size}), expected object file")
    string_offset = symbol_offset + symbol_count * SYMBOL_ENTRY_SIZE
    if string_offset > len(data):
        raise CoffError("symbol table truncated")
    strtab_len = 0
    if string_offset < len(data):
        if string_offset + 4 > len(data):
            raise CoffError("string table length field missing")
        strtab_len = struct.unpack_from("<L", data, string_offset)[0]
        if strtab_len < 4 or string_offset + strtab_len > len(data):
            raise CoffError("invalid string table length")

    sections = []
    offset = COFF_HEADER_SIZE
    for index in range(section_count):
        if offset + SECTION_HEADER_SIZE > len(data):
            raise CoffError("section header truncated")
        raw_name, _, _, size, raw_offset, reloc_offset, _, reloc_count, _, flags = \
            struct.unpack_from("<8sLLLLLLHHL", data, offset)
        sections.append({
            "index": index + 1,
            "name": raw_name.rstrip(b"\0").decode("ascii", "replace"),
            "size": size,
            "raw": raw_offset,
            "reloc": reloc_offset,
            "reloc_count": reloc_count,
            "flags": flags,
        })
        offset += SECTION_HEADER_SIZE
        # COFF represents uninitialized sections (normally .bss) with a
        # logical size but no file-backed payload.  MSVC emits
        # PointerToRawData == 0 for these sections, so their logical size can
        # legitimately exceed the object-file length.
        uninitialized = bool(flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        if not (uninitialized and raw_offset == 0) and raw_offset + size > len(data):
            raise CoffError(f"section {index + 1} raw data extends past file end")
        if reloc_count and reloc_offset + reloc_count * RELOC_ENTRY_SIZE > len(data):
            raise CoffError(f"section {index + 1} relocation table truncated")

    symbols = []
    by_index = {}
    idx = 0
    while idx < symbol_count:
        entry_offset = symbol_offset + idx * SYMBOL_ENTRY_SIZE
        if entry_offset + SYMBOL_ENTRY_SIZE > len(data):
            raise CoffError("symbol table entry truncated")
        zeroes, name_off = struct.unpack_from("<LL", data, entry_offset)
        if zeroes == 0:
            name = _c_string(data, string_offset, strtab_len, name_off)
        else:
            name = data[entry_offset:entry_offset + 8].rstrip(b"\0").decode("ascii", "replace")
        value, section_no, sym_type, storage, aux_count = \
            struct.unpack_from("<LhHBB", data, entry_offset + 8)
        if aux_count > 0 and idx + 1 + aux_count > symbol_count:
            raise CoffError("auxiliary symbol entries extend past symbol table")
        sym = {"index": idx, "name": name, "value": value,
               "section": section_no, "type": sym_type, "storage": storage}
        symbols.append(sym)
        by_index[idx] = sym
        idx += aux_count + 1

    return {"data": data, "sections": sections, "symbols": symbols, "by_index": by_index}


def symbol(obj, name):
    for item in obj["symbols"]:
        if item["name"] == name and item["section"] > 0:
            return item
    raise CoffError(f"section-relative symbol {name!r} not found")


def _infer_section_base(obj, fn_section_index):
    """Try to infer the image-base VA of *fn_section_index* by finding a
    symbol defined both as absolute and as section-relative in that section.
    Returns the inferred base or *None* when no such pair exists or when
    duplicate names make the proof ambiguous."""
    abs_syms = {}
    rel_syms = {}
    for s in obj["symbols"]:
        if s["section"] == IMAGE_SYM_ABSOLUTE:
            if s["name"] in abs_syms:
                return None
            abs_syms[s["name"]] = s["value"]
        elif s["section"] == fn_section_index:
            if s["name"] in rel_syms:
                return None
            rel_syms[s["name"]] = s["value"]
    for name, rel_val in rel_syms.items():
        abs_val = abs_syms.get(name)
        if abs_val is not None and abs_val >= rel_val:
            base = abs_val - rel_val
            for name2, rel_val2 in rel_syms.items():
                abs_val2 = abs_syms.get(name2)
                if abs_val2 is not None and (abs_val2 - rel_val2) != base:
                    return None
            return base
    return None


def _find_matching_rel_sym(obj, target, fn_sec_num, base_va):
    """Return the unique section-relative symbol that proves *target*
    (an absolute symbol) is a label within *fn_sec_num*.

    Requirements:
    - Exactly one absolute symbol with this name.
    - Exactly one section-relative symbol with this name,
      matching storage class and type.
    - Its value is consistent with *base_va*: abs_value - rel_value == base_va.

    Returns the matching section-relative symbol or *None* when proof
    is insufficient."""
    abs_all = [s for s in obj["symbols"]
               if s["name"] == target["name"]
               and s["section"] == IMAGE_SYM_ABSOLUTE]
    if len(abs_all) != 1:
        return None
    rel_all = [s for s in obj["symbols"]
               if s["name"] == target["name"]
               and s["section"] == fn_sec_num
               and s["storage"] == target["storage"]
               and s["type"] == target["type"]]
    if len(rel_all) != 1:
        return None
    if target["value"] - rel_all[0]["value"] != base_va:
        return None
    return rel_all[0]


def _section_bytes(obj, section):
    """Return the logical bytes of a COFF section.

    IMAGE_SCN_CNT_UNINITIALIZED_DATA sections may have a nonzero logical size
    with PointerToRawData == 0.  In that encoding the bytes are zero-filled by
    the loader; offset zero is the COFF header, not section data.
    """
    if (section["flags"] & IMAGE_SCN_CNT_UNINITIALIZED_DATA) \
            and section["raw"] == 0:
        return bytearray(section["size"])

    sec_end = section["raw"] + section["size"]
    if sec_end > len(obj["data"]):
        raise CoffError(
            f"section {section['index']} raw data extends past file end")
    return bytearray(obj["data"][section["raw"]:sec_end])


def _defined_noncode_destination(obj, source_section_number, target, addend):
    """Return a proven destination in another defined non-code section.

    csplit can encode a reference as an externally visible section anchor plus
    an addend while MSVC uses a local symbol at the destination.  Those forms
    are equivalent only when the nearest externally visible anchor at or below
    the destination is unique.  Section numbers alone are object-local and
    therefore are never used as cross-object identity.

    Ambiguous ownership, duplicate owner names, code sections, and destinations
    outside the target section deliberately remain symbolic.
    """
    target_section_number = target["section"]
    if target_section_number <= 0 \
            or target_section_number == source_section_number \
            or target_section_number > len(obj["sections"]):
        return None

    section = obj["sections"][target_section_number - 1]
    if section["flags"] & IMAGE_SCN_CNT_CODE:
        return None

    destination_offset = target["value"] + addend
    if not 0 <= destination_offset <= section["size"]:
        return None

    anchors = [
        item for item in obj["symbols"]
        if item["section"] == target_section_number
        and item["value"] <= destination_offset
        and item["storage"] == IMAGE_SYM_CLASS_EXTERNAL
        and item["type"] != 0x20
    ]
    if not anchors:
        # Compiler-generated local data such as an SEH scope table has no
        # external owner.  Admit a static owner only for the unambiguous
        # whole-section case; producer-specific names are still compared via
        # the relocation-normalized destination proof below.
        statics = [
            item for item in obj["symbols"]
            if item["section"] == target_section_number
            and item["storage"] == 3
            and item["type"] != 0x20
            and not item["name"].startswith(".")
        ]
        if len(statics) == 1 and statics[0]["value"] == 0:
            anchors = statics
        else:
            return None

    nearest_value = max(item["value"] for item in anchors)
    nearest = [item for item in anchors if item["value"] == nearest_value]
    if len(nearest) != 1:
        return None

    anchor = nearest[0]
    defined_with_anchor_name = [
        item for item in obj["symbols"]
        if item["section"] > 0 and item["name"] == anchor["name"]
    ]
    if len(defined_with_anchor_name) != 1:
        return None

    return [
        "defined-noncode",
        section["name"],
        anchor["name"],
        destination_offset - anchor["value"],
    ]


def _normalized_destination(obj, source_section_number, dest_section,
                            destination_offset):
    """Return fail-closed identity for a relocated non-code destination.

    MSVC can name an SEH scope table with compiler-local symbols while csplit
    re-anchors the same entries to their owning function.  Mask the relocated
    dwords and resolve every table relocation back into the referencing code
    section.  Any external, out-of-range, or otherwise unproved destination
    rejects this fallback.
    """
    data = _section_bytes(obj, dest_section)
    resolved = []
    source_section = obj["sections"][source_section_number - 1]
    for index in range(dest_section["reloc_count"]):
        offset = dest_section["reloc"] + index * RELOC_ENTRY_SIZE
        address, target_index, relocation_type = struct.unpack_from(
            "<LLH", obj["data"], offset)
        if target_index not in obj["by_index"]:
            return None
        if address + 4 > dest_section["size"]:
            return None
        target = obj["by_index"][target_index]
        addend = struct.unpack_from("<i", data, address)[0]
        if target["section"] != source_section_number:
            return None
        target_offset = target["value"] + addend
        if not 0 <= target_offset <= source_section["size"]:
            return None
        data[address:address + 4] = b"\0\0\0\0"
        resolved.append([
            address,
            relocation_type,
            "source-section",
            target_offset,
        ])
    return [
        dest_section["name"],
        destination_offset,
        hashlib.sha256(data).hexdigest(),
        resolved,
    ]


def section_info(obj, function_name):
    fn = symbol(obj, function_name)
    return section_info_by_number(obj, fn["section"])


def section_info_by_number(obj, fn_sec_num):
    """Describe one COFF section using the hardened relocation resolver.

    ``section_info`` remains the function-name convenience API.  Regression
    manifests also need identical, non-divergent semantics for data, BSS,
    directive, and debug sections, which may not have a unique public owner
    symbol.  This section-number entry point deliberately shares every proof
    rule with the function comparator.
    """
    if fn_sec_num <= 0 or fn_sec_num > len(obj["sections"]):
        raise CoffError(f"invalid section number {fn_sec_num}")
    section = obj["sections"][fn_sec_num - 1]
    raw = _section_bytes(obj, section)
    relocs = []
    base_va = _infer_section_base(obj, fn_sec_num)

    for ri in range(section["reloc_count"]):
        roff = section["reloc"] + ri * RELOC_ENTRY_SIZE
        address, target_index, reloc_type = \
            struct.unpack_from("<LLH", obj["data"], roff)
        if target_index not in obj["by_index"]:
            raise CoffError(f"relocation {ri} references non-existent symbol index {target_index}")
        target = obj["by_index"][target_index]

        if address + 4 > section["size"]:
            raise CoffError(
                f"relocation address {address} outside section {fn_sec_num} size {section['size']}")
        addend = struct.unpack_from("<i", raw, address)[0]
        raw[address:address + 4] = b"\0\0\0\0"

        if target["name"] in LINK_ABSOLUTE_ZERO_SYMBOLS \
                and target["section"] == 0:
            # The linker resolves this undefined external to zero and removes
            # the relocation.  The bytes remain part of normalized equality;
            # only the unobservable object-file relocation is omitted.
            continue

        sec_no = target["section"]
        raw_target = target["value"] + addend
        defined_destination = None

        def _within_section(val):
            return 0 <= val <= section["size"]

        if sec_no == fn_sec_num:
            if _within_section(raw_target):
                semantic_target = ["internal", raw_target]
            elif base_va is not None:
                candidate = raw_target - base_va
                if _within_section(candidate):
                    semantic_target = ["internal", candidate]
                else:
                    semantic_target = ["internal", raw_target]
            else:
                semantic_target = ["internal", raw_target]
        elif sec_no == IMAGE_SYM_ABSOLUTE:
            # Normalize to internal only when a unique, compatible
            # section-relative pair proves label identity.
            if base_va is not None and _find_matching_rel_sym(
                obj, target, fn_sec_num, base_va
            ) is not None:
                offset = raw_target - base_va
                if _within_section(offset):
                    semantic_target = ["internal", offset]
                else:
                    semantic_target = ["absolute", target["value"], addend]
            else:
                semantic_target = ["absolute", target["value"], addend]
        else:
            defined_destination = _defined_noncode_destination(
                obj, fn_sec_num, target, addend)
            if defined_destination is not None:
                semantic_target = defined_destination
            else:
                semantic_target = ["symbol", target["name"], addend]

        relocation = {
            "address": address,
            "type": reloc_type,
            "target": semantic_target,
        }
        if defined_destination is not None:
            relocation["symbolic_target"] = [
                "symbol", target["name"], addend]
            destination_section = obj["sections"][target["section"] - 1]
            if destination_section["reloc_count"]:
                normalized = _normalized_destination(
                    obj,
                    fn_sec_num,
                    destination_section,
                    target["value"] + addend,
                )
                if normalized is not None:
                    relocation["resolved_destination_normalized"] = normalized
        relocs.append(relocation)

    return {
        "size": section["size"],
        "relocation_count": len(relocs),
        "normalized_sha256": hashlib.sha256(raw).hexdigest(),
        "relocations": relocs,
    }


def relocation_infos_equal(left, right):
    """Compare relocation lists with pairwise, fail-closed alias handling.

    A defined non-code relocation carries both its proven section-relative
    destination and its original symbol spelling.  Two relocations match when
    their proven destinations match, or when both producers used exactly the
    same symbol name and addend.  The latter preserves ordinary external-symbol
    equality when one producer happens to define that symbol locally.
    """
    if len(left) != len(right):
        return False
    for left_item, right_item in zip(left, right):
        if left_item["address"] != right_item["address"] \
                or left_item["type"] != right_item["type"]:
            return False
        if left_item["target"] == right_item["target"]:
            continue

        left_symbolic = left_item.get("symbolic_target")
        if left_symbolic is None and left_item["target"][0] == "symbol":
            left_symbolic = left_item["target"]
        right_symbolic = right_item.get("symbolic_target")
        if right_symbolic is None and right_item["target"][0] == "symbol":
            right_symbolic = right_item["target"]
        if left_symbolic is not None and left_symbolic == right_symbolic:
            continue

        left_normalized = left_item.get("resolved_destination_normalized")
        right_normalized = right_item.get("resolved_destination_normalized")
        if left_normalized is not None and left_normalized == right_normalized:
            continue
        return False
    return True


def section_infos_equal(left, right):
    """Return strict pairwise equality for two section measurements."""
    return (
        left["size"] == right["size"]
        and left["relocation_count"] == right["relocation_count"]
        and left["normalized_sha256"] == right["normalized_sha256"]
        and relocation_infos_equal(left["relocations"], right["relocations"])
    )


def image_symbol_addresses(entries):
    """Build a fail-closed name-to-image-address map.

    A ``static`` COMDAT function emitted by several translation units can
    legitimately carry one name at several final addresses.  Such a name is
    unusable as address evidence: resolving through it would silently choose
    one instance.  Drop only names observed at distinct addresses; repeated
    records for the same name and address remain unambiguous.
    """
    addresses = {}
    ambiguous = set()
    for entry in entries:
        name = entry["name"]
        address = int(entry["file_offset"])
        if name in addresses and addresses[name] != address:
            ambiguous.add(name)
        addresses[name] = address
    for name in ambiguous:
        del addresses[name]
    return addresses


def section_info_resolved(obj, section_symbol_name, symbol_addresses):
    """Describe a symbol-owned section using final image destinations.

    This is intended for data sections recovered from an executable by
    csplit.  csplit may spell an internal pointer as ``previous_external +
    addend`` while MSVC spells the same pointer as ``section_symbol +
    offset``.  The encodings are equal only when the supplied, independently
    recovered image symbol addresses prove that both resolve to the same
    address.  Missing address evidence remains symbolic and therefore fails
    closed when the spellings differ.
    """
    owner = symbol(obj, section_symbol_name)
    section_number = owner["section"]
    section = obj["sections"][section_number - 1]
    if section_symbol_name not in symbol_addresses:
        raise CoffError(
            f"image address unavailable for section owner {section_symbol_name!r}")

    section_base = symbol_addresses[section_symbol_name] - owner["value"]
    raw = _section_bytes(obj, section)
    relocs = []
    for ri in range(section["reloc_count"]):
        relocation_offset = section["reloc"] + ri * RELOC_ENTRY_SIZE
        address, target_index, relocation_type = struct.unpack_from(
            "<LLH", obj["data"], relocation_offset)
        if target_index not in obj["by_index"]:
            raise CoffError(
                f"relocation {ri} references non-existent symbol index {target_index}")
        if address + 4 > section["size"]:
            raise CoffError(
                f"relocation address {address} outside section "
                f"{section_number} size {section['size']}")

        target = obj["by_index"][target_index]
        addend = struct.unpack_from("<i", raw, address)[0]
        raw[address:address + 4] = b"\0\0\0\0"

        if target["section"] == section_number:
            destination = [
                "address", section_base + target["value"] + addend]
        elif target["section"] == IMAGE_SYM_ABSOLUTE:
            destination = ["address", target["value"] + addend]
        elif target["name"] in symbol_addresses:
            destination = [
                "address", symbol_addresses[target["name"]] + addend]
        else:
            destination = ["symbol", target["name"], addend]

        relocs.append({
            "address": address,
            "type": relocation_type,
            "target": destination,
        })

    return {
        "size": section["size"],
        "relocation_count": section["reloc_count"],
        "normalized_sha256": hashlib.sha256(raw).hexdigest(),
        "relocations": relocs,
    }


# â”€â”€ COFF fixture builder (synthetic) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def build_coff(machine=0x14C, sections=None, symbols=None, strtab=None):
    """Build a minimal COFF object file in memory.
    *sections* is a list of dicts with keys: name, size, raw_data (bytes),
    reloc_count, reloc_data (bytes, optional).
    *symbols* is a list of dicts with keys: name, value, section, type,
    storage, aux_count.
    *strtab* is an optional bytes string table (including length field).
    """
    sec_payload = bytearray()
    sec_relocs = bytearray()
    sec_raw_offset = COFF_HEADER_SIZE + len(sections) * SECTION_HEADER_SIZE

    for i, sec in enumerate(sections):
        raw_data_bytes = sec.get("raw_data", b"") or b"\xcc" * sec["size"]
        if len(raw_data_bytes) < sec["size"]:
            raw_data_bytes += b"\xcc" * (sec["size"] - len(raw_data_bytes))
        sec_payload.extend(raw_data_bytes[:sec["size"]])

        reloc_bytes = sec.get("reloc_data", b"")
        sec_relocs.extend(reloc_bytes)

    # Symbol table
    sym_table = bytearray()
    strtab_entries = bytearray()
    strtab_offsets = {}
    string_pool_offset = 4  # first 4 bytes = total length

    for sym in (symbols or []):
        name = sym["name"].encode("ascii")
        if len(name) <= 8:
            short_name = name.ljust(8, b"\0")[:8]
            sym_table.extend(short_name)
        else:
            if name not in strtab_offsets:
                strtab_offsets[name] = string_pool_offset
                strtab_entries.extend(name + b"\0")
                string_pool_offset += len(name) + 1
            off = strtab_offsets[name]
            sym_table.extend(struct.pack("<LL", 0, off))
        sym_table.extend(struct.pack(
            "<LhHBB",
            sym.get("value", 0),
            sym.get("section", 0),
            sym.get("type", 0),
            sym.get("storage", 0x02),  # IMAGE_SYM_CLASS_EXTERNAL
            sym.get("aux_count", 0),
        ))

    # String table
    if strtab is not None:
        st_bytes = strtab
    else:
        st_len = 4 + len(strtab_entries)
        st_bytes = struct.pack("<L", st_len) + bytes(strtab_entries)

    reloc_field = bytes(sec_relocs)
    payload = bytes(sec_payload)

    # Calculate symbol table offset (after sections + relocs)
    sym_offset = sec_raw_offset + len(payload) + len(reloc_field)

    # Update relocation pointers to actual file offsets
    # (We rebuild section headers with correct offsets)
    sec_headers2 = bytearray()
    cum_data = 0
    cum_reloc = 0
    for i, sec in enumerate(sections):
        raw_start = sec_raw_offset + cum_data
        reloc_count = sec.get("reloc_count", len(sec.get("reloc_data", b"")) // 10)
        reloc_start = sec_raw_offset + len(payload) + cum_reloc if reloc_count else 0

        name_bytes = sec["name"].encode("ascii").ljust(8, b"\0")[:8]
        sec_headers2.extend(struct.pack(
            "<8sLLLLLLHHL",
            name_bytes,
            sec["size"],
            0,
            sec["size"],
            raw_start,
            reloc_start,
            0,
            reloc_count,
            0,
            sec.get("flags", 0xE0000060),
        ))
        cum_data += sec["size"]
        cum_reloc += reloc_count * RELOC_ENTRY_SIZE

    header = struct.pack(
        "<HHLLLHH",
        machine,
        len(sections),
        0,                    # TimeDateStamp
        sym_offset,
        len(symbols or []),
        0,                    # SizeOfOptionalHeader
        0,                    # Characteristics
    )

    return bytes(header) + bytes(sec_headers2) + payload + reloc_field + bytes(sym_table) + st_bytes


def make_section_raw(size, relocs, label_offsets=None):
    """Build raw section bytes with placeholder values at relocation addresses.
    *relocs*: list of (address, target_sym_index, reloc_type, addend_value).
    *label_offsets*: list of (name, offset) for diagnostics.
    Returns (raw_bytes, reloc_bytes).
    """
    raw = bytearray(b"\xcc" * size)
    reloc_buf = bytearray()
    for addr, sym_idx, rtype, addend_val in relocs:
        struct.pack_into("<i", raw, addr, addend_val)
        reloc_buf.extend(struct.pack("<LLH", addr, sym_idx, rtype))
    return bytes(raw), bytes(reloc_buf)


# â”€â”€ CLI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def main(argv=None):
    import argparse
    parser = argparse.ArgumentParser(
        description="Semantically compare COFF-i386 function sections")
    parser.add_argument("target")
    parser.add_argument("base")
    parser.add_argument("functions", nargs="+")
    args = parser.parse_args(argv)

    target_obj = load(args.target)
    base_obj = load(args.base)
    result = {"target": args.target, "base": args.base, "functions": {}}
    all_equal = True

    for name in args.functions:
        target_info = section_info(target_obj, name)
        base_info = section_info(base_obj, name)
        equal = section_infos_equal(target_info, base_info)
        all_equal &= equal
        result["functions"][name] = {"equal": equal, "target": target_info, "base": base_info}

    result["all_equal"] = all_equal
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if all_equal else 1)


if __name__ == "__main__":
    main()

