#!/usr/bin/env python3
"""Makes high-res textures of the menus' controller button icons
(port/assets/buttons/*.png) from hand-drawn SVG redraws of them
(port/assets/buttons/svg):

    python tools/button_assets.py layout --map assets/maps/ui.map
    python tools/button_assets.py build
    python tools/button_assets.py check --map assets/maps/ui.map --out /tmp/button_check

The icons beside the menus' key labels (A SELECT, B BACK, X DELETE, Y CREATE
NEW) are 32x32 bitmaps of ui.map with their art in the top left corner,
which the widgets' backgrounds draw at one texel to the pixel
(draw_bitmap_in_rect crops a bitmap larger than its widget, in coordinates
of the bitmap's size); the dialogs, the pause menu and the help screens draw
smaller A and B icons, bitmaps of their own in every map. So a texture of 8x
a bitmap's size, in its layout, draws in its place unchanged, as the
high-res HUD's do (port/linux/src/hud_hires.c), and with the high-res text
(display.high_res_text), as the menus' titles do.

Each redraw is drawn in the texels (viewBox 0 0 32 32) of the large icon it
is named after, over the map's art: a tilted oval with a pale rim, dark at
the top and bright at the bottom, and its letter in white (Y's casting a
dark shadow). A small icon is the same art smaller.

The menus' text draws the same buttons as message icons ("Press %a-button
to Join": draw_string_and_hack_in_icons, ui_widget.c), sprites of a sheet
of every map, hud_msg_icons_sm, that the game tints: stencils, each oval in
white with its letter cut out. They are drawn from the redraws' oval and
letter (their ids "oval" and "letter"), into a texture of the sheet's
layout that has only those sprites: the sheet's other icons (triggers and
sticks, with English words) keep the map's, so the texture stands for the
sprites alone, and the game draws them from it through a placeholder
(port/linux/game/hud_hires_tags.c).

layout: reads each icon's bitmap from the map (its size, format and the CRC
    of its pixels, which the game checks before drawing the texture in its
    place) and places its redraw on it: a small icon's scaled and moved as
    the map's small art is from its large art (so that their shapes' area
    and centre are the same), and a message icon's stencil where it covers
    the sprite's best. Then each icon's letter (id "letter", with Y's
    shadow, id "shadow") is moved on its oval to where the map's letter is
    on that icon: the map's letters do not sit alike on every icon, so one
    redraw's letter does not fit them all. Writes
    port/assets/buttons/buttons.json.
build: renders the redraws as buttons.json places them into
    port/assets/buttons/*.png (committed; the builds embed them:
    tools/embed_assets.py).
check: compares each PNG, reduced to its bitmap's size, with the map's
    bitmap and writes side-by-side images into --out.

Needs rsvg-convert, Pillow, NumPy and SciPy.
"""

import argparse
import json
import sys
import tempfile
import xml.etree.ElementTree as ElementTree
import zlib
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hud_assets import (FORMATS, XboxMap, bleed, decode_bitmap, level0_size, overlap,  # noqa: E402
                        pixel_rectangle, render_svg)

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "port/assets/buttons"
LIST = ASSETS / "buttons.json"
SCALE = 8

BITMAPS = "ui\\shell\\bitmaps\\"
# each icon's bitmap and its redraw (drawn over the bitmap it is named
# after): the button keys' (ui.map), and the smaller ones of the dialogs,
# the pause menu and the help screens (every map)
BUTTONS = {
    BITMAPS + "a_butn": "a_butn.svg",
    BITMAPS + "b_butn": "b_butn.svg",
    BITMAPS + "x_butn": "x_butn.svg",
    BITMAPS + "y_butn": "y_butn.svg",
    BITMAPS + "a_butn_sm": "a_butn.svg",
    BITMAPS + "b_butn_sm": "b_butn.svg",
}
# the message icons' sheet, and the redraw of each of its buttons, by the
# sequence of its sprite (on the sheet's first bitmap)
MESSAGE_ICONS = "ui\\hud\\bitmaps\\hud_msg_icons_sm"
MESSAGE_BUTTONS = {0: "a_butn.svg", 1: "b_butn.svg", 2: "x_butn.svg", 3: "y_butn.svg"}


def document(width: int, height: int, body: str) -> str:
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
            f'viewBox="0 0 {width} {height}">\n{body}\n</svg>\n')


# the redraws' letter, and the shadow Y's letter casts, which move together
LETTER_IDS = ("letter", "shadow")

ElementTree.register_namespace("", "http://www.w3.org/2000/svg")


