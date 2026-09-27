"""D1 consumer sweep: compile each unit from one or more alternate roots with W1's revgate.py (unit flags, alternate
root) and compare the objects.

    python -B sweep.py <out_dir> <units.txt> <name>=<root> [<name>=<root> ...] [--jobs N]

For every unit and root it writes <out_dir>/<name>/<unit_flat>.obj and <unit_flat>.gate.txt (revgate --quiet output).
Nothing outside <out_dir> is written. The comparison is done by objcmp.py / keyed_diff.py afterwards.
"""
import concurrent.futures
import os
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
REVGATE = os.path.join(WT, r'research\compiler_application_20260925\workers\W1\tools\revgate.py')

args = [a for a in sys.argv[1:] if not a.startswith('--jobs')]
jobs = 4
for a in sys.argv[1:]:
    if a.startswith('--jobs'):
        jobs = int(a.split('=')[1])
out_dir, units_file = args[0], args[1]
roots = [tuple(x.split('=', 1)) for x in args[2:]]
units = [u.strip() for u in open(units_file) if u.strip() and not u.startswith('#')]


def one(name, root, unit):
    flat = unit.replace('/', '__')
    d = os.path.join(out_dir, name)
    os.makedirs(d, exist_ok=True)
    obj = os.path.join(d, flat + '.obj')
    r = subprocess.run([sys.executable, '-B', REVGATE, root, unit, '--out', obj, '--quiet'], capture_output=True,
                       text=True, cwd=WT, encoding='latin-1')
    txt = r.stdout + r.stderr
    open(os.path.join(d, flat + '.gate.txt'), 'w', encoding='latin-1').write(txt)
    last = [ln for ln in txt.splitlines() if ln.startswith('==')]
    return name, unit, (last[-1] if last else 'FAILED rc=%d %s' % (r.returncode, txt[-300:]))


with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
    futs = [ex.submit(one, n, r, u) for u in units for n, r in roots]
    for f in concurrent.futures.as_completed(futs):
        n, u, s = f.result()
        print('%-8s %-50s %s' % (n, u, s), flush=True)
