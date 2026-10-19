#!/usr/bin/env python3
"""Compares a captured frame with its approved reference (docs/macos_host.md,
"Frame captures").

    python tools/frame_compare.py frame.png reference.png [--difference out.png]
        [--threshold 8] [--fraction 0.005] [--largest 64]

A pixel differs when one of its channels is more than `threshold` from the
reference's: less is a GPU's rounding. The frame matches when no more than
`fraction` of its pixels differ (an edge rasterised a pixel over) and none
by more than `largest` (a pixel drawn wrong). The difference image is the
reference, faint, with the differing pixels red. Exits 0 when the frame
matches, 1 when it does not.

The PNGs are read and written with zlib alone: 8-bit RGB or RGBA, not
interlaced, which is what ImageIO writes.
"""

import argparse
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path

SIGNATURE = b"\x89PNG\r\n\x1a\n"


@dataclass(frozen=True)
class Image:
    """RGBA, 8 bits a channel, rows from the top"""
    width: int
    height: int
    pixels: bytes

    def pixel(self, x, y):
        at = (y * self.width + x) * 4
        return tuple(self.pixels[at:at + 4])


@dataclass(frozen=True)
class Tolerance:
    threshold: int = 8
    fraction: float = 0.005
    largest: int = 64


@dataclass(frozen=True)
class Comparison:
    frame: Image
    reference: Image
    tolerance: Tolerance
    differing: int
    largest: int

    @property
    def same_size(self):
        return (self.frame.width, self.frame.height) == (self.reference.width, self.reference.height)

    @property
    def matches(self):
        return (self.same_size and self.differing <= self.tolerance.fraction * self.frame.width * self.frame.height
                and self.largest <= self.tolerance.largest)

    def describe(self):
        if not self.same_size:
            return (f"the frame is {self.frame.width}x{self.frame.height}, "
                    f"its reference {self.reference.width}x{self.reference.height}")
        verdict = "matches" if self.matches else "does not match"
        return (f"{verdict}: {self.differing} of {self.frame.width * self.frame.height} pixels differ by more than "
                f"{self.tolerance.threshold}, the most by {self.largest}")

    def difference(self):
        """the reference at a quarter of its brightness, the differing pixels red"""
        out = bytearray(len(self.reference.pixels))
        threshold = self.tolerance.threshold
        for at in range(0, len(out), 4):
            if max(abs(a - b) for a, b in zip(self.frame.pixels[at:at + 4], self.reference.pixels[at:at + 4])) \
                    > threshold:
                out[at:at + 4] = b"\xff\x00\x00\xff"
            else:
                red, green, blue, _ = self.reference.pixels[at:at + 4]
                out[at:at + 4] = bytes((red // 4, green // 4, blue // 4, 255))
        return Image(self.reference.width, self.reference.height, bytes(out))


def compare(frame, reference, tolerance=Tolerance()):
    if (frame.width, frame.height) != (reference.width, reference.height):
        return Comparison(frame, reference, tolerance, frame.width * frame.height, 255)
    differing = 0
    largest = 0
    for at in range(0, len(frame.pixels), 4):
        worst = max(abs(a - b) for a, b in zip(frame.pixels[at:at + 4], reference.pixels[at:at + 4]))
        largest = max(largest, worst)
        if worst > tolerance.threshold:
            differing += 1
    return Comparison(frame, reference, tolerance, differing, largest)


# ---------- PNGs

def _chunks(data):
    at = len(SIGNATURE)
    while at + 8 <= len(data):
        length, kind = struct.unpack(">I4s", data[at:at + 8])
        if at + 12 + length > len(data):
            raise ValueError("a PNG chunk runs past the file")
        yield kind, data[at + 8:at + 8 + length]
        at += 12 + length


def _paeth(left, up, corner):
    estimate = left + up - corner
    to_left, to_up, to_corner = abs(estimate - left), abs(estimate - up), abs(estimate - corner)
    if to_left <= to_up and to_left <= to_corner:
        return left
    return up if to_up <= to_corner else corner


def read_png(path):
    data = Path(path).read_bytes()
    if not data.startswith(SIGNATURE):
        raise ValueError(f"{path} is not a PNG")
    header = None
    compressed = bytearray()
    for kind, body in _chunks(data):
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            compressed += body
        elif kind == b"IEND":
            break
    if header is None:
        raise ValueError(f"{path} has no header")
    width, height, depth, colour_type, _, _, interlace = header
    if depth != 8 or colour_type not in (2, 6) or interlace != 0:
        raise ValueError(f"{path}: only 8-bit RGB and RGBA, not interlaced, are read")
    size = 4 if colour_type == 6 else 3
    stride = width * size
    raw = zlib.decompress(bytes(compressed))
    if len(raw) != height * (stride + 1):
        raise ValueError(f"{path}: its pixels are {len(raw)} bytes, not {height * (stride + 1)}")
    pixels = bytearray()
    previous = bytearray(stride)
    for y in range(height):
        kind = raw[y * (stride + 1)]
        row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for at in range(stride):
            left = row[at - size] if at >= size else 0
            up = previous[at]
            if kind == 1:
                row[at] = (row[at] + left) & 255
            elif kind == 2:
                row[at] = (row[at] + up) & 255
            elif kind == 3:
                row[at] = (row[at] + (left + up) // 2) & 255
            elif kind == 4:
                row[at] = (row[at] + _paeth(left, up, previous[at - size] if at >= size else 0)) & 255
            elif kind != 0:
                raise ValueError(f"{path}: row {y} has filter {kind}")
        if size == 4:
            pixels += row
        else:
            for at in range(0, stride, 3):
                pixels += row[at:at + 3] + b"\xff"
        previous = row
    return Image(width, height, bytes(pixels))


def write_png(path, image):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    stride = image.width * 4
    raw = b"".join(b"\x00" + image.pixels[y * stride:(y + 1) * stride] for y in range(image.height))
    Path(path).write_bytes(SIGNATURE + chunk(b"IHDR", struct.pack(">IIBBBBB", image.width, image.height, 8, 6, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def main(arguments=None):
    parser = argparse.ArgumentParser(description="Compares a captured frame with its approved reference.")
    parser.add_argument("frame", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--difference", type=Path, help="where to write the difference image")
    parser.add_argument("--threshold", type=int, default=Tolerance.threshold)
    parser.add_argument("--fraction", type=float, default=Tolerance.fraction)
    parser.add_argument("--largest", type=int, default=Tolerance.largest)
    options = parser.parse_args(arguments)
    result = compare(read_png(options.frame), read_png(options.reference),
                     Tolerance(options.threshold, options.fraction, options.largest))
    print(f"{options.frame}: {result.describe()}")
    if options.difference and result.same_size:
        write_png(options.difference, result.difference())
    return 0 if result.matches else 1


if __name__ == "__main__":
    sys.exit(main())
