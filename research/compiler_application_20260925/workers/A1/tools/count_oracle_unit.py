"""LAB ONLY (never lands): generic declared-name count oracle for any unit. Inserts `enum { lab_count_0, ... };`
(K constants) right after the line containing <anchor> in <unit>.c of a copy of <root>, runs W1's revgate.py, and
prints the residual functions per K. Used to measure how far a canary TU sits from its count band.

    python count_oracle_unit.py <root> <unit> <anchor-substring> <K...>
"""
import os
import shutil
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
REVGATE = os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools', 'revgate.py')
root, unit, anchor = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
ks = [int(k) for k in sys.argv[4:]]
lab = root + '_labu'
assert not os.path.exists(os.path.join(lab, 'xbox'))
if os.path.exists(lab):
    shutil.rmtree(lab)
os.makedirs(lab)
shutil.copytree(os.path.join(root, 'source'), os.path.join(lab, 'source'))  # never the xbox junction
path = os.path.join(lab, unit.replace('/', os.sep) + '.c')
orig = open(path, 'rb').read().decode('latin-1')
i = orig.index(anchor)
j = orig.index('\n', i) + 1
for k in ks:
    ins = ('enum\r\n{\r\n' + ''.join('\tlab_count_%d,\r\n' % n for n in range(k)) + '};\r\n') if k else ''
    open(path, 'wb').write((orig[:j] + ins + orig[j:]).encode('latin-1'))
    obj = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', '_labu_%d.obj' % k)
    r = subprocess.run([sys.executable, '-B', REVGATE, lab, unit, '--out', obj, '--quiet'], capture_output=True,
                       text=True, cwd=WT)
    res = [line.split()[1] for line in r.stdout.splitlines() if line.startswith('residual')]
    tail = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr[-300:]
    print('K=%3d  %s  %s' % (k, tail, ' '.join(res)))
    if os.path.exists(obj):
        os.remove(obj)
shutil.rmtree(lab)
