"""Identity of candidate-only COMDATs vs January's selected copies. usage: surplus_check.py cand.obj NAME..."""
import sys, glob
sys.path.insert(0, '.')
from tools import coff_compare as cc
cand = cc.load(open(sys.argv[1], 'rb').read())
names = sys.argv[2:]
provs = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    for s in o['symbols']:
        if s['name'] in names and s['section'] > 0 and s['storage'] == 2:
            provs.setdefault(s['name'], []).append((p, o))
for n in names:
    ci = cc.section_info(cand, n)
    if not provs.get(n):
        print('%-22s NO January provider' % n)
    for p, o in provs.get(n, []):
        ti = cc.section_info(o, n)
        print('%-22s provider %-45s identical=%s' % (n, p.replace(chr(92), '/'), cc.section_infos_equal(ti, ci)))
