"""Per-object hint packets for triage agents (read-only).

    python scratch/campaign/hints.py  -> scratch/campaign/hints.json

For each incomplete Halo unit on the board: missing functions with structural class
(S1 reference multiset differs / S2 instruction count differs / T tie-shaped), frame
sizes (target vs ours), park class + first 400 chars of park evidence, rejections,
audit/link failures, prior ledger file names and round-1 result files.
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)
src = open('tools/campaign/structural_split.py', encoding='utf-8').read()
ns = {'__file__': os.path.join(ROOT, 'tools', 'campaign', 'structural_split.py')}
exec(compile(src[:src.index('\nrows = []')], 'structural_split', 'exec'), ns)
read, real_code = ns['read'], ns['real_code']

FRAME = re.compile(rb'\x81\xec(....)|\x83\xec(.)', re.S)


def frame(body):
    m = FRAME.search(body[:64])
    if not m:
        return 0
    return int.from_bytes(m.group(1), 'little') if m.group(1) else m.group(2)[0]


board = json.load(open('scratch/campaign/board.json'))
parks = {(p['unit'], p['function']): p for p in json.load(open('config/parked.json'))['entries']}
rej = collections.defaultdict(list)
for r in json.load(open('config/object_admission_rejections.json'))['entries']:
    rej[r['unit']].append({'class': r['class'], 'symbol': r['symbol'], 'reopen': r.get('reopen', '')[:400]})
logs = sorted(os.listdir('docs/object_matching_logs'))
r1 = []
for dp, _d, fs in os.walk('research/fifty_objects_20260925/results'):
    r1 += [os.path.join(dp, f).replace('\\', '/') for f in fs]
out = {}
for row in board:
    unit = row['unit']
    base = unit.rsplit('/', 1)[1]
    tgt = read(os.path.join('build', 'split', unit + '.obj'))
    ours = read(os.path.join('build', 'base', unit + '.obj'))
    fns = []
    for name, size, st, pk in row['missing']:
        t, o = tgt.get(name), ours.get(name)
        cls = '?'
        info = {}
        if t and o:
            tr, orr = collections.Counter(t[1]), collections.Counter(o[1])
            ti, oi = real_code(t[0])[0], real_code(o[0])[0]
            cls = 'S1' if tr != orr else ('S2' if ti != oi else 'T')
            info = {'insn_target': ti, 'insn_ours': oi, 'frame_target': frame(t[0]), 'frame_ours': frame(o[0]),
                    'reloc_missing': sorted((tr - orr).elements())[:8], 'reloc_extra': sorted((orr - tr).elements())[:8]}
        elif t and not o:
            cls = 'UNWRITTEN'
        p = parks.get((unit, name))
        fns.append({'name': name, 'size': size, 'class': cls, **info, 'park': p['class'] if p else None,
                    'park_evidence': (p or {}).get('evidence', '')[:400]})
    out[unit] = {'tier': row['tier'], 'exact': row['exact'], 'fn': row['fn'], 'gap_bytes': row['gap_bytes'],
                 'data_gap': row['data_gap'], 'missing': fns, 'rejections': rej.get(unit, []),
                 'audit': row.get('audit'), 'audit_fail_lines': row.get('audit_fail_lines', []),
                 'link_fail': row.get('link_fail', []), 'surplus_different': row.get('surplus_different', []),
                 'ledgers': [l for l in logs if l.startswith(base + '_') or l.startswith(base + '.')],
                 'round1_results': [f for f in r1 if ('_' + base + '_') in f.replace('/', '_') or f.endswith('_' + base + '.md')],
                 'claim': row.get('claim')}
json.dump(out, open('scratch/campaign/hints.json', 'w'), indent=1)
print(len(out), 'units;', collections.Counter(f['class'] for u in out.values() for f in u['missing']))
