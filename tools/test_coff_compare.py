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


def _build_code_data(symbols, relocs, data_sections=None):
    """Build a code section followed by one or more non-code sections."""
    raw_bytes, reloc_bytes = coff_compare.make_section_raw(0x40, relocs)
    sections = [{
        "name": ".text", "size": 0x40, "raw_data": raw_bytes,
        "reloc_data": reloc_bytes, "flags": 0xE0000060,
    }]
    for item in data_sections or [(".rdata", 0x20)]:
        name, size = item
        sections.append({
            "name": name, "size": size, "raw_data": b"\0" * size,
            "flags": 0x40000040,
        })
    return coff_compare.load(
        coff_compare.build_coff(sections=sections, symbols=symbols))


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

    def test_defined_noncode_same_destination_normalizes(self):
        """Owner+addend and a local symbol at the same offset are equal."""
        anchor_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        local_symbols = anchor_symbols + [
            {"name": "$entry", "value": 8, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        anchor = _build_code_data(anchor_symbols, [(0x10, 1, 6, 8)])
        local = _build_code_data(local_symbols, [(0x10, 2, 6, 0)])
        self.assertTrue(coff_compare.section_infos_equal(
            coff_compare.section_info(anchor, "func"),
            coff_compare.section_info(local, "func")))

    def test_defined_noncode_different_offset_refuses(self):
        """Different resolved offsets in the same section remain unequal."""
        anchor_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        local_symbols = anchor_symbols + [
            {"name": "$entry", "value": 12, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        anchor = _build_code_data(anchor_symbols, [(0x10, 1, 6, 8)])
        local = _build_code_data(local_symbols, [(0x10, 2, 6, 0)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(anchor, "func"),
            coff_compare.section_info(local, "func")))

    def test_defined_noncode_different_section_refuses(self):
        """A destination in a differently identified section is unequal."""
        left_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        right_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 3,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "$entry", "value": 8, "section": 3,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        left = _build_code_data(left_symbols, [(0x10, 1, 6, 8)])
        right = _build_code_data(
            right_symbols, [(0x10, 2, 6, 0)],
            [(".rdata", 0x20), (".data", 0x20)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(left, "func"),
            coff_compare.section_info(right, "func")))

    def test_defined_noncode_undefined_remains_symbolic(self):
        """An undefined anchor is not inferred to be a defined section."""
        undefined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 0,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        defined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "$entry", "value": 8, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        undefined = _build_code_data(undefined_symbols, [(0x10, 1, 6, 8)])
        defined = _build_code_data(defined_symbols, [(0x10, 2, 6, 0)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(undefined, "func"),
            coff_compare.section_info(defined, "func")))

    def test_defined_noncode_absolute_remains_conservative(self):
        """An absolute target is not inferred to be a defined section."""
        absolute_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 8, "section": -1,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        defined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "$entry", "value": 8, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        absolute = _build_code_data(absolute_symbols, [(0x10, 1, 6, 0)])
        defined = _build_code_data(defined_symbols, [(0x10, 2, 6, 0)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(absolute, "func"),
            coff_compare.section_info(defined, "func")))

    def test_defined_noncode_ambiguous_owner_refuses(self):
        """Multiple external offset-zero owners prevent normalization."""
        ambiguous_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_alias", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        defined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "$entry", "value": 8, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        ambiguous = _build_code_data(
            ambiguous_symbols, [(0x10, 1, 6, 8)])
        defined = _build_code_data(defined_symbols, [(0x10, 2, 6, 0)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(ambiguous, "func"),
            coff_compare.section_info(defined, "func")))

    def test_defined_noncode_duplicate_owner_name_refuses(self):
        """A defined owner name reused by another section is ambiguous."""
        duplicate_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 3,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        defined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
            {"name": "$entry", "value": 8, "section": 2,
             "type": 0, "storage": 3, "aux_count": 0},
        ]
        duplicate = _build_code_data(
            duplicate_symbols, [(0x10, 1, 6, 8)],
            [(".rdata", 0x20), (".data", 0x20)])
        defined = _build_code_data(defined_symbols, [(0x10, 2, 6, 0)])
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(duplicate, "func"),
            coff_compare.section_info(defined, "func")))

    def test_defined_and_undefined_same_symbol_spelling_matches(self):
        """Definition ownership does not change a same-name relocation."""
        defined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 2,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        undefined_symbols = [
            {"name": "func", "value": 0, "section": 1,
             "type": 0x20, "storage": 2, "aux_count": 0},
            {"name": "_table", "value": 0, "section": 0,
             "type": 0, "storage": 2, "aux_count": 0},
        ]
        defined = _build_code_data(defined_symbols, [(0x10, 1, 6, 8)])
        undefined = _build_code_data(undefined_symbols, [(0x10, 1, 6, 8)])
        self.assertTrue(coff_compare.section_infos_equal(
            coff_compare.section_info(defined, "func"),
            coff_compare.section_info(undefined, "func")))

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

    def test_uninitialized_section_info_hashes_logical_zero_bytes(self):
        """A payload-less BSS must not hash bytes from the COFF header."""
        import struct as _struct

        data = bytearray(coff_compare.build_coff(
            sections=[{
                "name": ".bss",
                "size": 4,
                "raw_data": b"\0" * 4,
                "reloc_count": 0,
                "flags": coff_compare.IMAGE_SCN_CNT_UNINITIALIZED_DATA,
            }],
            symbols=[{
                "name": "_bss_owner",
                "value": 0,
                "section": 1,
                "type": 0,
                "storage": 2,
                "aux_count": 0,
            }],
        ))
        # Section characteristics are the final dword in the 40-byte header.
        _struct.pack_into(
            "<L", data, 56,
            coff_compare.IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        with_payload = coff_compare.section_info(
            coff_compare.load(data), "_bss_owner")

        # PointerToRawData is the fifth dword-like field in the section
        # header: COFF header (20) + name/virtual fields/size (20).
        _struct.pack_into("<L", data, 40, 0)
        without_payload = coff_compare.section_info(
            coff_compare.load(data), "_bss_owner")

        self.assertEqual(with_payload, without_payload)

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


class TestImageSymbolAddresses(unittest.TestCase):
    """Repeated COMDAT names are dropped only when address-ambiguous."""

    def test_unique_names_are_mapped(self):
        self.assertEqual(
            coff_compare.image_symbol_addresses([
                {"name": "_a", "file_offset": 0x1000},
                {"name": "_b", "file_offset": 0x2000},
            ]),
            {"_a": 0x1000, "_b": 0x2000},
        )

    def test_name_repeated_at_one_address_is_kept(self):
        self.assertEqual(
            coff_compare.image_symbol_addresses([
                {"name": "_a", "file_offset": 0x1000},
                {"name": "_a", "file_offset": 0x1000},
            ]),
            {"_a": 0x1000},
        )

    def test_name_at_several_addresses_is_dropped(self):
        self.assertEqual(
            coff_compare.image_symbol_addresses([
                {"name": "_D3DDevice_SetRenderState", "file_offset": 0x15D220},
                {"name": "_D3DDevice_SetRenderState", "file_offset": 0x15E2B0},
                {"name": "_unambiguous", "file_offset": 0x3000},
            ]),
            {"_unambiguous": 0x3000},
        )


class TestLinkAbsoluteZeroAndSehScopeTables(unittest.TestCase):
    """SEH object spellings compare only through complete destination proof."""

    @staticmethod
    def _seh_pair(filter_offset=0x51, handler_offset=0x63):
        import struct

        def obj(style, filter_value, handler_value):
            code_size = 0x90
            code = bytearray(b"\x90" * code_size)
            code[6:10] = b"\x00" * 4
            code[0x11:0x15] = b"\x00" * 4
            table = bytearray(b"\xff\xff\xff\xff" + b"\x00" * 8)
            code_relocations = bytearray()
            table_relocations = bytearray()
            symbols = [
                {"name": ".text", "value": 0, "section": 1,
                 "type": 0, "storage": 3},
                {"name": "_main", "value": 0, "section": 1,
                 "type": 0x20, "storage": 2},
            ]
            if style == "split":
                symbols.append({
                    "name": "_rdata_00", "value": 0, "section": 2,
                    "type": 0, "storage": 3,
                })
                scope_table_symbol = 2
                struct.pack_into("<i", table, 4, filter_value)
                struct.pack_into("<i", table, 8, handler_value)
                for address in (4, 8):
                    table_relocations += struct.pack("<LLH", address, 1, 6)
            else:
                symbols.append({
                    "name": "$T18229", "value": 0, "section": 2,
                    "type": 0, "storage": 3,
                })
                scope_table_symbol = 2
                symbols.append({
                    "name": "$L1", "value": filter_value, "section": 1,
                    "type": 0, "storage": 6,
                })
                symbols.append({
                    "name": "$L2", "value": handler_value, "section": 1,
                    "type": 0, "storage": 6,
                })
                for address, symbol_index in ((4, 3), (8, 4)):
                    table_relocations += struct.pack(
                        "<LLH", address, symbol_index, 6)
                symbols.append({
                    "name": "__except_list", "value": 0, "section": 0,
                    "type": 0, "storage": 2,
                })
                code_relocations += struct.pack("<LLH", 0x11, 5, 6)
            code_relocations += struct.pack(
                "<LLH", 6, scope_table_symbol, 6)
            return coff_compare.load(coff_compare.build_coff(
                sections=[
                    {
                        "name": ".text",
                        "size": code_size,
                        "raw_data": bytes(code),
                        "reloc_data": bytes(code_relocations),
                        "flags": 0xE0000060,
                    },
                    {
                        "name": ".rdata",
                        "size": 12,
                        "raw_data": bytes(table),
                        "reloc_data": bytes(table_relocations),
                        "flags": 0x40000040,
                    },
                ],
                symbols=symbols,
            ))

        return (
            obj("split", filter_offset, handler_offset),
            obj("msvc", filter_offset, handler_offset),
        )

    def test_scope_table_spellings_compare_equal(self):
        split, msvc = self._seh_pair()
        self.assertTrue(coff_compare.section_infos_equal(
            coff_compare.section_info(split, "_main"),
            coff_compare.section_info(msvc, "_main"),
        ))

    def test_except_list_relocation_is_dropped_from_count(self):
        _, msvc = self._seh_pair()
        info = coff_compare.section_info(msvc, "_main")
        self.assertEqual(info["relocation_count"], 1)

    def test_different_scope_handler_offset_stays_unequal(self):
        split, _ = self._seh_pair()
        _, other = self._seh_pair(handler_offset=0x60)
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info(split, "_main"),
            coff_compare.section_info(other, "_main"),
        ))

    def test_scope_table_data_sections_compare_source_relative(self):
        split, msvc = self._seh_pair()
        self.assertTrue(coff_compare.section_infos_equal(
            coff_compare.section_info_source_relative(
                split, "_rdata_00", "_main"),
            coff_compare.section_info_source_relative(
                msvc, "$T18229", "_main"),
        ))

    def test_source_relative_scope_table_keeps_handler_difference(self):
        split, _ = self._seh_pair()
        _, other = self._seh_pair(handler_offset=0x60)
        self.assertFalse(coff_compare.section_infos_equal(
            coff_compare.section_info_source_relative(
                split, "_rdata_00", "_main"),
            coff_compare.section_info_source_relative(
                other, "$T18229", "_main"),
        ))

    def test_source_relative_scope_table_requires_whole_function(self):
        split, _ = self._seh_pair()
        main = next(
            item for item in split["symbols"] if item["name"] == "_main")
        main["value"] = 1
        with self.assertRaisesRegex(
            coff_compare.CoffError, "whole-section function"
        ):
            coff_compare.section_info_source_relative(
                split, "_rdata_00", "_main")


if __name__ == "__main__":
    unittest.main()

