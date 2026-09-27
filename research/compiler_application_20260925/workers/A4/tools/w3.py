"""A4 copy of W2 helper: compile a source copy with the unit's real flags plus /W3 /Zs and print the warning census.

    python -B research/compiler_application_20260925/workers/W2/w3.py <unit> <source.c> [--show]

Mirrors tools/campaign/gate.py's flag extraction (build.ninja cflags, /I<unit dir>). /Zs = syntax check only,
so no object is written. The scratch source is written under scratch/campaign/workers/A4/.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

ap = argparse.ArgumentParser()
ap.add_argument('unit')
ap.add_argument('source')
ap.add_argument('--show', action='store_true')
a = ap.parse_args()
unit = a.unit.replace('\\', '/')
bn = open('build.ninja').read()
key = 'build\\base\\' + unit.replace('/', '\\').replace(' ', '$ ') + '.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('\nbuild ', j)
cf = bn[j + len('cflags = '):k].replace('$\n', ' ').replace('$\r\n', ' ')
cf = re.sub(r'\s+', ' ', cf).strip()
toks = re.findall(r'/I"[^"]+"|\S+', cf)
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]
toks = [t for t in toks if not re.match(r'^/W[0-4]$', t)]
code = open(a.source, encoding='latin-1').read()
os.makedirs('scratch/campaign/workers/A4', exist_ok=True)
scratch_src = 'scratch/campaign/workers/A4/_w3_%d.c' % os.getpid()
open(scratch_src, 'w', encoding='latin-1', newline='\n').write(code)
cl = os.environ.get('HALO_CL') or os.path.abspath(os.path.join('xbox', 'bin', 'vc7', 'CL.Exe'))
cmd = [cl, '/nologo', '/c'] + toks + ['/W3', '/Zs', '/I' + os.path.dirname(unit + '.c'), scratch_src]
r = subprocess.run(cmd, capture_output=True, text=True)
os.remove(scratch_src)
out = r.stdout + r.stderr
c = collections.Counter(re.findall(r'(warning C\d+|error C\d+)', out))
print('rc', r.returncode, dict(sorted(c.items())))
if a.show:
    for line in out.splitlines():
        if 'warning' in line or 'error' in line:
            print(re.sub(r'^.*?_w3_\d+\.c', '<src>', line))
