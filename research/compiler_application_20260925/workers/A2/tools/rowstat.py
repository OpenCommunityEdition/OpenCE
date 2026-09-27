"""A2 lab tool (read-only): strict row status (gate.py logic) of every January function of <unit> in an object.

    python -B rowstat.py <unit> <obj> [<obj2>]      with two objects, prints only rows whose status differs
"""
import os
import sys

ROOT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, ROOT)
from tools import coff_compare as cc  # noqa: E402


def fn_syms(o):
    secs = o['sections']
    out = {}
    for s in o['symbols']:
        if (s['type'] == 0x20 and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            out.setdefault(s['name'], s)
    return out


def rows(unit, path):
    t = cc.load(open(os.path.join(ROOT, 'build', 'split', unit + '.obj'), 'rb').read())
    o = cc.load(open(path, 'rb').read())
    ts, os_ = fn_syms(t), fn_syms(o)
    out = {}
    for n in sorted(ts):
        if n not in os_:
            out[n] = 'UNWRITTEN'
            continue
        out[n] = 'EXACT' if cc.section_infos_equal(cc.section_info(t, n), cc.section_info(o, n)) else 'residual'
    return out


unit = sys.argv[1]
A = rows(unit, sys.argv[2])
if len(sys.argv) > 3:
    B = rows(unit, sys.argv[3])
    ch = [n for n in A if A[n] != B.get(n)]
    for n in ch:
        print('%-50s %s -> %s' % (n, A[n], B.get(n)))
    print('%s: A exact %d, B exact %d, %d rows changed status' % (
        unit, sum(v == 'EXACT' for v in A.values()), sum(v == 'EXACT' for v in B.values()), len(ch)))
else:
    for n, v in A.items():
        print('%-10s %s' % (v, n))
    print('%s: exact %d of %d' % (unit, sum(v == 'EXACT' for v in A.values()), len(A)))