def placed(svg: Path, width: int, height: int, place: list, letter: list = (0, 0)) -> str:
    """The redraw as a picture of its bitmap's size, scaled by place[0] and
    moved by place[1:] (texels), its letter moved a further letter (texels
    of the bitmap) on its oval."""
    root = ElementTree.parse(svg).getroot()
    scale, x, y = place
    for element in root.iter():
        if element.get("id") in LETTER_IDS:
            element.set("transform",
                        f"translate({letter[0] / scale} {letter[1] / scale}) {element.get('transform', '')}")
    body = "".join(ElementTree.tostring(child, encoding="unicode") for child in root)
    return document(width, height, f'<g transform="translate({x} {y}) scale({scale})">{body}</g>')


def stencil(svg: str, cell: list, place: list, index: int, letter: list = (0, 0)) -> str:
    """A redraw as the message icons draw its button, in a sprite's cell
    (texels of its sheet): its oval in white with its letter cut out,
    scaled by place[0] and moved by place[1:] from the cell's corner, its
    letter moved a further letter (texels of the sheet), and kept to the
    cell; index names its clip and mask."""
    root = ElementTree.parse(ASSETS / "svg" / svg).getroot()
    parents = {child: parent for parent in root.iter() for child in parent}
    oval, cut = root.find(".//*[@id='oval']"), root.find(".//*[@id='letter']")
    left, top, right, bottom = cell
    scale, x, y = place
    moved = f"translate({left + x} {top + y}) scale({scale})"
    area = f'x="{left}" y="{top}" width="{right - left}" height="{bottom - top}"'
    return (f'<defs><clipPath id="cell{index}"><rect {area}/></clipPath>\n'
            f'<mask id="letter{index}" maskUnits="userSpaceOnUse" {area}><rect {area} fill="#ffffff"/>\n'
            f'<path transform="{moved} translate({letter[0] / scale} {letter[1] / scale}) {cut.get("transform")}" '
            f'fill="#000000" d="{cut.get("d")}"/></mask></defs>\n'
            f'<g clip-path="url(#cell{index})" mask="url(#letter{index})">'
            f'<ellipse transform="{moved} {parents[oval].get("transform")}" rx="{oval.get("rx")}" '
            f'ry="{oval.get("ry")}" fill="#ffffff"/></g>')


def render(text: str, scale: int) -> np.ndarray:
    """An SVG document at scale times its size."""
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "placed.svg"
        path.write_text(text)
        return render_svg(path, scale)


def icon(xbox_map: XboxMap, tag: str) -> dict:
    """An icon's bitmap (its group's only one)."""
    if ("bitm", tag) not in xbox_map.tags:
        sys.exit(f"{tag}: not in this map")
    bitmaps = xbox_map.bitmap_group(tag)["bitmaps"]
    if len(bitmaps) != 1:
        sys.exit(f"{tag}: {len(bitmaps)} bitmaps, not the icon's one")
    return bitmaps[0]


def shape(alpha: np.ndarray, scale: int = 1) -> tuple:
    """The area and centre of a shape, in texels, from its alpha drawn at
    scale times the texels."""
    alpha = alpha.astype(float) / 255
    ys, xs = np.mgrid[0:alpha.shape[0], 0:alpha.shape[1]]
    area = alpha.sum()
    return (area / scale ** 2, ((xs + 0.5) * alpha).sum() / area / scale,
            ((ys + 0.5) * alpha).sum() / area / scale)


def stencil_place(svg: str, alpha: np.ndarray) -> list:
    """Where a redraw's stencil goes in a sprite's cell, whose alpha it is:
    where it covers it best, from where it has its area and centre."""
    from scipy import optimize

    height, width = alpha.shape
    root = ElementTree.parse(ASSETS / "svg" / svg).getroot()
    size = int(root.get("width")), int(root.get("height"))
    area, x, y = shape(render(document(*size, stencil(svg, [0, 0, *size], [1, 0, 0], 0)), SCALE)[..., 3], SCALE)
    sprite_area, sprite_x, sprite_y = shape(alpha)
    scale = (sprite_area / area) ** 0.5
    target = alpha.astype(float) / 255

    def error(place: np.ndarray) -> float:
        drawn = render(document(width, height, stencil(svg, [0, 0, width, height], list(place), 0)), SCALE)
        reduced = drawn[..., 3].astype(float).reshape(height, SCALE, width, SCALE).mean(axis=(1, 3)) / 255
        return float(((reduced - target) ** 2).sum())

    best = optimize.minimize(error, [scale, sprite_x - scale * x, sprite_y - scale * y], method="Powell",
                             options={"xtol": 1e-3, "ftol": 1e-7}).x
    return [round(float(best[0]), 4), round(float(best[1]), 3), round(float(best[2]), 3)]


