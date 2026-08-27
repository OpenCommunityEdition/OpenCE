"""Find residuals blocked by the naming gap: a static variable that carries no
public symbol in the image, so csplit names its address as `<nearest public
symbol> + N` while our object names the static directly.

Signature, at the same relocation address: target = (symA, addend N != 0) and
ours = (symB, addend 0) with symA != symB. Fixing one means adding a
config/symbols.json entry at that address, after confirming from the source
declaration order and sizes that the identification is real - see
research/naming_gap notes and the ai_debug commit for the standard of proof.

usage: python tools/campaign/attest_scan.py [unit-substring]
"""
import sys, json, os, re
sys.path.insert(0, 'tools')
import coff_compare as cc

sel = sys.argv[1] if len(sys.argv) > 1 else None


def rel_list(si):
    out = []
    for r in si.get('relocations', []):
        tg = r.get('symbolic_target') or r.get('target')
        name = str(tg[1]) if len(tg) > 1 else '?'
        add = tg[2] if len(tg) > 2 else 0
        out.append((r['address'], name, add))
    return out


d = json.load(open('objdiff.json'))
cands = {}
for u in d['units']:
    if sel and sel not in u['name']:
        continue
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
        tr = {a: (nm, ad) for a, nm, ad in rel_list(ti)}
        orl = {a: (nm, ad) for a, nm, ad in rel_list(bi)}
        for a in tr:
            if a not in orl:
                continue
            tn, ta = tr[a]
            on, oa = orl[a]
            # Two precise classes only. Comparing relocations by address across
            # two functions whose code differs produces mostly noise, so an
            # addend-0 name mismatch on its own is NOT a signal.
            #   attest: a static named "<public symbol> + N" (target addend != 0)
            #   rename: csplit's placeholder name for a whole blob it could not
            #           identify, i.e. _bss_/_data_/_code_ + 8 hex digits
            placeholder = re.match(r'^_(bss|data)_[0-9a-f]{8}$', tn) is not None
            if tn != on and oa == 0 and (ta != 0 or placeholder):
                cands.setdefault((tn, ta, on), []).append((u['name'], n))

if not cands:
    print('no naming-gap candidates found')
print(f'{len(cands)} distinct candidates\n')
for (tn, ta, on), where in sorted(cands.items(), key=lambda kv: -len(kv[1])):
    units = sorted(set(w[0] for w in where))
    kind = 'rename' if ta == 0 else 'attest'
    print(f'[{kind}] target {tn} + {ta}   <->   ours {on}')
    print(f'    {len(where)} relocation sites in {len(units)} unit(s): {", ".join(units[:3])}')
    for u, f in sorted(set(where))[:6]:
        print(f'      {u}::{f}')
