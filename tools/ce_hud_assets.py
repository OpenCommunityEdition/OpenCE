"""Pack existing OpenCE redraws into CE sprite rectangles.

The committed recipe contains coordinates only. Build needs no game data:
    python tools/ce_hud_assets.py build
"""
import argparse
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'port/assets/hud'
RECIPE = ASSETS / 'ce_layout.json'


def build_asset(entry):
    if not 0 < entry['width'] * entry['scale'] <= 2048 or not 0 < entry['height'] * entry['scale'] <= 2048:
        raise ValueError('CE HUD atlas exceeds texture bounds')
    if entry['cells'] and 'source_png' in entry['cells'][0]:
        from hud_assets import build_asset as pack_supplied_cells
        return Image.fromarray(pack_supplied_cells(entry, {}))
    image = Image.new('RGBA', (entry['width'] * entry['scale'], entry['height'] * entry['scale']))
    for cell in entry['cells']:
        with Image.open(ASSETS / (cell['source'] + '.png')) as source:
            l, t, r, b = cell['from']
            if not 0 <= l < r <= source.width or not 0 <= t < b <= source.height:
                raise ValueError('source rectangle outside redraw')
            crop = source.crop(cell['from'])
        rect = [round(v * entry['scale']) for v in cell['to']]
        if not 0 <= rect[0] < rect[2] <= image.width or not 0 <= rect[1] < rect[3] <= image.height:
            raise ValueError('destination rectangle outside CE atlas')
        size = (rect[2] - rect[0], rect[3] - rect[1])
        # Meter red is categorical threshold data, never interpolate it.
        resized = crop.resize(size, Image.Resampling.BICUBIC)
        if entry.get('point_threshold'):
            channels = list(resized.split())
            channels[0] = crop.getchannel('R').resize(size, Image.Resampling.NEAREST)
            resized = Image.merge('RGBA', channels)
        image.paste(resized, rect[:2])
    return image


def build():
    for entry in json.loads(RECIPE.read_text())['assets']:
        build_asset(entry).save(ASSETS / (entry['name'] + '.png'), optimize=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['build'])
    parser.parse_args()
    build()