def whiteness(image: np.ndarray) -> np.ndarray:
    """How white each texel is, over black: the letters' white against the
    ovals' colours and rims, which do not move with the letter."""
    alpha = image[..., 3:4].astype(float) / 255
    return (image[..., :3].astype(float) * alpha).min(axis=2) / 255


def best_letter(error) -> list:
    """The letter's move (texels) with the least error, from none."""
    from scipy import optimize

    best = optimize.minimize(lambda move: error(list(move)), [0.0, 0.0], method="Powell",
                             options={"xtol": 1e-3, "ftol": 1e-8}).x
    return [round(float(best[0]), 3), round(float(best[1]), 3)]


def button_letter(entry: dict, bitmap: dict) -> list:
    """Where a button's letter goes on its oval (a move, in texels): where
    its white is closest to the map's."""
    target = whiteness(decode_bitmap(bitmap))
    width, height = entry["width"], entry["height"]

    def error(move: list) -> float:
        drawn = render(placed(ASSETS / "svg" / entry["svg"], width, height, entry["place"], move), SCALE)
        reduced = np.asarray(Image.fromarray(drawn, "RGBA").resize((width, height), Image.BOX))
        return float(((whiteness(reduced) - target) ** 2).sum())

    return best_letter(error)


def sprite_letter(sprite: dict, alpha: np.ndarray) -> list:
    """Where a message icon's letter goes on its oval (a move, in texels of
    the sheet): where its cut-out is closest to the map's sprite, whose
    alpha it is."""
    left, top, right, bottom = sprite["cell"]
    width, height = right - left, bottom - top
    target = alpha.astype(float) / 255
    cell = [0, 0, width, height]

    def error(move: list) -> float:
        drawn = render(document(width, height, stencil(sprite["svg"], cell, sprite["place"], 0, move)), SCALE)
        reduced = drawn[..., 3].astype(float).reshape(height, SCALE, width, SCALE).mean(axis=(1, 3)) / 255
        return float(((reduced - target) ** 2).sum())

    return best_letter(error)


def layout(arguments) -> None:
    xbox_map = XboxMap(Path(arguments.map))
    entries = []
    for tag, svg in BUTTONS.items():
        bitmap = icon(xbox_map, tag)
        width, height = bitmap["width"], bitmap["height"]
        drawn_over = BITMAPS + Path(svg).stem
        place = [1, 0, 0]
        if drawn_over != tag:
            area, x, y = shape(decode_bitmap(bitmap)[..., 3])
            drawn_area, drawn_x, drawn_y = shape(decode_bitmap(icon(xbox_map, drawn_over))[..., 3])
            scale = (area / drawn_area) ** 0.5
            place = [round(scale, 4), round(x - scale * drawn_x, 3), round(y - scale * drawn_y, 3)]
        entry = {
            "name": tag.split("\\")[-1] + "__0",
            "tag": tag,
            "bitmap": 0,
            "width": width,
            "height": height,
            "format": FORMATS[bitmap["format"]],
            "scale": SCALE,
            # (the map's, which the game checks before drawing this in its
            # place: other languages' maps and modified ones may differ)
            "crc": zlib.crc32(bitmap["pixels"][:level0_size(bitmap)]),
            "svg": svg,
            "place": place,
        }
        entry["letter"] = button_letter(entry, bitmap)
        entries.append(entry)
        print(f"{tag}: {width}x{height} {FORMATS[bitmap['format']]}, {svg} at {place[0]}x, moved {place[1]}, "
              f"{place[2]}; its letter moved {entry['letter'][0]}, {entry['letter'][1]}")
    # the message icons' buttons
    if ("bitm", MESSAGE_ICONS) not in xbox_map.tags:
        sys.exit(f"{MESSAGE_ICONS}: not in this map")
    group = xbox_map.bitmap_group(MESSAGE_ICONS)
    bitmap = group["bitmaps"][0]
    width, height = bitmap["width"], bitmap["height"]
    alpha = decode_bitmap(bitmap)[..., 3]
    sprites = []
    for sequence, svg in MESSAGE_BUTTONS.items():
        cells = group["sequences"][sequence]
        if len(cells) != 1 or cells[0][0] != 0:
            sys.exit(f"{MESSAGE_ICONS}: sequence {sequence} is not one sprite of its first bitmap")
        cell = pixel_rectangle(cells[0], width, height)
        place = stencil_place(svg, alpha[cell[1]:cell[3], cell[0]:cell[2]])
        sprite = {"sequence": sequence, "cell": cell, "svg": svg, "place": place}
        sprite["letter"] = sprite_letter(sprite, alpha[cell[1]:cell[3], cell[0]:cell[2]])
        sprites.append(sprite)
        print(f"{MESSAGE_ICONS} sequence {sequence}: cell {cell}, {svg} at {place[0]}x, moved {place[1]}, "
              f"{place[2]}; its letter moved {sprite['letter'][0]}, {sprite['letter'][1]}")
    entries.append({
        "name": MESSAGE_ICONS.split("\\")[-1] + "__0",
        "tag": MESSAGE_ICONS,
        "bitmap": 0,
        "width": width,
        "height": height,
        "format": FORMATS[bitmap["format"]],
        "scale": SCALE,
        "crc": zlib.crc32(bitmap["pixels"][:level0_size(bitmap)]),
        # (the sprites it stands for, not the whole bitmap)
        "sprites": sprites,
    })
    LIST.write_text(json.dumps({"assets": entries}, indent=1) + "\n")


