#!/usr/bin/env python
"""Generate a narrow native view of the Xbox rasterizer default textures.

The original initializer fills three CPU lock buffers with Xbox A4R4G4B4
words.  Its scalar ``word`` stores are little-endian on Xbox but big-endian on
PowerPC.  This view changes only those stores to explicit little-endian bytes;
all allocation, locking, pixel values, and error handling remain the original
source.
"""

from __future__ import print_function

import io
import os
from optparse import OptionParser


ROOT = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
SOURCE = os.path.join(ROOT, "source/rasterizer/xbox/rasterizer_xbox.c")
DEFAULT_OUTPUT = os.path.join(ROOT, "build/mac-audit/mac_rasterizer_xbox_default_pixels.c")

RECT_STORE = "((word *)d3d_locked_rect.pBits)[pixel_index] = pixels[pixel_index & 1];"
BOX_STORE = "((word *)d3d_locked_box.pBits)[pixel_index] = pixels[pixel_index & 1];"

MACRO = """
/* port: retain Xbox little-endian default A4R4G4B4 bytes on PPC. */
#define MAC_RASTERIZER_STORE_LE16(destination, value) \\
    do { \\
        unsigned char *mac_store_bytes = (unsigned char *)(destination); \\
        unsigned long mac_store_value = (unsigned long)(value); \\
        mac_store_bytes[0] = (unsigned char)(mac_store_value & 0xffUL); \\
        mac_store_bytes[1] = (unsigned char)((mac_store_value >> 8) & 0xffUL); \\
    } while (0)
"""


def generate(source):
    rect_count = source.count(RECT_STORE)
    box_count = source.count(BOX_STORE)
    if rect_count != 2 or box_count != 1:
        raise ValueError(
            "rasterizer source anchors changed: expected two default 2D/cube "
            "stores and one volume store, found rect=%s box=%s" % (rect_count, box_count)
        )

    result = source.replace(
        RECT_STORE,
        "MAC_RASTERIZER_STORE_LE16("
        "(unsigned char *)d3d_locked_rect.pBits + pixel_index * 2, "
        "pixels[pixel_index & 1]);",
    ).replace(
        BOX_STORE,
        "MAC_RASTERIZER_STORE_LE16("
        "(unsigned char *)d3d_locked_box.pBits + pixel_index * 2, "
        "pixels[pixel_index & 1]);",
    )

    anchor = '#include "rasterizer_xbox.h"'
    if result.count(anchor) != 1:
        raise ValueError("could not uniquely locate rasterizer_xbox.h include")
    return result.replace(anchor, anchor + "\n" + MACRO.strip(), 1)


def main():
    parser = OptionParser(usage="%prog [options] [output]", description=__doc__)
    parser.add_option("--source", default=SOURCE)
    options, args = parser.parse_args()
    if len(args) > 1:
        parser.error("expected at most one output path")
    output = args[0] if args else DEFAULT_OUTPUT
    with io.open(options.source, "r", encoding="utf-8") as source:
        rendered = generate(source.read())
    parent = os.path.dirname(os.path.abspath(output))
    if not os.path.isdir(parent):
        os.makedirs(parent)
    with io.open(output, "w", encoding="utf-8") as destination:
        destination.write(rendered)
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
