#!/usr/bin/env python3
"""Draws the Android app's Porting options art (port/assets/menus/ce/port/porting):

    python tools/porting_assets.py            # the pictures, their XML and the glyphs' header
    python tools/porting_assets.py --check DIRECTORY

The PC main menu's items are pictures of text, not text: each has a frame for
the item at rest (pale blue, half transparent) and one for the item selected
(white, with a blue glow), which the game's widgets switch between. PORTING
OPTIONS is drawn the same way, in OpenCE (port/assets/fonts, the typeface the
port sets its own titles in: tools/ce_menus.py), at the size, colour and glow
of the other items' pictures. It is twice as wide as theirs (512 units), as
its text is longer at the same size.

The Porting options screens opened from the main menu set their text in the
same typeface. The game has no font tag of it, so each character is a frame of
one bitmap (white, tinted as it is drawn), and
port/linux/include/halo_porting_font.h has their advances
(source/interface/ui_widget_porting.h draws them).

--check draws SETTINGS as PORTING OPTIONS is drawn and writes it beside the
item's own picture into DIRECTORY, with how far apart the two are. Needs
Pillow and NumPy.
"""

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
MENUS = ROOT / "port/assets/menus"
FONT = ROOT / "port/assets/fonts/OpenCE-Regular.ttf"
HEADER = ROOT / "port/linux/include/halo_porting_font.h"
XML = "ce/porting.port.xml"
FOLDER = "ce/port/porting"
REFERENCE = MENUS / "ce/shell/main_menu"

# the pictures have this many pixels to a unit of the menus' 640x480
SCALE = 4
# an item's picture: its capitals' top and height, as the other items' (pixels)
ITEM_HEIGHT = 64 * SCALE
ITEM_CAP_TOP = 26
ITEM_CAP = 78
# at rest: the text's colour and opacity
REST_COLOR = (39, 148, 255)
REST_ALPHA = 119
# selected: white text over a glow of this colour, which is the text blurred
# (GLOW_SIGMA pixels), GLOW_GAIN times as strong and at most GLOW_ALPHA opaque
GLOW_COLOR = (40, 150, 255)
GLOW_SIGMA = 13.0
GLOW_GAIN = 1.3
GLOW_ALPHA = 0.65

# the glyphs: their capitals' height, the cell's height and baseline, and the
# clear pixels each side of a glyph's advance (pixels)
GLYPH_CAP = 64
GLYPH_HEIGHT = 112
GLYPH_BASELINE = 84
GLYPH_PAD = 12
GLYPHS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/#%.()[]?-+',!&=<>"


def font_of_cap(cap: float) -> ImageFont.FreeTypeFont:
    """the typeface with capitals `cap` pixels tall"""
    probe = ImageFont.truetype(str(FONT), 1000)
    left, top, right, bottom = probe.getbbox("H")
    return ImageFont.truetype(str(FONT), cap * 1000 / (bottom - top))


def text_mask(text: str, width: int, height: int, font: ImageFont.FreeTypeFont, cap_top: float) -> np.ndarray:
    """the text's coverage (0..1), centred in the width, its capitals' top at cap_top"""
    image = Image.new("L", (width, height), 0)
    left = (width - font.getlength(text)) / 2
    ImageDraw.Draw(image).text((left, cap_top - font.getbbox("H")[1]), text, font=font, fill=255)
    return np.asarray(image, dtype=np.float32) / 255.0


def item_frames(text: str, width: int) -> list:
    """an item's two pictures: at rest and selected"""
    mask = text_mask(text, width, ITEM_HEIGHT, font_of_cap(ITEM_CAP), ITEM_CAP_TOP)
    rest = np.zeros((ITEM_HEIGHT, width, 4), dtype=np.float32)
    rest[..., :3] = REST_COLOR
    rest[..., 3] = mask * REST_ALPHA
    blurred = np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(GLOW_SIGMA)),
                         dtype=np.float32) / 255.0
    glow = np.minimum(blurred * GLOW_GAIN, GLOW_ALPHA)
    # (white text over the glow)
    alpha = mask + glow * (1.0 - mask)
    selected = np.zeros_like(rest)
    for channel in range(3):
        selected[..., channel] = (255.0 * mask + GLOW_COLOR[channel] * glow * (1.0 - mask)) / np.maximum(alpha, 1e-6)
    selected[..., :3][alpha <= 0.0] = GLOW_COLOR
    selected[..., 3] = alpha * 255.0
    return [Image.fromarray(np.clip(np.rint(frame), 0, 255).astype(np.uint8), "RGBA") for frame in (rest, selected)]


def has_glyph(font: ImageFont.FreeTypeFont, character: str) -> bool:
    """whether the typeface draws the character (not its missing glyph)"""
    return font.getmask(character).getbbox() is not None and \
        bytes(font.getmask(character)) != bytes(font.getmask("\U0010ffff"))


def glyph_frames() -> tuple:
    """each glyph's picture (white, its coverage as the opacity), advance and
    cell width, and the space's advance"""
    font = font_of_cap(GLYPH_CAP)
    cap_top = GLYPH_BASELINE - GLYPH_CAP
    glyphs = []
    for character in GLYPHS:
        if not has_glyph(font, character):
            print(f"{FONT.name} has no {character!r}: left out", file=sys.stderr)
            continue
        advance = int(round(font.getlength(character)))
        width = advance + 2 * GLYPH_PAD
        # (whole units, as the menus' pictures are)
        width += -width % SCALE
        image = Image.new("L", (width, GLYPH_HEIGHT), 0)
        ImageDraw.Draw(image).text((GLYPH_PAD, cap_top - font.getbbox("H")[1]), character, font=font, fill=255)
        box = image.getbbox()
        if box and (box[0] == 0 or box[2] == width or box[1] == 0 or box[3] == GLYPH_HEIGHT):
            raise SystemExit(f"{character!r} does not fit its cell: {box} of {width}x{GLYPH_HEIGHT}")
        picture = Image.new("RGBA", image.size, (255, 255, 255, 0))
        picture.putalpha(image)
        glyphs.append((character, picture, advance, width))
    return glyphs, int(round(font.getlength(" ")))


