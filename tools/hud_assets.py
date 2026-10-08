#!/usr/bin/env python3
"""Makes the high-res HUD textures (port/assets/hud/*.png) from the hand-drawn
SVG redraws of the Halo PC HUD sheets (and of the sniper rifle's scope
ladder, which its weapon's folder holds), laid out as the Xbox maps' sheets:

    python tools/hud_assets.py layout --map assets/maps/bloodgulch.map \\
        --hek ../halo-pc-restored/halopc-restored --svg ../halo-pc-restored/ui-svg-handmade
    python tools/hud_assets.py build
    python tools/hud_assets.py thresholds  # update only discrete segment data
    python tools/hud_assets.py check --map assets/maps/bloodgulch.map --out /tmp/hud_check

The game draws a HUD bitmap at its tag's size and samples it with normalised
coordinates, so a texture of 8x its size in the same layout draws in its
place unchanged (port/linux/src/hud_hires.c swaps it in as the bitmap is
uploaded). The redraws are drawn on the PC sheets, mostly 4x the Xbox
bitmaps (a few at their size), and the PC tags have more sprites than the
Xbox ones and place some elsewhere, so each Xbox sprite (or bitmap) is
copied from the redrawn sprite whose shape matches it best, aligned by its
content.

layout: reads the Xbox map's HUD bitmaps (sizes, formats, sprite rectangles,
    pixels) and the PC (HEK) tags' sprite rectangles, renders the redraws,
    matches and aligns each cell, and writes port/assets/hud/layout.json
    (numbers only) with copies of the redraws used in port/assets/hud/svg.
    Needs a map and the restored tags; its output is committed. A bitmap
    with a cell that no redraw matches is left out.
build: renders port/assets/hud/svg with layout.json into port/assets/hud/*.png
    (committed; the builds embed them).
check: compares each PNG, reduced to the tag's size, with the map's bitmap
    and writes side-by-side images into --out.

Needs rsvg-convert, Pillow, NumPy and SciPy.
"""

import argparse
import json
import re
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
# the textures' size over the Xbox bitmaps', but no side longer than every
# OpenGL ES 3 device can take
SCALE = 8
MAXIMUM_SIZE = 2048

# the Xbox maps' tag data is loaded here (cache_files.c)
XBOX_TAG_BASE = 0x803A6000
BITMAP_LINEAR_FLAG = 0x10
FORMATS = {0: "a8", 1: "y8", 2: "ay8", 3: "a8y8", 6: "r5g6b5", 8: "a1r5g5b5", 9: "a4r4g4b4",
           10: "x8r8g8b8", 11: "a8r8g8b8", 14: "dxt1", 15: "dxt3", 16: "dxt5"}

HUD = "ui\\hud\\bitmaps\\"
# the folders of the HUD's bitmaps: the HUD's own, and the sniper rifle's
# (its scope's elevation ladder)
FOLDERS = (HUD, "weapons\\sniper rifle\\bitmaps\\")
# the meter sheets: their redraws in the Xbox channel order (the fill
# threshold the meter shader reads in the grey, the art in the alpha)
METERS = {
    HUD + "combined\\hud_unit_meters": ("hud_xbox_order/bitmaps/combined/hud_unit_meters.svg",
                                       "extra/xbox_order_hud_meters/tags/ui/hud/bitmaps/combined/hud_unit_meters.bitmap"),
    HUD + "combined\\hud_ammo_meters": ("hud_xbox_order/bitmaps/combined/hud_ammo_meters.svg",
                                       "extra/xbox_order_hud_meters/tags/ui/hud/bitmaps/combined/hud_ammo_meters.bitmap"),
}
# redraws not used: those with English text (other languages' maps have
# their own; the Xbox message icons all have some), the sniper's zoom labels
# (2x and 8x, where the Xbox's are 2x and 10x), and the meters in the PC
# channel order
UNUSED = {
    "hud/bitmaps/combined/hud_reticle_warnings.svg",
    "hud/bitmaps/combined/hud_msg_icons__0.svg",
    "hud/bitmaps/combined/hud_msg_icons__1.svg",
    "hud/bitmaps/combined/hud_msg_icons__2.svg",
    "hud/bitmaps/combined/hud_msg_icons__3.svg",
    "hud/bitmaps/combined/hud_msg_icons__4.svg",
    "hud/bitmaps/hud_msg_icons_sm__0.svg",
    "hud/bitmaps/hud_msg_icons_sm__1.svg",
    "hud/bitmaps/combined/hud_reticles__7.svg",
    "hud/bitmaps/combined/hud_reticles__13.svg",
    "hud/bitmaps/combined/hud_unit_backgrounds__8.svg",
    "hud/bitmaps/combined/hud_unit_backgrounds__9.svg",
    "hud/bitmaps/combined/hud_unit_backgrounds__10.svg",
    "hud/bitmaps/combined/hud_unit_backgrounds__11.svg",
    "hud/bitmaps/combined/hud_unit_backgrounds__12.svg",
    "hud/bitmaps/sniper/caption.svg",
    "hud/bitmaps/sniper/hud_reticles_scope.svg",
    "hud/bitmaps/combined/hud_unit_meters.svg",
    "hud/bitmaps/combined/hud_ammo_meters.svg",
}
# how each redraw's texels are written, by its PC bitmap's format (as the
# Xbox decodes that format: xbox_textures.c, convert_texel): AY8 is its one
# value in every channel (the HUD blends premultiplied), A8Y8 grey and alpha,
# A8 white with alpha, Y8 grey and opaque, colour formats as drawn; DXT ones
# as drawn with the colour carried into the transparent texels, as theirs is
KINDS = {0: "a8", 1: "y8", 2: "alpha", 3: "grey", 6: "colour", 8: "colour", 9: "colour", 10: "colour", 11: "colour",
         14: "dxt", 15: "dxt", 16: "dxt"}

