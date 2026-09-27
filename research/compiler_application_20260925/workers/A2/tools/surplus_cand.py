"""A2 lab tool (read-only): scratch/tools/surplus_identity.py for an arbitrary candidate object.

    python -B surplus_cand.py <unit> <candidate.obj> [--split-root DIR]

Same logic as scratch/tools/surplus_identity.py (every candidate-only EXTERNAL .text COMDAT vs January's
selected copy = the split object(s) that define it, coff_compare.section_infos_equal), plus a list of
candidate-only STATIC .text sections (code January's object does not have).
"""
import glob
import os
import sys

ROOT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, ROOT)
from tools import coff_compare as cc  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith('--')]
split_root = os.path.join(ROOT, 'build', 'split')
for a in sys.argv[1:]:
    if a.startswith('--split-root='):
        split_root = a.split('=', 1)[1]
unit, cand = args[0], args[1]

split_defs = {}
for p in glob.glob(os.path.join(ROOT, 'build', 'split', '**', '*.obj'), recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            split_defs.setdefault(s['name'], []).append((p, o, s))

t = cc.load(open(os.path.join(split_root, unit + '.obj'), 'rb').read())
tdef = {s['name'] for s in t['symbols'] if s['section'] > 0}
o = cc.load(open(cand, 'rb').read())
bad = n = 0
statics = []
for s in o['symbols']:
    if s['section'] <= 0 or s['name'] in tdef:
        continue
    sec = o['sections'][s['section'] - 1]
    if sec['name'] != '.text' or s['value'] != 0 or s['name'].startswith('.') or s['name'].startswith('$'):
        continue
    if s['storage'] == 3:
        statics.append((s['name'], sec['size']))
        continue
    if s['storage'] != 2:
        continue
    n += 1
    provs = split_defs.get(s['name'], [])
    if not provs:
        print('  %-34s NO January provider' % s['name'])
        bad += 1
        continue
    for p, po, ps in provs:
        eq = cc.section_infos_equal(cc.section_info_by_number(po, ps['section']),
                                    cc.section_info_by_number(o, s['section']))
        if not eq:
            bad += 1
        print('  %-34s vs %-40s %s' % (s['name'], os.path.relpath(p, os.path.join(ROOT, 'build', 'split')),
                                       'IDENTICAL' if eq else 'DIFFERENT'))
print('%s: %d candidate-only code COMDATs, %d not identical' % (unit, n, bad))
for name, size in statics:
    print('  STATIC candidate-only code %-40s %d' % (name, size))
print('%d candidate-only static code sections' % len(statics))
