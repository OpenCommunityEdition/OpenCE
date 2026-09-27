"""Header include closure of a unit compiled from <root> (cl /showIncludes /Zs with the unit's re-rooted flags).
    python incclosure.py <root> <unit> -> prints normalized header paths"""
import os, re, subprocess, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(WT, r'research\compiler_application_20260925\workers\W1\tools'))
from flags import unit_flags
root, unit = os.path.abspath(sys.argv[1]), sys.argv[2]
toks = unit_flags(unit, os.path.join(WT, 'build.ninja'))
out = []
for t in toks:
    if t.startswith('/I'):
        p = t[2:]
        out.append('/I' + (os.path.join(WT, p) if p.startswith('xbox') else os.path.join(root, p)))
    elif t != '/c':
        out.append(t)
cl = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')
cmd = [cl, '/nologo'] + out + ['/Zs', '/showIncludes', '/I' + os.path.join(root, os.path.dirname(unit)), os.path.join(root, unit + '.c')]
r = subprocess.run(cmd, capture_output=True, text=True, cwd=root, encoding='latin-1')
seen = set()
for line in (r.stdout + r.stderr).splitlines():
    m = re.match(r'Note: including file:\s*(.*)$', line)
    if m:
        p = os.path.normcase(os.path.normpath(m.group(1).strip()))
        if p.startswith(os.path.normcase(root)):
            p = 'ROOT' + p[len(os.path.normcase(root)):]
        seen.add(p)
for p in sorted(seen):
    print(p)
