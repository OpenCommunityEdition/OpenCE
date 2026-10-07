"""Tests for reading the pictures of Custom Edition maps (port/linux/game/bmp_files.c).

The tests write small bmp files, of each kind the module reads and of many
kinds it must refuse, and run the report tool port/tools/bmp_file_report.c
on them. The tool is compiled with clang, warnings as errors and undefined
behaviour trapping, so a crash or an undefined operation on a malformed file
fails the test that caused it. The module is also compiled as the game
compiles it (gnu89).
"""
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "port/linux/game/bmp_files.c"
TOOL = ROOT / "port/tools/bmp_file_report.c"

STRICT_FLAGS = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
UB_TRAP_FLAGS = ["-fsanitize=undefined", "-fsanitize-trap=undefined"]

OK = "ok"
NOT_BMP = "not a bmp file, or too short for its headers"
UNSUPPORTED_HEADER = "a bmp information header of an unknown size"
BAD_DIMENSIONS = "a picture of no size, a negative width, or too large"
UNSUPPORTED_PIXELS = "not 24-bit or 32-bit uncompressed color"
BAD_PIXEL_RANGE = "pixel rows outside the file"

BLUE_GREEN_RED_MASKS = (0x00FF0000, 0x0000FF00, 0x000000FF)


# ---------- the report tool


def find_clang():
    for candidate in (shutil.which("clang"), r"C:\Program Files\LLVM\bin\clang.exe"):
        if candidate and Path(candidate).is_file():
            return candidate
    return None


