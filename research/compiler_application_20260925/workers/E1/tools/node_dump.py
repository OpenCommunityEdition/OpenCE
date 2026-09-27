"""E1 encoder census with IL node/operand words for ONE function (observation only; W7's sealed driver).

    python -B node_dump.py <label> <src.c> <unit> <fn> [lo_hex hi_hex]

bps [MARK, ENC 0x10751347]; chains: node (EBX, 0x24 dwords), src operand (node+0x28), dst operand (node+0x2c),
encoded bytes (ECX). Node words: +4 opcode, +8 kind byte, +9 byte, +0xa type word, +0x14 line, +0x28 src, +0x2c dst.
Operand words: +8 kind byte, +0xa type word, +0x18 symbol (kind 1)."""
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[1] / 'W7' / 'tools'))
import w7trace as W  # noqa: E402
from sched_trace import fns_all  # noqa: E402

W.RUNS = W.ROOT / 'scratch/campaign/workers/E1/runs'


def words(dw):
    return b''.join(struct.pack('<I', x) for x in dw) if dw else b''


def fields(dw):
    if not dw:
        return None
    b = words(dw)
    return dict(opcode=dw[1], kind=b[8], b9=b[9], type=struct.unpack_from('<H', b, 10)[0], w0c=dw[3], w14=dw[5],
                w18=dw[6], w1c=dw[7], w20=dw[8], w24=dw[9], src=dw[10] if len(dw) > 10 else None,
                dst=dw[11] if len(dw) > 11 else None, raw=[hex(x) for x in dw])


def main():
    label, src, unit, fn = sys.argv[1:5]
    lo = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0
    hi = int(sys.argv[6], 16) if len(sys.argv) > 6 else 1 << 30
    _order, o = W.stock_order(src, unit, label)
    order = fns_all(o)
    k = order.index(fn) + 1
    events, meta = W.run(label, src, unit, [W.MARK, W.ENC], gate=(k, k),
                         chains='1 24;1 24 28;1 24 2c;2 4', note='E1 node census of %s' % fn)
    rows = []
    for e in events:
        if e['kind'] != 'LT' or e['bp'] != 1:
            continue
        r = e['regs']
        if not (lo <= r['esi'] <= hi):
            continue
        rows.append(dict(offset=r['esi'], node=r['ebx'], bytes=words(e['chains'].get(3) or [])[:r['edx']].hex(),
                         n=fields(e['chains'].get(0)), s=fields(e['chains'].get(1)), d=fields(e['chains'].get(2))))
    adir = W.RUNS.parent / 'analysis'
    adir.mkdir(exist_ok=True)
    (adir / (label + '.nodes.json')).write_text(json.dumps(dict(label=label, fn=fn, ordinal=k, rows=rows), indent=1) + '\n')
    print('stock-equal', meta['receipt']['equal_ignoring_only_coff_timestamp'], 'ordinal', k)
    for x in rows:
        n, s, d = x['n'] or {}, x['s'] or {}, x['d'] or {}
        print('%4x %-14s node %08x op %03x k %02x b9 %02x ty %04x line %s | src k %s ty %s | dst k %s ty %s' % (
            x['offset'], x['bytes'], x['node'], n.get('opcode', 0), n.get('kind', 0), n.get('b9', 0), n.get('type', 0),
            n.get('w14'), '%02x' % s['kind'] if s else '--', '%04x' % s['type'] if s else '----',
            '%02x' % d['kind'] if d else '--', '%04x' % d['type'] if d else '----'))


if __name__ == '__main__':
    try:
        main()
    except W.TraceError as e:
        print('ABORT (fail-closed):', e)
        sys.exit(2)
