"""E1: the IL of ONE function at the first post-MARK walk, with each label's first jump references (observation only).

    python -B il_walk.py <label> <src.c> <unit> <mark_ordinal> [pass_bp_hex]

bps [MARK, 0x10720b39 (first pass after MARK; ESI = node) or the given walk bp]; gate (k, k); rmem = ESI node
(16 dwords: +4 opcode, +8 kind byte, +0x14 line word, +0x1c label id); chains = the label's +0x20 ref list, refs 1..3
(ref +0 next, +0xc referencing branch node) - fable5 phase-vk LEDGER 2.1/2.4 and harness/loc_lref.py layout.
Prints the walk with ordinals; for labels (kind 0x1b) the referencing branch ordinals head-first."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'W7' / 'tools'))
import w7trace as W  # noqa: E402

W.RUNS = W.ROOT / 'scratch/campaign/workers/E1/runs'


def main():
    label, src, unit, k = sys.argv[1:5]
    k = int(k)
    walk_bp = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0x10720b39
    events, meta = W.run(label, src, unit, [W.MARK, walk_bp], gate=(k, k), rmem=(4, 16),
                         chains='4 8 20;4 8 20 0;4 8 20 0 0', note='E1 IL walk fn %d' % k)
    seg = 0
    walk = []
    for e in events:
        if e['kind'] == 'GATE':
            seg += 1
            continue
        if e['kind'] == 'LT' and e['bp'] == 1 and seg == k:
            walk.append(e)
    idx = {e['regs']['esi']: i for i, e in enumerate(walk)}
    rows = []
    for i, e in enumerate(walk):
        r = e.get('rmem') or [0] * 16
        kind = r[2] & 0xff
        row = dict(i=i, node=hex(e['regs']['esi']), opcode=hex(r[1] & 0xfff), kind=hex(kind), line=r[5] & 0xffff,
                   w1c=hex(r[7]), raw=[hex(x) for x in r[:16]])
        if kind == 0x1b:
            refs = []
            for ci in range(3):
                rec = e['chains'].get(ci)
                if rec:
                    refs.append('N%d' % idx[rec[3]] if rec[3] in idx else 'x%08x' % rec[3])
            row['refs'] = refs
        rows.append(row)
    adir = W.RUNS.parent / 'analysis'
    adir.mkdir(exist_ok=True)
    (adir / (label + '.il.json')).write_text(json.dumps(dict(label=label, k=k, stock_equal=meta['receipt']
                                                               ['equal_ignoring_only_coff_timestamp'], rows=rows), indent=1) + '\n')
    print('stock-equal', meta['receipt']['equal_ignoring_only_coff_timestamp'], 'walk', len(rows))
    for x in rows:
        extra = (' refs ' + ' '.join(x['refs'])) if 'refs' in x else ''
        print('N%-4d %s op %s kind %s line %d w1c %s%s' % (x['i'], x['node'], x['opcode'], x['kind'], x['line'], x['w1c'], extra))


if __name__ == '__main__':
    try:
        main()
    except W.TraceError as e:
        print('ABORT (fail-closed):', e)
        sys.exit(2)
