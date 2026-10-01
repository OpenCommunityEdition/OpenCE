#!/usr/bin/env python3
"""Makes the high-res HUD textures (port/assets/hud/*.png) from the hand-drawn
SVG redraws of the Halo PC HUD sheets, laid out as the Xbox maps' sheets:

    python tools/hud_assets.py layout --map assets/maps/bloodgulch.map \\
        --hek ../halo-pc-restored/halopc-restored --svg ../halo-pc-restored/ui-svg-handmade
    python tools/hud_assets.py build
    python tools/hud_assets.py check --map assets/maps/bloodgulch.map --out /tmp/hud_check

The game draws a HUD bitmap at its tag's size and samples it with normalised
coordinates, so a texture of 8x its size in the same layout draws in its
place unchanged (port/linux/src/hud_hires.c swaps it in as the bitmap is
uploaded). The SVGs are drawn on the PC sheets, 4x the Xbox ones, and are
rendered at twice their size; the PC tags place some sprites elsewhere on
their sheets, so each Xbox sprite (or bitmap) is copied from its PC
counterpart, aligned by its content.

layout: reads the Xbox map's bitmap tags (sizes, formats, sprite rectangles)
    and the PC (HEK) tags' sprite rectangles, renders the SVGs, finds each
    cell's alignment, and writes port/assets/hud/layout.json (numbers only)
    with copies of the SVGs in port/assets/hud/svg. Needs a map and the
    restored tags; its output is committed.
build: renders port/assets/hud/svg with layout.json into port/assets/hud/*.png
    (committed; the builds embed them).
check: compares each PNG, reduced to the tag's size, with the map's bitmap
    and writes side-by-side images into --out.

Needs rsvg-convert, Pillow, NumPy and SciPy.
"""

import argparse
import json
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "port/assets/hud"
LAYOUT = ASSETS / "layout.json"
# the textures' size, and the SVGs' (the PC sheets'), over the Xbox bitmaps'
SCALE = 8
SOURCE_SCALE = 4

# the Xbox maps' tag data is loaded here (cache_files.c)
XBOX_TAG_BASE = 0x803A6000
BITMAP_LINEAR_FLAG = 0x10
FORMATS = {0: "a8", 1: "y8", 2: "ay8", 3: "a8y8"}

COMBINED = "ui\\hud\\bitmaps\\combined\\"

# the sheets replaced by high-res redraws so far: (Xbox tag, bitmap, SVG, PC
# tag, kind). A "meter" keeps the SVG's grey (the meter shader's fill
# threshold, Xbox channel order) and alpha; any other is drawn as the PC
# sheets' AY8 texels are, its alpha in every channel (the HUD blends
# premultiplied).
TARGETS = [
    ("hud_unit_meters", 0, "hud_xbox_order/bitmaps/combined/hud_unit_meters.svg",
     "extra/xbox_order_hud_meters/tags/ui/hud/bitmaps/combined/hud_unit_meters.bitmap", "meter"),
    ("hud_ammo_meters", 0, "hud_xbox_order/bitmaps/combined/hud_ammo_meters.svg",
     "extra/xbox_order_hud_meters/tags/ui/hud/bitmaps/combined/hud_ammo_meters.bitmap", "meter"),
    ("hud_counter_numbers", 0, "hud/bitmaps/combined/hud_counter_numbers.svg",
     "tags/ui/hud/bitmaps/combined/hud_counter_numbers.bitmap", "alpha"),
] + [
    ("hud_ammo_alphas", index, f"hud/bitmaps/combined/hud_ammo_alphas__{index}.svg", None, "alpha")
    for index in range(8)
] + [
    ("hud_ammo_outlines", 0, "hud/bitmaps/combined/hud_ammo_outlines__0.svg", None, "alpha"),
    ("hud_ammo_type_icons", 2, "hud/bitmaps/combined/hud_ammo_type_icons__2.svg", None, "alpha"),
    ("hud_weapon_backgrounds", 3, "hud/bitmaps/combined/hud_weapon_backgrounds__3.svg", None, "alpha"),
    ("hud_weapon_backgrounds", 4, "hud/bitmaps/combined/hud_weapon_backgrounds__4.svg", None, "alpha"),
]

# how far (in texels of the PC sheet) a PC cell may sit from its Xbox one
SEARCH = 12


def asset_name(tag: str, bitmap: int) -> str:
    return f"{tag}__{bitmap}"


# ---------- Xbox maps


