# -*- coding: utf-8 -*-
"""dbg32 config for the breakable_surfaces cross-product operand-tie probe.

Usage (from the worktree root):
  python tools/c2dbg32/gen_config_cross.py <c2 VA> [more VAs...]
  cd tools/c2dbg32 && ./dbg32.exe
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
CL = os.path.join(ROOT, 'xbox', 'bin', 'vc7', 'CL.Exe')
UNIT = 'source/physics/breakable_surfaces'
SRC = 'scratch/probe_cross.c'


def cflags_for(unit):
    bn = re.sub(r'\s*\$\r?\n\s*', ' ', open(os.path.join(ROOT, 'build.ninja')).read())
    key = 'build build' + chr(92) + 'base' + chr(92) + unit.replace('/', chr(92)) + '.obj:'
    i = bn.index(key)
    j = bn.index('cflags = ', i)
    k = bn.index('\nbuild ', j)
    cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
    return [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t
            for t in re.findall(r'/I"[^"]+"|\S+', cf)]


def main(bp_vas, src=SRC):
    argv = ([CL] + cflags_for(UNIT) +
            ['/Fo' + os.path.join(HERE, 'dbg_cross.obj'),
             os.path.join(ROOT, src.replace('/', os.sep))])
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    open(os.path.join(HERE, 'dbg_cmd.u16'), 'wb').write((cmdline + '\0').encode('utf-16-le'))
    open(os.path.join(HERE, 'dbg_dir.u16'), 'wb').write((ROOT + '\0').encode('utf-16-le'))
    open(os.path.join(HERE, 'dbg_bp.txt'), 'wb').write((' '.join(bp_vas) + '\n').encode())
    for stale in ('dbg_gate.txt', 'dbg_wp.txt', 'dbg_scan.txt'):
        p = os.path.join(HERE, stale)
        if os.path.exists(p):
            os.remove(p)
    print('wrote config; src=%s bps=%s' % (src, ' '.join(bp_vas)))


if __name__ == '__main__':
    main(sys.argv[1:])
