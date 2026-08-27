"""Tree-wide flag-provenance scan from callee relocation identities.

Level-1 evidence (research/flag_provenance_findings.md): the CRT helper a unit
calls proves which per-unit flags its TU carried.

  __ftol2 present in ours, absent in target  ->  target TU had /QIfist
  __ftol2 present in target, absent in ours  ->  target TU had NO /QIfist
  _fmod   vs __CIfmod (and other __CI*)      ->  /Op vs no /Op

Prints one line per disagreeing unit with its current cflags, so the fix is a
config/config.json edit.
"""
import sys, json, os
from collections import Counter
sys.path.insert(0, 'tools')
import coff_compare as cc

PAIRS = [('__ftol2', '/QIfist'), ('__ftol', '/QIfist')]
CI = ['__CIfmod', '__CIsqrt', '__CIsin', '__CIcos', '__CIatan2', '__CIacos', '__CIasin', '__CIpow', '__CIexp', '__CIlog', '__CItan']
LIB = ['_fmod', '_sqrt', '_sin', '_cos', '_atan2', '_acos', '_asin', '_pow', '_exp', '_log', '_tan']


def helper_counts(path):
    o = cc.load(open(path, 'rb').read())
    secs = o['sections']
    c = Counter()
    for s in o['symbols']:
        if not (s['name'].startswith('_') and s['section'] > 0 and s['storage'] in (2, 3) and s['value'] == 0):
            continue
        if secs[s['section'] - 1]['name'] != '.text':
            continue
        try:
            si = cc.section_info(o, s['name'])
        except Exception:
            continue
        for r in si['relocations']:
            t = r.get('symbolic_target') or r.get('target')
            n = str(t[1])
            if n in ('__ftol2', '__ftol') or n in CI or n in LIB:
                c[n] += 1
    return c


cfg = json.load(open('config/config.json'))
flags = {}


def walk(o):
    if isinstance(o, dict):
        if 'name' in o and isinstance(o.get('name'), str) and o['name'].endswith('.c'):
            flags[o['name'][:-2]] = o.get('options', {}).get('cflags')
        for v in o.values():
            walk(v)
    elif isinstance(o, list):
        for x in o:
            walk(x)


walk(cfg)

d = json.load(open('objdiff.json'))
findings = []
for u in d['units']:
    tp, bp = u.get('target_path'), u.get('base_path')
    if not tp or not os.path.exists(tp) or not bp or not os.path.exists(bp):
        continue
    try:
        t, b = helper_counts(tp), helper_counts(bp)
    except Exception:
        continue
    verdicts = []
    for h in ('__ftol2', '__ftol'):
        if b[h] and not t[h]:
            verdicts.append(f'ADD /QIfist  (ours {b[h]} x {h}, target 0)')
        elif t[h] and not b[h]:
            verdicts.append(f'REMOVE /QIfist  (target {t[h]} x {h}, ours 0)')
    for ci, lib in zip(CI, LIB):
        if t[ci] and b[lib] and not b[ci]:
            verdicts.append(f'REMOVE /Op  (target {ci}, ours {lib})')
        if t[lib] and b[ci] and not t[ci]:
            verdicts.append(f'ADD /Op  (target {lib}, ours {ci})')
    if verdicts:
        findings.append((u['name'], flags.get(u['name']), verdicts))

for name, cf, v in findings:
    print(f'{name}')
    print(f'    current cflags: {cf if cf else "(default)"}')
    for x in v:
        print(f'    -> {x}')
print(f'\n{len(findings)} units with flag-provenance disagreements')
