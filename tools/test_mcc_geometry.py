"""Exercise MCC model/BSP conversion, the real vertex compressor, and cleanup.

GPU allocation is mocked. These checks establish data/ownership behavior;
they do not substitute for an in-game rendering test.
"""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


def geometry_map():
    base, tag_offset = 0x50000000, 0xC00
    data = bytearray(0x1700)
    def u(o, v):
        struct.pack_into("<I", data, o, v)
    def cc(s):
        return int.from_bytes(s.encode(), "big")
    for o, v in {0: cc("head"), 4: 13, 8: len(data), 16: tag_offset, 20: 0xB00,
                 0x68: 7, 0x7FC: cc("foot"), tag_offset: base+40,
                 tag_offset+4: 0xE1740000, tag_offset+12: 3,
                 tag_offset+20: 0xB00, tag_offset+28: 204, tag_offset+32: 210,
                 tag_offset+36: cc("tags")}.items():
        u(o, v)
    data[32:41] = b"geometry\0"
    data[64:69] = b"test\0"
    for i, (group, name, address) in enumerate([
            ("scnr", 0x90, 0x200), ("sbsp", 0x95, 0), ("mod2", 0x99, 0x800)]):
        struct.pack_into("<8I", data, tag_offset+40+i*32, cc(group), 0xFFFFFFFF,
                         0xFFFFFFFF, 0xE1740000+i*0x10001, base+name,
                         base+address if address else 0, 0, 0)
    data[tag_offset+0x90:tag_offset+0x9D] = b"scnr\0bsp\0mod\0"
    bsp = base+0x4000000-0x300
    struct.pack_into("<6I", data, 0x800, bsp+24, 0, 0, 0, 0, cc("sbsp"))
    struct.pack_into("<3I", data, tag_offset+0x200+0x5A4, 1, base+0x7B0, 0)
    struct.pack_into("<8I", data, tag_offset+0x7B0, 0x800, 0x300, bsp, 0,
                     cc("sbsp"), base+0x95, 0, 0xE1750001)
    for o, v in {0x8B8: 1, 0x8BC: base+0xA00, 0x8D0: 1, 0x8D4: base+0x900,
                 0x8DC: 1, 0x8E0: base+0xAA0, 0x924: 1, 0x928: base+0x940,
                 0x984: 1, 0x988: 1, 0x994: 4, 0x998: 3}.items():
        u(tag_offset+o, v)
    data[tag_offset+0x946:tag_offset+0x948] = b"\xff\xff"
    for i in range(3):
        struct.pack_into("<14f2h2f", data, 0xB00+i*68,
                         float(i == 1), float(i == 2), 0., 0., 0., 1., 0., 1., 0.,
                         1., 0., 0., 0., 0., 0, 0, 1., 0.)
    struct.pack_into("<3H", data, 0xB00+204, 0, 1, 2)
    return data


@pytest.fixture(scope="session")
def geometry_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler or sys.platform != "win32":
        pytest.skip("this harness uses the native x86 Windows game headers")
    output = tmp_path_factory.mktemp("mcc-geometry")/"geometry.exe"
    includes = ["port/windows/include/crt", "port/windows/include", "source", "source/cseries",
                "source/math", "source/tag_files", "source/rasterizer", "source/structures",
                "source/cache", "source/render", "port/include/xdk", "port/linux/game"]
    command = [compiler, "--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-fms-extensions",
               "-std=gnu89", "-O1", "-DDEBUG", "-Dxbox", "-D_CRT_SECURE_NO_WARNINGS",
               "-Wno-nonportable-include-path", "-Wno-c99-compat", "-Wno-visibility",
               "-include", str(ROOT/"port/windows/include/halo_windows_prefix.h"),
               "-iquote", str(ROOT/"port/linux/include")]
    command += ["-I"+str(ROOT/p) for p in includes]
    command += [str(ROOT/p) for p in ["port/tools/mcc_geometry_check.c",
                "port/linux/game/mcc_geometry.c", "port/linux/game/mcc_cache_format.c",
                "source/rasterizer/rasterizer_geometry.c"]]
    command += ["-o", str(output), "-lkernel32"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


def run(tool, path, *args):
    result = subprocess.run([str(tool), str(path), *map(str, args)], capture_output=True, text=True)
    assert result.returncode in (0, 1), result.stderr
    fields = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    assert fields["active_buffers"] == "0", result.stderr
    return result.returncode, fields


def test_model_conversion_and_dispose(geometry_tool, tmp_path):
    path = tmp_path/"geometry.map"
    path.write_bytes(geometry_map())
    code, fields = run(geometry_tool, path)
    assert code == 0
    assert fields["created_buffers"] == "2"


@pytest.mark.parametrize("failure_at", [0, 1])
def test_partial_gpu_failure_cleanup(geometry_tool, tmp_path, failure_at):
    path = tmp_path/"geometry.map"
    path.write_bytes(geometry_map())
    code, _ = run(geometry_tool, path, failure_at)
    assert code == 1


@pytest.mark.parametrize("shader,expected", [(0xffff, 0), (1, 1), (0x8000, 1)])
def test_absent_model_material_keeps_native_skip_semantics(geometry_tool, tmp_path, shader, expected):
    data = geometry_map()
    struct.pack_into("<H", data, 0xC00+0x940+4, shader)
    path = tmp_path / "material.map"
    path.write_bytes(data)
    code, _ = run(geometry_tool, path)
    assert code == expected


@pytest.mark.parametrize("local", [False, True])
def test_many_node_palette_is_owned_by_part(geometry_tool, tmp_path, local):
    data = geometry_map()
    # Give the model 64 real node records outside the original tag payload.
    node_offset = len(data) - 0xC00
    data.extend(bytes(64 * 156))
    struct.pack_into("<I", data, 8, len(data))
    struct.pack_into("<I", data, 20, len(data) - 0xC00)
    struct.pack_into("<2I", data, 0xC00 + 0x8B8, 64, 0x50000000 + node_offset)
    if local:
        struct.pack_into("<I", data, 0xC00 + 0x800, 2)
        data[0xC00 + 0x940 + 0x6B] = 1
        data[0xC00 + 0x940 + 0x6C] = 63
    else:
        for vertex in range(3):
            struct.pack_into("<2h", data, 0xB00 + vertex * 68 + 56, 63, 63)
    path = tmp_path / "palette.map"
    path.write_bytes(data)
    code, fields = run(geometry_tool, path)
    assert code == 0
    assert fields["palette_parts"] == "1"
    assert fields["palette_sum"] == "63"


@pytest.mark.parametrize("offset,encoding,value", [
    (0xC00+0x940+0x64, "I", 0xFFFFFFFC),
    (0xB00+204, "H", 7),
    (0xB00+56, "h", 9),
    (0xB00+12, "f", float("nan")),
])
def test_invalid_model_data_is_refused(geometry_tool, tmp_path, offset, encoding, value):
    data = geometry_map()
    struct.pack_into("<"+encoding, data, offset, value)
    path = tmp_path/"bad.map"
    path.write_bytes(data)
    code, _ = run(geometry_tool, path)
    assert code == 1


def test_optional_real_geometry(geometry_tool):
    fixture = os.environ.get("MCC_TEST_MAP")
    if not fixture:
        pytest.skip("set MCC_TEST_MAP to opt in to local fixture conversion")
    code, fields = run(geometry_tool, Path(fixture))
    assert code == 0
    assert int(fields["created_buffers"]) > 0