def target_flag_sets():
    if sys.platform == "win32":
        return [["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]]
    # the game is 32-bit; the module is written for any width
    return [["-m32"], []]


@pytest.fixture(scope="session")
def report_tool(tmp_path_factory):
    clang = find_clang()
    if clang is None:
        pytest.skip("clang is needed to build the report tool")
    folder = tmp_path_factory.mktemp("bmp-file-report")
    output = folder / ("bmp_file_report.exe" if sys.platform == "win32" else "bmp_file_report")
    errors = []
    for target in target_flag_sets():
        command = [clang, *target, "-std=c99", *STRICT_FLAGS, *UB_TRAP_FLAGS, "-O1", "-g",
                   f"-I{MODULE.parent}", str(MODULE), str(TOOL), "-o", str(output)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0:
            return output
        errors.append(result.stdout + result.stderr)
    # a compile error is a failure, not a missing tool: report it
    pytest.fail("could not build the report tool:\n" + "\n".join(errors))


def test_module_compiles_as_game_code_without_warnings(tmp_path):
    """The game compiles port/linux/game with -std=gnu89 -w; check the
    warnings it hides."""
    clang = find_clang()
    if clang is None:
        pytest.skip("clang is needed")
    for target in target_flag_sets():
        command = [clang, *target, "-std=gnu89", *STRICT_FLAGS, "-Wno-long-long", "-c", str(MODULE),
                   "-o", str(tmp_path / "bmp_files.o")]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0:
            return
    pytest.fail(result.stdout + result.stderr)


def report(tool, tmp_path, data, *options):
    """Runs the tool on `data` as a file; returns its exit status and the
    fields of its report."""
    path = tmp_path / "picture.bmp"
    path.write_bytes(data)
    result = subprocess.run([str(tool), *map(str, options), str(path)], capture_output=True, text=True)
    # 0: read, 1: refused; anything else is a crash or an undefined-behaviour trap
    assert result.returncode in (0, 1), (result.returncode, result.stdout, result.stderr)
    fields = {}
    for line in result.stdout.splitlines():
        key, _, value = line.partition(": ")
        fields[key] = value
    return result.returncode, fields


def status(tool, tmp_path, data):
    return report(tool, tmp_path, data)[1]["status"]


def fit(tool, tmp_path, data, width, height, shape=(0, 0)):
    """The picture of `data` fitted to `width` by `height`, as rows of
    (red, green, blue), the top row first."""
    output = tmp_path / "fit.raw"
    returncode, fields = report(tool, tmp_path, data, "--fit", width, height, *shape, output)
    assert returncode == 0, fields
    values = struct.unpack(f"<{width * height}I", output.read_bytes())
    assert all(value >> 24 == 0xFF for value in values)
    pixels = [((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF) for value in values]
    return [pixels[row * width:(row + 1) * width] for row in range(height)]


# ---------- synthetic files


def bmp(rows, bits=24, top_down=False, header_size=40, compression=0, masks=BLUE_GREEN_RED_MASKS, gap=0):
    """A bmp file of `rows` of (red, green, blue), the top row first, stored as
    Windows stores them: rows bottom to top unless `top_down`, each padded to
    four bytes, after the headers and `gap` bytes (a color table, say)."""
    width, height = len(rows[0]), len(rows)
    row_bytes = (width * bits + 31) // 32 * 4
    pixels = bytearray()
    for row in (rows if top_down else rows[::-1]):
        stored = bytearray()
        for red, green, blue in row:
            stored += bytes((blue, green, red)) + (b"\x00" if bits == 32 else b"")
        pixels += stored + bytes(row_bytes - len(stored))
    information = struct.pack("<IiiHHIIiiII", header_size, width, -height if top_down else height, 1, bits,
                              compression, len(pixels), 2835, 2835, 0, 0)
    if header_size > 40:
        # the later versions start with the bit masks; the rest stays zero
        information += struct.pack("<III", *masks) + bytes(header_size - 52)
    elif compression == 3:
        information += struct.pack("<III", *masks)
    offset = 14 + len(information) + gap
    return struct.pack("<2sIHHI", b"BM", offset + len(pixels), 0, 0, offset) + information + bytes(gap) + pixels


def patched(data, offset, value, form="<I"):
    data = bytearray(data)
    struct.pack_into(form, data, offset, value)
    return bytes(data)


# a picture whose every pixel differs
PICTURE = [[(10, 20, 30), (40, 50, 60), (70, 80, 90)],
           [(100, 110, 120), (130, 140, 150), (160, 170, 180)]]


# ---------- what is read


def test_reads_bottom_up_24_bit_rows_with_their_padding(report_tool, tmp_path):
    data = bmp(PICTURE)
    returncode, fields = report(report_tool, tmp_path, data)
    assert returncode == 0
    assert fields == {"file": str(tmp_path / "picture.bmp"), "status": OK, "width": "3", "height": "2",
                      "bits_per_pixel": "24", "top_down": "0"}
    assert fit(report_tool, tmp_path, data, 3, 2) == PICTURE


@pytest.mark.parametrize("options", [
    {"top_down": True},
    {"bits": 32},
    {"bits": 32, "compression": 3},
    {"bits": 32, "compression": 3, "header_size": 52},
    {"bits": 32, "compression": 3, "header_size": 108},
    {"bits": 32, "header_size": 124, "top_down": True},
    {"header_size": 56, "gap": 16},
], ids=["top-down", "32-bit", "bit-fields", "v2-header", "v4-header", "v5-header", "color-table"])
def test_reads_every_kind_of_uncompressed_color(report_tool, tmp_path, options):
    assert fit(report_tool, tmp_path, bmp(PICTURE, **options), 3, 2) == PICTURE


def test_averages_the_pixels_a_smaller_picture_covers(report_tool, tmp_path):
    rows = [[(0, 0, 0), (2, 4, 8), (200, 0, 0), (201, 0, 0)],
            [(4, 8, 16), (6, 12, 24), (202, 0, 0), (203, 0, 0)],
            [(0, 255, 0), (0, 255, 0), (1, 1, 1), (2, 2, 2)],
            [(0, 255, 0), (0, 254, 0), (3, 3, 3), (4, 4, 4)]]
    assert fit(report_tool, tmp_path, bmp(rows), 2, 2) == [
        [(3, 6, 12), (202, 0, 0)],
        [(0, 255, 0), (3, 3, 3)],
    ]


def test_repeats_the_pixels_of_a_larger_picture(report_tool, tmp_path):
    rows = [[(10, 20, 30), (40, 50, 60)]]
    assert fit(report_tool, tmp_path, bmp(rows), 4, 2) == [
        [(10, 20, 30), (10, 20, 30), (40, 50, 60), (40, 50, 60)],
    ] * 2


def test_takes_the_middle_of_a_wider_picture(report_tool, tmp_path):
    rows = [[(x * 20, y * 40, 7) for x in range(8)] for y in range(4)]
    assert fit(report_tool, tmp_path, bmp(rows), 4, 4, shape=(1, 1)) == [
        [((2 + x) * 20, y * 40, 7) for x in range(4)] for y in range(4)]


def test_takes_the_middle_of_a_taller_picture(report_tool, tmp_path):
    rows = [[(x * 50, y * 30, 1) for x in range(2)] for y in range(6)]
    assert fit(report_tool, tmp_path, bmp(rows), 2, 2, shape=(1, 1)) == [
        [(x * 50, (2 + y) * 30, 1) for x in range(2)] for y in range(2)]


def test_reads_the_largest_picture_it_accepts(report_tool, tmp_path):
    data = bmp([[(1, 2, 3)] * 8192])
    assert fit(report_tool, tmp_path, data, 4, 1) == [[(1, 2, 3)] * 4]


# ---------- what is refused


VALID = bmp(PICTURE)
BIT_FIELDS = bmp(PICTURE, bits=32, compression=3)


@pytest.mark.parametrize("data, expected", [
    pytest.param(b"", NOT_BMP, id="empty"),
    pytest.param(VALID[:53], NOT_BMP, id="short"),
    pytest.param(b"MB" + VALID[2:], NOT_BMP, id="signature"),
    pytest.param(patched(VALID, 14, 12), UNSUPPORTED_HEADER, id="core-header"),
    pytest.param(patched(VALID, 14, 64), UNSUPPORTED_HEADER, id="os2-header"),
    pytest.param(patched(VALID, 18, 0), BAD_DIMENSIONS, id="no-width"),
    pytest.param(patched(VALID, 18, -3, "<i"), BAD_DIMENSIONS, id="negative-width"),
    pytest.param(patched(VALID, 18, 8193), BAD_DIMENSIONS, id="too-wide"),
    pytest.param(patched(VALID, 22, 0), BAD_DIMENSIONS, id="no-height"),
    pytest.param(patched(VALID, 22, 8193), BAD_DIMENSIONS, id="too-tall"),
    pytest.param(patched(VALID, 22, -8193, "<i"), BAD_DIMENSIONS, id="too-tall-top-down"),
    pytest.param(patched(VALID, 22, 0x80000000), BAD_DIMENSIONS, id="most-negative-height"),
    pytest.param(patched(VALID, 26, 2, "<H"), UNSUPPORTED_PIXELS, id="planes"),
    pytest.param(patched(VALID, 28, 8, "<H"), UNSUPPORTED_PIXELS, id="8-bit"),
    pytest.param(patched(VALID, 28, 16, "<H"), UNSUPPORTED_PIXELS, id="16-bit"),
    pytest.param(patched(VALID, 30, 1), UNSUPPORTED_PIXELS, id="run-length"),
    pytest.param(patched(VALID, 30, 3), UNSUPPORTED_PIXELS, id="24-bit-bit-fields"),
    pytest.param(patched(VALID, 30, 4), UNSUPPORTED_PIXELS, id="jpeg"),
    pytest.param(patched(VALID, 30, 5), UNSUPPORTED_PIXELS, id="png"),
    pytest.param(patched(BIT_FIELDS, 54, 0xFF000000), UNSUPPORTED_PIXELS, id="other-masks"),
    pytest.param(BIT_FIELDS[:60], NOT_BMP, id="short-masks"),
    pytest.param(patched(VALID, 10, 20), BAD_PIXEL_RANGE, id="pixels-in-headers"),
    pytest.param(patched(VALID, 10, len(VALID)), BAD_PIXEL_RANGE, id="pixels-after-end"),
    pytest.param(VALID[:-1], BAD_PIXEL_RANGE, id="short-last-row"),
    pytest.param(patched(patched(VALID, 18, 8192), 22, 8192), BAD_PIXEL_RANGE, id="rows-beyond-file"),
])
def test_refuses_what_it_does_not_read(report_tool, tmp_path, data, expected):
    returncode, fields = report(report_tool, tmp_path, data)
    assert returncode == 1
    assert fields["status"] == expected


def test_every_truncation_is_refused(report_tool, tmp_path):
    for data in (VALID, BIT_FIELDS, bmp(PICTURE, header_size=124, top_down=True)):
        for length in range(len(data)):
            assert status(report_tool, tmp_path, data[:length]) != OK, length
        assert status(report_tool, tmp_path, data) == OK


def test_corrupted_headers_never_crash_the_reader(report_tool, tmp_path):
    """Random bytes over the headers: the reader may read or refuse the file,
    and a file it reads must fit without a fault."""
    generator = random.Random(1790661460)
    sources = (VALID, BIT_FIELDS, bmp(PICTURE, bits=32, header_size=108, compression=3))
    for trial in range(200):
        data = bytearray(generator.choice(sources))
        for _ in range(generator.randint(1, 6)):
            offset = generator.randrange(min(len(data), 70))
            data[offset] = generator.randrange(256)
        returncode, fields = report(report_tool, tmp_path, bytes(data))
        if returncode == 0:
            fit(report_tool, tmp_path, bytes(data), 5, 3, shape=(7, 3))
