"""E1 list-scheduler trace of ONE function (observation only; built on W7's sealed w7trace driver).

    python -B sched_trace.py <label> <src.c> <unit> <fn> [lo_hex hi_hex]

Gated (k, k) compile of function k under the pinned dbg32c with bps
  [MARK, 0x1074f646 ready-list insert (ECX = record), 0x1074efd7 selection (EDX = record),
   0x1074f071 cycle counter increment, 0x10751347 encoder (EBX node, ESI offset, ECX bytes, EDX count)].
Record layout (fable5 decal-sched LEDGER, static decode + trace): node at +0x1c (dword 7); +0x2c primary ready-list
key (dword 11, descending); +0x36 tie-break word (high word of dword 13, ascending = original list index); +0x3a unit
class (high word of dword 14). Readiness = the cycle of ready-list insertion.
Prints, for every encoded instruction of the function (optionally only offsets lo..hi), its node, bytes, first insert
cycle, +0x2c, +0x36, unit and selected cycle. Runs go to scratch/campaign/workers/E1/runs/<label>; the result is
read back only through W7's re-hashed seal (load_sealed)."""
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'W7' / 'tools'))
import w7trace as W  # noqa: E402

W.RUNS = W.ROOT / 'scratch/campaign/workers/E1/runs'
W.RUNS.mkdir(parents=True, exist_ok=True)
INSERT, SELECT, CYCLE = 0x1074f646, 0x1074efd7, 0x1074f071


def words(dw):
    return b''.join(struct.pack('<I', x) for x in dw) if dw else b''


def rec(r):
    return dict(node=r[7], key2c=r[11], w36=r[13] >> 16, w34=r[13] & 0xffff, unit=r[14] >> 16,
                w20=r[8] & 0xffff, w22=r[8] >> 16, w24=r[9] & 0xffff)


def fns_all(obj):
    """COFF function order including '@' fastcall names (W7's fns_in_order keeps '_' names only)."""
    secs = obj['sections']
    seen = {}
    for s in obj['symbols']:
        if (s['name'][:1] in ('_', '@') and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            seen.setdefault(s['section'], s['name'])
    return [seen[k] for k in sorted(seen)]


def main():
    label, src, unit, fn = sys.argv[1:5]
    lo = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0
    hi = int(sys.argv[6], 16) if len(sys.argv) > 6 else 1 << 30
    _order, o = W.stock_order(src, unit, label)
    order = fns_all(o)
    k = order.index(fn) + 1
    print('target', fn, 'ordinal', k, 'of', len(order))
    events, meta = W.run(label, src, unit, [W.MARK, INSERT, SELECT, CYCLE, W.ENC], gate=(k, k),
                         chains='2 24;3 24', note='E1 list-scheduler trace of %s' % fn)
    cycle = 0
    enc = {}
    ins, sel = {}, {}
    order_sel = []
    for e in events:
        if e['kind'] != 'LT':
            continue
        if e['bp'] == 3:
            cycle += 1
        elif e['bp'] == 1:
            r = e['chains'].get(0)
            if r:
                x = rec(r)
                ins.setdefault(x['node'], []).append(dict(cycle=cycle, **x))
        elif e['bp'] == 2:
            r = e['chains'].get(1)
            if r:
                x = rec(r)
                sel.setdefault(x['node'], []).append(dict(cycle=cycle, **x))
                order_sel.append(x['node'])
        elif e['bp'] == 4:
            g = e['regs']
            enc[g['ebx']] = (g['esi'], words(e['chains'].get(0) or [])[:g['edx']].hex())
    rows = []
    for node, (off, b) in sorted(enc.items(), key=lambda t: t[1][0]):
        if not (lo <= off <= hi):
            continue
        i = ins.get(node, [{}])
        s = sel.get(node, [{}])
        rows.append(dict(offset=off, node=node, bytes=b, insert_cycle=i[0].get('cycle'), n_inserts=len(ins.get(node, [])),
                         key2c=i[-1].get('key2c'), w36=i[-1].get('w36'), unit=i[-1].get('unit'),
                         w22=i[-1].get('w22'), select_cycle=s[0].get('cycle'), n_selects=len(sel.get(node, []))))
    out = dict(label=label, fn=fn, ordinal=k, meta_receipt=meta['receipt']['equal_ignoring_only_coff_timestamp'],
               rows=rows, cycles=cycle)
    adir = W.RUNS.parent / 'analysis'
    adir.mkdir(exist_ok=True)
    (adir / (label + '.sched.json')).write_text(json.dumps(out, indent=1) + '\n')
    print('stock-equal', out['meta_receipt'], 'cycles', cycle, 'encoded', len(enc))
    print(' off  bytes            node      ins  key2c   w36 unit  w22  sel')
    for x in rows:
        print('%4x  %-16s %08x  %4s  %6s  %4s %4s %4s  %4s' % (
            x['offset'], x['bytes'], x['node'], x['insert_cycle'], hex(x['key2c']) if x['key2c'] is not None else '-',
            x['w36'], x['unit'], x['w22'], x['select_cycle']))


if __name__ == '__main__':
    try:
        main()
    except W.TraceError as e:
        print('ABORT (fail-closed):', e)
        sys.exit(2)
