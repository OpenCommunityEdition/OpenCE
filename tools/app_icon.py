#!/usr/bin/env python3
"""Makes the game's icons from its artwork, port/assets/icon:

    python tools/app_icon.py

- opence-icon.png, the helmet on its background (512x512): the Windows
  executable's icon (port/windows/opence-icon.ico, which port/windows/halo.rc
  puts in halo.exe) and the desktop builds' window icon (opence-icon-256.png,
  which tools/embed_assets.py embeds and sdl_platform.c gives the window);
- opence-icon-render.png, the helmet alone on transparency (1024x1024, the same
  framing): the Android app's launcher icon.

The launcher icon is an adaptive one (Android 8 and later): the render as its
foreground layer, the background's gradient as its background layer, and the
render's light parts (the armour, not the visor) as its monochrome (themed
icon) layer. Launchers mask adaptive icons to their own shapes and show only
the middle 72 dp of the 108 dp layers, so the render fills those 72 dp, as
the picture fills the other icons, and its edges are carried out to the
layers' edges for the launchers that move the layers. Writes the layers for
every screen density into port/android/app/src/main/res (mipmap-*), and the
icon's definition (mipmap-anydpi-v26/ic_launcher.xml) and background
(drawable/ic_launcher_background.xml). Run it again when the artwork
changes. Needs Pillow and NumPy.
"""

from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ARTWORK = ROOT / "port/assets/icon"
ICON = ARTWORK / "opence-icon.png"
RENDER = ARTWORK / "opence-icon-render.png"
WINDOW_ICON = ARTWORK / "opence-icon-256.png"
WINDOWS_ICON = ROOT / "port/windows/opence-icon.ico"
RESOURCES = ROOT / "port/android/app/src/main/res"

# the layers' size, and the part of them a launcher shows, in dp
LAYER_DP = 108
VISIBLE_DP = 72
# pixels per dp of each density
DENSITIES = {"mdpi": 1.0, "hdpi": 1.5, "xhdpi": 2.0, "xxhdpi": 3.0, "xxxhdpi": 4.0}
WINDOWS_SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]


def background_gradient(icon: Image.Image, render: Image.Image) -> tuple:
    """the icon's background as a line through its rows' colours (where the
    render leaves it bare): the colours at the top and the bottom of the
    launcher's layer, which reaches (LAYER_DP - VISIBLE_DP) / 2 beyond the
    picture at each side"""
    pixels = np.asarray(icon.convert("RGB"), dtype=np.float64)
    bare = np.asarray(render.resize(icon.size, Image.LANCZOS).getchannel("A")) == 0
    rows, colours = [], []
    for y in range(icon.height):
        if bare[y].any():
            rows.append(y)
            colours.append(pixels[y][bare[y]].mean(axis=0))
    fit = np.polyfit(np.array(rows, dtype=np.float64), np.array(colours), 1)
    margin = (LAYER_DP - VISIBLE_DP) / 2 / VISIBLE_DP * icon.height

    def at(y: float) -> tuple:
        return tuple(int(round(min(255.0, max(0.0, fit[0][c] * y + fit[1][c])))) for c in range(3))

    return at(-margin), at(icon.height + margin)


def carried_out(image: Image.Image, size: int) -> Image.Image:
    """image in the middle of a square of size, its edge pixels carried out
    to the square's edges"""
    pad = (size - image.width) // 2
    data = np.asarray(image)
    padded = np.pad(data, ((pad, size - image.height - pad), (pad, size - image.width - pad), (0, 0)), mode="edge")
    return Image.fromarray(padded, "RGBA")


def monochrome(render: Image.Image) -> Image.Image:
    """the render's light parts, white with their lightness as opacity: the
    armour without the darker visor"""
    data = np.asarray(render, dtype=np.float64)
    lightness = (0.2126 * data[..., 0] + 0.7152 * data[..., 1] + 0.0722 * data[..., 2]) / 255.0
    alpha = np.clip(data[..., 3] / 255.0 * np.clip((lightness - 0.25) / 0.5, 0.0, 1.0), 0.0, 1.0)
    out = np.zeros(data.shape, dtype=np.uint8)
    out[..., :3] = 255
    out[..., 3] = np.round(alpha * 255).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def main() -> None:
    icon = Image.open(ICON).convert("RGBA")
    render = Image.open(RENDER).convert("RGBA")

    # the desktop's icons
    icon.resize((256, 256), Image.LANCZOS).save(WINDOW_ICON, optimize=True)
    print(f"{WINDOW_ICON.relative_to(ROOT)}: 256x256")
    icon.save(WINDOWS_ICON, sizes=[(size, size) for size in WINDOWS_SIZES])
    print(f"{WINDOWS_ICON.relative_to(ROOT)}: {', '.join(str(size) for size in WINDOWS_SIZES)}")

    # the launcher's layers
    themed = monochrome(render)
    for density, scale in DENSITIES.items():
        size = round(LAYER_DP * scale)
        visible = round(VISIBLE_DP * scale)
        directory = RESOURCES / f"mipmap-{density}"
        directory.mkdir(parents=True, exist_ok=True)
        for name, layer in (("ic_launcher_foreground", render), ("ic_launcher_monochrome", themed)):
            carried_out(layer.resize((visible, visible), Image.LANCZOS), size).save(
                directory / f"{name}.png", optimize=True)
        print(f"{directory.relative_to(ROOT)}: {size}x{size}")

    top, bottom = background_gradient(icon, render)
    drawable = RESOURCES / "drawable"
    drawable.mkdir(parents=True, exist_ok=True)
    (drawable / "ic_launcher_background.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        "<!-- the launcher icon's background (tools/app_icon.py) -->\n"
        '<shape xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <gradient android:angle="270" android:startColor="#{:02X}{:02X}{:02X}"'
        ' android:endColor="#{:02X}{:02X}{:02X}" />\n'.format(*top, *bottom) +
        "</shape>\n", encoding="utf-8")
    adaptive = RESOURCES / "mipmap-anydpi-v26"
    adaptive.mkdir(parents=True, exist_ok=True)
    (adaptive / "ic_launcher.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        "<!-- the launcher icon (tools/app_icon.py, from port/assets/icon) -->\n"
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@drawable/ic_launcher_background" />\n'
        '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
        '    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />\n'
        "</adaptive-icon>\n", encoding="utf-8")
    print("background #{:02X}{:02X}{:02X} to #{:02X}{:02X}{:02X}".format(*top, *bottom))


if __name__ == "__main__":
    main()
