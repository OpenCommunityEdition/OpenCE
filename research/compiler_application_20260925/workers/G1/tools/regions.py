"""G1 DIAGNOSTIC reader for W6 w6trace.py 'region' runs: list scheduling regions (0x1074e68f: EDI start node,
EBX end node) of the traced function with the encoded offsets of their start/end nodes.

usage: python -B regions.py <run_dir> [lo_hex hi_hex]
Nodes that never encode (hidden records) are shown as '-'. The region END node is the first node NOT in the region.
"""
import re
import struct
import sys
from pathlib import Path


def load(work):
    ev = []
    for line in (Path(work) / 'dbg_result.txt').read_text().splitlines():
        if not line.startswith('LT '):
            continue
        w = line.split()
        e = dict(bp=int(w[1][2:], 16), regs={k: int(v, 16) for k, v in re.findall(r'(e\w+)=(0x[0-9a-f]+)', line)},
                 chains={})
        for m in re.finditer(r'chain(\d):((?: 0x[0-9a-f]+)+)', line):
            e['chains'][int(m[1])] = [int(x, 16) for x in m[2].split()]
        ev.append(e)
    return ev


def main():
    work = sys.argv[1]
    lo = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
    hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 1 << 30
    ev = load(work)
    enc = {}
    for e in ev:
        if e['bp'] == 2:
            enc.setdefault(e['regs']['ebx'], e['regs']['esi'])
    n = 0
    for e in ev:
        if e['bp'] != 1:
            continue
        s, t = e['regs']['edi'], e['regs']['ebx']
        so, to = enc.get(s), enc.get(t)
        n += 1
        if (so is not None and lo <= so <= hi) or (to is not None and lo <= to <= hi):
            print('region %3d  start node %08x @%s  end node %08x @%s' % (
                n, s, '%x' % so if so is not None else '-', t, '%x' % to if to is not None else '-'))


if __name__ == '__main__':
    main()
