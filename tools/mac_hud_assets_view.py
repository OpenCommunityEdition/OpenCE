#!/usr/bin/env python
"""Embeds the Mac's high-res HUD textures (port/assets/hud_mac, made by
tools/mac_hud_reduce.py) as C data for the Mac build (port/mac/mac_hud_hires.h):

    python tools/mac_hud_assets_view.py OUTPUT.c

Each texture is listed with the bitmap it stands for (its tag, index and the
CRC-32 of the Xbox bitmap's pixels, from port/assets/hud/layout.json). The
data are bytes, so that a big-endian host reads the PNG as it is. Runs under
Python 2.6 (the Mac build machine's) and 3."""
from __future__ import print_function
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read(path):
    with open(path, "rb") as stream:
        return stream.read()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: mac_hud_assets_view.py OUTPUT.c")
    listing = json.loads(read(os.path.join(ROOT, "port/assets/hud/layout.json")).decode("utf-8"))["assets"]
    lines = ['#include "mac_hud_hires.h"', ""]
    table = []
    for asset in listing:
        if any(cell["kind"] == "meter" for cell in asset.get("cells", [])):
            continue
        data = read(os.path.join(ROOT, "port/assets/hud_mac", asset["name"] + ".png"))
        if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
            raise SystemExit("%s: not a PNG" % asset["name"])
        width, height, depth, colour, _, _, interlace = struct.unpack(">IIBBBBB", data[16:29])
        if depth != 8 or colour != 6 or interlace != 0:
            raise SystemExit("%s: must be 8-bit RGBA and not interlaced" % asset["name"])
        index = len(table)
        lines.append("static const unsigned char hud_%d[] = {" % index)
        for start in range(0, len(data), 24):
            lines.append("\t" + ", ".join("0x%02x" % (ord(byte) if not isinstance(byte, int) else byte)
                                          for byte in data[start:start + 24]) + ",")
        lines.append("};")
        table.append('\t{ "%s", %d, %d, %d, 0x%08xu, hud_%d, %d },' % (
            asset["tag"].replace("\\", "\\\\"), asset["bitmap"], width, height, asset["crc"], index, len(data)))
    lines += ["", "const struct mac_hud_asset mac_hud_assets[] = {"] + table + ["\t{ 0, 0, 0, 0, 0, 0, 0 },", "};",
                                                                          "const unsigned int mac_hud_assets_count = %d;" % len(table), ""]
    out = "\n".join(lines)
    path = sys.argv[1]
    if not os.path.isfile(path) or read(path).decode("ascii") != out:
        with open(path, "w") as stream:
            stream.write(out)
    print("generated %d embedded high-res HUD textures" % len(table))


main()