class XboxMap:
    """The tags of an Xbox cache file (its body after the 2 KB header is
    zlib-compressed on the disc)."""

    def __init__(self, path: Path):
        raw = path.read_bytes()
        self.data = raw[:0x800] + zlib.decompressobj().decompress(raw[0x800:])
        offset, size = struct.unpack_from("<ii", self.data, 0x10)
        self.tags_data = self.data[offset:offset + size]
        instances, = struct.unpack_from("<I", self.tags_data, 0)
        count, = struct.unpack_from("<I", self.tags_data, 0xC)
        self.tags = {}
        for index in range(count):
            group, _, _, _, name, address, _, _ = struct.unpack("<4sIIIIIII", self.read(instances + index * 0x20, 0x20))
            name = self.read(name, 256).split(b"\0")[0].decode()
            self.tags[(group[::-1].decode(), name)] = address

    def read(self, address: int, size: int) -> bytes:
        offset = address - XBOX_TAG_BASE
        return self.tags_data[offset:offset + size]

    def bitmap_group(self, name: str) -> dict:
        address = self.tags[("bitm", name)]
        header = self.read(address, 0x6C)
        sequence_count, sequences = struct.unpack_from("<II", header, 0x54)
        bitmap_count, bitmaps = struct.unpack_from("<II", header, 0x60)
        group = {"bitmaps": [], "sequences": []}
        for index in range(bitmap_count):
            bitmap = self.read(bitmaps + index * 0x30, 0x30)
            width, height, _, _, format, flags = struct.unpack_from("<hhhhhH", bitmap, 4)
            offset, size = struct.unpack_from("<ii", bitmap, 0x18)
            group["bitmaps"].append({"width": width, "height": height, "format": format, "flags": flags,
                                     "pixels": self.data[offset:offset + size]})
        for index in range(sequence_count):
            sequence = self.read(sequences + index * 0x40, 0x40)
            sprite_count, sprites = struct.unpack_from("<II", sequence, 0x34)
            group["sequences"].append([sprite_from(self.read(sprites + j * 0x20, 0x20), "<") for j in range(sprite_count)])
        return group


def sprite_from(data: bytes, order: str) -> tuple:
    bitmap, = struct.unpack_from(order + "h", data, 0)
    left, right, top, bottom = struct.unpack_from(order + "4f", data, 8)
    return bitmap, left, top, right, bottom


def morton(x: int, y: int, width: int, height: int) -> int:
    """The Xbox's swizzled texel order: x and y bits interleaved, x first,
    the longer side's leftover bits on top."""
    index = bit = 0
    x_bits, y_bits = width.bit_length() - 1, height.bit_length() - 1
    shift_x = shift_y = 0
    while shift_x < x_bits or shift_y < y_bits:
        if shift_x < x_bits:
            index |= ((x >> shift_x) & 1) << bit
            bit += 1
            shift_x += 1
        if shift_y < y_bits:
            index |= ((y >> shift_y) & 1) << bit
            bit += 1
            shift_y += 1
    return index


def decode_bitmap(bitmap: dict) -> np.ndarray:
    """An Xbox HUD bitmap's level 0 as RGBA, as xbox_textures.c decodes it."""
    width, height, format = bitmap["width"], bitmap["height"], bitmap["format"]
    texel_size = {"a8": 1, "y8": 1, "ay8": 1, "a8y8": 2}[FORMATS[format]]
    pixels = bitmap["pixels"]
    image = np.zeros((height, width, 4), np.uint8)
    linear = bitmap["flags"] & BITMAP_LINEAR_FLAG
    for y in range(height):
        for x in range(width):
            offset = (y * width + x if linear else morton(x, y, width, height)) * texel_size
            kind = FORMATS[format]
            if kind == "a8":
                image[y, x] = (255, 255, 255, pixels[offset])
            elif kind == "y8":
                image[y, x] = (pixels[offset],) * 3 + (255,)
            elif kind == "ay8":
                image[y, x] = (pixels[offset],) * 4
            else:
                image[y, x] = (pixels[offset],) * 3 + (pixels[offset + 1],)
    return image


# ---------- PC (HEK) tags


def hek_sprites(path: Path) -> list:
    """The sprites of a HEK bitmap tag (big-endian; its blocks follow the
    0x40-byte file header and the 0x6C-byte tag in order)."""
    data = path.read_bytes()
    header = data[0x40:0x40 + 0x6C]
    compressed_size, = struct.unpack_from(">I", header, 0x1C)
    pixels_size, = struct.unpack_from(">I", header, 0x30)
    sequence_count, = struct.unpack_from(">I", header, 0x54)
    position = 0x40 + 0x6C + compressed_size + pixels_size
    counts = [struct.unpack_from(">I", data, position + index * 0x40 + 0x34)[0] for index in range(sequence_count)]
    position += sequence_count * 0x40
    sequences = []
    for count in counts:
        sequences.append([sprite_from(data[position + j * 0x20:position + (j + 1) * 0x20], ">") for j in range(count)])
        position += count * 0x20
    return sequences


