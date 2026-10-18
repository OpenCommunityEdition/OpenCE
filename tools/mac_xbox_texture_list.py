#!/usr/bin/env python3
"""List the uncompressed bitmaps of an Xbox map for tools/mac_xbox_texture_probe.c:
one line each, "d3d_format width height file_offset linear", from the map's own
bitmap tags (read with Python, so on any host). Prints nothing the map holds."""
import struct, sys, zlib

TABLE = {0: 0x19, 1: 0x00, 2: 0x01, 3: 0x1a, 6: 0x05, 8: 0x02, 9: 0x04, 10: 0x07, 11: 0x06}
LINEAR = {0x19: 0x1f, 0x00: 0x13, 0x01: 0x1b, 0x1a: 0x20, 0x05: 0x11, 0x02: 0x10, 0x04: 0x1d, 0x07: 0x1e, 0x06: 0x12}
BASE = 0x803a6000

def load(path):
    d = open(path, 'rb').read()
    return d[:0x800] + zlib.decompressobj().decompress(d[0x800:])

def main():
    image = load(sys.argv[1])
    off, _ = struct.unpack_from('<II', image, 0x10)
    count, = struct.unpack_from('<I', image, off + 0xc)
    inst = struct.unpack_from('<I', image, off)[0] - BASE + off
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else 400
    out = 0
    for i in range(count):
        group, _, _, _, _, address = struct.unpack_from('<6I', image, inst + i * 32)
        if group != 0x6269746d:
            continue
        root = address - BASE + off
        n, p = struct.unpack_from('<II', image, root + 0x60)
        for k in range(n):
            e = p - BASE + off + k * 0x30
            _, w, h, depth, typ, fmt, flags = struct.unpack_from('<4sHHHHHH', image, e)
            pixels, = struct.unpack_from('<I', image, e + 0x18)
            if typ != 0 or fmt not in TABLE or depth != 1 or (w & (w - 1)) or (h & (h - 1)):
                continue
            linear = 1 if flags & 0x10 else 0
            d3d = LINEAR[TABLE[fmt]] if linear else TABLE[fmt]
            print(d3d, w, h, pixels, linear)
            out += 1
            if out >= limit:
                return

main()
