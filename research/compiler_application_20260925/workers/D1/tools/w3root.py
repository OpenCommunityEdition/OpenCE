"""D1 /W3 warning census on ALTERNATE source roots (header-change consumer sweeps).

    python -B w3root.py <units.txt> <rootA> <rootB> [--level /W3] [--show]

Each unit's <root>/<unit>.c is compiled with its build.ninja flags re-rooted (as W1's revgate.py does) plus
<level> /Zs (syntax only, no object). Warnings are normalised by stripping the root prefix and line numbers, then
compared per unit as multisets: ADDED/REMOVED texts are printed. Writes nothing.
"""
import collections
import os
import re
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(WT, r'research\compiler_application_20260925\workers\W1\tools'))
from flags import unit_flags  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith('--')]
level = '/W3'
for i, a in enumerate(sys.argv):
    if a == '--level':
        level = sys.argv[i + 1]
        args.remove(level)
show = '--show' in sys.argv
units_file, ra, rb = args[0], os.path.abspath(args[1]), os.path.abspath(args[2])
units = [u.strip() for u in open(units_file) if u.strip() and not u.startswith('#')]
cl = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')


def run(root, unit):
    toks = []
    for t in unit_flags(unit, os.path.join(WT, 'build.ninja')):
        if t == '/c' or re.match(r'^/W[0-4]$', t):
            continue
        if t.startswith('/I'):
            p = t[2:]
            toks.append('/I' + (os.path.join(WT, p) if p.startswith('xbox') else os.path.join(root, p)))
        else:
            toks.append(t)
    src = os.path.join(root, unit + '.c')
    cmd = [cl] + toks + [level, '/Zs', '/I' + os.path.join(root, os.path.dirname(unit)), src]
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=root, encoding='latin-1')
    lines = [ln for ln in (r.stdout + r.stderr).splitlines() if ': warning C' in ln or ': error C' in ln]
    norm = []
    for ln in lines:
        ln = re.sub(re.escape(root), '<root>', ln, flags=re.I)
        ln = re.sub(r'\(\d+\) : ', ' : ', ln)
        norm.append(ln)
    return r.returncode, norm


tot = collections.Counter()
for u in units:
    rca, wa = run(ra, u)
    rcb, wb = run(rb, u)
    ca, cb = collections.Counter(wa), collections.Counter(wb)
    added, removed = cb - ca, ca - cb
    tot['units'] += 1
    tot['A'] += len(wa)
    tot['B'] += len(wb)
    tag = 'SAME' if not added and not removed else 'CHANGED'
    print('%-44s rc %d/%d  %s %3d -> %3d  %s' % (u, rca, rcb, level, len(wa), len(wb), tag))
    for w in sorted(added.elements()):
        print('    ADDED   ' + w)
    for w in sorted(removed.elements()):
        print('    REMOVED ' + w)
    if show:
        for w in wb:
            print('    B: ' + w)
print('total %s: %d -> %d over %d units' % (level, tot['A'], tot['B'], tot['units']))
