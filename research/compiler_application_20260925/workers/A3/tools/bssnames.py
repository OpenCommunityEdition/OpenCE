"""A3 diagnostic: rename the two snapshot statics in final/pao and report .bss symbol offsets (zero credit)."""
import os
import subprocess
import sys

WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(WT, 'tools'))
import coff_compare as cc  # noqa: E402

A = os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'A3')
base = open(os.path.join(A, 'final', 'pao', 'source', 'ai', 'path_obstacle_avoidance.c'), 'rb').read().decode('latin-1')
DECL = 'static struct obstacle_path debug_path;\r\nstatic struct obstacles debug_obstacles;\r\n'
assert base.count(DECL) == 1
import re


def variant(p, o, swap=False):
    t = base
    if swap:
        t = t.replace(DECL, 'static struct obstacles debug_obstacles;\r\nstatic struct obstacle_path debug_path;\r\n')
    t = re.sub(r'\bdebug_path\b', p, t)
    t = re.sub(r'\bdebug_obstacles\b', o, t)
    return t


for tag, p, o, swap in (('D0', 'debug_path', 'debug_obstacles', False),
                        ('D1', 'obstacle_path_snapshot', 'obstacles_snapshot', False),
                        ('D2', 'failed_path', 'failed_obstacles', False),
                        ('D3', 'debug_path', 'debug_obstacles', True)):
    src = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A3', 'bssdiag_%s.c' % tag)
    obj = src[:-2] + '.obj'
    open(src, 'wb').write(variant(p, o, swap).encode('latin-1'))
    r = subprocess.run([sys.executable, '-B', 'tools/campaign/gate.py', 'source/ai/path_obstacle_avoidance', '--source', src,
                        '--all', '--out', obj], capture_output=True, text=True, cwd=WT)
    last = [l for l in r.stdout.splitlines() if l.startswith('==')]
    o_ = cc.load(open(obj, 'rb').read())
    bss = {}
    for s in o_['symbols']:
        if s['section'] > 0 and o_['sections'][s['section'] - 1]['name'] == '.bss' and not s['name'].startswith('.'):
            bss[s['name']] = s['value']
    print(tag, p, o, 'swap' if swap else '', '|', last[-1] if last else r.stdout[-300:], '|',
          ', '.join('%s@0x%x' % (n, v) for n, v in sorted(bss.items(), key=lambda x: x[1])))
