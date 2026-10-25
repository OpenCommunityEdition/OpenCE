"""tools/frame_compare.py: its PNGs and its tolerance, on synthetic images.

A frame matches its reference when few of its pixels differ by more than a
GPU's rounding, and none by much more (docs/macos_host.md, "Frame
captures").
"""

import struct
import subprocess
import sys
import zlib
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import frame_compare  # noqa: E402
from tools.frame_compare import Image, Tolerance, compare  # noqa: E402


def gradient(width=64, height=48):
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            pixels += bytes((x * 4 % 256, y * 5 % 256, (x + y) % 256, 255))
    return Image(width, height, bytes(pixels))


def changed(image, change):
    """image with change(x, y, rgba) -> rgba applied to each pixel"""
    pixels = bytearray(image.pixels)
    for y in range(image.height):
        for x in range(image.width):
            at = (y * image.width + x) * 4
            pixels[at : at + 4] = bytes(change(x, y, tuple(pixels[at : at + 4])))
    return Image(image.width, image.height, bytes(pixels))


def clamp(value):
    return max(0, min(255, value))


# ---------- the tolerance


def test_an_identical_frame_matches():
    result = compare(gradient(), gradient())
    assert result.matches and result.differing == 0 and result.largest == 0


def test_rounding_within_the_threshold_matches():
    # every pixel one or two steps off, as another GPU's rounding is
    nudged = changed(gradient(), lambda x, y, p: (clamp(p[0] + 2), clamp(p[1] - 1), p[2], p[3]))
    result = compare(nudged, gradient())
    assert result.matches and result.differing == 0 and result.largest == 2


def test_a_few_edge_pixels_beyond_the_threshold_match():
    # (an edge rasterised a pixel over: 3 of 3072 pixels, within 0.5%)
    edge = changed(
        gradient(),
        lambda x, y, p: (clamp(p[0] + 40), p[1], p[2], p[3]) if (x, y) in {(10, 10), (11, 10), (12, 10)} else p,
    )
    result = compare(edge, gradient())
    assert result.matches and result.differing == 3 and result.largest == 40


def test_too_many_differing_pixels_do_not_match():
    # a stripe of 64 pixels (2%) a little off: past the fraction
    stripe = changed(gradient(), lambda x, y, p: (p[0], p[1] + 20, p[2], p[3]) if y == 5 else p)
    result = compare(stripe, gradient())
    assert not result.matches and result.differing == 64
    assert "64 of 3072 pixels" in result.describe()


def test_one_pixel_far_off_does_not_match():
    # a pixel wrong outright, not rounded: past the largest difference
    wrong = changed(gradient(), lambda x, y, p: ((p[0] + 128) % 256, p[1], p[2], p[3]) if (x, y) == (30, 30) else p)
    result = compare(wrong, gradient())
    assert not result.matches and result.differing == 1 and result.largest > 64


def test_alpha_counts():
    clear = changed(gradient(), lambda x, y, p: (p[0], p[1], p[2], 0))
    assert not compare(clear, gradient()).matches


def test_the_tolerance_can_be_set():
    nudged = changed(gradient(), lambda x, y, p: (clamp(p[0] + 6), p[1], p[2], p[3]))
    assert compare(nudged, gradient()).matches
    assert not compare(nudged, gradient(), Tolerance(threshold=4)).matches
    stripe = changed(gradient(), lambda x, y, p: (p[0], p[1] + 20, p[2], p[3]) if y == 5 else p)
    assert compare(stripe, gradient(), Tolerance(fraction=0.05)).matches


def test_frames_of_other_sizes_do_not_match():
    result = compare(gradient(64, 48), gradient(48, 64))
    assert not result.matches and "64x48" in result.describe() and "48x64" in result.describe()


