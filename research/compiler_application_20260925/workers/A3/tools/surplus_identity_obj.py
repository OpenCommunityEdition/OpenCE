"""A3 copy: accepts <unit>=<obj> to compare a candidate object instead of build/base.
For every candidate-only external .text COMDAT a unit defines, compare it with
January's selected copy (the split object that defines it): section_infos_equal.

    python scratch/tools/surplus_identity.py <unit> [<unit> ...]
"""
import glob
import os
import sys

sys.path.insert(0, '.')
from tools import coff_compare as cc  # noqa: E402

split_defs = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            split_defs.setdefault(s['name'], []).append((p, o, s))

for arg in sys.argv[1:]:
    unit, _, objpath = arg.partition('=')
    objpath = objpath or 'build/base/%s.obj' % unit
    t = cc.load(open('build/split/%s.obj' % unit, 'rb').read())
    tdef = {s['name'] for s in t['symbols'] if s['section'] > 0}
    o = cc.load(open(objpath, 'rb').read())
    bad = 0
    n = 0
    for s in o['symbols']:
        if s['section'] <= 0 or s['storage'] != 2 or s['name'] in tdef:
            continue
        sec = o['sections'][s['section'] - 1]
        if sec['name'] != '.text':
            continue
        n += 1
        provs = split_defs.get(s['name'], [])
        if not provs:
            print('  %-30s NO January provider' % s['name'])
            bad += 1
            continue
        for p, po, ps in provs:
            eq = cc.section_infos_equal(cc.section_info_by_number(po, ps['section']),
                                        cc.section_info_by_number(o, s['section']))
            if not eq:
                bad += 1
            print('  %-30s vs %-40s %s' % (s['name'], os.path.relpath(p, 'build/split'), 'IDENTICAL' if eq else 'DIFFERENT'))
    print('%s: %d candidate-only code COMDATs, %d not identical' % (unit, n, bad))
