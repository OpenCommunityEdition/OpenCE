"""Independent version-13 reader tests using generated, non-game cache data.

Set MCC_TEST_MAP to a legally available real map to opt in to fixture auditing.
The fixture remains outside the repository and no map contents are checked in.
"""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x50000000
TAG_OFFSET = 0xB00


def put32(data, offset, value):
    struct.pack_into("<I", data, offset, value)


def fourcc(text):
    return int.from_bytes(text.encode("ascii"), "big")


def minimal_map():
    data = bytearray(TAG_OFFSET + 0x650)
    for offset, value in {
        0: fourcc("head"), 4: 13, 8: len(data), 16: TAG_OFFSET,
        20: 0x650, 0x68: 7, 0x7FC: fourcc("foot"),
        TAG_OFFSET: BASE + 40, TAG_OFFSET + 4: 0xE1740000,
        TAG_OFFSET + 12: 2, TAG_OFFSET + 36: fourcc("tags"),
    }.items():
        put32(data, offset, value)
    data[32:40] = b"fixture\0"
    data[64:72] = b"testing\0"
    for i, (group, handle, name, address) in enumerate([
        ("scnr", 0xE1740000, BASE+0x68, BASE+0x80),
        ("sbsp", 0xE1750001, BASE+0x6D, 0),
    ]):
        struct.pack_into("<8I", data, TAG_OFFSET+40+i*32,
                         fourcc(group), 0xFFFFFFFF, 0xFFFFFFFF,
                         handle, name, address, 0, 0)
    data[TAG_OFFSET+0x68:TAG_OFFSET+0x71] = b"scnr\0bsp\0"
    bsp_address = BASE+0x4000000-0x300
    struct.pack_into("<6I", data, 0x800, bsp_address+24, 0, 0, 0, 0, fourcc("sbsp"))
    struct.pack_into("<3I", data, TAG_OFFSET+0x80+0x5A4, 1, BASE+0x630, 0)
    struct.pack_into("<8I", data, TAG_OFFSET+0x630,
                     0x800, 0x300, bsp_address, 0, fourcc("sbsp"), BASE+0x6D, 0, 0xE1750001)
    return data


@pytest.fixture(scope="session")
def report_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for the standalone MCC reader")
    output = tmp_path_factory.mktemp("mcc-reader") / ("report.exe" if sys.platform == "win32" else "report")
    command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror"]
    if sys.platform == "win32":
        command += ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    command += ["-I" + str(ROOT/"port/linux/game"), str(ROOT/"port/linux/game/mcc_cache_format.c"),
                str(ROOT/"port/tools/mcc_cache_report.c"), "-o", str(output)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    return output


def audit(tool, path):
    result = subprocess.run([str(tool), str(path)], capture_output=True, text=True)
    return result.returncode, dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)


def test_complete_minimal_map(report_tool, tmp_path):
    path = tmp_path/"different_filename.map"
    path.write_bytes(minimal_map())
    code, report = audit(report_tool, path)
    assert code == 0, report
    assert report["name"] == "fixture"
    assert report["tag_base"] == "0x50000000"
    assert report["bsps"] == "1"


@pytest.mark.parametrize("offset,value,error", [
    (4, 609, "not a Halo 1 MCC version-13 cache"),
    (4, 5, "not a Halo 1 MCC version-13 cache"),
    (0x7FC, 0, "invalid MCC cache header"),
    (8, 0x7FFFFFFF, "invalid MCC map file length"),
    (0x10, 0xFFFFFFF0, "invalid MCC tag data range"),
    (0x14, 0x4000001, "invalid MCC tag data range"),
    (0x60, 2, "MCC map is not singleplayer or multiplayer"),
    (TAG_OFFSET, 0x10, "invalid MCC tag index"),
    (TAG_OFFSET+12, 0xFFFFFFFF, "invalid MCC tag index"),
    (TAG_OFFSET+40+16, BASE-1, "invalid MCC tag entry"),
    (TAG_OFFSET+40+12, 0xE1740001, "invalid MCC tag entry"),
    (TAG_OFFSET+40+24, 1, "external indexed MCC tags require a resource provider"),
    (TAG_OFFSET+0x630+4, 0xFFFFFFFF, "invalid MCC BSP or vertex range"),
    (0x804, 0xFFFFFFFF, "invalid MCC BSP or vertex range"),
])
def test_invalid_ranges_are_rejected(report_tool, tmp_path, offset, value, error):
    data = minimal_map()
    put32(data, offset, value)
    path = tmp_path/"bad.map"
    path.write_bytes(data)
    code, report = audit(report_tool, path)
    assert code == 1
    assert report["status"] == error


def test_unterminated_name(report_tool, tmp_path):
    data = minimal_map()
    data[32:64] = b"a" * 32
    path = tmp_path/"bad.map"
    path.write_bytes(data)
    code, report = audit(report_tool, path)
    assert code == 1
    assert report["status"] == "unterminated MCC header string"


def test_optional_real_mcc_map(report_tool):
    fixture = os.environ.get("MCC_TEST_MAP")
    if not fixture:
        pytest.skip("set MCC_TEST_MAP to opt in to a local MCC fixture")
    code, report = audit(report_tool, Path(fixture))
    assert code == 0, report
    assert int(report["tag_count"]) > 0
    assert int(report["bsps"]) > 0
