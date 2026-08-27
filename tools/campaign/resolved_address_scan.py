"""Find residuals whose relocations resolve to the SAME image address on both
sides but are named through different (symbol, addend) pairs.

This happens when the compiler folds a constant bias into an address - e.g.
`&table[i - 32]` becomes a reference to `table - 256` - and csplit, finding
nothing named at the resulting address, reaches it from the nearest preceding
public symbol instead. The bytes are identical and the linked image would be
identical; only the naming differs.

Resolution uses config/symbols.json to map symbol names to file offsets, so a
pair can only be judged when BOTH names are known there.

usage: python tools/campaign/resolved_address_scan.py
"""
import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

sym = {}
_d = json.load(open('config/symbols.json'))
for e in (_d if isinstance(_d, list) else _d['symbols']):
    if isinstance(e, dict):
        sym.setdefault(e['name'], e['file_offset'])


def resolve(name, addend):
    base = sym.get(name)
    return None if base is None else base + addend


def rl(si):
    out = []
    for r in si.get('relocations', []):
        tg = r.get('symbolic_target') or r.get('target')
        out.append((r['address'], str(tg[1]) if len(tg) > 1 else '?',
                    tg[2] if len(tg) > 2 else 0))
    return out


d = json.load(open('objdiff.json'))
rows = []
for u in d['units']:
    tp, bp = u.get('target_path'), u.get('base_path')
    if not tp or not bp or not os.path.exists(tp) or not os.path.exists(bp):
        continue
    target = cc.load(open(tp, 'rb').read())
    base = cc.load(open(bp, 'rb').read())
    secs = target['sections']
    seen = set()
    for s in target['symbols']:
        n = s['name']
        if (not n.startswith('_') or s['section'] <= 0 or s['storage'] not in (2, 3)
                or s['value'] != 0 or secs[s['section'] - 1]['name'] != '.text'
                or s['section'] in seen):
            continue
        seen.add(s['section'])
        try:
            ti = cc.section_info(target, n)
            bi = cc.section_info(base, n)
        except Exception:
            continue
        if ti is None or bi is None or cc.section_infos_equal(ti, bi):
            continue
        if (ti['size'] != bi['size'] or ti['relocation_count'] != bi['relocation_count']
                or ti['normalized_sha256'] != bi['normalized_sha256']):
            continue
        same, diff = [], []
        for (a, tn, ta), (_, on, oa) in zip(rl(ti), rl(bi)):
            if (tn, ta) == (on, oa):
                continue
            rt, ro = resolve(tn, ta), resolve(on, oa)
            (same if (rt is not None and rt == ro) else diff).append((a, tn, ta, on, oa, rt, ro))
        if same and not diff:
            rows.append((u['name'], n, ti['size'], same))

print(f'{len(rows)} residuals are byte-identical AND every differing relocation '
      f'resolves to the same image address\n')
for unit, n, size, same in rows:
    print(f'{unit}::{n}   {size} B')
    for a, tn, ta, on, oa, rt, ro in same:
        print(f'    @{a:#06x}  target {tn}+{ta}  ours {on}+{oa}  -> both {rt}')
