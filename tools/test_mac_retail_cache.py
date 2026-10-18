"""Asset-free tests for the read-only Macintosh retail v7 cache view."""

import ctypes
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "port/mac/retail_cache.c"
HEADER = ROOT / "port/mac/retail_cache.h"
BASE = 0x40440000


class RetailBsp(ctypes.Structure):
    _fields_ = [("file_offset", ctypes.c_uint32), ("size", ctypes.c_uint32),
                ("base", ctypes.c_uint32), ("root", ctypes.c_uint32),
                ("handle", ctypes.c_uint32)]


class RetailCache(ctypes.Structure):
    _fields_ = [
        ("file", ctypes.c_void_p), ("tags", ctypes.c_void_p),
        ("file_size", ctypes.c_uint32), ("tag_offset", ctypes.c_uint32),
        ("tag_size", ctypes.c_uint32), ("instances_offset", ctypes.c_uint32),
        ("tags_end", ctypes.c_uint32), ("tag_count", ctypes.c_uint32),
        ("scenario_handle", ctypes.c_uint32), ("scenario_offset", ctypes.c_uint32),
        ("model_offset", ctypes.c_uint32), ("model_size", ctypes.c_uint32),
        ("model_indices_offset", ctypes.c_uint32), ("bsp_count", ctypes.c_uint32),
        ("bsps", RetailBsp * 32), ("name", ctypes.c_char * 32),
        ("build", ctypes.c_char * 32),
    ]


class RetailTag(ctypes.Structure):
    _fields_ = [("group", ctypes.c_uint32),
                ("parents", ctypes.c_uint32 * 2), ("handle", ctypes.c_uint32),
                ("address", ctypes.c_uint32), ("name", ctypes.c_char_p)]


class RetailResources(ctypes.Structure):
    _fields_ = [("bitmap_entries", ctypes.c_uint32),
                ("sound_entries", ctypes.c_uint32),
                ("external_bitmaps", ctypes.c_uint32),
                ("external_sounds", ctypes.c_uint32)]


def put32(buf, offset, value):
    buf[offset:offset + 4] = (int(value) & 0xFFFFFFFF).to_bytes(4, "little")


def make_cache():
    """Return a small valid v7 file with a scenario and one local BSP."""
    data = bytearray(0x3000)
    tag_offset, tag_size = 0x800, 0x1000
    model_offset, model_size = 0x1800, 0x100
    bsp_file, bsp_size, bsp_base = 0x1900, 0x400, BASE + 0x2000
    index = tag_offset
    scenario_def = 0x200
    refs = 0x800

    put32(data, 0x00, 0x68656164)
    put32(data, 0x04, 7)
    put32(data, 0x08, len(data))
    put32(data, 0x0C, 0)
    put32(data, 0x10, tag_offset)
    put32(data, 0x14, tag_size)
    data[0x20:0x20 + len(b"Synthetic v7 map\0")] = b"Synthetic v7 map\0"
    data[0x40:0x40 + len(b"test build 1\0")] = b"test build 1\0"
    put32(data, 0x7FC, 0x666f6f74)

    put32(data, index + 0x00, BASE + 0x40)
    put32(data, index + 0x04, 0x12340000)
    put32(data, index + 0x0C, 2)
    put32(data, index + 0x14, model_offset)
    put32(data, index + 0x1C, 0)
    put32(data, index + 0x20, model_size)
    put32(data, index + 0x24, 0x74616773)

    # Scenario entry.
    entry = index + 0x40
    put32(data, entry + 0x00, 0x73636e72)
    put32(data, entry + 0x0C, 0x12340000)
    put32(data, entry + 0x10, BASE + 0x100)
    put32(data, entry + 0x14, BASE + scenario_def)

    # BSP entry. Its definition address is absent in retail v7.
    entry = index + 0x60
    put32(data, entry + 0x00, 0x73627370)
    put32(data, entry + 0x0C, 0x12340001)
    put32(data, entry + 0x10, BASE + 0x110)

    data[index + 0x100:index + 0x100 + len(b"levels/test\0")] = b"levels/test\0"
    data[index + 0x110:index + 0x110 + len(b"levels/test\0")] = b"levels/test\0"

    # The scenario structure-BSP block and its one 32-byte reference.
    put32(data, index + scenario_def + 0x5A4, 1)
    put32(data, index + scenario_def + 0x5A8, BASE + refs)
    ref = index + refs
    put32(data, ref + 0x00, bsp_file)
    put32(data, ref + 0x04, bsp_size)
    put32(data, ref + 0x08, bsp_base)
    put32(data, ref + 0x1C, 0x12340001)

    # PC-style BSP header, followed by space for its 0x288-byte root.
    put32(data, bsp_file + 0x00, bsp_base + 0x18)
    put32(data, bsp_file + 0x14, 0x73627370)
    return data


