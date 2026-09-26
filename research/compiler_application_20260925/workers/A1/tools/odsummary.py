"""Summarise an objdiff 3.3.1 mini-project report + its audit_semantic_matches ledger (units prod/finalA/finalB)."""
import json
import os
import sys
from collections import Counter, defaultdict

od = sys.argv[1]
r = json.load(open(os.path.join(od, 'report.json')))
print('objdiff-cli 3.3.1 mini-project %s (target build/split, prod build/base)' % od)
base = None
for u in r['units']:
    m = u.get('measures', {})
    fn = {f['name']: f.get('fuzzy_match_percent') for f in u.get('functions', [])}
    base = base or fn
    deltas = [n for n, p in fn.items() if base.get(n) != p]
    sec = {s['name']: s.get('fuzzy_match_percent') for s in u.get('sections', []) if s['name'] in ('.data', '.bss')}
    print('%-7s code %s/%s  fns %s/%s  data %s/%s  .data %s%%  .bss %s%%  per-function deltas vs prod: %d' % (
        u['name'], m.get('matched_code'), m.get('total_code'), m.get('matched_functions'), m.get('total_functions'),
        m.get('matched_data'), m.get('total_data'), sec.get('.data'), sec.get('.bss'), len(deltas)))
    print('        objdiff <100%%: %s' % sorted((f['name'], round(f['fuzzy_match_percent'], 2)) for f in u.get('functions', [])
                                        if f.get('fuzzy_match_percent') != 100.0))
s = json.load(open(os.path.join(od, 'semantic_report.json')))
per = defaultdict(Counter)
only = defaultdict(list)
for e in s['accepted_ledger']:
    per[e['unit']][','.join(e['proof_sources'])] += 1
    if 'objdiff' not in e['proof_sources']:
        only[e['unit']].append(e['function'])
print('audit_semantic_matches summary:', {k: s['summary'][k] for k in ('units_scanned', 'functions_evaluated', 'semantic_exact',
                                                                     'unit_errors', 'accepted_exact')})
for u in sorted(per):
    print('  accepted_ledger %-7s %s total %d; semantic-coff only: %s' % (u, dict(per[u]), sum(per[u].values()), sorted(only[u])))
