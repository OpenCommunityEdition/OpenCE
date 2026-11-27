"""Pack supplied HUD pictures directly: remove black, resize, and place.

Run: python tools/hud_corrections.py
No tracing, redrawing or font substitution. CE's existing 8x is retained.
"""
import json
from PIL import Image
import hud_assets

ASSETS = hud_assets.ASSETS


def cell(rect, name, crop):
    return dict(xbox=rect, source_png='corrections/'+name+'.png', crop=crop, kind='alpha')


def scope_cells(kind):
    # Stock visible coverage is 9x9 texels: Xbox [36,4,45,13],
    # CE [52,1,61,10], and MCC [112,4,148,40] at four times the scale.
    # This supplied crop includes black padding, so a 10x10 placement
    # reproduces that footprint within one output pixel. Keep the crop,
    # its antialiasing and the stock centre; do not fill the sprite cell.
    if kind == 'ce':
        return [cell([0, 0, 25, 18], 'scope', [50, 26, 188, 112]),
                cell([51.5, 0.5, 61.5, 10.5], 'scope', [241, 12, 301, 73])]
    # Xbox labels fill the stock glyph bounds, not the padded sprite cells.
    # Keep the independently packed MCC sheet in its own coordinate system.
    rectangles = ([[4, 4, 100, 64], [4, 76, 148, 140], [110, 2, 150, 42]] if kind == 'mcc' else
                  [[4.125, 4.875, 26.75, 19.0], [5.25, 28.75, 38.5, 43.125], [35.5, 3.5, 45.5, 13.5]])
    return [cell(rect, 'scope', crop) for rect, crop in zip(rectangles,
        [[50, 26, 188, 112], [50, 168, 247, 253], [241, 12, 301, 73]])]


def write_asset(entry):
    Image.fromarray(hud_assets.build_asset(entry, {})).save(ASSETS / (entry['name']+'.png'), optimize=True)


def rebuild():
    layout = json.loads(hud_assets.LAYOUT.read_text())
    for entry in layout['assets']:
        if entry['name'] == 'hud_reticles__7':
            entry['cells'] = [cell([0, 0, 64, 64], 'rocket', [92, 92, 1188, 1188])]
        elif entry['name'] == 'sniper__hud_reticles_scope__0':
            entry['cells'] = scope_cells('xbox')
        else:
            continue
        entry['correction'] = True
        write_asset(entry)
    hud_assets.LAYOUT.write_text(json.dumps(layout, indent=1)+'\n')
    ce = json.loads((ASSETS / 'ce_layout.json').read_text())
    entry = dict(name='ce_sniper_scope', tag=r'ui\hud\bitmaps\sniper\hud_reticles_scope', bitmap=0,
                 width=64, height=64, scale=8, format='a8r8g8b8', ce_only=True,
                 size=16384, crc=3359213591, cells=scope_cells('ce'),
                 sprites=[{'sequence': 0}, {'sequence': 1}])
    ce['assets'] = [e for e in ce['assets'] if e['name'] != entry['name']] + [entry]
    (ASSETS / 'ce_layout.json').write_text(json.dumps(ce, indent=1)+'\n')
    listing = ASSETS / 'custom_edition.json'
    compatible = json.loads(listing.read_text())
    compatible['assets'] = [e for e in compatible['assets'] if e['tag'] != entry['tag']] + [
        {key: entry[key] for key in ('tag', 'bitmap', 'width', 'height', 'size', 'crc')} |
        {'replacement': entry['name']}]
    listing.write_text('{\n' + ',\n'.join(' ' + json.dumps(k) + ': ' +
        ('[\n' + ',\n'.join('  '+json.dumps(e) for e in v) + '\n ]' if isinstance(v, list) else json.dumps(v))
        for k, v in compatible.items()) + '\n}\n')
    write_asset(entry)
    mcc = {'assets': [dict(name='mcc_rocket_reticle', tag=r'mcc\hud\rocket_reticle', bitmap=0,
        width=256, height=256, scale=2, crc=0, cells=[cell([0, 0, 256, 256], 'rocket', [92, 92, 1188, 1188])]),
        dict(name='mcc_sniper_scope', tag=r'mcc\hud\sniper_scope', bitmap=0, width=256, height=256, scale=2, crc=0,
             cells=scope_cells('mcc'))]}
    (ASSETS / 'mcc_layout.json').write_text(json.dumps(mcc, indent=1)+'\n')
    for entry in mcc['assets']:
        write_asset(entry)
    for entry in json.loads(button_assets.LIST.read_text())['assets']:
        if entry['name'] in {f'classic_{letter}__0' for letter in 'abxy'}:
            image = button_assets.bleed(button_assets.render(button_assets.classic(
                entry['svg'], entry['classic'], entry.get('letter', (0, 0))), entry['scale']))
            Image.fromarray(image).save(button_assets.ASSETS / (entry['name']+'.png'), optimize=True)


if __name__ == '__main__':
    rebuild()