def make_resource_cache():
    """Add one bitmap and one sound with payloads in their resource maps."""
    data = make_cache()
    index = 0x800
    put32(data, index + 0x0C, 4)
    for i, (group, name_offset, root_offset) in enumerate((
            (0x6269746D, 0x130, 0x400), (0x736E6421, 0x140, 0x500)), start=2):
        entry = index + 0x40 + i * 0x20
        put32(data, entry, group)
        put32(data, entry + 0x0C, 0x12340000 + i)
        put32(data, entry + 0x10, BASE + name_offset)
        put32(data, entry + 0x14, BASE + root_offset)
        name = (b"levels/bitmap\0" if i == 2 else b"levels/sound\0")
        data[index + name_offset:index + name_offset + len(name)] = name

    # One bitmap pixel range in bitmaps.map: flags bit 8 marks external data.
    bitmap = index + 0x400
    put32(data, bitmap + 0x60, 1)
    put32(data, bitmap + 0x64, BASE + 0x900)
    bitmap_entry = index + 0x900
    data[bitmap_entry + 14:bitmap_entry + 16] = (256).to_bytes(2, "little")
    put32(data, bitmap_entry + 24, 0x100)
    put32(data, bitmap_entry + 28, 0x80)

    # One sound pitch and permutation, with its sample range in sounds.map.
    sound = index + 0x500
    put32(data, sound + 0x98, 1)
    put32(data, sound + 0x9C, BASE + 0xA00)
    pitch = index + 0xA00
    put32(data, pitch + 0x3C, 1)
    put32(data, pitch + 0x40, BASE + 0xA80)
    permutation = index + 0xA80
    put32(data, permutation + 0x40, 0x100)
    put32(data, permutation + 0x44, 1)
    put32(data, permutation + 0x48, 0x200)
    return data


class RetailCacheTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
        if not compiler:
            raise unittest.SkipTest("a C compiler is required for the reader tests")
        cls.temp = tempfile.TemporaryDirectory(prefix="mac-retail-cache-")
        library = Path(cls.temp.name) / "libretail_cache.so"
        subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        "-fPIC", "-shared", "-I", str(HEADER.parent),
                        str(SOURCE), "-o", str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.mac_retail_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                            ctypes.POINTER(RetailCache),
                                            ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.mac_retail_open.restype = ctypes.c_int
        cls.lib.mac_retail_tag_get.argtypes = [ctypes.POINTER(RetailCache), ctypes.c_uint32,
                                               ctypes.POINTER(RetailTag)]
        cls.lib.mac_retail_tag_get.restype = ctypes.c_int
        cls.lib.mac_retail_tag_span.argtypes = [ctypes.POINTER(RetailCache), ctypes.c_uint32,
                                                ctypes.c_uint32]
        cls.lib.mac_retail_tag_span.restype = ctypes.c_void_p
        cls.lib.mac_retail_resources_check.argtypes = [
            ctypes.POINTER(RetailCache), ctypes.c_uint32, ctypes.c_uint32,
            ctypes.POINTER(RetailResources), ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.mac_retail_resources_check.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "temp"):
            cls.temp.cleanup()

    def open(self, data):
        backing = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        result = RetailCache()
        error = ctypes.c_char_p()
        ok = self.lib.mac_retail_open(backing, len(data), ctypes.byref(result),
                                      ctypes.byref(error))
        return ok, result, error.value, backing

    def assert_rejected(self, data, diagnostic=None):
        ok, result, error, _ = self.open(data)
        self.assertEqual(ok, 0)
        self.assertIsNotNone(error)
        self.assertEqual(bytes(result), bytes(ctypes.sizeof(result)))
        if diagnostic:
            self.assertIn(diagnostic.encode(), error)

    def test_valid_scenario_and_bsp_metadata(self):
        ok, cache, error, backing = self.open(make_cache())
        self.assertEqual((ok, error), (1, None))
        self.assertEqual(cache.file_size, len(backing))
        self.assertEqual(cache.tag_count, 2)
        self.assertEqual(cache.scenario_handle, 0x12340000)
        self.assertEqual(cache.scenario_offset, 0x200)
        self.assertEqual(cache.bsp_count, 1)
        self.assertEqual((cache.bsps[0].file_offset, cache.bsps[0].size), (0x1900, 0x400))
        self.assertEqual(cache.bsps[0].base, BASE + 0x2000)
        self.assertEqual(cache.bsps[0].root, BASE + 0x2018)
        self.assertEqual(cache.bsps[0].handle, 0x12340001)

    def test_tag_lookup_and_spans_are_bounded(self):
        ok, cache, _, _ = self.open(make_cache())
        self.assertEqual(ok, 1)
        tag = RetailTag()
        self.assertEqual(self.lib.mac_retail_tag_get(ctypes.byref(cache), 0, ctypes.byref(tag)), 1)
        self.assertEqual(tag.group, 0x73636e72)
        self.assertEqual(tag.name, b"levels/test")
        self.assertEqual(tag.address, BASE + 0x200)
        self.assertEqual(self.lib.mac_retail_tag_get(ctypes.byref(cache), 2, ctypes.byref(tag)), 0)
        self.assertTrue(self.lib.mac_retail_tag_span(ctypes.byref(cache), BASE + 0x200, 0x5B0))
        self.assertFalse(self.lib.mac_retail_tag_span(ctypes.byref(cache), BASE + 0xFFF, 2))
        self.assertFalse(self.lib.mac_retail_tag_span(ctypes.byref(cache), BASE + 0x10, 1))
        self.assertFalse(self.lib.mac_retail_tag_span(ctypes.byref(cache), BASE + 0x200, 0))

    def test_rejects_truncated_headers_and_bad_signatures(self):
        valid = make_cache()
        for length in (0, 0x7FF, 0x800):
            with self.subTest(length=length):
                self.assert_rejected(valid[:length])
        bad = bytearray(valid)
        put32(bad, 0, 0)
        self.assert_rejected(bad, "header signatures")
        bad = bytearray(valid)
        put32(bad, 0x7FC, 0)
        self.assert_rejected(bad, "header signatures")

    def test_rejects_wrong_version_declared_length_and_compression(self):
        for offset, value in ((4, 6), (8, 0x4000), (8, 0x700), (12, 1)):
            bad = make_cache()
            put32(bad, offset, value)
            with self.subTest(offset=offset, value=value):
                self.assert_rejected(bad)

    def test_rejects_wrapping_or_overlapping_top_level_ranges(self):
        changes = ((0x10, 0xFFFFFFF0), (0x14, 0xFFFFFFFF),
                   (0x10, 0x1800), (0x14, 0x100),
                   (0x14, 0x10000001))
        for offset, value in changes:
            bad = make_cache()
            put32(bad, offset, value)
            with self.subTest(offset=offset, value=value):
                self.assert_rejected(bad)
        bad = make_cache()
        put32(bad, 0x10, 0x7FF)
        self.assert_rejected(bad)
        bad = make_cache()
        put32(bad, 0x800 + 0x14, 0xFFFFFFF0)
        self.assert_rejected(bad)

    def test_rejects_bad_index_table_and_handles(self):
        for relative, value in ((0x24, 0), (0x00, 0xFFFFFFFC),
                                (0x0C, 0xFFFFFFFF), (0x0C, 0)):
            bad = make_cache()
            put32(bad, 0x800 + relative, value)
            with self.subTest(relative=relative, value=value):
                self.assert_rejected(bad)
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x0C, 0x12340002)
        self.assert_rejected(bad, "tag handle")

    def test_rejects_bad_names_and_resource_definitions(self):
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x10, BASE + 0x1000)
        self.assert_rejected(bad, "tag name address")
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x10, BASE + 0xF00)
        bad[0x800 + 0xF00:0x800 + 0x1000] = b"x" * 0x100
        self.assert_rejected(bad, "tag name termination")
        bad = make_cache()
        bad[0x800 + 0x100:0x800 + 0x100 + 0x100] = b"x" * 0x100
        self.assert_rejected(bad, "tag name termination")
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x18, 1)
        self.assert_rejected(bad, "indexed resource definitions")
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x14, BASE + 0x202)
        self.assert_rejected(bad, "tag definition address")

    def test_rejects_scenario_handle_group_and_short_definition(self):
        bad = make_cache()
        put32(bad, 0x800 + 4, 0x12340001)
        self.assert_rejected(bad, "scenario handle/group")
        bad = make_cache()
        put32(bad, 0x800 + 0x40, 0x12345678)
        self.assert_rejected(bad, "scenario handle/group")
        bad = make_cache()
        put32(bad, 0x800 + 0x40 + 0x14, BASE + 0xB80)
        self.assert_rejected(bad, "scenario range")

    def test_rejects_malformed_scenario_bsp_block(self):
        for count, address in ((33, BASE + 0x800), (1, BASE + 0xFFF),
                               (1, BASE + 0x801)):
            bad = make_cache()
            put32(bad, 0x800 + 0x200 + 0x5A4, count)
            put32(bad, 0x800 + 0x200 + 0x5A8, address)
            with self.subTest(count=count, address=address):
                self.assert_rejected(bad)

    def test_rejects_bad_bsp_handle_ranges_and_header(self):
        mutations = (
            (0x800 + 0x800 + 0x1C, 0x12340000),
            (0x800 + 0x800 + 0x00, 0xFFFFFFF0),
            (0x800 + 0x800 + 0x08, 0x40440FFC),
            (0x800 + 0x800 + 0x08, 0x40442001),
            (0x800 + 0x800 + 0x04, 0xFFFFFFFF),
            (0x800 + 0x800 + 0x00, 0x1800),
        )
        for offset, value in mutations:
            bad = make_cache()
            put32(bad, offset, value)
            with self.subTest(offset=offset, value=value):
                self.assert_rejected(bad)
        for offset in (4, 8, 12, 16):
            bad = make_cache()
            put32(bad, 0x1900 + offset, 1)
            with self.subTest(buffer_field=offset):
                self.assert_rejected(bad, "PC BSP header/root")
        bad = make_cache()
        put32(bad, 0x1900 + 0x14, 0)
        self.assert_rejected(bad, "PC BSP header/root")
        bad = make_cache()
        put32(bad, 0x1900, BASE + 0x2000)
        self.assert_rejected(bad, "PC BSP header/root")

    def test_rejects_overlapping_bsp_ranges(self):
        data = make_cache()
        # Add a second BSP and make its file bytes overlap the first one.
        put32(data, 0x800 + 0x0C, 3)
        entry = 0x800 + 0x80
        put32(data, entry, 0x73627370)
        put32(data, entry + 0x0C, 0x12340002)
        put32(data, entry + 0x10, BASE + 0x120)
        data[0x800 + 0x120:0x800 + 0x120 + len(b"levels/test2\0")] = b"levels/test2\0"
        ref2 = 0x800 + 0x820
        put32(data, 0x800 + 0x200 + 0x5A4, 2)
        put32(data, 0x800 + 0x200 + 0x5A8, BASE + 0x800)
        put32(data, ref2 + 0, 0x1A00)
        put32(data, ref2 + 4, 0x400)
        put32(data, ref2 + 8, BASE + 0x2400)
        put32(data, ref2 + 28, 0x12340002)
        put32(data, 0x1A00, BASE + 0x2418)
        put32(data, 0x1A00 + 0x14, 0x73627370)
        # Reference one is valid, while reference two overlaps it.
        put32(data, ref2 + 0, 0x1B00)
        self.assert_rejected(data, "overlapping BSP file ranges")

    def check_resources(self, data, bitmap_size, sound_size):
        ok, cache, _, backing = self.open(data)
        self.assertEqual(ok, 1)
        resources = RetailResources()
        error = ctypes.c_char_p()
        checked = self.lib.mac_retail_resources_check(
            ctypes.byref(cache), bitmap_size, sound_size,
            ctypes.byref(resources), ctypes.byref(error))
        return checked, resources, error.value, backing

    def test_resource_ranges_accept_external_map_payloads(self):
        checked, resources, error, _ = self.check_resources(make_resource_cache(), 0x1000, 0x2000)
        self.assertEqual((checked, error), (1, None))
        self.assertEqual((resources.bitmap_entries, resources.sound_entries), (1, 1))
        self.assertEqual((resources.external_bitmaps, resources.external_sounds), (1, 1))

    def test_resource_check_requires_missing_external_maps(self):
        for bitmap_size, sound_size, expected in ((0, 0x2000, b"missing bitmaps.map"),
                                                  (0x1000, 0, b"missing sounds.map")):
            checked, resources, error, _ = self.check_resources(
                make_resource_cache(), bitmap_size, sound_size)
            self.assertEqual(checked, 0)
            self.assertEqual(bytes(resources), bytes(ctypes.sizeof(resources)))
            self.assertIn(expected, error)

    def test_resource_check_rejects_bad_nested_blocks(self):
        bad = make_resource_cache()
        put32(bad, 0x800 + 0x400 + 0x60, 0xFFFFFFFF)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"bitmap block", error)

        bad = make_resource_cache()
        # Bitmap entry block points to a span that cannot fit in the tag data.
        put32(bad, 0x800 + 0x400 + 0x64, BASE + 0xFF0)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"bitmap block", error)

        bad = make_resource_cache()
        # A sound pitch points to a malformed permutation block address.
        put32(bad, 0x800 + 0xA00 + 0x40, BASE + 0xFFF)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"sound permutations", error)

        bad = make_resource_cache()
        put32(bad, 0x800 + 0xA00 + 0x3C, 0xFFFFFFFF)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"sound permutations", error)

    def test_resource_check_rejects_payload_outside_map_file(self):
        bad = make_resource_cache()
        put32(bad, 0x800 + 0x900 + 24, 0xFFF)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"bitmap payload range", error)

        bad = make_resource_cache()
        put32(bad, 0x800 + 0xA80 + 0x48, 0x1FFF)
        checked, _, error, _ = self.check_resources(bad, 0x1000, 0x2000)
        self.assertEqual(checked, 0)
        self.assertIn(b"sound payload range", error)


if __name__ == "__main__":
    unittest.main()
