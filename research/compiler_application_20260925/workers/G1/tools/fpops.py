"""G1 census helper (read-only): FP ARITHMETIC-count difference between January and ours per function.

usage: python -B fpops.py <unit> <fn> [...] [--obj ours.obj]
Counts x87 arithmetic operations by operation class (add/sub/mul/div, reversed forms folded, pops ignored) plus
fchs/fabs/fsqrt and FP loads of constants, on both sides. Different counts mean a different expression TREE or
arithmetic (e.g. a CSE of a grouped subexpression that one tree has and the other lacks, a folded constant, a
distributed form) - January byte evidence about grouping. Equal counts mean any difference is order/allocation.
"""
import collections
import re
import subprocess
import sys


def ops(unit, fn, obj):
    cmd = ['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--max-lines', '0', '--include-equal']
    if obj:
        cmd += ['--ours-object', obj]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    c = {'T': collections.Counter(), 'O': collections.Counter()}
    for line in out.splitlines():
        m = re.match(r'\s+([TO])\s+[0-9a-f]+\s+(\S+)\s*(.*)$', line)
        if not m:
            continue
        side, mn, rest = m.groups()
        k = None
        mm = re.fullmatch(r'fi?(add|sub|subr|mul|div|divr)p?', mn)
        if mm:
            k = {'subr': 'sub', 'divr': 'div'}.get(mm.group(1), mm.group(1))
        elif mn in ('fchs', 'fabs', 'fsqrt', 'fsin', 'fcos', 'fptan', 'fpatan'):
            k = mn
        elif mn in ('fld', 'fild') and '__real@' in rest:
            k = 'fldK:' + re.search(r'__real@([0-9a-f]+)', rest).group(1)
        elif mn in ('fmul', 'fadd', 'fsub', 'fdiv') and '__real@' in rest:
            k = None
        if k:
            c[side][k] += 1
        if mn.startswith('f') and '__real@' in rest and mn not in ('fld', 'fild'):
            c[side]['K-operand:' + re.search(r'__real@([0-9a-f]+)', rest).group(1)] += 1
    return c


def main():
    args = sys.argv[1:]
    obj = None
    if '--obj' in args:
        i = args.index('--obj')
        obj = args[i + 1]
        del args[i:i + 2]
    unit = args[0]
    for fn in args[1:]:
        c = ops(unit, fn, obj)
        t, o = c['T'], c['O']
        diff = {k: (t[k], o[k]) for k in set(t) | set(o) if t[k] != o[k]}
        print('%-46s %s' % (fn, 'EQUAL FP arithmetic' if not diff else 'DIFF ' + ' '.join(
            '%s J%d/O%d' % (k, a, b) for k, (a, b) in sorted(diff.items()))))


if __name__ == '__main__':
    main()
