import sys, struct
sys.path.insert(0, 'tools')
import coff_compare as cc
path = sys.argv[1]
obj = cc.load(path)
data = obj['data']
bss = [s for s in obj['sections'] if s['flags'] & 0x80]
bssidx = {s['index'] for s in bss}
# function name per section
fn_of = {}
for y in obj['symbols']:
    if y['section'] > 0 and y['value'] == 0 and y['type'] == 0x20:
        fn_of[y['section']] = y['name']
labels = sorted([(y['value'], y['name']) for y in obj['symbols'] if y['section'] in bssidx and not y['name'].startswith('.')])
def owner(o):
    best = None
    for v, n in labels:
        if v <= o: best = (n, o - v)
    return best
for s in obj['sections']:
    if not s['reloc_count'] or s['index'] in bssidx: continue
    raw = cc._section_bytes(obj, s)
    for i in range(s['reloc_count']):
        off = s['reloc'] + i * 10
        va, si, typ = struct.unpack_from('<LLH', data, off)
        t = obj['by_index'][si]
        if t['section'] not in bssidx: continue
        add = struct.unpack_from('<l', raw, va)[0]
        tgt = t['value'] + add
        n, d = owner(tgt)
        print('%-52s +0x%03X type=%2d -> %s%+d (bss 0x%X) [ctx %s]' % (fn_of.get(s['index'], s['name']), va, typ, n, d, tgt, raw[max(0, va-3):va].hex()))
