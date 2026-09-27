"""Difference signature per remaining row (read-only): aligned differing instructions, relocation-identity multiset
delta, frame delta, first differing instruction pair.  python -B scratch/campaign/sig.py > scratch/campaign/sig.txt"""
import collections
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'tools/campaign'))
from tools import coff_compare as cc  # noqa: E402
import alndiff  # noqa: E402


def ident(r):
    t = r['target']
    if t[0] == 'internal':
        return 'internal'
    if t[0] == 'defined-noncode' and t[1] == '.rdata':
        # csplit spells literal pools `symbol:X`, our compiled objects `defined-noncode:.rdata:X`: same destination
        return 'symbol:%s%s' % (t[2], ('+%s' % (t[3],)) if len(t) > 3 and t[3] else '')
    return '%s:%s%s' % (t[0], t[1], ('+%s' % (t[2],)) if len(t) > 2 and t[2] else '')


def main():
    rows = json.load(open(ROOT / 'scratch/campaign/amap_base.json'))
    out = []
    for r in rows:
        if r['fn'].endswith('_jmptable') or r['verdict'] != 'R':
            continue
        tp = ROOT / 'build/split' / (r['unit'] + '.obj')
        op = ROOT / 'build/base' / (r['unit'] + '.obj')
        try:
            T, O = cc.load(tp.read_bytes()), cc.load(op.read_bytes())
            jt = alndiff.disassemble_function(T, r['fn'])
            ot = alndiff.disassemble_function(O, r['fn'])
            ops = alndiff.aligned_opcodes(jt, ot)
            dj = sum(a1 - a0 for tag, a0, a1, b0, b1 in ops if tag != 'equal')
            do = sum(b1 - b0 for tag, a0, a1, b0, b1 in ops if tag != 'equal')
            groups = sum(1 for o in ops if o[0] != 'equal')
            first = next(((jt[a0] if a0 < len(jt) else '-', ot[b0] if b0 < len(ot) else '-')
                          for tag, a0, a1, b0, b1 in ops if tag != 'equal'), None)
            tj = collections.Counter(ident(x) for x in cc.section_info(T, r['fn'])['relocations'])
            to = collections.Counter(ident(x) for x in cc.section_info(O, r['fn'])['relocations'])
            miss = dict(tj - to)
            extra = dict(to - tj)
        except Exception as e:  # noqa: BLE001
            out.append(dict(r, error=str(e)[:80]))
            continue
        out.append(dict(unit=r['unit'], fn=r['fn'], meaningful=r.get('jan_meaningful'), park=r['park'],
                        reserved=r['reserved'], jan_insns=len(jt), groups=groups, jan_diff=dj, our_diff=do,
                        frame=(r.get('jan_frame'), r.get('our_frame')), dsize=r.get('our_size', 0) - r.get('jan_size', 0),
                        reloc_missing=miss, reloc_extra=extra, first=first))
    json.dump(out, open(ROOT / 'scratch/campaign/sig.json', 'w'), indent=1, default=str)
    out.sort(key=lambda x: (x.get('jan_diff', 9999)))
    for x in out:
        if 'error' in x:
            print('ERR', x['unit'], x['fn'], x['error'])
            continue
        print('%-44s %5s grp %3d J %4d/%4d O %4d fr %s ds %4d %-22s %s miss %s extra %s' % (
            x['fn'][:44], x['meaningful'], x['groups'], x['jan_diff'], x['jan_insns'], x['our_diff'], x['frame'],
            x['dsize'], (x['park'] or '-')[:22], (x['reserved'] or '')[:6], json.dumps(x['reloc_missing'])[:80],
            json.dumps(x['reloc_extra'])[:80]))


if __name__ == '__main__':
    main()
