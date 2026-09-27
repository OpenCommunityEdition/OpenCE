"""G1 census helper (read-only): count int<->x87 slot transpositions (W6 hidden-temp family signature).

usage: python -B xmoves.py <unit> <fn> [...] [--obj ours.obj] [--show]
A 'move' = an instruction whose normalised text is deleted in one alndiff block and inserted in another block within
8 instructions, with at least one instruction of the OTHER class (x87 vs integer) between the two positions.
"""
import re
import subprocess
import sys

sys.path.insert(0, __file__.rsplit('\\', 1)[0] if '\\' in __file__ else __file__.rsplit('/', 1)[0])
from moves import norm  # noqa: E402


def blocks(unit, fn, obj):
    cmd = ['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--max-lines', '0', '--include-equal']
    if obj:
        cmd += ['--ours-object', obj]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    T, O = [], []
    for line in out.splitlines():
        m = re.match(r'\s+([TO])\s+([0-9a-f]+)\s+(.*)$', line)
        if m:
            (T if m.group(1) == 'T' else O).append((int(m.group(2), 16), norm(line)))
    return T, O


def isx87(s):
    return s.split(' ')[0].startswith('f')


def main():
    args = sys.argv[1:]
    obj = None
    show = '--show' in args
    if show:
        args.remove('--show')
    if '--obj' in args:
        i = args.index('--obj')
        obj = args[i + 1]
        del args[i:i + 2]
    unit = args[0]
    import difflib
    for fn in args[1:]:
        T, O = blocks(unit, fn, obj)
        a = [x[1] for x in T]
        b = [x[1] for x in O]
        sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
        dels, ins = [], []
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag in ('delete', 'replace'):
                dels += [(i, a[i]) for i in range(i1, i2)]
            if tag in ('insert', 'replace'):
                ins += [(j, b[j], i1) for j in range(j1, j2)]
        moves = []
        used = set()
        for i, s in dels:
            for j, s2, anchor in ins:
                if s2 == s and (j, s2) not in used and abs(anchor - i) <= 8:
                    lo, hi = sorted((i, anchor))
                    between = a[lo:hi + 1]
                    if any(isx87(x) != isx87(s) for x in between if x != s):
                        moves.append((T[i][0], s, isx87(s)))
                        used.add((j, s2))
                        break
        ncross = len(moves)
        nint = sum(1 for m in moves if not m[2])
        print('%-46s J %4d  int<->x87 moves %3d (int moved %d)' % (fn, len(T), ncross, nint))
        if show:
            for off, s, x in moves:
                print('     J+%04x %s %s' % (off, 'x87' if x else 'int', s))


if __name__ == '__main__':
    main()