def test_the_difference_image_marks_what_differs():
    wrong = changed(gradient(), lambda x, y, p: ((p[0] + 128) % 256, p[1], p[2], p[3]) if (x, y) == (30, 30) else p)
    picture = compare(wrong, gradient()).difference()
    assert (picture.width, picture.height) == (64, 48)
    assert picture.pixel(30, 30) == (255, 0, 0, 255)
    # (the rest faint, so that what differs stands out)
    red, green, blue, alpha = picture.pixel(5, 5)
    assert alpha == 255 and max(red, green, blue) < 128


# ---------- PNGs


def test_a_png_round_trips(tmp_path):
    frame_compare.write_png(tmp_path / "frame.png", gradient())
    assert frame_compare.read_png(tmp_path / "frame.png") == gradient()


def png(width, height, colour_type, rows):
    """a PNG of 8-bit rows, each (filter, bytes), as other writers make them"""

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    raw = b"".join(bytes((kind,)) + data for kind, data in rows)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, colour_type, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw))
        + chunk(b"IEND", b"")
    )


def paeth(left, up, corner):
    estimate = left + up - corner
    nearest = min((abs(estimate - left), 0, left), (abs(estimate - up), 1, up), (abs(estimate - corner), 2, corner))
    return nearest[2]


def filtered(kind, row, previous, size):
    """row filtered as PNG's filter `kind` does, `size` bytes a pixel"""
    out = bytearray()
    for at, value in enumerate(row):
        left = row[at - size] if at >= size else 0
        up = previous[at]
        corner = previous[at - size] if at >= size else 0
        predicted = [0, left, up, (left + up) // 2, paeth(left, up, corner)][kind]
        out.append((value - predicted) % 256)
    return bytes(out)


@pytest.mark.parametrize("colour_type,size", [(6, 4), (2, 3)])
def test_each_filter_and_rgb_are_read(tmp_path, colour_type, size):
    image = gradient(8, 10)
    rows, previous = [], bytes(8 * size)
    for y in range(10):
        row = b"".join(image.pixels[(y * 8 + x) * 4 : (y * 8 + x) * 4 + size] for x in range(8))
        rows.append((y % 5, filtered(y % 5, row, previous, size)))
        previous = row
    (tmp_path / "frame.png").write_bytes(png(8, 10, colour_type, rows))
    assert frame_compare.read_png(tmp_path / "frame.png") == image


def test_what_is_not_an_8_bit_png_is_refused(tmp_path):
    (tmp_path / "text.png").write_bytes(b"not a png")
    with pytest.raises(ValueError):
        frame_compare.read_png(tmp_path / "text.png")
    sixteen = bytearray(png(1, 1, 6, [(0, bytes(8))]))
    sixteen[24] = 16
    (tmp_path / "sixteen.png").write_bytes(bytes(sixteen))
    with pytest.raises(ValueError):
        frame_compare.read_png(tmp_path / "sixteen.png")


# ---------- the command


def test_the_command_says_whether_a_frame_matches(tmp_path):
    frame_compare.write_png(tmp_path / "reference.png", gradient())
    frame_compare.write_png(tmp_path / "same.png", gradient())
    stripe = changed(gradient(), lambda x, y, p: (p[0], p[1] + 20, p[2], p[3]) if y == 5 else p)
    frame_compare.write_png(tmp_path / "stripe.png", stripe)
    tool = [sys.executable, str(ROOT / "tools/frame_compare.py")]
    same = subprocess.run(
        tool + [str(tmp_path / "same.png"), str(tmp_path / "reference.png")], capture_output=True, text=True
    )
    assert same.returncode == 0 and "matches" in same.stdout
    stripe_run = subprocess.run(
        tool
        + [
            str(tmp_path / "stripe.png"),
            str(tmp_path / "reference.png"),
            "--difference",
            str(tmp_path / "difference.png"),
        ],
        capture_output=True,
        text=True,
    )
    assert stripe_run.returncode == 1 and "64 of 3072 pixels" in stripe_run.stdout
    assert frame_compare.read_png(tmp_path / "difference.png").pixel(0, 5) == (255, 0, 0, 255)
