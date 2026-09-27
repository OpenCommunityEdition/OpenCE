"""A3 copy of scratch/tools/pdb_storage.py accepting <unit>=<obj>."""
"""Check every defined symbol of a unit against cachebeta.pdb PUBLIC membership.

    python scratch/tools/pdb_storage.py <unit> [<unit> ...]

January truth: a symbol listed as a PDB public was external; a symbol absent
from the publics was file-static (or discarded). For each symbol defined in
January's split object (build/split) and ours (build/base) it prints:
  split storage, ours storage, PDB public?  and flags every disagreement.
"""
import re
import sys

sys.path.insert(0, '.')
from tools import coff_compare as cc  # noqa: E402

pub = set()
for line in open('scratch/tools/cachebeta_publics.txt', encoding='latin-1'):
    m = re.match(r'PublicSymbol: \[[0-9A-F]+\]\[[0-9A-F]+:[0-9A-F]+\] (\S+?)\(', line)
    if m:
        pub.add(m.group(1))


def defs(path):
    o = cc.load(open(path, 'rb').read())
    return {s['name']: s['storage'] for s in o['symbols']
            if s['section'] > 0 and s['storage'] in (2, 3)
            and not s['name'].startswith('.') and not s['name'].startswith('$')}


for arg in sys.argv[1:]:
    unit, _, objpath = arg.partition('=')
    objpath = objpath or 'build/base/%s.obj' % unit
    t = defs('build/split/%s.obj' % unit)
    o = defs(objpath)
    bad = []
    for name in sorted(set(t) | set(o)):
        if name.startswith('??_C@') or name.startswith('__real@') or name.startswith('__xmm@'):
            continue
        ts, os_ = t.get(name), o.get(name)
        truth = 2 if name in pub else 3
        if ts is not None and (ts != truth or (os_ is not None and os_ != truth)):
            bad.append('%-50s split %s ours %s PDB-public %s' % (name, ts, os_, name in pub))
        elif ts is None and os_ is not None and os_ == 2 and name not in pub:
            bad.append('%-50s split -  ours %s PDB-public %s (candidate-only external)' % (name, os_, name in pub))
    print('%s: %d split symbols, %d disagreements with PDB publics' % (unit, len(t), len(bad)))
    for b in bad:
        print('   ' + b)