def build(arguments) -> None:
    description = json.loads(LIST.read_text())
    names = {f"{entry['name']}.png" for entry in description["assets"]}
    for stale in ASSETS.glob("*.png"):
        if stale.name not in names:
            stale.unlink()
    for entry in description["assets"]:
        if "sprites" in entry:
            body = "\n".join(stencil(sprite["svg"], sprite["cell"], sprite["place"], index,
                                     sprite.get("letter", (0, 0)))
                             for index, sprite in enumerate(entry["sprites"]))
            image = render(document(entry["width"], entry["height"], body), entry["scale"])
            # (grey the same as alpha, as the sheet's)
            image[..., :3] = image[..., 3:4]
        else:
            image = render(placed(ASSETS / "svg" / entry["svg"], entry["width"], entry["height"], entry["place"],
                                  entry.get("letter", (0, 0))), entry["scale"])
            # (the colour carried into the transparent texels, so that
            # filtering and the mip levels keep it at the oval's edge)
            image = bleed(image)
        Image.fromarray(image, "RGBA").save(ASSETS / f"{entry['name']}.png", optimize=True)
        print(f"{entry['name']}.png: {image.shape[1]}x{image.shape[0]}")


def check(arguments) -> None:
    xbox_map = XboxMap(Path(arguments.map))
    output = Path(arguments.out)
    output.mkdir(parents=True, exist_ok=True)
    for entry in json.loads(LIST.read_text())["assets"]:
        if ("bitm", entry["tag"]) not in xbox_map.tags:
            print(f"{entry['name']}: not in this map")
            continue
        bitmap = xbox_map.bitmap_group(entry["tag"])["bitmaps"][entry["bitmap"]]
        if zlib.crc32(bitmap["pixels"][:level0_size(bitmap)]) != entry["crc"]:
            print(f"{entry['name']}: this map's bitmap is not the one laid out")
        xbox = decode_bitmap(bitmap)
        image = Image.open(ASSETS / f"{entry['name']}.png").convert("RGBA")
        reduced = np.asarray(image.resize((entry["width"], entry["height"]), Image.BOX))
        scale = entry["scale"]
        # (each sprite's cell, or the whole bitmap)
        cells = [sprite["cell"] for sprite in entry.get("sprites", [])] or [[0, 0, entry["width"], entry["height"]]]
        for index, (left, top, right, bottom) in enumerate(cells):
            score = overlap(reduced[top:bottom, left:right, 3].astype(float), xbox[top:bottom, left:right, 3].astype(float))
            name = entry["name"] + (f"__sprite{index}" if "sprites" in entry else "")
            print(f"{name}: alpha overlap {score:.2f}")
            # the map's | ours reduced | ours, over the menus' dark blue
            ours = image.crop((left * scale, top * scale, right * scale, bottom * scale))
            panels = [Image.fromarray(np.ascontiguousarray(picture[top:bottom, left:right]), "RGBA").resize(
                ours.size, Image.NEAREST) for picture in (xbox, reduced)]
            panels.append(ours)
            width, height = ours.size
            sheet = Image.new("RGB", (width * 3 + 16, height), (255, 0, 0))
            for place, picture in enumerate(panels):
                background = Image.new("RGBA", picture.size, (16, 26, 60, 255))
                sheet.paste(Image.alpha_composite(background, picture).convert("RGB"), (place * (width + 8), 0))
            sheet.save(output / f"{name}.png")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("layout")
    command.add_argument("--map", required=True)
    commands.add_parser("build")
    command = commands.add_parser("check")
    command.add_argument("--map", required=True)
    command.add_argument("--out", required=True)
    arguments = parser.parse_args()
    {"layout": layout, "build": build, "check": check}[arguments.command](arguments)


if __name__ == "__main__":
    sys.exit(main())
