#!/usr/bin/env python3
"""Makes the Mac build's high-res HUD textures (port/assets/hud_mac/*.png) from
the redraws the other builds use (port/assets/hud, made by tools/hud_assets.py):

    python3 tools/mac_hud_reduce.py

Each texture is that PNG reduced to twice the size of the bitmap it stands for
(a box filter, over colour weighted by alpha, so that no dark fringe comes
from the transparent texels): the Mac draws the game's 640x480 picture, which
is at most about twice as large on its displays, and a 4x or 8x texture would
take most of a 32 MB card's memory and a long decoding for no sharper a
picture. Meters (cells of kind "meter") are left out: the Mac draws their
bitmaps as they are (port/mac/mac_hud_assets.h). port/mac/mac_hud_assets_view.py
embeds the result in the Mac build. Needs Pillow.
"""

import json
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "port/assets/hud"
OUTPUT = ROOT / "port/assets/hud_mac"
SCALE = 2


def main() -> None:
    assets = json.loads((SOURCE / "layout.json").read_text())["assets"]
    OUTPUT.mkdir(exist_ok=True)
    names = []
    for asset in assets:
        if any(cell["kind"] == "meter" for cell in asset.get("cells", [])):
            continue
        scale = asset["scale"]
        if scale % SCALE:
            sys.exit(f"{asset['name']}: scale {scale} is not a multiple of {SCALE}")
        image = Image.open(SOURCE / f"{asset['name']}.png")
        if image.size != (asset["width"] * scale, asset["height"] * scale):
            sys.exit(f"{asset['name']}: {image.size}, not {scale}x its bitmap")
        size = (asset["width"] * SCALE, asset["height"] * SCALE)
        reduced = image.convert("RGBa").resize(size, Image.BOX).convert("RGBA")
        reduced.save(OUTPUT / f"{asset['name']}.png", optimize=True, compress_level=9)
        names.append(asset["name"])
    print(f"{len(names)} textures in {OUTPUT}")


if __name__ == "__main__":
    main()
