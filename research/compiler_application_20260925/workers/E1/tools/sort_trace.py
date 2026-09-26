"""E1 trace of the C2 commutative-operand sort (fable5 commute-order decode M1/M2) for ONE node (observation only).

    python -B sort_trace.py <label> <src.c> <unit> <fn> <op_offset_hex>

bps [MARK, 0x1070d420 (before sort: ESI node, EAX incoming head), 0x1070d42d (after sort: ESI node, EAX sorted head),
ENC 0x10751347 (EBX node, ESI offset)]; gate (k-1, k) so that function k's pre-marker optimizer (after MARK k-1) and
its backend/encoder (after MARK k) are both observed. Chains (ESI = node):
  c0 = in1 entry (node+0x28 -> 0x16 dwords), c1 = in2 entry (in1 +0 next), c2 = in1 entry+0x18 record, c3 = in2
  entry+0x18 record.
Entry words (commute-order LEDGER): +4 opcode, +8 kind byte / type word (+0xa), +0x10 KEY, +0x18 symbol/subtree,
+0x1c reg, +0x30 displacement, +0x34 base. Symbol record: +0x1c id, +4 storage byte, +0x14 defining subtree.
The node is identified as the encoder EBX at <op_offset> in the LAST gated segment; its sort events are the bp1/bp2
hits with ESI == node in the segment before."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[1] / 'W7' / 'tools'))
import w7trace as W  # noqa: E402
from sched_trace import fns_all  # noqa: E402

W.RUNS = W.ROOT / 'scratch/campaign/workers/E1/runs'
PRE, POST = 0x1070d420, 0x1070d42d


def entry(r):
    if not r:
        return None
    return dict(opc=hex(r[1]), kind=r[2] & 0xff, type=hex(r[2] >> 16), key=hex(r[4]), sym=hex(r[6]), reg=hex(r[7]),
                disp30=hex(r[12]) if len(r) > 12 else None, base34=hex(r[13]) if len(r) > 13 else None)


def symrec(r):
    if not r:
        return None
    return dict(storage=r[1] & 0xff, w14=hex(r[5]), id=r[7], raw=[hex(x) for x in r[:12]])


def main():
    label, src, unit, fn, off = sys.argv[1:6]
    off = int(off, 16)
    _order, o = W.stock_order(src, unit, label)
    order = fns_all(o)
    k = int(W.os.environ['E1_MARK']) if W.os.environ.get('E1_MARK') else order.index(fn) + 1
    if k < 2:
        raise W.TraceError('function is first in the TU: gate (k-1, k) impossible')
    chains = W.os.environ.get('E1_CHAINS', '4 16 28;4 16 28 0;4 16 28 18;4 16 28 0 18')
    events, meta = W.run(label, src, unit, [W.MARK, PRE, POST, W.ENC], gate=(k - 1, k),
                         chains=chains, note='E1 sort trace %s +0x%x chains %s' % (fn, off, chains))
    seg = 0
    for e in events:
        if e['kind'] == 'GATE':
            seg += 1
        e['segment'] = seg
    last = k  # dbg32c prints a GATE line at EVERY MARK; the backend/encoder of function k is segment k
    enc = [e for e in events if e['kind'] == 'LT' and e['bp'] == 3 and e['segment'] == last and e['regs']['esi'] == off]
    if len(enc) != 1:
        raise W.TraceError('encoder events at +0x%x in segment %d: %d' % (off, last, len(enc)))
    node = enc[0]['regs']['ebx']
    sorts = []
    for e in events:
        if e['kind'] == 'LT' and e['bp'] in (1, 2) and e['regs']['esi'] == node:
            c = e['chains']
            sorts.append(dict(seq=e['seq'], segment=e['segment'], when='pre' if e['bp'] == 1 else 'post',
                              head=hex(e['regs']['eax']), in1=entry(c.get(0)), in2=entry(c.get(1)),
                              in1_sym=symrec(c.get(2)), in2_sym=symrec(c.get(3)),
                              raw={i: [hex(x) for x in (c.get(i) or [])] for i in range(4)}))
    out = dict(label=label, fn=fn, ordinal=k, offset=hex(off), node=hex(node), segments=last,
               stock_equal=meta['receipt']['equal_ignoring_only_coff_timestamp'], sorts=sorts)
    adir = W.RUNS.parent / 'analysis'
    adir.mkdir(exist_ok=True)
    (adir / (label + '.sort.json')).write_text(json.dumps(out, indent=1) + '\n')
    print('stock-equal', out['stock_equal'], 'ordinal', k, 'segments', last, 'node', hex(node), 'sort events', len(sorts))
    for s in sorts:
        print(s['when'], 'seg', s['segment'], 'head', s['head'])
        print('   in1', s['in1'], s['in1_sym'] and ('id %s storage %s w14 %s' % (s['in1_sym']['id'], s['in1_sym']['storage'], s['in1_sym']['w14'])))
        print('   in2', s['in2'], s['in2_sym'] and ('id %s storage %s w14 %s' % (s['in2_sym']['id'], s['in2_sym']['storage'], s['in2_sym']['w14'])))


if __name__ == '__main__':
    try:
        main()
    except W.TraceError as e:
        print('ABORT (fail-closed):', e)
        sys.exit(2)
