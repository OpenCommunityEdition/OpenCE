"""Asset-free tests for the bounded Macintosh retail BSP geometry reader."""

import ctypes
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

from test_mac_retail_cache import BASE, RetailCache, make_cache, put32


ROOT = Path(__file__).resolve().parents[1]
GEOMETRY_SOURCE = ROOT / "port/mac/retail_geometry.c"
CACHE_SOURCE = ROOT / "port/mac/retail_cache.c"
BSP_FILE = 0x1900
BSP_BASE = BASE + 0x2000
BSP_SIZE = 0x1000
SURFACES_OFFSET = 0x2B0
LIGHTS_OFFSET = 0x300
MATERIALS_OFFSET = 0x340
VERTICES_OFFSET = 0x600


class RetailVertex(ctypes.Structure):
    _fields_ = [("position", ctypes.c_float * 3), ("normal", ctypes.c_float * 3),
                ("binormal", ctypes.c_float * 3), ("tangent", ctypes.c_float * 3),
                ("uv", ctypes.c_float * 2)]


class RetailLightVertex(ctypes.Structure):
    _fields_ = [("normal", ctypes.c_float * 3), ("uv", ctypes.c_float * 2)]


class RetailMaterial(ctypes.Structure):
    _fields_ = [("shader_handle", ctypes.c_uint32),
                ("vertex_count", ctypes.c_uint32),
                ("light_vertex_count", ctypes.c_uint32),
                ("triangle_count", ctypes.c_uint32),
                ("lightmap_bitmap", ctypes.c_int32),
                ("vertices", ctypes.POINTER(RetailVertex)),
                ("light_vertices", ctypes.POINTER(RetailLightVertex)),
                ("indices", ctypes.POINTER(ctypes.c_uint16))]


def put16(buf, offset, value):
    buf[offset:offset + 2] = int(value).to_bytes(2, "little")


def put_floats(buf, offset, values):
    buf[offset:offset + 4 * len(values)] = struct.pack("<" + "f" * len(values), *values)


def make_geometry_cache(material_count=1, bad_second_vertex=False):
    """Build a retail cache with one BSP material and native-sized geometry."""
    data = make_cache()
    tag_index = 0x800
    bsp_size = BSP_SIZE
    put32(data, tag_index + 0x800 + 4, bsp_size)
    put32(data, tag_index + 0x800 + 8, BSP_BASE)

    # Grow the file to contain the larger synthetic BSP.
    if len(data) < BSP_FILE + bsp_size:
        data.extend(b"\0" * (BSP_FILE + bsp_size - len(data)))
        put32(data, 8, len(data))

    root = BSP_FILE + 0x18

    # The root's surface and lightmap blocks.
    put32(data, root + 0xF8, material_count)
    put32(data, root + 0xFC, BSP_BASE + SURFACES_OFFSET)
    put32(data, root + 0x104, 1)
    put32(data, root + 0x108, BSP_BASE + LIGHTS_OFFSET)

    # One lightmap whose material block contains `material_count` records.
    put16(data, BSP_FILE + LIGHTS_OFFSET, 7)
    put32(data, BSP_FILE + LIGHTS_OFFSET + 20, material_count)
    put32(data, BSP_FILE + LIGHTS_OFFSET + 24, BSP_BASE + MATERIALS_OFFSET)

    # Each material has one triangle and three vertices; the surface list is
    # an array of three uint16 indices per material.
    for i in range(material_count):
        material = BSP_FILE + MATERIALS_OFFSET + i * 0x100
        vertex_data = BSP_FILE + VERTICES_OFFSET + i * 0x100
        put32(data, material + 12, 0x56780010 + i)
        put32(data, material + 20, i)
        put32(data, material + 24, 1)
        put16(data, material + 0xB0, 0)
        put32(data, material + 0xB4, 3)
        put32(data, material + 0xC8, 3)
        put32(data, material + 0xD8, 3 * 56 + 3 * 20)
        put32(data, material + 0xE4, BSP_BASE + VERTICES_OFFSET + i * 0x100)

        values = [
            (1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.25, 0.5),
            (4.0, 5.0, 6.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.75, 0.5),
            (7.0, 8.0, 9.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.5, 1.0),
        ]
        for vertex, floats in enumerate(values):
            put_floats(data, vertex_data + vertex * 56, floats)
        if bad_second_vertex and i == 1:
            # Quiet NaN in the second material's first position component.
            data[vertex_data:vertex_data + 4] = bytes.fromhex("0000c07f")
        light_values = [(0.0, 0.0, 1.0, 0.0, 0.0)] * 3
        for vertex, floats in enumerate(light_values):
            put_floats(data, vertex_data + 3 * 56 + vertex * 20, floats)

        index_offset = BSP_FILE + SURFACES_OFFSET + i * 6
        for index, value in enumerate((0, 1, 2)):
            put16(data, index_offset + index * 2, value)
    return data


class RetailGeometryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
        if not compiler:
            raise unittest.SkipTest("a C compiler is required for the geometry tests")
        cls.temp = tempfile.TemporaryDirectory(prefix="mac-retail-geometry-")
        library = Path(cls.temp.name) / "libretail_geometry.so"
        subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        "-fPIC", "-shared", "-I", str(ROOT / "port/mac"),
                        str(CACHE_SOURCE), str(GEOMETRY_SOURCE), "-o", str(library)],
                       check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                                             ctypes.POINTER(RetailMaterial))
        cls.lib.mac_retail_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                            ctypes.POINTER(RetailCache),
                                            ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.mac_retail_open.restype = ctypes.c_int
        cls.lib.mac_retail_bsp_geometry.argtypes = [ctypes.POINTER(RetailCache), ctypes.c_uint32,
                                                    cls.callback_type, ctypes.c_void_p,
                                                    ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.mac_retail_bsp_geometry.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "temp"):
            cls.temp.cleanup()

    def open(self, data):
        backing = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        cache = RetailCache()
        error = ctypes.c_char_p()
        ok = self.lib.mac_retail_open(backing, len(data), ctypes.byref(cache),
                                      ctypes.byref(error))
        self.assertEqual((ok, error.value), (1, None))
        return cache, backing

    def visit(self, cache, callback):
        error = ctypes.c_char_p()
        result = self.lib.mac_retail_bsp_geometry(ctypes.byref(cache), 0, callback,
                                                  None, ctypes.byref(error))
        return result, error.value

    def test_decodes_native_vertices_light_vertices_and_uint16_indices(self):
        cache, backing = self.open(make_geometry_cache())
        seen = []

        @self.callback_type
        def callback(_context, material_ptr):
            material = material_ptr.contents
            seen.append({
                "shader": material.shader_handle,
                "vertex_count": material.vertex_count,
                "light_vertex_count": material.light_vertex_count,
                "triangle_count": material.triangle_count,
                "bitmap": material.lightmap_bitmap,
                "position": tuple(material.vertices[0].position),
                "normal": tuple(material.vertices[0].normal),
                "uv": tuple(material.vertices[0].uv),
                "light_normal": tuple(material.light_vertices[0].normal),
                "light_uv": tuple(material.light_vertices[0].uv),
                "indices": tuple(material.indices[i] for i in range(3)),
            })
            return 1

        result, error = self.visit(cache, callback)
        self.assertEqual((result, error), (1, None))
        self.assertEqual(len(backing), cache.file_size)
        self.assertEqual(seen, [{
            "shader": 0x56780010,
            "vertex_count": 3,
            "light_vertex_count": 3,
            "triangle_count": 1,
            "bitmap": 7,
            "position": (1.0, 2.0, 3.0),
            "normal": (0.0, 0.0, 1.0),
            "uv": (0.25, 0.5),
            "light_normal": (0.0, 0.0, 1.0),
            "light_uv": (0.0, 0.0),
            "indices": (0, 1, 2),
        }])

    def test_preflight_rejects_late_nan_before_any_callback(self):
        cache, _ = self.open(make_geometry_cache(material_count=2, bad_second_vertex=True))
        calls = []

        @self.callback_type
        def callback(_context, _material):
            calls.append(1)
            return 1

        result, error = self.visit(cache, callback)
        self.assertEqual(result, 0)
        self.assertIn(b"vertex float/vector", error)
        self.assertEqual(calls, [])

    def test_rejects_truncated_surface_and_lightmap_spans_before_callback(self):
        for relative_field, value, expected in (
                (0x18 + 0xFC, BSP_BASE + 0xFFC, b"surface/lightmap block"),
                (0x18 + 0x108, BSP_BASE + 0xFFC, b"surface/lightmap block")):
            data = make_geometry_cache()
            # BSP addresses are relative to its virtual base; this end address
            # cannot hold the requested block.
            put32(data, BSP_FILE + relative_field, value)
            cache, _ = self.open(data)
            calls = []

            @self.callback_type
            def callback(_context, _material):
                calls.append(1)
                return 1

            result, error = self.visit(cache, callback)
            self.assertEqual(result, 0)
            self.assertIn(expected, error)
            self.assertEqual(calls, [])

    def test_rejects_excessive_surface_lightmap_and_material_counts(self):
        mutations = (
            (BSP_FILE + 0x18 + 0xF8, 0xFFFFFFFF, b"surface/lightmap block"),
            (BSP_FILE + 0x18 + 0x104, 0xFFFFFFFF, b"surface/lightmap block"),
            (BSP_FILE + LIGHTS_OFFSET + 20, 0xFFFFFFFF, b"material block"),
        )
        for offset, value, expected in mutations:
            data = make_geometry_cache()
            put32(data, offset, value)
            cache, _ = self.open(data)
            calls = []

            @self.callback_type
            def callback(_context, _material):
                calls.append(1)
                return 1

            result, error = self.visit(cache, callback)
            self.assertEqual(result, 0)
            self.assertIn(expected, error)
            self.assertEqual(calls, [])

    def test_rejects_material_counts_vertex_ranges_and_indices_in_preflight(self):
        mutations = (
            (BSP_FILE + LIGHTS_OFFSET + 24, BSP_BASE + 0xFFC, b"material block"),
            (BSP_FILE + MATERIALS_OFFSET + 0xB4, 0xFFFFFFFF, b"material counts/type"),
            (BSP_FILE + MATERIALS_OFFSET + 0xE4, BSP_BASE + 0xFF0, b"vertex data range"),
            (BSP_FILE + SURFACES_OFFSET, 3, b"triangle vertex index"),
        )
        for offset, value, expected in mutations:
            data = make_geometry_cache()
            if offset == BSP_FILE + SURFACES_OFFSET:
                put16(data, offset, value)
            else:
                put32(data, offset, value)
            cache, _ = self.open(data)
            calls = []

            @self.callback_type
            def callback(_context, _material):
                calls.append(1)
                return 1

            result, error = self.visit(cache, callback)
            self.assertEqual(result, 0)
            self.assertIn(expected, error)
            self.assertEqual(calls, [])

    def test_callback_failure_is_reported(self):
        cache, _ = self.open(make_geometry_cache())
        calls = []

        @self.callback_type
        def callback(_context, _material):
            calls.append(1)
            return 0

        result, error = self.visit(cache, callback)
        self.assertEqual(result, 0)
        self.assertIn(b"geometry callback", error)
        self.assertEqual(calls, [1])

    def test_bad_arguments_do_not_call_callback(self):
        cache, _ = self.open(make_geometry_cache())
        calls = []

        @self.callback_type
        def callback(_context, _material):
            calls.append(1)
            return 1

        error = ctypes.c_char_p()
        result = self.lib.mac_retail_bsp_geometry(ctypes.byref(cache), 1, callback,
                                                  None, ctypes.byref(error))
        self.assertEqual(result, 0)
        self.assertIn(b"geometry arguments", error.value)
        self.assertEqual(calls, [])


if __name__ == "__main__":
    unittest.main()
