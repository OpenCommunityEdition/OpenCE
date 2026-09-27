"""E1 helper: gate.py's per-function verdicts against an ALTERNATE split root (read-only).

    python -B sgate.py <unit> <ours.obj> <split_root>

Same test as tools/campaign/gate.py (fn_syms + coff_compare.section_infos_equal), but the January target object is
<split_root>/<unit>.obj (e.g. a scratch csplit regenerated from a proposed symbols.json)."""
import sys
sys.path.insert(0, 'tools')
import coff_compare as cc  # noqa: E402

unit, ours_path, root = sys.argv[1:4]
target = cc.load(open('%s/%s.obj' % (root, unit), 'rb').read())
ours = cc.load(open(ours_path, 'rb').read())


def fn_syms(o):
    secs = o['sections']
    out = {}
    for s in o['symbols']:
        if (s['type'] == 0x20 and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            out.setdefault(s['name'], s)
    return out


tsyms = fn_syms(target)
osyms = fn_syms(ours)
ne = nr = nu = 0
for name in sorted(tsyms):
    ti = cc.section_info(target, name)
    if name not in osyms:
        nu += 1
        print('UNWRITTEN %5d  %s' % (ti['size'], name))
        continue
    oi = cc.section_info(ours, name)
    if cc.section_infos_equal(ti, oi):
        ne += 1
        print('EXACT     %5d  %s' % (ti['size'], name))
    else:
        nr += 1
        print('residual  %5d  %s' % (ti['size'], name))
print('== exact %d  residual %d  unwritten %d  (of %d listed)' % (ne, nr, nu, len(tsyms)))
