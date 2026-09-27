"""scratch/tools/pdb_storage.py with an explicit candidate object and optional split object.
    python pdb_storage_cand.py <unit> [ours.obj] [split.obj]
Same truth rule: PDB public => external(2); absent => static(3)."""
import os, re, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
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
unit = sys.argv[1]
ours = sys.argv[2] if len(sys.argv) > 2 else 'build/base/%s.obj' % unit
split = sys.argv[3] if len(sys.argv) > 3 else 'build/split/%s.obj' % unit
t, o = defs(split), defs(ours)
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
print('%s: %d split symbols, %d disagreements with PDB publics  [ours=%s split=%s]' % (unit, len(t), len(bad), ours, split))
for b in bad:
    print('   ' + b)