def write_header(glyphs: list, space: int) -> None:
    lines = [
        "/* The Porting options screens' typeface (the main menu's): one frame of the",
        "   bitmap pc\\porting\\font for each character. Generated by",
        "   tools/porting_assets.py; do not edit. Sizes are in the frames' pixels. */",
        "#ifndef HALO_PORTING_FONT_H",
        "#define HALO_PORTING_FONT_H",
        "",
        '#define HALO_PORTING_FONT_TAG "pc\\\\porting\\\\font"',
        '#define HALO_PORTING_ITEM_TAG "pc\\\\porting\\\\menu_porting_options"',
        f"#define HALO_PORTING_FONT_CAP {GLYPH_CAP}",
        f"#define HALO_PORTING_FONT_HEIGHT {GLYPH_HEIGHT}",
        f"#define HALO_PORTING_FONT_BASELINE {GLYPH_BASELINE}",
        f"#define HALO_PORTING_FONT_PAD {GLYPH_PAD}",
        f"#define HALO_PORTING_FONT_SPACE {space}",
        f"#define HALO_PORTING_FONT_GLYPHS {len(glyphs)}",
        "",
        "/* frame `index` draws `code`; its cell is `width` wide, the pen moves `advance` */",
        "struct halo_porting_glyph { char code; unsigned char advance, width; };",
        "static const struct halo_porting_glyph halo_porting_glyphs[HALO_PORTING_FONT_GLYPHS] = {",
    ]
    for start in range(0, len(glyphs), 6):
        row = ", ".join("{'%s', %d, %d}" % (("\\" + c if c in "'\\" else c), advance, width)
                        for c, _, advance, width in glyphs[start:start + 6])
        lines.append(f"    {row},")
    lines += ["};", "", "#endif", ""]
    HEADER.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def write_assets() -> None:
    folder = MENUS / FOLDER
    folder.mkdir(parents=True, exist_ok=True)
    for stale in folder.glob("*.png"):
        stale.unlink()
    pngs = []
    xml = ['<?xml version="1.0" encoding="UTF-8"?>',
           "<!-- The Android app's Porting options art (tools/porting_assets.py) -->", "<menus>",
           '\t<bitmap name="porting/menu_porting_options" platform="android">']
    item_width = 512 * SCALE
    for index, frame in enumerate(item_frames("PORTING OPTIONS", item_width)):
        png = f"{FOLDER}/menu_porting_options__{index}.png"
        frame.save(MENUS / png, optimize=True)
        pngs.append(png)
        xml.append(f'\t\t<frame png="{png}" width="512" height="64"/>')
    xml += ["\t</bitmap>", '\t<bitmap name="porting/font" platform="android">']
    glyphs, space = glyph_frames()
    for index, (character, picture, advance, width) in enumerate(glyphs):
        png = f"{FOLDER}/font__{index:02d}.png"
        picture.save(MENUS / png, optimize=True)
        pngs.append(png)
        xml.append(f'\t\t<frame png="{png}" width="{width // SCALE}" '
                   f'height="{GLYPH_HEIGHT // SCALE}"/>')
    xml += ["\t</bitmap>", "</menus>", ""]
    (MENUS / XML).write_text("\n".join(xml), encoding="utf-8", newline="\n")
    write_header(glyphs, space)

    # menus.json lists what the game embeds: the XML files, then the pictures;
    # these only in the Android build (android_files: tools/embed_assets.py)
    listing = MENUS / "menus.json"
    menus = json.loads(listing.read_text(encoding="utf-8"))
    files = [name for name in menus["files"] if name != XML and not name.startswith(FOLDER + "/")]
    files += [XML, *pngs]
    menus["files"] = sorted((name for name in files if name.endswith(".xml"))) + \
        sorted(name for name in files if not name.endswith(".xml"))
    menus["android_files"] = [XML, *sorted(pngs)]
    listing.write_text(json.dumps(menus, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"{len(pngs)} pictures in {folder.relative_to(ROOT)}, {XML}, {HEADER.relative_to(ROOT)}")


def check(directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for index, frame in enumerate(item_frames("SETTINGS", 256 * SCALE)):
        reference = Image.open(REFERENCE / f"menu_settings__{index}.png").convert("RGBA")
        ours = np.asarray(frame, dtype=np.float32)
        theirs = np.asarray(reference, dtype=np.float32)
        # (compared as drawn over black: colour times opacity)
        difference = np.abs(ours[..., :3] * ours[..., 3:] - theirs[..., :3] * theirs[..., 3:]) / 255.0
        both = Image.new("RGBA", (frame.width, frame.height * 2), (0, 0, 0, 255))
        both.alpha_composite(reference, (0, 0))
        both.alpha_composite(frame, (0, frame.height))
        both.crop((0, 0, frame.width, frame.height * 2)).save(directory / f"settings__{index}.png")
        print(f"frame {index}: mean difference {difference.mean() / 255.0 * 100:.2f}% of full scale, "
              f"opacity {ours[..., 3].max():.0f} against {theirs[..., 3].max():.0f}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", type=Path, metavar="DIRECTORY",
                        help="draw SETTINGS beside the main menu's own picture of it, to compare")
    arguments = parser.parse_args()
    if arguments.check:
        check(arguments.check)
    else:
        write_assets()


if __name__ == "__main__":
    main()