# ---------- rendering


def render_svg(svg: Path, zoom: int = 1) -> np.ndarray:
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "render.png"
        subprocess.run(["rsvg-convert", "-z", str(zoom), "-o", str(output), str(svg)], check=True)
        return np.asarray(Image.open(output).convert("RGBA")).copy()


def pixel_rectangle(sprite: tuple, width: int, height: int) -> list:
    _, left, top, right, bottom = sprite
    return [round(left * width), round(top * height), round(right * width), round(bottom * height)]


def window(image: np.ndarray, left: int, top: int, width: int, height: int) -> np.ndarray:
    """image[top:top+height, left:left+width], transparent outside it."""
    result = np.zeros((height, width, 4), image.dtype)
    x0, y0 = max(left, 0), max(top, 0)
    x1, y1 = min(left + width, image.shape[1]), min(top + height, image.shape[0])
    if x0 < x1 and y0 < y1:
        result[y0 - top:y1 - top, x0 - left:x1 - left] = image[y0:y1, x0:x1]
    return result


def align(render: np.ndarray, xbox: np.ndarray, cell: list, source: list) -> list:
    """The offset of the render's window (at source's corner, the Xbox cell's
    size on the PC sheet) whose alpha best matches the Xbox cell's, enlarged."""
    left, top, right, bottom = cell
    width, height = (right - left) * SOURCE_SCALE, (bottom - top) * SOURCE_SCALE
    target = np.asarray(Image.fromarray(xbox[top:bottom, left:right, 3]).resize((width, height), Image.BILINEAR), float)
    best = None
    for dy in range(-SEARCH, SEARCH + 1):
        for dx in range(-SEARCH, SEARCH + 1):
            alpha = window(render, source[0] + dx, source[1] + dy, width, height)[:, :, 3].astype(float)
            error = np.abs(alpha - target).mean()
            if best is None or error < best[0] - 1e-9 or (abs(error - best[0]) < 1e-9 and abs(dx) + abs(dy) < sum(map(abs, best[1]))):
                best = (error, [dx, dy])
    return best[1]


def layout(arguments) -> None:
    xbox_map = XboxMap(Path(arguments.map))
    hek = Path(arguments.hek)
    svg_root = Path(arguments.svg)
    (ASSETS / "svg").mkdir(parents=True, exist_ok=True)
    entries = []
    for tag, bitmap_index, svg, hek_tag, kind in TARGETS:
        group = xbox_map.bitmap_group(COMBINED + tag)
        bitmap = group["bitmaps"][bitmap_index]
        width, height = bitmap["width"], bitmap["height"]
        svg_name = svg.replace("/", "__")
        shutil.copyfile(svg_root / svg, ASSETS / "svg" / svg_name)
        render = render_svg(svg_root / svg)
        xbox = decode_bitmap(bitmap)
        if hek_tag:
            pc_sequences = hek_sprites(hek / hek_tag)
            pairs = []
            for sequence_index, sequence in enumerate(group["sequences"]):
                for sprite_index, sprite in enumerate(sequence):
                    if sprite[0] != bitmap_index:
                        continue
                    cell = pixel_rectangle(sprite, width, height)
                    source = pixel_rectangle(pc_sequences[sequence_index][sprite_index], render.shape[1], render.shape[0])
                    pairs.append((cell, source))
        else:
            pairs = [([0, 0, width, height], [0, 0, render.shape[1], render.shape[0]])]
        cells = []
        for cell, source in pairs:
            # (the PC cell's middle over the Xbox cell's, enlarged)
            corner = [round((source[0] + source[2]) / 2 - (cell[2] - cell[0]) * SOURCE_SCALE / 2),
                      round((source[1] + source[3]) / 2 - (cell[3] - cell[1]) * SOURCE_SCALE / 2)]
            offset = align(render, xbox, cell, corner)
            cells.append({"xbox": cell, "source": [corner[0] + offset[0], corner[1] + offset[1]], "clip": source})
        entries.append({
            "name": asset_name(tag, bitmap_index),
            "tag": COMBINED + tag,
            "bitmap": bitmap_index,
            "width": width,
            "height": height,
            "format": FORMATS[bitmap["format"]],
            "kind": kind,
            "svg": svg_name,
            "cells": cells,
        })
        print(f"{asset_name(tag, bitmap_index)}: {width}x{height} {FORMATS[bitmap['format']]}, {len(cells)} cells")
    # (cells' sources in texels of the SVGs as drawn, the PC sheets)
    LAYOUT.write_text(json.dumps({"scale": SCALE, "source_scale": SOURCE_SCALE, "assets": entries}, indent=1) + "\n")


# ---------- building


