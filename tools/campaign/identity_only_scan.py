"""List residuals whose ONLY difference is relocation identity.

Size, relocation count and normalized bytes all match, so the reconstruction is
byte-for-byte correct and the gate fails purely because a relocation names a
different symbol. These are closable with symbol work (config/symbols.json
attestation or rename) rather than codegen work - see
tools/campaign/attest_scan.py and the ai_debug/shaders/decals commits.

usage: python tools/campaign/identity_only_scan.py
"""
import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

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
    for sym in target['symbols']:
        n = sym['name']
        if (not n.startswith('_') or sym['section'] <= 0 or sym['storage'] not in (2, 3)
                or sym['value'] != 0 or secs[sym['section'] - 1]['name'] != '.text'
                or sym['section'] in seen):
            continue
        seen.add(sym['section'])
        try:
            ti = cc.section_info(target, n)
            bi = cc.section_info(base, n)
        except Exception:
            continue
        if ti is None or bi is None or cc.section_infos_equal(ti, bi):
            continue
        if (ti['size'] == bi['size']
                and ti['relocation_count'] == bi['relocation_count']
                and ti['normalized_sha256'] == bi['normalized_sha256']):
            # find the differing relocation pairs
            def rl(si):
                out = []
                for r in si.get('relocations', []):
                    tg = r.get('symbolic_target') or r.get('target')
                    out.append((r['address'], str(tg[1]) if len(tg) > 1 else '?',
                                tg[2] if len(tg) > 2 else 0))
                return out
            diffs = [(a, tn, ta, on, oa) for (a, tn, ta), (_, on, oa) in zip(rl(ti), rl(bi))
                     if (tn, ta) != (on, oa)]
            rows.append((u['name'], n, ti['size'], diffs))

print(f'{len(rows)} residuals differ ONLY in relocation identity\n')
for unit, n, size, diffs in sorted(rows, key=lambda r: -r[2]):
    print(f'{unit}::{n}   {size} B   {len(diffs)} differing relocation(s)')
    for a, tn, ta, on, oa in diffs[:8]:
        print(f'    @{a:#06x}  target {tn} + {ta}    ours {on} + {oa}')
