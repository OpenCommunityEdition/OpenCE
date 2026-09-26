"""G1 census helper (read-only): find int/x87 TRANSPOSITIONS (the hidden-temp issue-slot family) in residual rows.

usage: python -B moves.py <unit> <fn> [...]   [--obj ours.obj]
For each function: normalised instruction multisets of January vs ours (relocation spelling normalised). Reports
whether the function is a pure reorder (equal multisets) and lists alndiff delete/insert pairs whose instruction moved
(same normalised text deleted on one side and inserted on the other), flagging moves where an integer instruction
crosses x87 instructions or vice versa.
"""
import collections
import re
import subprocess
import sys


def norm(r):
    r = re.sub(r'defined-noncode:\.[a-z]+:', 'symbol:', r)
    r = re.sub(r'rel\+0x[0-9a-f]+', 'rel', r)
    r = re.sub(r'call 0x[0-9a-f]+', 'call', r)
    r = re.sub(r'\bj([a-z]+) 0x[0-9a-f]+', r'j\1', r)
    parts = r.split(None, 2)
    return parts[2] if len(parts) > 2 else ''


def rows(unit, fn, obj):
    cmd = ['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--max-lines', '0', '--include-equal']
    if obj:
        cmd += ['--ours-object', obj]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    t, o = [], []
    for line in out.splitlines():
        m = re.match(r'\s+([TO])\s+([0-9a-f]+)\s+(.*)$', line)
        if m:
            (t if m.group(1) == 'T' else o).append((int(m.group(2), 16), norm(line)))
    return t, o


def main():
    args = sys.argv[1:]
    obj = None
    if '--obj' in args:
        i = args.index('--obj')
        obj = args[i + 1]
        del args[i:i + 2]
    unit = args[0]
    for fn in args[1:]:
        t, o = rows(unit, fn, obj)
        ct = collections.Counter(x[1] for x in t)
        co = collections.Counter(x[1] for x in o)
        only_t = ct - co
        only_o = co - ct
        pure = not only_t and not only_o
        print('%-44s J %4d O %4d  %s  J-only %d  O-only %d' % (fn, len(t), len(o), 'PURE-REORDER' if pure else 'content-diff',
                                                             sum(only_t.values()), sum(only_o.values())))
        if len(only_t) + len(only_o) <= 12:
            for k in only_t:
                print('     J-only x%d  %s' % (only_t[k], k))
            for k in only_o:
                print('     O-only x%d  %s' % (only_o[k], k))


if __name__ == '__main__':
    main()