def bleed(image: np.ndarray) -> np.ndarray:
    """Gives each transparent texel the grey of the nearest covered one, so
    that filtering and mip levels at a meter's edges read its own fill
    thresholds."""
    from scipy import ndimage

    covered = image[:, :, 3] > 0
    if not covered.any():
        return image
    _, (rows, columns) = ndimage.distance_transform_edt(~covered, return_indices=True)
    result = image.copy()
    for channel in range(3):
        result[:, :, channel] = image[rows, columns, channel]
    result[:, :, 3] = image[:, :, 3]
    return result


def build_asset(entry: dict, scale: int, source_scale: int) -> np.ndarray:
    # (the SVG drawn at the textures' scale, its cells' coordinates with it)
    zoom = scale // source_scale
    render = render_svg(ASSETS / "svg" / entry["svg"], zoom)
    image = np.zeros((entry["height"] * scale, entry["width"] * scale, 4), np.uint8)
    for cell in entry["cells"]:
        left, top, right, bottom = cell["xbox"]
        width, height = (right - left) * scale, (bottom - top) * scale
        # (only the PC sprite's own texels, not its neighbours')
        clip_left, clip_top, clip_right, clip_bottom = (value * zoom for value in cell["clip"])
        clipped = np.zeros_like(render)
        clipped[clip_top:clip_bottom, clip_left:clip_right] = render[clip_top:clip_bottom, clip_left:clip_right]
        image[top * scale:top * scale + height, left * scale:left * scale + width] = \
            window(clipped, cell["source"][0] * zoom, cell["source"][1] * zoom, width, height)
    if entry["kind"] == "meter":
        grey = np.round(image[:, :, :3].astype(float).mean(axis=2)).astype(np.uint8)
        grey[image[:, :, 3] == 0] = 0
        image[:, :, 0] = image[:, :, 1] = image[:, :, 2] = grey
        image = bleed(image)
    else:
        for channel in range(3):
            image[:, :, channel] = image[:, :, 3]
    return image


def build(arguments) -> None:
    description = json.loads(LAYOUT.read_text())
    for entry in description["assets"]:
        image = build_asset(entry, description["scale"], description["source_scale"])
        Image.fromarray(image, "RGBA").save(ASSETS / f"{entry['name']}.png", optimize=True)
        print(f"{entry['name']}.png: {image.shape[1]}x{image.shape[0]}")


# ---------- checking


def check(arguments) -> None:
    xbox_map = XboxMap(Path(arguments.map))
    output = Path(arguments.out)
    output.mkdir(parents=True, exist_ok=True)
    description = json.loads(LAYOUT.read_text())
    for entry in description["assets"]:
        bitmap = xbox_map.bitmap_group(entry["tag"])["bitmaps"][entry["bitmap"]]
        xbox = decode_bitmap(bitmap).astype(float)
        image = Image.open(ASSETS / f"{entry['name']}.png").convert("RGBA")
        reduced = np.asarray(image.resize((entry["width"], entry["height"]), Image.BOX), float)
        covered = xbox[:, :, 3] > 0
        alpha_error = np.sqrt(((reduced[:, :, 3] - xbox[:, :, 3]) ** 2).mean())
        grey_error = np.sqrt(((reduced[:, :, 0] - xbox[:, :, 0])[covered] ** 2).mean()) if covered.any() else 0.0
        print(f"{entry['name']}: alpha rms {alpha_error:.1f}, grey rms (covered) {grey_error:.1f}")
        # Xbox | ours reduced | ours, each over blue, then each one's alpha
        panels = []
        for picture in (xbox, reduced):
            picture = Image.fromarray(picture.astype(np.uint8), "RGBA").resize(image.size, Image.NEAREST)
            panels.append(picture)
        panels.append(image)
        width, height = image.size
        sheet = Image.new("RGB", (width * 3 + 16, height * 2 + 8), (255, 0, 0))
        for index, picture in enumerate(panels):
            background = Image.new("RGBA", picture.size, (40, 40, 90, 255))
            sheet.paste(Image.alpha_composite(background, picture).convert("RGB"), (index * (width + 8), 0))
            sheet.paste(picture.getchannel("A").convert("RGB"), (index * (width + 8), height + 8))
        sheet.save(output / f"{entry['name']}.png")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("layout")
    command.add_argument("--map", required=True)
    command.add_argument("--hek", required=True)
    command.add_argument("--svg", required=True)
    commands.add_parser("build")
    command = commands.add_parser("check")
    command.add_argument("--map", required=True)
    command.add_argument("--out", required=True)
    arguments = parser.parse_args()
    {"layout": layout, "build": build, "check": check}[arguments.command](arguments)


if __name__ == "__main__":
    sys.exit(main())
