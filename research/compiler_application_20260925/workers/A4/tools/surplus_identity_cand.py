"""scratch/tools/surplus_identity.py with an explicit candidate object and optional split object.
    python surplus_identity_cand.py <unit> [ours.obj] [split.obj]"""
import glob, os, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
sys.path.insert(0, '.')
from tools import coff_compare as cc  # noqa: E402
unit = sys.argv[1]
ours = sys.argv[2] if len(sys.argv) > 2 else 'build/base/%s.obj' % unit
split = sys.argv[3] if len(sys.argv) > 3 else 'build/split/%s.obj' % unit
split_defs = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            split_defs.setdefault(s['name'], []).append((p, o, s))
t = cc.load(open(split, 'rb').read())
tdef = {s['name'] for s in t['symbols'] if s['section'] > 0}
o = cc.load(open(ours, 'rb').read())
bad = n = 0
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
        eq = cc.section_infos_equal(cc.section_info_by_number(po, ps['section']), cc.section_info_by_number(o, s['section']))
        if not eq:
            bad += 1
        print('  %-30s vs %-40s %s' % (s['name'], os.path.relpath(p, 'build/split'), 'IDENTICAL' if eq else 'DIFFERENT'))
print('%s: %d candidate-only code COMDATs, %d not identical  [ours=%s]' % (unit, n, bad, ours))