# how far (in Xbox texels) a redrawn cell may sit from its Xbox one, and how
# much it must overlap it (the overlap of their shapes over their union: thin
# and small shapes, drawn cleaner than the Xbox's blurred texels, overlap
# little even where they match)
SEARCH = 4
MATCH = 0.25


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


def morton_order(width: int, height: int) -> np.ndarray:
    """Each texel's index in the Xbox's swizzled order: x and y bits
    interleaved, x first, the longer side's leftover bits on top."""
    xs, ys = np.meshgrid(np.arange(width), np.arange(height))
    index = np.zeros((height, width), np.int64)
    bit = shift_x = shift_y = 0
    x_bits, y_bits = width.bit_length() - 1, height.bit_length() - 1
    while shift_x < x_bits or shift_y < y_bits:
        if shift_x < x_bits:
            index |= ((xs >> shift_x) & 1) << bit
            bit += 1
            shift_x += 1
        if shift_y < y_bits:
            index |= ((ys >> shift_y) & 1) << bit
            bit += 1
            shift_y += 1
    return index


def level0_size(bitmap: dict) -> int:
    """The bytes of a bitmap's first mip level."""
    width, height, kind = bitmap["width"], bitmap["height"], FORMATS[bitmap["format"]]
    if kind.startswith("dxt"):
        return max(1, width // 4) * max(1, height // 4) * (8 if kind == "dxt1" else 16)
    texel = {"a8": 1, "y8": 1, "ay8": 1, "a8y8": 2, "r5g6b5": 2, "a1r5g5b5": 2, "a4r4g4b4": 2}.get(kind, 4)
    return width * height * texel


def decode_dxt(kind: str, data: bytes, width: int, height: int) -> np.ndarray:
    """DXT blocks (row by row, not swizzled) as RGBA."""
    image = np.zeros((height, width, 4), np.uint8)
    block_size = 8 if kind == "dxt1" else 16
    blocks_wide = max(1, width // 4)
    for block in range(max(1, width // 4) * max(1, height // 4)):
        bx, by = block % blocks_wide * 4, block // blocks_wide * 4
        chunk = data[block * block_size:(block + 1) * block_size]
        colour = chunk[-8:]
        c0, c1, bits = struct.unpack("<HHI", colour)
        def rgb(value):
            return [((value >> 11) & 31) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31]
        palette = [rgb(c0) + [255], rgb(c1) + [255]]
        if kind != "dxt1" or c0 > c1:
            palette += [[(2 * a + b) // 3 for a, b in zip(palette[0][:3], palette[1][:3])] + [255],
                        [(a + 2 * b) // 3 for a, b in zip(palette[0][:3], palette[1][:3])] + [255]]
        else:
            palette += [[(a + b) // 2 for a, b in zip(palette[0][:3], palette[1][:3])] + [255], [0, 0, 0, 0]]
        if kind == "dxt3":
            alphas = [((int.from_bytes(chunk[:8], "little") >> (4 * i)) & 15) * 17 for i in range(16)]
        elif kind == "dxt5":
            a0, a1 = chunk[0], chunk[1]
            table = [a0, a1] + ([((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)] if a0 > a1 else
                                [((5 - i) * a0 + i * a1) // 5 for i in range(1, 5)] + [0, 255])
            codes = int.from_bytes(chunk[2:8], "little")
            alphas = [table[(codes >> (3 * i)) & 7] for i in range(16)]
        else:
            alphas = None
        for i in range(16):
            x, y = bx + i % 4, by + i // 4
            if x < width and y < height:
                texel = list(palette[(bits >> (2 * i)) & 3])
                if alphas is not None:
                    texel[3] = alphas[i]
                image[y, x] = texel
    return image


def decode_bitmap(bitmap: dict) -> np.ndarray:
    """An Xbox HUD bitmap's level 0 as RGBA, as xbox_textures.c decodes it."""
    width, height, kind = bitmap["width"], bitmap["height"], FORMATS[bitmap["format"]]
    pixels = np.frombuffer(bitmap["pixels"][:level0_size(bitmap)], np.uint8)
    if kind.startswith("dxt"):
        return decode_dxt(kind, bitmap["pixels"], width, height)
    order = (np.arange(width * height).reshape(height, width) if bitmap["flags"] & BITMAP_LINEAR_FLAG else
             morton_order(width, height))
    texel = level0_size(bitmap) // (width * height)
    raw = pixels.reshape(-1, texel)[order]
    image = np.zeros((height, width, 4), np.uint8)
    if kind == "a8":
        image[..., :3], image[..., 3] = 255, raw[..., 0]
    elif kind == "y8":
        image[..., :3], image[..., 3] = raw[..., :1], 255
    elif kind == "ay8":
        image[...] = raw[..., :1]
    elif kind == "a8y8":
        image[..., :3], image[..., 3] = raw[..., :1], raw[..., 1]
    elif kind == "a4r4g4b4":
        value = raw[..., 0].astype(np.uint16) | (raw[..., 1].astype(np.uint16) << 8)
        for channel, shift in ((3, 12), (0, 8), (1, 4), (2, 0)):
            image[..., channel] = ((value >> shift) & 15) * 17
    elif kind in ("a8r8g8b8", "x8r8g8b8"):
        image[..., 0], image[..., 1], image[..., 2] = raw[..., 2], raw[..., 1], raw[..., 0]
        image[..., 3] = raw[..., 3] if kind == "a8r8g8b8" else 255
    else:
        raise ValueError(f"no decoder for {kind}")
    return image


# ---------- PC (HEK) tags


def hek_bitmap_group(path: Path) -> dict:
    """A HEK bitmap tag's bitmaps (sizes and formats) and sprites (big-endian;
    its blocks follow the 0x40-byte file header and the 0x6C-byte tag in
    order: data, sequences with their sprites, bitmaps)."""
    data = path.read_bytes()
    header = data[0x40:0x40 + 0x6C]
    compressed_size, = struct.unpack_from(">I", header, 0x1C)
    pixels_size, = struct.unpack_from(">I", header, 0x30)
    sequence_count, = struct.unpack_from(">I", header, 0x54)
    bitmap_count, = struct.unpack_from(">I", header, 0x60)
    position = 0x40 + 0x6C + compressed_size + pixels_size
    counts = [struct.unpack_from(">I", data, position + index * 0x40 + 0x34)[0] for index in range(sequence_count)]
    position += sequence_count * 0x40
    group = {"sequences": [], "bitmaps": []}
    for count in counts:
        group["sequences"].append([sprite_from(data[position + j * 0x20:position + (j + 1) * 0x20], ">")
                                   for j in range(count)])
        position += count * 0x20
    for index in range(bitmap_count):
        width, height, _, _, format = struct.unpack_from(">HHHhh", data, position + index * 0x30 + 4)
        group["bitmaps"].append({"width": width, "height": height, "format": format})
    return group


# ---------- rendering and matching


def render_svg(svg: Path, zoom: float = 1) -> np.ndarray:
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "render.png"
        subprocess.run(["rsvg-convert", "-z", str(zoom), "-o", str(output), str(svg)], check=True)
        return np.asarray(Image.open(output).convert("RGBA")).copy()


def pixel_rectangle(sprite: tuple, width: int, height: int) -> list:
    _, left, top, right, bottom = sprite
    return [round(left * width), round(top * height), round(right * width), round(bottom * height)]


def window(image: np.ndarray, left: int, top: int, width: int, height: int) -> np.ndarray:
    """image[top:top+height, left:left+width], transparent outside it."""
    result = np.zeros((height, width) + image.shape[2:], image.dtype)
    x0, y0 = max(left, 0), max(top, 0)
    x1, y1 = min(left + width, image.shape[1]), min(top + height, image.shape[0])
    if x0 < x1 and y0 < y1:
        result[y0 - top:y1 - top, x0 - left:x1 - left] = image[y0:y1, x0:x1]
    return result


def clipped(image: np.ndarray, clip: list) -> np.ndarray:
    """image with only clip's texels (a sprite's own, not its neighbours')."""
    left, top, right, bottom = clip
    result = np.zeros_like(image)
    result[top:bottom, left:right] = image[top:bottom, left:right]
    return result


def overlap(a: np.ndarray, b: np.ndarray) -> float:
    union = np.maximum(a, b).sum()
    return float(np.minimum(a, b).sum() / union) if union else 0.0


def match(alpha: np.ndarray, render: np.ndarray, clip: list, scale: int) -> tuple:
    """How well the redrawn sprite in clip (on the render, scale times the
    Xbox texels) matches the Xbox cell's alpha, and where: (overlap, the
    render's corner for the cell). Searched in Xbox texels on the render
    reduced, then refined in its own texels."""
    height, width = alpha.shape
    middle = ((clip[0] + clip[2]) / 2, (clip[1] + clip[3]) / 2)
    corner = [round(middle[0] - width * scale / 2), round(middle[1] - height * scale / 2)]
    alpha = alpha.astype(float) / 255
    source = clipped(render[..., 3].astype(float) / 255, clip)
    best = (-1.0, corner)
    for dy in range(-SEARCH, SEARCH + 1):
        for dx in range(-SEARCH, SEARCH + 1):
            at = [corner[0] + dx * scale, corner[1] + dy * scale]
            reduced = window(source, at[0], at[1], width * scale, height * scale).reshape(
                height, scale, width, scale).mean(axis=(1, 3))
            score = overlap(reduced, alpha)
            if score > best[0]:
                best = (score, at)
    target = np.asarray(Image.fromarray((alpha * 255).astype(np.uint8)).resize(
        (width * scale, height * scale), Image.BILINEAR), float) / 255
    coarse = best[1]
    best = (-1.0, coarse)
    for dy in range(-scale + 1, scale):
        for dx in range(-scale + 1, scale):
            at = [coarse[0] + dx, coarse[1] + dy]
            score = overlap(window(source, at[0], at[1], width * scale, height * scale), target)
            if score > best[0]:
                best = (score, at)
    return best


def redraws(svg_root: Path, hek_root: Path, tag: str) -> list:
    """The redraws of a tag's bitmaps: (SVG path relative to svg_root, PC
    bitmap index, PC bitmap group)."""
    if tag in METERS:
        svg, hek = METERS[tag]
        return [(svg, 0, hek_bitmap_group(hek_root / hek))]
    # (the redraws mirror the tags: those under ui from tags/ui, others from tags)
    parts = tag.split("\\")
    relative = Path(*parts[1:]) if parts[0] == "ui" else Path(*parts)
    hek = hek_root / "tags" / Path(*parts).with_suffix(".bitmap")
    if not hek.exists():
        return []
    group = hek_bitmap_group(hek)
    found = []
    for svg in sorted((svg_root / relative.parent).glob(relative.name + "*.svg")):
        parsed = re.fullmatch(re.escape(relative.name) + r"(?:__(\d+))?\.svg", svg.name)
        name = str(svg.relative_to(svg_root))
        if parsed and name not in UNUSED:
            found.append((name, int(parsed.group(1) or 0), group))
    return found


def layout(arguments) -> None:
    xbox_map = XboxMap(Path(arguments.map))
    svg_root, hek_root = Path(arguments.svg), Path(arguments.hek)
    renders = {}
    entries = []
    sources = {}
    for group_tag, tag in sorted(xbox_map.tags):
        if group_tag != "bitm" or not tag.startswith(FOLDERS):
            continue
        candidates = redraws(svg_root, hek_root, tag)
        if not candidates:
            continue
        group = xbox_map.bitmap_group(tag)
        for index, bitmap in enumerate(group["bitmaps"]):
            width, height = bitmap["width"], bitmap["height"]
            # (the combined sheets by their own names, as before the others;
            # the weapon's by its name and theirs)
            stem = tag[len(HUD):].replace("combined\\", "") if tag.startswith(HUD) else \
                tag.split("\\", 1)[1].replace("\\bitmaps\\", "\\")
            name = stem.replace("\\", "__").replace(" ", "_") + f"__{index}"
            xbox = decode_bitmap(bitmap)
            # (each cell with the sprites, as sequence and sprite numbers, it is)
            cells = {}
            for sequence_index, sequence in enumerate(group["sequences"]):
                for sprite_index, sprite in enumerate(sequence):
                    if sprite[0] == index:
                        cells.setdefault(tuple(pixel_rectangle(sprite, width, height)), set()).add(
                            (sequence_index, sprite_index))
            cells = cells or {(0, 0, width, height): set()}
            matched = []
            for cell, numbers in sorted(cells.items()):
                cell = list(cell)
                left, top, right, bottom = cell
                # (the cell's shape: its alpha, or its grey where its alpha is
                # flat: the Xbox's DXT1 blip is opaque, and its "Km" glyph
                # has only grey)
                alpha = xbox[top:bottom, left:right, 3]
                if alpha.min() == alpha.max():
                    alpha = xbox[top:bottom, left:right, 0]
                if not alpha.any():
                    continue
                scored = []
                for svg, pc_index, pc_group in candidates:
                    pc_bitmap = pc_group["bitmaps"][pc_index]
                    if pc_bitmap["width"] % width:
                        continue
                    scale = pc_bitmap["width"] // width
                    if svg not in renders:
                        renders[svg] = render_svg(svg_root / svg)
                    render = renders[svg]
                    pc_cells = [(pixel_rectangle(sprite, render.shape[1], render.shape[0]), (sequence_index, sprite_index))
                                for sequence_index, sequence in enumerate(pc_group["sequences"])
                                for sprite_index, sprite in enumerate(sequence) if sprite[0] == pc_index]
                    for clip, pc_numbers in pc_cells or [([0, 0, render.shape[1], render.shape[0]], None)]:
                        # (about the same size)
                        if not (0.6 < (clip[2] - clip[0]) / ((right - left) * scale) < 1.6 and
                                0.6 < (clip[3] - clip[1]) / ((bottom - top) * scale) < 1.6):
                            continue
                        score, corner = match(alpha, render, clip, scale)
                        scored.append((score, svg, scale, corner, clip, KINDS.get(pc_bitmap["format"]),
                                       pc_numbers in numbers))
                # the best match; but the sprite of the same numbers where the
                # sheets agree and it matches nearly as well (small glyphs, a
                # dot or a colon, overlap others as much as their own)
                best = max(scored, default=None)
                same = max((entry for entry in scored if entry[6]), default=None)
                if same is not None and best is not None and same[0] >= max(MATCH, 0.6 * best[0]):
                    best = same
                if best is None or best[0] < MATCH or best[5] is None:
                    matched = None
                    print(f"{name}: left out, cell {cell} " +
                          ("has no redraw" if best is None else f"matches best {best[1]} at {best[0]:.2f}"))
                    break
                score, svg, scale, corner, clip, kind, _ = best
                if tag in METERS:
                    kind = "meter"
                flat = svg.replace("/", "__").replace(" ", "_")
                sources[flat] = svg
                match = {"xbox": cell, "svg": flat, "source_scale": scale,
                         "source": corner, "clip": clip, "kind": kind, "score": round(score, 3)}
                if tag == HUD + "combined\\hud_unit_meters":
                    original = xbox[top:bottom, left:right]
                    values = set(np.unique(original[..., 2][original[..., 3] > 0]))
                    health = set(range(30, 241, 30))
                    if health <= values <= health | {0}:
                        match["thresholds"] = sorted(health)
                matched.append(match)
            if not matched:
                if matched is not None:
                    print(f"{name}: left out, empty")
                continue
            scale = SCALE
            while max(width, height) * scale > MAXIMUM_SIZE:
                scale //= 2
            entries.append({
                "name": name,
                "tag": tag,
                "bitmap": index,
                "width": width,
                "height": height,
                "format": FORMATS[bitmap["format"]],
                "scale": scale,
                # (the Xbox bitmap's, which the game checks before drawing this
                # in its place: other languages' maps and modified ones differ)
                "crc": zlib.crc32(bitmap["pixels"][:level0_size(bitmap)]),
                "cells": matched,
            })
            print(f"{name}: {width}x{height} {FORMATS[bitmap['format']]} at {scale}x, {len(matched)} cells, "
                  f"worst match {min(cell['score'] for cell in matched):.2f}")
    if (ASSETS / "svg").exists():
        shutil.rmtree(ASSETS / "svg")
    (ASSETS / "svg").mkdir(parents=True)
    for svg in sorted({cell["svg"] for entry in entries for cell in entry["cells"]}):
        shutil.copyfile(svg_root / sources[svg], ASSETS / "svg" / svg)
    LAYOUT.write_text(json.dumps({"assets": entries}, indent=1) + "\n")


# ---------- building


def bleed(image: np.ndarray) -> np.ndarray:
    """Gives each transparent texel the colour of the nearest covered one: so
    that filtering and mip levels at a meter's edges read its own fill
    thresholds, and a DXT texture keeps its colour where it is transparent."""
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


def coverage(alpha: np.ndarray) -> np.ndarray:
    """How much of each texel a meter's shapes cover: all of it inside them
    (their dim cells too: their alpha is faint by design), and on their
    outline (covered texels next to uncovered ones) the share their alpha has
    of their brightest neighbour's. The meter shader reads only blue and
    alpha; the game eases the meter's darkening of what is behind it by this
    (port/linux/src/nv2a_psh.c, coverage_alpha), which on the Xbox stopped at
    its point-sampled texels' edges, and filtered would leave a dark fringe."""
    from scipy import ndimage

    covered = alpha > 0
    outline = covered & ndimage.binary_dilation(~covered, structure=np.ones((3, 3), bool))
    brightest = ndimage.maximum_filter(alpha, size=3).astype(float)
    result = np.where(covered, 255.0, 0.0)
    result[outline] = np.clip(alpha[outline] * 255.0 / np.maximum(brightest[outline], 1.0), 0, 255)
    return np.round(result).astype(np.uint8)


def recipe(texels: np.ndarray, kind: str) -> np.ndarray:
    """A redraw's texels written as its kind is (KINDS)."""
    result = texels.copy()
    grey = np.round(texels[..., :3].astype(float).mean(axis=-1)).astype(np.uint8)
    if kind == "alpha":
        result[..., :3] = texels[..., 3:4]
    elif kind in ("grey", "meter"):
        result[..., :3] = grey[..., None]
        result[texels[..., 3] == 0, :3] = 0
    elif kind == "a8":
        result[..., :3] = 255
    elif kind == "y8":
        result[..., :3], result[..., 3] = grey[..., None], 255
    return result


def meter_thresholds(texels: np.ndarray, values: list) -> np.ndarray:
    """Categorical segment data, independent of antialiased edge RGB/alpha.

    Each solid-colour shape's opaque stroke supplies its exact threshold.
    Assign that value to its whole core, then extend it to the nearest edge
    and transparent texel. Low-alpha SVG texels cannot supply reliable RGB:
    unpremultiplying 8-bit colour there can turn 150 into 128 (or 60 into 0).
    This is only for the explicitly listed discrete health sprites; shield
    and ammunition gradients retain their existing continuous data.
    """
    from scipy import ndimage

    cores, count = ndimage.label(texels[..., 3] >= 127)
    exact = np.zeros(texels.shape[:2], np.uint8)
    seen = set()
    for index in range(1, count + 1):
        core = cores == index
        opaque = texels[core & (texels[..., 3] == 255), :3]
        if not len(opaque):
            continue
        colours = np.unique(opaque, axis=0)
        if len(colours) != 1 or not np.all(colours[0] == colours[0, 0]):
            raise ValueError("discrete meter shape must have one solid grey threshold")
        value = int(colours[0, 0])
        if value not in values:
            raise ValueError(f"unexpected discrete meter threshold {value}")
        exact[core] = value
        seen.add(value)
    if seen != set(values):
        raise ValueError(f"missing discrete meter thresholds: {set(values) - seen}")
    _, (rows, columns) = ndimage.distance_transform_edt(exact == 0, return_indices=True)
    return exact[rows, columns]


def add_meter_thresholds(image: np.ndarray, entry: dict) -> np.ndarray:
    """Red is an unfiltered categorical field; zero selects normal blue data.
    Green coverage and alpha artwork remain filtered, including their mips.
    """
    cells = [cell for cell in entry["cells"] if cell.get("thresholds")]
    if cells:
        image[..., 0] = 0
        scale = entry["scale"]
        for cell in cells:
            left, top, right, bottom = [value * scale for value in cell["xbox"]]
            texels = image[top:bottom, left:right]
            # The meter's blue is still the original greyscale artwork data.
            source = texels.copy()
            source[..., :3] = source[..., 2:3]
            texels[..., 0] = meter_thresholds(source, cell["thresholds"])
    return image


def build_asset(entry: dict, renders: dict) -> np.ndarray:
    scale = entry["scale"]
    image = np.zeros((entry["height"] * scale, entry["width"] * scale, 4), np.uint8)
    for cell in entry["cells"]:
        # (the redraw drawn at the texture's scale, its coordinates with it)
        zoom = scale // cell["source_scale"]
        assert zoom * cell["source_scale"] == scale, entry["name"]
        key = (cell["svg"], zoom)
        if key not in renders:
            renders[key] = render_svg(ASSETS / "svg" / cell["svg"], zoom)
        left, top, right, bottom = cell["xbox"]
        width, height = (right - left) * scale, (bottom - top) * scale
        texels = window(clipped(renders[key], [value * zoom for value in cell["clip"]]),
                        cell["source"][0] * zoom, cell["source"][1] * zoom, width, height)
        image[top * scale:top * scale + height, left * scale:left * scale + width] = recipe(texels, cell["kind"])
    if any(cell["kind"] in ("meter", "dxt") for cell in entry["cells"]):
        image = bleed(image)
    if any(cell["kind"] == "meter" for cell in entry["cells"]):
        image[..., 1] = coverage(image[..., 3])
    return add_meter_thresholds(image, entry)


def thresholds(arguments) -> None:
    """Rebuild only categorical data in committed PNGs, retaining their art."""
    for entry in json.loads(LAYOUT.read_text())["assets"]:
        if any(cell.get("thresholds") for cell in entry["cells"]):
            path = ASSETS / f"{entry['name']}.png"
            image = np.asarray(Image.open(path).convert("RGBA")).copy()
            Image.fromarray(add_meter_thresholds(image, entry), "RGBA").save(path, optimize=True)
            print(f"{path.name}: rebuilt exact segment thresholds; artwork unchanged")


def build(arguments) -> None:
    description = json.loads(LAYOUT.read_text())
    names = {f"{entry['name']}.png" for entry in description["assets"]}
    for stale in ASSETS.glob("*.png"):
        if stale.name not in names:
            stale.unlink()
    renders = {}
    for entry in description["assets"]:
        image = build_asset(entry, renders)
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
        if zlib.crc32(bitmap["pixels"][:level0_size(bitmap)]) != entry["crc"]:
            print(f"{entry['name']}: this map's bitmap is not the one laid out")
        xbox = decode_bitmap(bitmap)
        image = Image.open(ASSETS / f"{entry['name']}.png").convert("RGBA")
        reduced = np.asarray(image.resize((entry["width"], entry["height"]), Image.BOX))
        score = overlap(reduced[..., 3].astype(float), xbox[..., 3].astype(float))
        print(f"{entry['name']}: alpha overlap {score:.2f}")
        # Xbox | ours reduced | ours, each over blue, then each one's alpha
        panels = [Image.fromarray(picture, "RGBA").resize(image.size, Image.NEAREST) for picture in (xbox, reduced)]
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
    commands.add_parser("thresholds")
    command = commands.add_parser("check")
    command.add_argument("--map", required=True)
    command.add_argument("--out", required=True)
    arguments = parser.parse_args()
    {"layout": layout, "build": build, "thresholds": thresholds, "check": check}[arguments.command](arguments)


if __name__ == "__main__":
    sys.exit(main())
