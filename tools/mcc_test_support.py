"""Small native compiler and generated PNG fixtures for MCC regressions."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zlib

import pytest

ROOT = Path(__file__).resolve().parents[1]


def compile_tool(work, sources):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    exe = work / ("test.exe" if sys.platform == "win32" else "test")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if sys.platform == "win32" else []
    result = subprocess.run([clang, *flags, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
                             "-I", str(work), "-I", str(ROOT / "port/linux/game"),
                             *map(str, sources), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def png(width=3, height=2, channels=4, filter_type=0, *, extra=b"", raw=None):
    pixels = bytes((i * 37 + 11) % 256 for i in range(width * height * channels))
    stride = width * channels
    def paeth(a, b, c):
        p = a + b - c
        return min((a, b, c), key=lambda v: abs(p - v))
    rows = bytearray()
    for y in range(height):
        rows.append(filter_type)
        for x in range(stride):
            a = pixels[y * stride + x - channels] if x >= channels else 0
            b = pixels[(y - 1) * stride + x] if y else 0
            c = pixels[(y - 1) * stride + x - channels] if y and x >= channels else 0
            predictor = (0, a, b, (a + b) // 2, paeth(a, b, c))[filter_type] if filter_type < 5 else 0
            rows.append((pixels[y * stride + x] - predictor) % 256)
    header = struct.pack(">IIBBBBB", width, height, 8, 6 if channels == 4 else 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + extra + chunk(b"IDAT", zlib.compress(rows if raw is None else raw)) + chunk(b"IEND", b""), pixels
