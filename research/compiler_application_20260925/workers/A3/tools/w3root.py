"""A3: /W3 /Zs warning census for a unit compiled from an ALTERNATE source root (headers re-rooted).

    python -B research/compiler_application_20260925/workers/A3/tools/w3root.py <root> <unit> [--src FILE] [--show]

Same flag extraction as W2's w3.py; /Isource... flags are re-rooted to <root>, xbox/include stays the worktree's.
/Zs = syntax check only (no object).
"""
import argparse
import collections
import os
import re
import subprocess

WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
ap = argparse.ArgumentParser()
ap.add_argument('root')
ap.add_argument('unit')
ap.add_argument('--src')
ap.add_argument('--show', action='store_true')
a = ap.parse_args()
root = os.path.abspath(a.root)
unit = a.unit.replace('\\', '/')
bn = open(os.path.join(WT, 'build.ninja')).read()
key = 'build\\base\\' + unit.replace('/', '\\').replace(' ', '$ ') + '.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('\nbuild ', j)
cf = re.sub(r'\s+', ' ', bn[j + len('cflags = '):k].replace('$\n', ' ').replace('$\r\n', ' ')).strip()
toks = re.findall(r'/I"[^"]+"|\S+', cf)
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]
toks = [t for t in toks if not re.match(r'^/W[0-4]$', t)]
out = []
for t in toks:
    if t.startswith('/I'):
        p = t[2:]
        out.append('/I' + (os.path.join(WT, p) if p.startswith('xbox') else os.path.join(root, p)))
    else:
        out.append(t)
src = os.path.abspath(a.src) if a.src else os.path.join(root, unit + '.c')
cl = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')
cmd = [cl, '/nologo', '/c'] + out + ['/W3', '/Zs', '/I' + os.path.join(root, os.path.dirname(unit)), src]
r = subprocess.run(cmd, capture_output=True, text=True, cwd=root, encoding='latin-1')
txt = r.stdout + r.stderr
c = collections.Counter(re.findall(r'(warning C\d+|error C\d+)', txt))
print('rc', r.returncode, dict(sorted(c.items())))
if a.show:
    for line in txt.splitlines():
        if 'warning' in line or 'error' in line:
            print(line.replace(root, '<root>'))
