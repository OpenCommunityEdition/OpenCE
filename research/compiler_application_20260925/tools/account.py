"""Cumulative strict accounting against the frozen base (campaign item 8). Read-only.

    python -B scratch/campaign/account.py <snapshot.json> [--base scratch/campaign/base_stable.json]

From two tools.campaign.stable_verdicts snapshots (keyed by unit + target section index):
- newly exact rows (status -> 'E'): count, padded bytes (target section size) and meaningful bytes (January's
  meaningful size from amap_base.json where the row was a ledger row);
- LOST exact rows (must be empty);
- objects whose every tracked section is now 'E' and was not before (newly complete objects);
- Halo (source/) and vendor (libs/) are reported separately. Data sections, aliases, register-only diagnostics,
  local improvements and held candidates are NOT counted here (they never appear as new 'E' function rows)."""
import argparse
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('snapshot')
    ap.add_argument('--base', default=str(ROOT / 'scratch/campaign/base_stable_d890c2db.json'))
    a = ap.parse_args()
    B, N = json.load(open(a.base)), json.load(open(a.snapshot))
    mean = {(r['unit'], r['fn']): r.get('jan_meaningful') for r in json.load(open(ROOT / 'scratch/campaign/amap_base.json'))}
    gained, lost = [], []
    for k in sorted(B.keys() | N.keys()):
        b, n = B.get(k, {}), N.get(k, {})
        if n.get('status') == 'E' and b.get('status') != 'E':
            gained.append((k, n))
        elif b.get('status') == 'E' and n.get('status') != 'E':
            lost.append((k, b, n))
    per_obj_b, per_obj_n = defaultdict(list), defaultdict(list)
    for k, v in B.items():
        per_obj_b[k.split('::')[0]].append(v.get('status'))
    for k, v in N.items():
        per_obj_n[k.split('::')[0]].append(v.get('status'))
    newly_complete = sorted(u for u in per_obj_n if all(s == 'E' for s in per_obj_n[u])
                            and not all(s == 'E' for s in per_obj_b.get(u, ['?'])))
    for scope in ('source/', 'libs/'):
        g = [(k, n) for k, n in gained if k.startswith(scope)]
        padded = sum(n.get('size') or 0 for _, n in g)
        meaningful = sum(int(mean.get((k.split('::')[0], n.get('name'))) or 0) for k, n in g)
        print('%-8s newly exact %d rows | padded %d B | meaningful %d B (ledger rows only)' % (
            scope, len(g), padded, meaningful))
        for k, n in g:
            print('   + %-70s %-55s %5s B  meaningful %s' % (k, n.get('name'), n.get('size'),
                                                           mean.get((k.split('::')[0], n.get('name')), '?')))
    print('LOST exact rows: %d' % len(lost))
    for k, b, n in lost:
        print('   - %-70s %-55s %s -> %s' % (k, b.get('name'), b.get('status'), n.get('status')))
    print('newly complete objects: %d %s' % (len(newly_complete), newly_complete))


if __name__ == '__main__':
    main()
