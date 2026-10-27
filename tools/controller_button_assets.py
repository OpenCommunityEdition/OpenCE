#!/usr/bin/env python3
"""Build supplied controller glyphs without changing the legacy Xbox assets.

python tools/controller_button_assets.py
Source PNGs are retained unchanged under port/assets/controller_buttons/source.
The generated bounds exclude transparent padding when fitting gameplay cells.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'port/assets/controller_buttons'
STYLES = ['xbox_modern', 'ps_classic', 'ps_gems', 'ps_dark_gems', 'ps_modern',
          'switch_classic', 'switch_gems', 'switch_modern']


def sources():
    entries = []
    for style in STYLES:
        family, variant = style.split('_', 1)
        suffix = {'classic': 'simple', 'gems': 'gem', 'dark_gems': 'darkgem'}.get(variant, variant)
        # SDL face positions: south, east, west, north. Switch's printed letters
        # are B/A/Y/X at those positions; never remap gameplay to match a label.
        letters = ['A', 'B', 'X', 'Y'] if family == 'xbox' else (
            ['X-PS', 'circle', 'square', 'triangle'] if family == 'ps' else ['B', 'A', 'Y', 'X'])
        for slot, letter in enumerate(letters):
            filename = (f'{letter}_switch.png' if variant == 'gems' else f'{letter}-switch_{suffix}.png') if family == 'switch' else f'{letter}_{suffix}.png'
            entries.append(dict(name=f'{style}_{slot}', source=f'{family}/{filename}', cell=[21, 26]))
    for family in ('xbox', 'ps', 'switch'):
        shoulders = ['bumper_right_modern.png', 'bumper_left_modern.png'] if family == 'xbox' else (
            ['bumper_right_ps.png', 'bumper_left_ps.png'] if family == 'ps' else ['bumper_right.png', 'bumper_left.png'])
        controls = shoulders + ['trigger_left.png', 'trigger_right.png'] + [
            f'thumbstick{state}_{side}.png' for state in ('', '-halfway', '-full') for side in ('left', 'right')]
        for slot, filename in enumerate(controls, 4):
            entry = dict(name=f'{family}_control_{slot}', source=f'{family}/{filename}',
                         cell=[21, 26] if slot < 6 else [44, 33] if slot < 8 else [32, 34])
            if family != 'xbox' and slot < 8:
                entry['label'] = (['R1', 'L1', 'L2', 'R2'] if family == 'ps' else ['R', 'L', 'ZL', 'ZR'])[slot - 4]
                entry['trigger'] = slot >= 6
            entries.append(entry)
    return entries


def correct_label(face, text, trigger):
    """Replace the supplied cutout lettering; preserve the exterior silhouette.

    The trigger's large inner cavity is intentional and is not a letter.
    Source PNGs stay unchanged. Labels use the project's Overpass font.
    """
    from scipy import ndimage
    pixels = np.asarray(face).copy()
    solid = pixels[..., 3] >= 128
    holes = ndimage.binary_fill_holes(solid) & ~solid
    components, _ = ndimage.label(holes)
    mask = np.zeros(solid.shape, dtype=bool)
    for number, box in enumerate(ndimage.find_objects(components), 1):
        if trigger and box[0].stop - box[0].start > face.height // 2:
            continue
        if np.count_nonzero(components[box] == number) > 100:
            mask |= components == number
    ys, xs = np.where(mask)
    if not len(xs):
        raise ValueError('No supplied control lettering found')
    left, top, right, bottom = int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1
    # Include the old lettering's antialiased edges without touching its outline.
    pixels[ndimage.binary_dilation(mask, iterations=4)] = [255, 255, 255, 255]
    font_path = ROOT / 'port/assets/fonts/Overpass-900.ttf'
    # These ASCII labels were authored with Basic layout. Raqm availability
    # must not change their metrics or rasterized pixels on another platform.
    font = ImageFont.truetype(str(font_path), 1000, layout_engine=ImageFont.Layout.BASIC)
    box = font.getbbox(text)
    size = int(1000 * min((right-left) / (box[2]-box[0]), (bottom-top) / (box[3]-box[1])))
    font = ImageFont.truetype(str(font_path), size, layout_engine=ImageFont.Layout.BASIC)
    box = font.getbbox(text)
    ink = Image.new('L', face.size)
    ImageDraw.Draw(ink).text(((left+right-box[2]-box[0])/2, (top+bottom-box[3]-box[1])/2), text, font=font, fill=255)
    pixels[..., 3] = np.minimum(pixels[..., 3], 255 - np.asarray(ink))
    return Image.fromarray(pixels)


def render(entry):
    source = Image.open(ASSETS / 'source' / entry['source']).convert('RGBA')
    bounds = source.getchannel('A').getbbox()
    if not bounds:
        raise ValueError(f"Empty artwork: {entry['source']}")
    face = source.crop(bounds)
    if 'label' in entry:
        face = correct_label(face, entry['label'], entry['trigger'])
    # Keep the supplied aspect ratio. The renderer scales the visible bounds
    # to the established face/control dimensions, centered on the prompt text.
    # Preserve the committed single-pass RGBA resize across Pillow versions.
    face.thumbnail((entry['cell'][0] * 8 - 8, entry['cell'][1] * 8 - 8),
                   Image.Resampling.LANCZOS, reducing_gap=None)
    result = Image.new('RGBA', (512, 512))
    result.paste(face, ((512 - face.width) // 2, (512 - face.height) // 2))
    return result


def build():
    assets = []
    bounds = []
    for entry in sources():
        image = render(entry)
        image.save(ASSETS / (entry['name'] + '.png'), optimize=True)
        alpha = np.asarray(image)[..., 3]
        ys, xs = np.where(alpha >= 128)
        bounds.append([int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1])
        assets.append(dict(name=entry['name'], tag='ui\\controller_buttons\\' + entry['name'],
                           bitmap=0, width=64, height=64, format='a8r8g8b8', scale=8, crc=0))
    (ASSETS / 'buttons.json').write_text(json.dumps({'assets': assets}, indent=1) + '\n')
    header = ['/* Generated by tools/controller_button_assets.py. */',
              'static struct controller_glyph_asset const controller_glyph_assets[] = {']
    for entry, box in zip(assets, bounds):
        header.append('    {' + json.dumps(entry['tag']) + ', {' + ', '.join(map(str, box)) + '}},')
    header += ['};', '']
    (ROOT / 'port/linux/game/controller_glyph_assets.h').write_text('\n'.join(header))


if __name__ == '__main__':
    build()
