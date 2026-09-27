"""Scan every object under a root for DIR32 relocations whose resolved image address lands in the
rasterizer_lights .bss block (results .. count), using config/symbols.json addresses.

Resolution: a relocation's target symbol is either external/undefined (resolved by name in symbols.json)
or defined in one of the object's own sections (resolved by the named symbol in that section with a
symbols.json address). The inline addend is added. Only DIR32 (type 6) can address data absolutely.
"""
import sys, struct, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

syms = json.load(open('config/symbols.json'))
addr = {}
for e in syms:
    addr.setdefault(e['name'], e['file_offset'])
RES = addr['_local_lens_flare_occlusion_test_results']
CNT = addr['_local_lens_flare_count']
LO, HI = RES, CNT + 4
root = sys.argv[1]
hits = []
nobj = nrel = 0
for dp, dn, fns in os.walk(root):
    for f in fns:
        if not f.endswith('.obj'):
            continue
        p = os.path.join(dp, f)
        try:
            obj = cc.load(p)
        except Exception as ex:
            print('LOADFAIL', p, ex)
            continue
        nobj += 1
        data = obj['data']
        for s in obj['sections']:
            if not s['reloc_count'] or (s['flags'] & 0x80):
                continue
            raw = cc._section_bytes(obj, s)
            for i in range(s['reloc_count']):
                off = s['reloc'] + i * 10
                va, si, typ = struct.unpack_from('<LLH', data, off)
                if typ != 6:
                    continue
                nrel += 1
                t = obj['by_index'][si]
                add = struct.unpack_from('<l', raw, va)[0]
                if t['section'] > 0:
                    base = addr.get(t['name'])
                    if base is None:
                        cands = [y for y in obj['symbols']
                                 if y['section'] == t['section'] and y['name'] in addr]
                        if not cands:
                            continue
                        y = min(cands, key=lambda y: y['value'])
                        base = addr[y['name']] - y['value'] + t['value']
                else:
                    base = addr.get(t['name'])
                    if base is None:
                        continue
                a = base + add
                if LO <= a < HI:
                    rel = os.path.relpath(p, root).replace(os.sep, '/')
                    hits.append((rel, s['name'], va, t['name'], add, a))
print('objects scanned %d, DIR32 relocations %d, block [0x%X, 0x%X)' % (nobj, nrel, LO, HI))
for h in sorted(set(hits)):
    print('%s %s +0x%X -> %s%+d = 0x%X' % h)
