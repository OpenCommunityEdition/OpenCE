"""Run the whole-object admission battery on production build/base for a list of units.

    python scratch/campaign/audit_battery.py <unit> [<unit> ...]   (units like ai/path)
Prints per unit: gate totals, object_audit verdict (+ failing lines), PDB disagreements,
surplus identity, provider-link verdict (+ failing symbols), report data %.
"""
import json
import subprocess
import sys

rep = {u['name']: u for u in json.load(open('build/report.json'))['units']}


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return (r.stdout + r.stderr).strip().splitlines()


for u in sys.argv[1:]:
    unit = 'source/' + u
    g = run([sys.executable, 'tools/campaign/gate.py', unit, '--all'])
    oa = run([sys.executable, 'scratch/tools/object_audit.py', unit])
    pdb = run([sys.executable, 'scratch/tools/pdb_storage.py', unit])
    si = run([sys.executable, 'scratch/tools/surplus_identity.py', unit])
    pl = run([sys.executable, 'scratch/tools/provider_link.py', unit])
    m = rep.get(unit, {}).get('measures', {})
    print('=' * 20, u)
    print('  gate   :', g[-1] if g else '?')
    bad = [l for l in oa if 'DIFF' in l or 'MISSING' in l or 'storage' in l.lower() and '!=' in l]
    print('  audit  :', oa[-1] if oa else '?', '|', ' ; '.join(x.strip() for x in bad[:6]))
    print('  pdb    :', pdb[-1] if pdb else '?')
    print('  surplus:', si[-1] if si else '?', '|', ' ; '.join(x.strip() for x in si if 'DIFFERENT' in x)[:300])
    fails = [l.split()[0] for l in pl if l.rstrip().endswith('FAIL') or 'NO JANUARY PROVIDER' in l]
    print('  link   :', pl[-1] if pl else '?', fails)
    print('  data   : %s/%s' % (m.get('matched_data'), m.get('total_data')))
