"""A3: list January split objects (build/split) that DEFINE or REFERENCE given symbols.

    python -B research/compiler_application_20260925/workers/A3/tools/symscan.py _sym1 _sym2 ... [--root build/split]

Read-only. A symbol with section > 0 is a definition; section == 0 (undefined) is a reference.
"""
import glob
import os
import sys

WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(WT, 'tools'))
import coff_compare as cc  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith('--')]
root = 'build/split'
for a in sys.argv[1:]:
    if a.startswith('--root='):
        root = a.split('=', 1)[1]
names = set(args)
defs = {n: [] for n in names}
refs = {n: [] for n in names}
for p in glob.glob(os.path.join(WT, root, '**', '*.obj'), recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    rel = os.path.relpath(p, os.path.join(WT, root)).replace(os.sep, '/')
    for s in o['symbols']:
        if s['name'] in names:
            if s['section'] > 0:
                defs[s['name']].append('%s(storage %d)' % (rel, s['storage']))
            elif s['section'] == 0:
                refs[s['name']].append(rel)
for n in sorted(names):
    print('%s  DEF(%d): %s' % (n, len(set(defs[n])), ', '.join(sorted(set(defs[n])))))
    print('%s  REF(%d): %s' % (n, len(set(refs[n])), ', '.join(sorted(set(refs[n])))))
