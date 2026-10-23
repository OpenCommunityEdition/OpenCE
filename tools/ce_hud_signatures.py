"""Record stock CE HUD fingerprints without distributing game pixels.

Run with an unmodified Custom Edition bitmaps.map. Only the existing layout
allowlist is updated. Runtime pixels use rasterizer_swizzle.c's Morton order
for uncompressed power-of-two textures; compressed blocks remain unchanged.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

LISTING = Path(__file__).resolve().parents[1] / "port/assets/hud/custom_edition.json"


def region(data, offset, size):
    if offset < 0 or size < 0 or offset > len(data) - size:
        raise ValueError("bitmap resource range outside file")
    return data[offset:offset + size]


def fingerprint(data, item, index):
    count, offset = struct.unpack("<II", region(item, 0x60, 8))
    if not 0 <= index < count:
        raise ValueError("bitmap index outside group")
    bitmap = region(item, offset + index * 48, 48)
    width, height, depth, kind, fmt, flags = struct.unpack_from("<6H", bitmap, 4)
    start, stored_size = struct.unpack_from("<II", bitmap, 0x18)
    if kind or depth != 1 or not 0 < width <= 4096 or not 0 < height <= 4096:
        raise ValueError("expected a 2D HUD bitmap")
    compressed = fmt in (14, 15, 16)
    if compressed:
        size = max(1, (width + 3) // 4) * max(1, (height + 3) // 4) * (8 if fmt == 14 else 16)
    else:
        bpp = {0: 1, 1: 1, 2: 1, 3: 2, 6: 2, 8: 2, 9: 2, 10: 4, 11: 4}[fmt]
        size = width * height * bpp
    if size > stored_size:
        raise ValueError("first mip exceeds stored pixels")
    pixels = region(data, start, size)
    if not compressed:
        if flags & 16:
            pitch = (width * bpp + 63) & ~63
            pixels = b"".join(pixels[y * width * bpp:(y + 1) * width * bpp] +
                              bytes(pitch - width * bpp) for y in range(height))
        else:
            if width & (width - 1) or height & (height - 1):
                raise ValueError("non-power-of-two swizzled bitmap")
            ordered = bytearray(size)
            for y in range(height):
                for x in range(width):
                    dest = shift = 0
                    for bit in range(max(width, height).bit_length() - 1):
                        if (1 << bit) < width:
                            dest |= ((x >> bit) & 1) << shift
                            shift += 1
                        if (1 << bit) < height:
                            dest |= ((y >> bit) & 1) << shift
                            shift += 1
                    src = (y * width + x) * bpp
                    ordered[dest * bpp:(dest + 1) * bpp] = pixels[src:src + bpp]
            pixels = ordered
    return dict(width=width, height=height, size=len(pixels), crc=zlib.crc32(pixels))


def update(resource, listing):
    data = resource.read_bytes()
    kind, names, index, count = struct.unpack("<4I", region(data, 0, 16))
    if kind != 1:
        raise ValueError("not a bitmap resource map")
    entries = region(data, index, count * 12)
    items = {}
    for i in range(count):
        name, size, offset = struct.unpack_from("<3I", entries, i * 12)
        start = names + name
        region(data, start, 1)
        end = data.find(b"\0", start, min(start + 1024, len(data)))
        if end < 0:
            raise ValueError("unterminated resource name")
        items[data[start:end].decode("ascii")] = (offset, size)
    result = json.loads(listing.read_text())
    for entry in result.get("assets", result.get("bitmaps", [])):
        offset, size = items[entry["tag"]]
        entry.update(fingerprint(data, region(data, offset, size), entry["bitmap"]))
    result["resource_sha256"] = hashlib.sha256(data).hexdigest()
    lines = []
    for key, value in result.items():
        text = json.dumps(key) + ": "
        if isinstance(value, list):
            text += "[\n" + ",\n".join("  " + json.dumps(entry) for entry in value) + "\n ]"
        else:
            text += json.dumps(value)
        lines.append(" " + text)
    listing.write_text("{\n" + ",\n".join(lines) + "\n}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("resource", type=Path)
    parser.add_argument("--listing", type=Path, default=LISTING)
    args = parser.parse_args()
    update(args.resource, args.listing)
