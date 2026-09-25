"""Live opportunity board of unfinished Halo objects (read-only; parallel audit battery).

    python scratch/campaign/board.py [--stable scratch/baseline/stable.json] [--jobs 12] [--no-audit]

Writes scratch/campaign/board.json and board.md. Per incomplete Halo unit:
strict functions (stable snapshot), missing functions with park class, objdiff data gap,
admission rejections, object_audit verdict, PDB storage disagreements, surplus identity,
selected-provider link, prior-ledger count, claim/reservation, and a priority tier:
  T1 = every function exact (blocked only by ownership/linkage/names/data/layout)
  T2 = one or two residual functions, total gap <= 2,048 padded bytes
  T3 = other one/two-function objects
  T4 = three or more residual functions
Ties inside a tier sort by gap bytes, then data gap. Fuzzy percentage is NOT used.
"""
import collections
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)
args = sys.argv[1:]
stable_path = args[args.index('--stable') + 1] if '--stable' in args else 'scratch/baseline/stable.json'
jobs = int(args[args.index('--jobs') + 1]) if '--jobs' in args else 12
audit = '--no-audit' not in args

cfg = json.load(open('config/config.json'))
halo = [p for p in cfg['projects'] if p['name'] == 'halobetacache'][0]
status = {o['name'].rsplit('.', 1)[0]: o.get('status', '') for o in halo['objects']}
stable = json.load(open(stable_path))
per = collections.defaultdict(lambda: {'fn': 0, 'exact': 0, 'bytes': 0, 'exact_bytes': 0, 'missing': []})
for key, v in stable.items():
    unit = key.split('::')[0]
    d = per[unit]
    d['fn'] += 1
    d['bytes'] += v.get('size', 0)
    if v.get('status') == 'E':
        d['exact'] += 1
        d['exact_bytes'] += v.get('size', 0)
    else:
        d['missing'].append([v.get('name'), v.get('size'), v.get('status')])
rep = {u['name']: u for u in json.load(open('build/report.json'))['units']}
parks = json.load(open('config/parked.json'))['entries']
park = {(p['unit'], p['function']): p.get('class') for p in parks}
rej = collections.defaultdict(list)
for r in json.load(open('config/object_admission_rejections.json'))['entries']:
    rej[r['unit']].append('%s:%s' % (r['class'], r['symbol'][:40]))
claims = {}
if os.path.exists('scratch/campaign/claims.json'):
    claims = json.load(open('scratch/campaign/claims.json'))
logs = os.listdir('docs/object_matching_logs')


def ledger_count(unit):
    base = unit.rsplit('/', 1)[1]
    return sum(1 for f in logs if f.startswith(base + '_') or f.startswith(base + '.'))


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return (r.stdout + r.stderr).strip().splitlines()


def battery(unit):
    out = {}
    oa = run([sys.executable, 'scratch/tools/object_audit.py', unit])
    out['audit'] = oa[-1] if oa else '?'
    out['audit_fail_lines'] = [l.strip() for l in oa if ('DIFF' in l or 'MISSING' in l or '!=' in l)][:8]
    pdb = run([sys.executable, 'scratch/tools/pdb_storage.py', unit])
    out['pdb'] = pdb[-1] if pdb else '?'
    si = run([sys.executable, 'scratch/tools/surplus_identity.py', unit])
    out['surplus'] = si[-1] if si else '?'
    out['surplus_different'] = [l.strip() for l in si if 'DIFFERENT' in l][:6]
    pl = run([sys.executable, 'scratch/tools/provider_link.py', unit])
    out['link'] = pl[-1] if pl else '?'
    out['link_fail'] = [l.split()[0] for l in pl if l.rstrip().endswith('FAIL') or 'NO JANUARY PROVIDER' in l][:10]
    return out


rows = []
for unit, st in status.items():
    if not unit.startswith('source/') or st == 'Matching' or unit == 'source/linker_common':
        continue
    d = per.get(unit, {'fn': 0, 'exact': 0, 'bytes': 0, 'exact_bytes': 0, 'missing': []})
    m = rep.get(unit, {}).get('measures', {})
    total_data = int(m.get('total_data', 0) or 0)
    matched_data = int(m.get('matched_data', 0) or 0)
    missing = sorted(d['missing'], key=lambda x: x[1] or 0)
    for x in missing:
        x.append(park.get((unit, x[0]), '-'))
    gap_fn = d['fn'] - d['exact']
    gap_b = d['bytes'] - d['exact_bytes']
    tier = 'T1' if gap_fn == 0 else ('T2' if gap_fn <= 2 and gap_b <= 2048 else ('T3' if gap_fn <= 2 else 'T4'))
    c = claims.get(unit)
    rows.append({'unit': unit, 'tier': tier, 'fn': d['fn'], 'exact': d['exact'], 'gap_fn': gap_fn,
                 'gap_bytes': gap_b, 'data_total': total_data, 'data_gap': total_data - matched_data,
                 'rejections': rej.get(unit, []), 'missing': missing, 'ledgers': ledger_count(unit),
                 'claim': (c['state'] + ':' + c['agent']) if c and c['state'] in ('claimed', 'reserved') else ''})
if audit:
    with cf.ThreadPoolExecutor(jobs) as ex:
        res = dict(zip([r['unit'] for r in rows], ex.map(battery, [r['unit'] for r in rows])))
    for r in rows:
        r.update(res[r['unit']])
rows.sort(key=lambda r: (r['tier'], r['gap_bytes'], r['data_gap']))
json.dump(rows, open('scratch/campaign/board.json', 'w'), indent=1)
with open('scratch/campaign/board.md', 'w', encoding='utf-8') as fh:
    fh.write('| tier | unit | fns | gapB | dataGap | missing (size, park) | rejections | audit | pdb | link fails | ledgers | claim |\n')
    fh.write('|---|---|---|---|---|---|---|---|---|---|---|---|\n')
    for r in rows:
        miss = '; '.join('%s %s %s' % (x[0], x[1], x[3]) for x in r['missing'][:4])
        fh.write('| %s | %s | %d/%d | %d | %d | %s | %s | %s | %s | %s | %d | %s |\n' % (
            r['tier'], r['unit'][7:], r['exact'], r['fn'], r['gap_bytes'], r['data_gap'], miss,
            ', '.join(r['rejections']), r.get('audit', '')[:30], r.get('pdb', '')[:30],
            ' '.join(r.get('link_fail', [])), r['ledgers'], r['claim']))
print('incomplete Halo objects:', len(rows), collections.Counter(r['tier'] for r in rows))
