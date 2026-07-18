"""Tests for coff_compare: COFF-i386 semantic comparator with
function-local relocation normalization."""

import unittest
from tools import coff_compare


# â”€â”€ helper â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def _build(symbols, relocs, sec_size=0x200, sec_name=".text"):
    """Build a COFF with one section and the given symbols + relocations.
    *relocs* is a list of (address, sym_idx, type, addend_value).
    Returns the loaded COFF dict.
    """
    raw_bytes, reloc_bytes = coff_compare.make_section_raw(sec_size, relocs)
    secs = [{"name": sec_name, "size": sec_size,
             "raw_data": raw_bytes, "reloc_data": reloc_bytes}]
    data = coff_compare.build_coff(sections=secs, symbols=symbols)
    return coff_compare.load(data)


def _idx(symbols, name):
    """Return index of first symbol matching *name*."""
    for i, s in enumerate(symbols):
        if s["name"] == name:
            return i
    raise ValueError(f"symbol {name!r} not found")


# â”€â”€ common symbol table scaffold â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

ANCHOR_SYMS = [
    {"name": "func",     "value": 0,             "section": 1,  "type": 0, "storage": 2, "aux_count": 0},
    {"name": "_anchor",  "value": 0,             "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
    {"name": "_anchor",  "value": 0x00401000,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
]


# ====================================================================
# Tests
# ====================================================================

class TestCoffCompare(unittest.TestCase):

    # â”€â”€ 1. zero vs absolute-image placeholder â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_zero_vs_absolute_placeholder_internal(self):
        """Same internal label encoded via zero (section-relative)
        vs absolute-image placeholder (section-start + abs addend)."""
        symbols_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        symbols_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]

        # Producer A: zero placeholder, label symbol
        idx_a = _idx(symbols_a, "$L1")
        obj_a = _build(symbols_a, [(0x100, idx_a, 6, 0)])

        # Producer B: absolute-image placeholder, symbol=func (value=0)
        idx_func = _idx(symbols_b, "func")
        obj_b = _build(symbols_b, [(0x100, idx_func, 6, 0x00401050)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    def test_zero_vs_absolute_placeholder_absolute_sym(self):
        """Same internal label via absolute symbol (proven by matching
        section-relative entry) vs section-relative symbol."""
        # Object A: $L1 appears both section-relative (idx=3) and
        # absolute (idx=4).  Relocation references the absolute entry.
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x00401050,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        # Object B: $L1 is section-relative only (idx=3).
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 4, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    # â”€â”€ 2. genuinely different internal target must fail â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_different_internal_target_fails(self):
        """Two different internal label offsets produce not-equal."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x80, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        idx_a = _idx(syms_a, "$L1")
        idx_b = _idx(syms_b, "$L1")
        obj_a = _build(syms_a, [(0x100, idx_a, 6, 0)])
        obj_b = _build(syms_b, [(0x100, idx_b, 6, 0)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_different_internal_absolute_target_fails(self):
        """Different absolute-symbol values produce different section-
        relative offsets after base_va normalisation."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x00401050, "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x00401080, "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        idx_a = _idx(syms_a, "$L1")
        idx_b = _idx(syms_b, "$L1")
        obj_a = _build(syms_a, [(0x100, idx_a, 6, 0)])
        obj_b = _build(syms_b, [(0x100, idx_b, 6, 0)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    # â”€â”€ 3. different external symbol/addend must fail â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_different_external_symbol_fails(self):
        """Relocations referencing different external symbols differ."""
        syms_a = ANCHOR_SYMS + [
            {"name": "printf", "value": 0, "section": 0, "type": 0, "storage": 2, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "puts", "value": 0, "section": 0, "type": 0, "storage": 2, "aux_count": 0},
        ]
        idx_a = _idx(syms_a, "printf")
        idx_b = _idx(syms_b, "puts")
        obj_a = _build(syms_a, [(0x100, idx_a, 6, 0)])
        obj_b = _build(syms_b, [(0x100, idx_b, 6, 0)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_different_external_addend_fails(self):
        """Same external symbol but different addend bytes differ."""
        syms = ANCHOR_SYMS + [
            {"name": "printf", "value": 0, "section": 0, "type": 0, "storage": 2, "aux_count": 0},
        ]
        idx = _idx(syms, "printf")
        obj_a = _build(syms, [(0x100, idx, 6, 0)])
        obj_b = _build(syms, [(0x100, idx, 6, 42)])

        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    # â”€â”€ 4. unchanged exact objects still pass â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_identical_section_relative(self):
        """Two identical COFFs with section-relative relocations match."""
        syms = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        idx = _idx(syms, "$L1")
        obj_a = _build(syms, [(0x100, idx, 6, 0)])
        obj_b = _build(syms, [(0x100, idx, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    def test_identical_external(self):
        """Two identical COFFs with external relocations match."""
        syms = ANCHOR_SYMS + [
            {"name": "printf", "value": 0, "section": 0, "type": 0, "storage": 2, "aux_count": 0},
        ]
        idx = _idx(syms, "printf")
        obj_a = _build(syms, [(0x100, idx, 6, 0)])
        obj_b = _build(syms, [(0x100, idx, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    def test_identical_absolute_sym(self):
        """Two identical COFFs with absolute-symbol relocations match."""
        syms = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x00401050, "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        idx = _idx(syms, "$L1")
        obj_a = _build(syms, [(0x100, idx, 6, 0)])
        obj_b = _build(syms, [(0x100, idx, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    # â”€â”€ 5. malformed/truncated COFF fails clearly â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_truncated_header(self):
        """Less than 20 bytes raises CoffError."""
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(b"\x00" * 10)

    def test_truncated_section(self):
        """Valid header but section headers truncated."""
        data = coff_compare.build_coff(sections=[
            {"name": ".text", "size": 0x100, "raw_data": b"\xcc" * 0x100, "reloc_data": b""},
        ], symbols=ANCHOR_SYMS)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(data[:30])  # cut after header

    def test_uninitialized_section_without_raw_payload(self):
        """A .bss section has a logical size but no file-backed bytes."""
        import struct as _struct

        header = _struct.pack("<HHLLLHH", 0x14C, 1, 0, 60, 0, 0, 0)
        section = _struct.pack(
            "<8sLLLLLLHHL",
            b".bss\0\0\0\0",
            0,
            0,
            0x100000,
            0,
            0,
            0,
            0,
            0,
            coff_compare.IMAGE_SCN_CNT_UNINITIALIZED_DATA,
        )
        obj = coff_compare.load(header + section + _struct.pack("<L", 4))
        self.assertEqual(obj["sections"][0]["size"], 0x100000)
        self.assertEqual(obj["sections"][0]["raw"], 0)

    def test_initialized_section_without_raw_payload_is_rejected(self):
        """Only an explicitly uninitialized section may omit its payload."""
        import struct as _struct

        header = _struct.pack("<HHLLLHH", 0x14C, 1, 0, 60, 0, 0, 0)
        section = _struct.pack(
            "<8sLLLLLLHHL",
            b".data\0\0\0",
            0,
            0,
            0x100000,
            0,
            0,
            0,
            0,
            0,
            0x40,
        )
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(header + section + _struct.pack("<L", 4))

    def test_truncated_symbol_table(self):
        """Symbol table entry truncated."""
        syms = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        relocs = [(0x100, _idx(syms, "$L1"), 6, 0)]
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x200,
                       "raw_data": coff_compare.make_section_raw(0x200, relocs)[0],
                       "reloc_data": coff_compare.make_section_raw(0x200, relocs)[1]}],
            symbols=syms,
        )
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(data[:len(data) - 5])  # truncate at end

    def test_bad_machine_type(self):
        """Non-i386 machine raises CoffError."""
        syms = ANCHOR_SYMS
        secs = [{"name": ".text", "size": 0x100, "raw_data": b"\xcc" * 0x100, "reloc_data": b""}]
        data = coff_compare.build_coff(machine=0x14D, sections=secs, symbols=syms)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(data)

    def test_optional_header_present(self):
        """Optional header present raises CoffError."""
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x100,
                       "raw_data": b"\xcc" * 0x100, "reloc_data": b""}],
            symbols=ANCHOR_SYMS,
        )
        # corrupt optional_size field to non-zero
        data = bytearray(data)
        struct = __import__("struct")
        struct.pack_into("<H", data, 16, 0xE0)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(bytes(data))

    # â”€â”€ 6. absolute constant NOT normalised to internal â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_absolute_constant_not_normalized_to_internal(self):
        """An absolute symbol whose value lands inside the inferred VA
        range, but which appears ONLY as absolute (no section-relative
        counterpart), must NOT be normalised into an internal label."""
        syms_abs = ANCHOR_SYMS + [
            {"name": "CONST", "value": 0x00401050, "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_rel = ANCHOR_SYMS + [
            {"name": "$L1",   "value": 0x50,       "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        idx_const = _idx(syms_abs, "CONST")
        idx_label = _idx(syms_rel, "$L1")
        obj_abs = _build(syms_abs, [(0x100, idx_const, 6, 0)])
        obj_rel = _build(syms_rel, [(0x100, idx_label, 6, 0)])

        info_abs = coff_compare.section_info(obj_abs, "func")
        info_rel = coff_compare.section_info(obj_rel, "func")
        # The absolute constant stays ["absolute", 0x00401050, 0];
        # the section-relative becomes ["internal", 0x50].
        self.assertNotEqual(info_abs, info_rel)

    # â”€â”€ 7. malformed string table â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_string_table_length_field_missing(self):
        """Truncated string table length field raises CoffError."""
        import struct as _struct
        syms = ANCHOR_SYMS + [
            {"name": "longlabel", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x200, "raw_data": b"\xcc" * 0x200}],
            symbols=syms,
        )
        _, section_count, _, sym_off, sym_cnt, _, _ = \
            _struct.unpack_from("<HHLLLHH", data, 0)
        str_off = sym_off + sym_cnt * 18
        truncated = data[:str_off + 2]
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(truncated)

    def test_string_table_no_nul(self):
        """String table entry without NUL terminator raises CoffError."""
        import struct as _struct
        syms = ANCHOR_SYMS + [
            {"name": "longlabel", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x200, "raw_data": b"\xcc" * 0x200}],
            symbols=syms,
        )
        _, section_count, _, sym_off, sym_cnt, _, _ = \
            _struct.unpack_from("<HHLLLHH", data, 0)
        str_off = sym_off + sym_cnt * 18
        # The string table: 4-byte length + "longlabel\0"
        nul_pos = str_off + 4 + len("longlabel")
        mangled = bytearray(data)
        mangled[nul_pos] = 0x41
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(bytes(mangled))

    def test_string_table_bad_offset(self):
        """Long-name offset pointing past string table raises CoffError."""
        import struct as _struct
        syms = ANCHOR_SYMS + [
            {"name": "longlabel", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x200, "raw_data": b"\xcc" * 0x200}],
            symbols=syms,
        )
        # The longlabel symbol is the 4th symbol entry (idx=3).
        # Its name_off field is at bytes 4-7 of that entry.
        _, section_count, _, sym_off, sym_cnt, _, _ = \
            _struct.unpack_from("<HHLLLHH", data, 0)
        name_off_addr = sym_off + 3 * 18 + 4
        mangled = bytearray(data)
        _struct.pack_into("<L", mangled, name_off_addr, 0xFFFFFF00)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(bytes(mangled))

    # â”€â”€ 8. relocation with bad symbol index â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_relocation_bad_symbol_index(self):
        """Relocation referencing a non-existent symbol index raises CoffError."""
        syms = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        # 4 symbols (indices 0-3), so index 99 is out of range
        with self.assertRaises(coff_compare.CoffError):
            obj = _build(syms, [(0x100, 99, 6, 0)])
            coff_compare.section_info(obj, "func")

    # â”€â”€ 9. aux entry bounds â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_aux_entry_bounds_exceed_symbol_table(self):
        """Auxiliary symbols extending past the symbol table raise CoffError."""
        syms = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 10},
        ]
        with self.assertRaises(coff_compare.CoffError):
            _build(syms, [])

    # â”€â”€ 10. ambiguous-pair protection (6 negative) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_abs_reloc_diff_storage(self):
        """Same name but different storage class: not a valid pair."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x00401050,    "section": -1, "type": 0, "storage": 2, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 4, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_abs_reloc_diff_type(self):
        """Same name but different symbol type: not a valid pair."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0x20, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x00401050,    "section": -1, "type": 0,    "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0x20, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 4, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_abs_reloc_dup_section_relative(self):
        """Duplicate section-relative symbols with the same name:
        ambiguous pair refused."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x60,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x00401050,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 5, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_abs_reloc_dup_absolute(self):
        """Duplicate absolute symbols with the same name:
        ambiguous pair refused."""
        syms_a = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x00401050,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1", "value": 0x00401060,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 3, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_inconsistent_anchor_bases(self):
        """Two anchor pairs implying different section bases: refuse
        absolute-symbol normalisation (base_va becomes None)."""
        syms_a = [
            {"name": "func",     "value": 0,             "section": 1,  "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_anchor",  "value": 0,             "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor",  "value": 0x00401000,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor2", "value": 0x200,         "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor2", "value": 0x00403000,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1",      "value": 0x00401050,   "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 5, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    def test_abs_constant_same_name_as_local(self):
        """Absolute constant sharing name with an unrelated local label
        (values inconsistent with base_va) must not be normalised."""
        syms_a = ANCHOR_SYMS + [
            {"name": "FOO", "value": 0x80,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "FOO", "value": 0x00401050,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "FOO", "value": 0x80,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 4, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertNotEqual(info_a, info_b)

    # â”€â”€ 11. valid-pair positive tests (2) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_unique_compatible_abs_rel_normalizes(self):
        """A different symbol name with a unique compatible abs/rel
        pair still normalizes correctly."""
        syms_a = ANCHOR_SYMS + [
            {"name": "mylabel",   "value": 0x80,       "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "mylabel",   "value": 0x00401080, "section": -1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "mylabel",   "value": 0x80,       "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 4, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    def test_multiple_anchor_pairs_same_base(self):
        """Multiple distinct anchor pairs all implying the same base
        are accepted; absolute-symbol normalisation still works."""
        syms_a = [
            {"name": "func",     "value": 0,             "section": 1,  "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_anchor",  "value": 0,             "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor",  "value": 0x00401000,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor2", "value": 0x100,         "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
            {"name": "_anchor2", "value": 0x00401100,    "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1",      "value": 0x00401050,   "section": -1, "type": 0, "storage": 3, "aux_count": 0},
            {"name": "$L1",      "value": 0x50,          "section": 1,  "type": 0, "storage": 3, "aux_count": 0},
        ]
        syms_b = ANCHOR_SYMS + [
            {"name": "$L1", "value": 0x50, "section": 1, "type": 0, "storage": 3, "aux_count": 0},
        ]
        obj_a = _build(syms_a, [(0x100, 6, 6, 0)])
        obj_b = _build(syms_b, [(0x100, 3, 6, 0)])
        info_a = coff_compare.section_info(obj_a, "func")
        info_b = coff_compare.section_info(obj_b, "func")
        self.assertEqual(info_a, info_b)

    # â”€â”€ 12. bounds audit â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    def test_bounds_symbol_count_huge(self):
        """Huge symbol count producing offset past file end raises CoffError."""
        import struct as _struct
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x100, "raw_data": b"\xcc" * 0x100}],
            symbols=ANCHOR_SYMS,
        )
        mangled = bytearray(data)
        # symbol_count is at header offset 12
        _struct.pack_into("<L", mangled, 12, 0xFFFF)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(bytes(mangled))

    def test_bounds_section_size_exceeds_file(self):
        """Section size field exceeding available data raises CoffError."""
        import struct as _struct
        data = coff_compare.build_coff(
            sections=[{"name": ".text", "size": 0x100, "raw_data": b"\xcc" * 0x100}],
            symbols=ANCHOR_SYMS,
        )
        mangled = bytearray(data)
        # section header starts at offset 20; size is field 4 (offset 16 within header)
        _struct.pack_into("<L", mangled, 36, 0x7FFFFFFF)
        with self.assertRaises(coff_compare.CoffError):
            coff_compare.load(bytes(mangled))

    def test_resolved_data_pointer_aliases_compare_equal(self):
        target_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_previous", "value": 0, "section": 0,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        base_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        target = _build(
            target_symbols, [(0x20, 1, 6, 6)], sec_name=".data")
        base = _build(
            base_symbols, [(0x20, 0, 6, 0)], sec_name=".data")
        addresses = {"_previous": 0x1000, "_data": 0x1006}
        self.assertEqual(
            coff_compare.section_info_resolved(target, "_data", addresses),
            coff_compare.section_info_resolved(base, "_data", addresses),
        )

    def test_resolved_data_pointer_missing_evidence_refuses_alias(self):
        target_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_previous", "value": 0, "section": 0,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        base_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        target = _build(
            target_symbols, [(0x20, 1, 6, 6)], sec_name=".data")
        base = _build(
            base_symbols, [(0x20, 0, 6, 0)], sec_name=".data")
        addresses = {"_data": 0x1006}
        self.assertNotEqual(
            coff_compare.section_info_resolved(target, "_data", addresses),
            coff_compare.section_info_resolved(base, "_data", addresses),
        )

    def test_resolved_data_pointer_different_destination_fails(self):
        target_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_previous", "value": 0, "section": 0,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        base_symbols = [
            {"name": "_data", "value": 0, "section": 1,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        target = _build(
            target_symbols, [(0x20, 1, 6, 6)], sec_name=".data")
        base = _build(
            base_symbols, [(0x20, 0, 6, 0)], sec_name=".data")
        target_addresses = {"_previous": 0x2000, "_data": 0x1006}
        base_addresses = {"_previous": 0x1000, "_data": 0x1006}
        self.assertNotEqual(
            coff_compare.section_info_resolved(
                target, "_data", target_addresses),
            coff_compare.section_info_resolved(
                base, "_data", base_addresses),
        )


if __name__ == "__main__":
    unittest.main()

