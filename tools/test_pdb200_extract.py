import importlib.util
import struct
import sys
import tempfile
import unittest
from collections import Counter
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("pdb200_extract.py")
SPEC = importlib.util.spec_from_file_location("pdb200_extract", MODULE_PATH)
pdb = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = pdb
SPEC.loader.exec_module(pdb)


def make_pe(section_size=0x1000, raw_size=None):
    raw_size = section_size if raw_size is None else raw_size
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x14C, 1, 0, 0, 0, 0xE0, 0)
    table = 0x80 + 24 + 0xE0
    data[table:table + 8] = b".text\0\0\0"
    struct.pack_into("<IIII", data, table + 8, section_size, 0x1000, raw_size, 0x200)
    return pdb.PeImage(bytes(data))


def zproc(name, offset=0x20, length=0x30, segment=1):
    body = bytearray(37)
    struct.pack_into("<H", body, 0, pdb.S_GPROC_V3)
    struct.pack_into("<I", body, 14, length)
    struct.pack_into("<I", body, 26, 0x1000)
    struct.pack_into("<I", body, 30, offset)
    struct.pack_into("<H", body, 34, segment)
    body.extend(name.encode("ascii") + b"\0")
    return struct.pack("<H", len(body)) + body


def make_type_stream(records):
    body = b"".join(records)
    header = struct.pack("<IIIIIHHIIIIIIII", 20040203, 56, 0x1000, 0x1000 + len(records),
                         len(body), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    return header + body


def type_record(leaf, body):
    payload = struct.pack("<H", leaf) + body
    return struct.pack("<H", len(payload)) + payload


def ppublic(name, offset=0x20, segment=1):
    body = struct.pack("<HIIH", pdb.S_PUB_V2, 0, offset, segment)
    encoded = name.encode("ascii")
    body += bytes([len(encoded)]) + encoded
    return struct.pack("<H", len(body)) + body


class PeTests(unittest.TestCase):
    def test_rejects_invalid_section_and_range(self):
        image = make_pe()
        self.assertIsNone(image.resolve(2, 0, 4))
        self.assertIsNone(image.resolve(1, 0xFFF, 2))
        self.assertIsNone(image.resolve(1, 0, 0))

    def test_maps_file_offset_to_rva(self):
        self.assertEqual(make_pe().file_offset_to_rva(0x220), 0x1020)

    def test_raw_alignment_padding_is_not_a_valid_rva(self):
        self.assertIsNone(make_pe(0x100, 0x200).resolve(1, 0x180, 1))


class SymbolTests(unittest.TestCase):
    def test_parses_complete_function(self):
        stats = Counter()
        result = pdb.parse_symbols(zproc("_known"), make_pe(), {}, stats)
        self.assertEqual(result[0]["rva"], 0x1020)
        self.assertEqual(result[0]["length"], 0x30)

    def test_truncated_record_is_dropped_and_counted(self):
        stats = Counter()
        result = pdb.parse_symbols(zproc("_bad")[:-1], make_pe(), {}, stats)
        self.assertEqual(result, [])
        self.assertEqual(stats["symbol_stream_tail_dropped"], 1)

    def test_unterminated_name_is_dropped(self):
        stats = Counter()
        result = pdb.parse_symbols(zproc("_bad")[:-1] + b"x", make_pe(), {}, stats)
        self.assertEqual(result, [])
        self.assertEqual(stats["symbols_partial_dropped"], 1)

    def test_invalid_segment_and_rva_are_dropped(self):
        stats = Counter()
        data = zproc("_segment", segment=2) + zproc("_range", offset=0xFF0, length=0x40)
        self.assertEqual(pdb.parse_symbols(data, make_pe(), {}, stats), [])
        self.assertEqual(stats["symbols_partial_dropped"], 2)

    def test_zero_length_is_dropped(self):
        stats = Counter()
        self.assertEqual(pdb.parse_symbols(zproc("_zero", length=0), make_pe(), {}, stats), [])

    def test_public_address_is_emitted_as_zero_length_label(self):
        stats = Counter()
        result = pdb.parse_symbols(ppublic("_public"), make_pe(), {}, stats)
        self.assertEqual(result[0]["kind"], "LABEL")
        self.assertEqual(result[0]["length"], 0)

    def test_ambiguous_duplicates_are_preserved(self):
        stats = Counter()
        symbols = pdb.parse_symbols(zproc("_same", 0x20) + zproc("_same", 0x80), make_pe(), {}, stats)
        pdb._mark_ambiguities(symbols, stats)
        self.assertEqual(len(symbols), 2)
        self.assertTrue(all(item["ambiguous"] for item in symbols))

    def test_exact_duplicates_do_not_crash_range_sort(self):
        stats = Counter()
        symbols = pdb.parse_symbols(zproc("_same") + zproc("_same"), make_pe(), {}, stats)
        pdb._mark_ambiguities(symbols, stats)
        self.assertEqual(len(symbols), 2)

    def test_overlaps_are_preserved_and_flagged(self):
        stats = Counter()
        symbols = pdb.parse_symbols(zproc("_one", 0x20, 0x40) + zproc("_two", 0x40, 0x40),
                                    make_pe(), {}, stats)
        pdb._mark_ambiguities(symbols, stats)
        self.assertEqual(len(symbols), 2)
        self.assertTrue(all(item["ambiguous"] for item in symbols))


class TypeTests(unittest.TestCase):
    def test_forward_type_is_counted_and_dropped(self):
        body = struct.pack("<HHIIIH", 0, pdb.CV_PROP_FORWARD, 0, 0, 0, 4) + b"forward\0"
        stats = Counter()
        types, widths = pdb.parse_types(make_type_stream([type_record(pdb.LF_STRUCTURE_V3, body)]), stats)
        self.assertEqual(types, [])
        self.assertNotIn(0x1000, widths)
        self.assertEqual(stats["types_forward_dropped"], 1)

    def test_structure_without_fieldlist_emits_names_only(self):
        body = struct.pack("<HHIIIH", 0, 0, 0x2222, 0, 0, 8) + b"pair\0"
        stats = Counter()
        types, widths = pdb.parse_types(make_type_stream([type_record(pdb.LF_STRUCTURE_V3, body)]), stats)
        self.assertEqual(types[0]["fields"], [])
        self.assertEqual(types[0]["size"], 8)

    def test_v2_structure_fields_come_only_from_fieldlist(self):
        member_name = b"value"
        member = struct.pack("<HHIH", pdb.LF_MEMBER_V2, 0, 0x22, 0)
        member += bytes([len(member_name)]) + member_name
        fieldlist = type_record(pdb.LF_FIELDLIST_V2, member)
        structure_name = b"holder"
        structure = struct.pack("<HHIIIH", 1, 0, 0x1000, 0, 0, 4)
        structure += bytes([len(structure_name)]) + structure_name
        stats = Counter()
        types, widths = pdb.parse_types(
            make_type_stream([fieldlist, type_record(pdb.LF_STRUCTURE_V2, structure)]), stats
        )
        self.assertEqual(types[0]["fields"][0]["name"], "value")
        self.assertEqual(types[0]["fields"][0]["offset"], 0)
        self.assertEqual(types[0]["fields"][0]["width"], 4)

    def test_enum_uses_underlying_short_width(self):
        name = b"small_enum"
        body = struct.pack("<HHII", 1, 0, 0x21, 0) + bytes([len(name)]) + name
        types, widths = pdb.parse_types(
            make_type_stream([type_record(pdb.LF_ENUM_V2, body)]), Counter()
        )
        self.assertEqual(types[0]["size"], 2)

    def test_unresolved_user_type_is_not_guessed_as_pointer(self):
        self.assertIsNone(pdb._basic_width(0x1203))

    def test_malformed_field_padding_drops_entire_fieldlist(self):
        member_name = b"value"
        member = struct.pack("<HHIH", pdb.LF_MEMBER_V2, 0, 0x22, 0)
        member += bytes([len(member_name)]) + member_name + b"\xff"
        fieldlist = type_record(pdb.LF_FIELDLIST_V2, member)
        name = b"holder"
        structure = struct.pack("<HHIIIH", 1, 0, 0x1000, 0, 0, 4)
        structure += bytes([len(name)]) + name
        stats = Counter()
        types, widths = pdb.parse_types(
            make_type_stream([fieldlist, type_record(pdb.LF_STRUCTURE_V2, structure)]), stats
        )
        self.assertEqual(types[0]["fields"], [])
        self.assertEqual(stats["fieldlists_partial_dropped"], 1)

    def test_truncated_type_record_fails_closed(self):
        stream = make_type_stream([type_record(0x7777, b"abcd")])[:-1]
        with self.assertRaises(pdb.PdbError):
            pdb.parse_types(stream, Counter())


class ContainerTests(unittest.TestCase):
    def test_bad_magic_is_rejected(self):
        with self.assertRaises(pdb.PdbError):
            pdb.Msf2(b"not a pdb")

    def test_invalid_toc_block_is_rejected(self):
        data = bytearray(1024)
        data[:len(pdb.MAGIC)] = pdb.MAGIC
        struct.pack_into("<IHHIIH", data, len(pdb.MAGIC), 1024, 0, 1, 4, 0, 2)
        with self.assertRaises(pdb.PdbError):
            pdb.Msf2(bytes(data))

    def test_toc_block_beyond_declared_count_is_rejected_even_with_trailing_data(self):
        data = bytearray(3072)
        data[:len(pdb.MAGIC)] = pdb.MAGIC
        struct.pack_into("<IHHIIH", data, len(pdb.MAGIC), 1024, 0, 2, 4, 0, 2)
        struct.pack_into("<I", data, 2048, 0)
        with self.assertRaises(pdb.PdbError):
            pdb.Msf2(bytes(data))

    def test_truncated_dbi_module_table_is_rejected(self):
        data = bytearray(64)
        struct.pack_into("<I", data, 0, 0xFFFFFFFF)
        struct.pack_into("<I", data, 24, 4)
        with self.assertRaises(pdb.PdbError):
            pdb._module_streams(bytes(data))


class DeltaTests(unittest.TestCase):
    def test_mixed_confirmation_and_conflict_are_both_reported(self):
        symbols = [
            {"name": "_mixed", "rva": 0x1020},
            {"name": "_mixed", "rva": 0x1040},
        ]
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "symbols.json"
            config.write_text('[{"name":"_mixed","file_offset":544}]', encoding="utf-8")
            delta = pdb._delta(symbols, config, make_pe())
        self.assertEqual(delta["confirmations"][0]["rvas"], [0x1020])
        self.assertEqual(delta["conflicts"][0]["pdb_rvas"], [0x1040])
        self.assertTrue(delta["conflicts"][0]["ambiguous"])


if __name__ == "__main__":
    unittest.main()
