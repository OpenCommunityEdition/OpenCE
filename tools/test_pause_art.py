"""Check the shipped pause artwork at its logical widget coordinates."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET

from PIL import Image
import pytest

ROOT = Path(__file__).resolve().parents[1]
MENUS = ROOT / 'port/assets/menus'


@pytest.mark.parametrize('piece', ['left', 'center', 'right'])
@pytest.mark.parametrize('frame,height', [(0, 194), (1, 229), (2, 195)])
def test_button_strip(piece, frame, height):
    image = Image.open(MENUS / f'ce/port/pause/pausebox_{piece}__{frame}.png').convert('RGBA')
    scale = image.height // 256
    x = image.width // 2
    assert image.getpixel((x, (height - 15) * scale)) == (0, 0, 0, 255)
    if frame == 2:
        assert_campaign_gradient(image, 2, height - 29, piece)
    else:
        assert image.getpixel((x, 40 * scale)) == pytest.approx((10, 38, 73, 191), abs=1)
    assert image.getpixel((x, (height + 1) * scale))[3] == 0
    assert image.getpixel((x, (height - 28) * scale))[:3] == (40, 150, 255)


@pytest.mark.parametrize('piece', ['left', 'center', 'right'])
def test_objective_header(piece):
    name = f'ce/port/pause/objectives_{piece}__0.png'
    image = Image.open(MENUS / name).convert('RGBA')
    scale = image.height // 256
    x = image.width // 2
    assert image.getpixel((x, 15 * scale)) == (0, 0, 0, 255)
    assert image.getpixel((x, 28 * scale))[:3] == (40, 150, 255)
    assert_campaign_gradient(image, 29, 192, piece)
    assert image.getpixel((x, 196 * scale))[3] == 0
    if piece != 'center':
        assert image.getpixel((0 if piece == 'left' else image.width - 1, 0))[3] == 0
    definitions = ET.parse(MENUS / 'ce/bitmaps.xml').getroot()
    bitmap = definitions.find(f"bitmap[@name='pause/objectives_{piece}']")
    assert [frame.get('png') for frame in bitmap] == [name]
    assert name in json.loads((MENUS / 'menus.json').read_text())['files']


def assert_campaign_gradient(image, top, bottom, piece):
    scale = image.height // 256
    x = image.width // 2
    x = image.width - 1 if piece == "left" else 0 if piece == "right" else x
    samples = [image.getpixel((x, y * scale)) for y in range(top, bottom + 1)]
    assert samples[0] == pytest.approx((10, 38, 73, 191), abs=2)
    assert samples[-1] == pytest.approx((5, 13, 28, 191), abs=2)
    assert all(pixel[3] == 191 for pixel in samples)
    for a, b in zip(samples, samples[1:]):
        assert all(b[channel] <= a[channel] for channel in range(3))
