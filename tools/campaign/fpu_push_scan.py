"""Find residuals tree-wide that are the float-argument-passing family.

Mechanism (found on hs.c, 2026-08-17): January routes `real` arguments through the
FPU - `fld dword [src]` then `push reg` / `fstp dword ptr [esp]` - where our build
integer-copies them (`mov reg,[src]` / `push reg`). Declaring the local `double`
and letting the implicit conversion happen at the prototyped call reproduces
January's form; see research/hs_volatile_credited_functions_audit.md.

This scans every residual function for the signature: target emits more
`fstp dword ptr [esp]` (or `fstp dword ptr [ebp-N]` following an `fld`) than we do.
Those are candidates for the same lever.

usage: python tools/campaign/fpu_push_scan.py [unit-substring]
"""
import sys, json, os, re
sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
sel = sys.argv[1] if len(sys.argv) > 1 else None


def body(obj, name):
    secs = obj['sections']
    for s in obj['symbols']:
        if (s['name'] == name and s['section'] > 0 and s['value'] == 0
                and secs[s['section'] - 1]['name'] == '.text'):
            data = cc._section_bytes(obj, secs[s['section'] - 1])
            return [f'{i.mnemonic} {i.op_str}' for i in md.disasm(bytes(data), 0)]
    return None


def counts(text):
    j = ' | '.join(text)
    return (j.count('fstp dword ptr [esp]'), j.count('fld dword ptr'))


d = json.load(open('objdiff.json'))
hits = []
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
        tt, ot = body(target, n), body(base, n)
        if not tt or not ot:
            continue
        tpush, tfld = counts(tt)
        opush, ofld = counts(ot)
        if tpush > opush:
            hits.append((tpush - opush, u['name'], n, ti['size'], bi['size'], tpush, opush))

hits.sort(reverse=True)
print(f'{len(hits)} residual functions where January FPU-pushes more floats than we do\n')
print(f'{"delta":>5} {"unit":42s} {"function":40s} {"tgt":>6} {"ours":>6}  fstp[esp] t/o')
for delta, unit, n, ts, bs, tp_, op_ in hits:
    print(f'{delta:5d} {unit:42s} {n:40s} {ts:6d} {bs:6d}  {tp_}/{op_}')
