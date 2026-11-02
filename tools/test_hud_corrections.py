"""Supplied artwork fidelity, classic cutouts, and CE sprite selection."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

from harness import function
from hud_assets import build_asset
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'port/assets/hud'



def test_supplied_sources_are_unchanged():
    manifest = json.loads((ASSETS / 'corrections/source-sha256.json').read_text())
    for name, record in manifest.items():
        assert hashlib.sha256((ASSETS / 'corrections' / name).read_bytes()).hexdigest() == record['sha256']


def test_packed_corrections_use_supplied_pixels():
    for filename in ['layout.json', 'ce_layout.json', 'mcc_layout.json']:
        for entry in json.loads((ASSETS / filename).read_text())['assets']:
            if not any('source_png' in cell for cell in entry['cells']):
                continue
            actual = np.asarray(Image.open(ASSETS / (entry['name']+'.png')))
            assert np.array_equal(actual, build_asset(entry, {}))
            # These assets are monochrome premultiplied coverage, not opaque
            # white pictures with black backgrounds or black letter holes.
            assert np.array_equal(actual[..., 0], actual[..., 3])
            assert np.array_equal(actual[..., 1], actual[..., 3])
            assert np.array_equal(actual[..., 2], actual[..., 3])
            for cell in entry['cells']:
                with Image.open(ASSETS / cell['source_png']) as supplied:
                    source = supplied.convert('L').crop(cell['crop'])
                x0,y0,x1,y1 = [round(v*entry['scale']) for v in cell['xbox']]
                ratio = min((x1-x0)/source.width, (y1-y0)/source.height)
                size = tuple(round(n*ratio) for n in source.size)
                x0 += (x1-x0-size[0])//2
                y0 += (y1-y0-size[1])//2
                expected = np.asarray(source.resize(size, Image.Resampling.LANCZOS))
                assert np.array_equal(actual[y0:y0+size[1], x0:x0+size[0], 3], expected)


def test_sniper_reticle_keeps_stock_visible_size_and_position():
    # Measured stock alpha bounds (>=128), not the padded sprite rectangles.
    # MCC's original sheet has four times Xbox/CE's texture resolution.
    for name, scale, sprite, bounds in [
        ('sniper__hud_reticles_scope__0', 8, [32, 0, 50, 18], [36, 4, 45, 13]),
        ('ce_sniper_scope', 8, [51, 0, 63, 12], [52, 1, 61, 10]),
        ('mcc_sniper_scope', 2, [108, 0, 152, 44], [112, 4, 148, 40]),
    ]:
        alpha = Image.open(ASSETS / (name+'.png')).getchannel('A')
        x0,y0,x1,y1 = [n*scale for n in sprite]
        visible = np.asarray(alpha.crop((x0,y0,x1,y1))) >= 128
        ys,xs = np.where(visible)
        actual = np.array([x0+xs.min(), y0+ys.min(), x0+xs.max()+1, y0+ys.max()+1])
        # Proportional resampling of the supplied image can round one edge
        # by one high-res pixel (1/8 Xbox/CE texel), not one stock texel.
        assert np.all(np.abs(actual - np.array(bounds)*scale) <= 1), name



def test_ce_keeps_8x_and_only_selects_verified_scope(tmp_path):
    body = function((ROOT / 'port/linux/game/hud_hires_tags.c').read_text(), 'hud_hires_crosshair_bitmap')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
struct bitmap_data {int id;};
static struct bitmap_data stock={0},custom={1},replacement={2};
static struct {struct bitmap_data *bitmap;long asset;} hires_sprite_bitmaps[1]={{&stock,0}};
static long hires_sprite_bitmap_count=1;
static int ce=1,enabled=1,verified=1,calls;
static char const *name="ui\\hud\\bitmaps\\sniper\\hud_reticles_scope";
static int custom_edition_cache_tags_loaded(void) {return ce;}
static int config_boolean(char const *s) {(void)s;return enabled;}
static char const *hud_hires_asset_tag(long i) {(void)i;return name;}
static struct bitmap_data const *hud_hires_sprite_bitmap(struct bitmap_data const *b,short s) {
    (void)s;calls++;return verified?&replacement:b;
}
''' + body + r'''
int main(void) {
    assert(hud_hires_crosshair_bitmap(&stock,0,0)==&replacement);
    assert(hud_hires_crosshair_bitmap(&stock,1,0)==&replacement);
    assert(hud_hires_crosshair_bitmap(&stock,1,1)==&stock && calls==2);
    assert(hud_hires_crosshair_bitmap(&custom,0,0)==&custom);
    enabled=0;assert(hud_hires_crosshair_bitmap(&stock,0,0)==&stock);
    enabled=1;ce=0;assert(hud_hires_crosshair_bitmap(&stock,0,0)==&stock);
    ce=1;verified=0;assert(hud_hires_crosshair_bitmap(&stock,0,0)==&stock);
    verified=1;name="custom";assert(hud_hires_crosshair_bitmap(&stock,0,0)==&stock);
    return 0;
}
''')
