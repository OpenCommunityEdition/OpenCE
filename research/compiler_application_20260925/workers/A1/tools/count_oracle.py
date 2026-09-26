"""LAB ONLY (never lands): K dummy enum constants appended inside the TU's MULTIPLAYER_MAXIMUM_PLAYERS enum,
compiled from a copy of <root> for each K; prints which January functions go residual. Measures the
declared-name count band of game_engine.c (C1 number mod-64 law) so the margin of a count-sensitive genuine
edit is known. The copy lives next to <root> as <root>_lab and is deleted afterwards.

    python count_oracle.py <root> <K...>
"""
import os
import shutil
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
REVGATE = os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools', 'revgate.py')
CRLF = '\r\n'

root = os.path.abspath(sys.argv[1])
ks = [int(k) for k in sys.argv[2:]]
lab = root + '_lab'
assert not os.path.exists(os.path.join(lab, 'xbox'))
if os.path.exists(lab):
    shutil.rmtree(lab)
os.makedirs(lab)
shutil.copytree(os.path.join(root, 'source'), os.path.join(lab, 'source'))  # never copy the xbox junction
ge = os.path.join(lab, 'source', 'game', 'game_engine.c')
orig = open(ge, 'rb').read().decode('latin-1')
anchor = 'MULTIPLAYER_MAXIMUM_PLAYERS = 16,' + CRLF + '};' + CRLF
assert orig.count(anchor) == 1
for k in ks:
    names = ''.join('\tlab_count_%d,' % i + CRLF for i in range(k))
    new_anchor = 'MULTIPLAYER_MAXIMUM_PLAYERS = 16,' + CRLF + names + '};' + CRLF
    open(ge, 'wb').write(orig.replace(anchor, new_anchor).encode('latin-1'))
    obj = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', 'lab_count_%d.obj' % k)
    r = subprocess.run([sys.executable, '-B', REVGATE, lab, 'source/game/game_engine', '--out', obj, '--quiet'],
                       capture_output=True, text=True, cwd=WT)
    res = [line.split()[1] for line in r.stdout.splitlines() if line.startswith('residual')]
    tail = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr[-300:]
    print('K=%3d  %s  %s' % (k, tail, ' '.join(res)))
    if os.path.exists(obj):
        os.remove(obj)
shutil.rmtree(lab)
