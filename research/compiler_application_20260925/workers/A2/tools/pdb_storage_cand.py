"""A2 copy of scratch/tools/pdb_storage.py for an arbitrary candidate object and split root (same logic).

    python -B pdb_storage_cand.py <unit> <candidate.obj> [<split_root>]

January truth: a symbol listed as a cachebeta.pdb PUBLIC was external; absent from the publics = file-static.
Flags every split/candidate storage disagreement with that truth, and candidate-only externals that are not public.
"""
import os
import re
import sys

ROOT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, ROOT)
from tools import coff_compare as cc  # noqa: E402

pub = set()
for line in open(os.path.join(ROOT, 'scratch', 'tools', 'cachebeta_publics.txt'), encoding='latin-1'):
    m = re.match(r'PublicSymbol: \[[0-9A-F]+\]\[[0-9A-F]+:[0-9A-F]+\] (\S+?)\(', line)
    if m:
        pub.add(m.group(1))


def defs(path):
    o = cc.load(open(path, 'rb').read())
    return {s['name']: s['storage'] for s in o['symbols']
            if s['section'] > 0 and s['storage'] in (2, 3)
            and not s['name'].startswith('.') and not s['name'].startswith('$')}


unit, cand = sys.argv[1], sys.argv[2]
split_root = sys.argv[3] if len(sys.argv) > 3 else os.path.join(ROOT, 'build', 'split')
t = defs(os.path.join(split_root, unit + '.obj'))
o = defs(cand)
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
