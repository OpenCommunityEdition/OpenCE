"""A3: compile consumer units from an alternate source root with W1's revgate.py and keyed-diff each object
against build/base (and optionally against a second root's object). Read-only for the worktree.

    python -B research/compiler_application_20260925/workers/A3/tools/hdrsweep.py <root> <tag> unit [unit ...]

Writes objects to scratch/campaign/workers/A3/sweep/<tag>/<unit_basename>.obj and prints, per unit, the revgate
'== exact' line and the keyed_diff summary vs build/base.
"""
import os
import subprocess
import sys

WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
REVGATE = os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools', 'revgate.py')
KEYED = os.path.join(WT, 'scratch', 'campaign', 'keyed_diff.py')

root, tag = sys.argv[1], sys.argv[2]
units = sys.argv[3:]
outdir = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A3', 'sweep', tag)
os.makedirs(outdir, exist_ok=True)
bad = 0
for u in units:
    obj = os.path.join(outdir, os.path.basename(u) + '.obj')
    if os.path.exists(obj):
        os.remove(obj)
    r = subprocess.run([sys.executable, '-B', REVGATE, root, u, '--out', obj, '--quiet'],
                       capture_output=True, text=True, cwd=WT)
    gate = [l for l in (r.stdout + r.stderr).splitlines() if l.startswith('==') or 'FAILED' in l or l.startswith('residual')]
    k = subprocess.run([sys.executable, '-B', KEYED, os.path.join('build', 'base', u + '.obj'), obj, '--vs-january', u],
                       capture_output=True, text=True, cwd=WT)
    ks = [l for l in k.stdout.splitlines() if l.startswith('summary') or l.startswith('CHANGED') or l.startswith('ADDED') or l.startswith('REMOVED')]
    summ = ks[-1] if ks else '(no keyed summary) ' + k.stderr[-200:]
    if 'summary: 0 changed, 0 added, 0 removed' not in summ:
        bad += 1
    print('%-40s %s | %s' % (u, ' ; '.join(gate)[:90], summ))
    for l in ks[:-1][:8]:
        print('      ' + l)
print('UNITS %d  NOT-IDENTICAL %d' % (len(units), bad))
