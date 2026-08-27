# -*- coding: utf-8 -*-
"""Generate dbg32.exe config for a live C2.Dll debug of any repro-lab compile.

Self-contained: derives the unit's cflags straight from build.ninja (the same
way tools/campaign/lab_compile.py does), so it does not depend on the old
ad/gate harness in a temp directory.

Usage (run from the worktree root):
  python tools/c2dbg32/gen_config_lab.py <repro.c> <c2 VA> [more VAs...]

Writes, next to dbg32.exe (tools/c2dbg32/):
  dbg_cmd.u16  UTF-16LE CL.Exe command line, null-terminated
  dbg_dir.u16  UTF-16LE working directory, null-terminated
  dbg_bp.txt   breakpoint VAs (C2.Dll loads at its fixed base 0x10700000)
and removes any stale dbg_gate.txt (a single-function TU needs no gating).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
CL = os.path.join(ROOT, 'xbox', 'bin', 'vc7', 'CL.Exe')


def cflags_for(unit):
    """unit is e.g. 'source/units/units' -- pull its cflags out of build.ninja."""
    bn = re.sub(r'\s*\$\r?\n\s*', ' ', open(os.path.join(ROOT, 'build.ninja')).read())
    key = 'build build' + chr(92) + 'base' + chr(92) + unit.replace('/', chr(92)) + '.obj:'
    i = bn.index(key)
    j = bn.index('cflags = ', i)
    k = bn.index('\nbuild ', j)
    cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
    return [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t
            for t in re.findall(r'/I"[^"]+"|\S+', cf)]


def main(src, bp_vas, unit='source/units/units'):
    argv = ([CL] + cflags_for(unit) +
            ['/I' + os.path.join(ROOT, 'source', 'units'),
             '/Fo' + os.path.join(HERE, 'dbg_lab.obj'),
             os.path.join(ROOT, src)])
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    open(os.path.join(HERE, 'dbg_cmd.u16'), 'wb').write((cmdline + '\0').encode('utf-16-le'))
    open(os.path.join(HERE, 'dbg_dir.u16'), 'wb').write((ROOT + '\0').encode('utf-16-le'))
    open(os.path.join(HERE, 'dbg_bp.txt'), 'wb').write((' '.join(bp_vas) + '\n').encode())
    gate_file = os.path.join(HERE, 'dbg_gate.txt')
    if os.path.exists(gate_file):
        os.remove(gate_file)
    print('wrote config for %s; %d breakpoints: %s' % (src, len(bp_vas), ' '.join(bp_vas)))


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2:])
