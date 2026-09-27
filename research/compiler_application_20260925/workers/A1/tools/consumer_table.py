"""Per-consumer A/B table for the header patches: for every build unit that includes a changed header (transitive
/showIncludes census from consumers.py JSON, plus a plain `grep -rl '#include "<header>"'` over the root),
run W1's revgate.py (gates vs January build/split) on the HEAD root and on the candidate root, and attach the
keyed per-section diff vs build/base from a sweep.py JSON of the candidate.

    python consumer_table.py <head_root> <cand_root> <consumers.json> <sweep.json> <out.md>
"""
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
REVGATE = os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools', 'revgate.py')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sweep import all_units  # noqa: E402

head_root, cand_root, cons_json, sweep_json, out_md = sys.argv[1:6]
cons = json.load(open(cons_json))
units = dict(all_units())
by_unit = {}
for h, users in cons.items():
    for u in users:
        by_unit.setdefault(u, set()).add(os.path.basename(h))
# grep-direct consumers (the brief's list), restricted to build units
for h in list(cons):
    base = os.path.basename(h)
    pat = re.compile(r'#include\s+"[^"]*' + re.escape(base) + '"')
    for dirpath, _, files in os.walk(os.path.join(cand_root, 'source')):
        for f in files:
            if not f.endswith(('.c', '.cpp')):
                continue
            p = os.path.join(dirpath, f)
            if pat.search(open(p, encoding='latin-1').read()):
                u = os.path.relpath(p, cand_root).replace(os.sep, '/').rsplit('.', 1)[0]
                if u in units:
                    by_unit.setdefault(u, set()).add(base + '(grep)')
sweep = {r['unit']: r for r in json.load(open(sweep_json))}


def gate(root, unit):
    obj = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', '_ct', os.path.basename(root) + '__' +
                       unit.replace('/', '__') + '.obj')
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    r = subprocess.run([sys.executable, '-B', REVGATE, root, unit, '--out', obj, '--quiet'], capture_output=True,
                       text=True, cwd=WT)
    line = [l for l in r.stdout.splitlines() if l.startswith('==')]
    res = [l.split()[1] for l in r.stdout.splitlines() if l.startswith('residual')]
    return (line[-1] if line else 'FAILED ' + r.stdout[-200:]), res


rows = []
with cf.ThreadPoolExecutor(max_workers=8) as ex:
    futs = {u: (ex.submit(gate, head_root, u), ex.submit(gate, cand_root, u)) for u in sorted(by_unit)}
    for u in sorted(by_unit):
        (hl, hres), (cl, cres) = futs[u][0].result(), futs[u][1].result()
        s = sweep.get(u, {})
        kd = 'n/a'
        if s:
            kd = 'SAME' if not (s['changed'] or s['added'] or s['removed']) else \
                'changed %d added %d removed %d' % (len(s['changed']), len(s['added']), len(s['removed']))
        hn = re.search(r'exact (\d+)\s+residual (\d+)\s+unwritten (\d+)', hl)
        cn = re.search(r'exact (\d+)\s+residual (\d+)\s+unwritten (\d+)', cl)
        same = (hn and cn and hn.groups() == cn.groups() and hres == cres)
        rows.append((u, ', '.join(sorted(by_unit[u])), hn.groups() if hn else hl, cn.groups() if cn else cl,
                     'identical' if same else 'DIFFERENT', kd))
with open(out_md, 'w') as f:
    f.write('| consumer TU | changed headers it includes | HEAD root revgate exact/residual/unwritten | '
            'candidate revgate | per-function A/B | keyed diff vs build/base |\n|---|---|---|---|---|---|\n')
    for r in rows:
        f.write('| %s | %s | %s | %s | %s | %s |\n' % (r[0], r[1], '/'.join(r[2]) if isinstance(r[2], tuple) else r[2],
                                                     '/'.join(r[3]) if isinstance(r[3], tuple) else r[3], r[4], r[5]))
diff = [r for r in rows if r[4] != 'identical' or r[5] != 'SAME']
print('%d consumer TUs; %d with any difference' % (len(rows), len(diff)))
for r in diff:
    print('  ', r)
