"""/W3 /Zs warning census for <root>/<unit>.c with the unit's build.ninja flags re-rooted to <root> (like W1 revgate).
    python w3root.py <root> <unit> [--show]"""
import collections, os, re, subprocess, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(WT, r'research\compiler_application_20260925\workers\W1\tools'))
from flags import unit_flags  # noqa: E402
root, unit = os.path.abspath(sys.argv[1]), sys.argv[2]
toks = unit_flags(unit, os.path.join(WT, 'build.ninja'))
out = []
for t in toks:
    if re.match(r'^/W[0-4]$', t):
        continue
    if t.startswith('/I'):
        p = t[2:]
        out.append('/I' + (os.path.join(WT, p) if p.startswith('xbox') else os.path.join(root, p)))
    else:
        out.append(t)
cl = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')
src = os.path.join(root, unit + '.c')
cmd = [cl, '/nologo'] + out + ['/W3', '/Zs', '/I' + os.path.join(root, os.path.dirname(unit)), src]
r = subprocess.run(cmd, capture_output=True, text=True, cwd=root, encoding='latin-1')
txt = r.stdout + r.stderr
c = collections.Counter(re.findall(r'(warning C\d+|error C\d+)', txt))
print('rc', r.returncode, dict(sorted(c.items())))
if '--show' in sys.argv:
    for line in txt.splitlines():
        if 'warning' in line or 'error' in line:
            print('  ' + line.replace(root, '<root>'))
