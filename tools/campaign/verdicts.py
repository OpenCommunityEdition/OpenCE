"""Dump a per-function verdict map for the whole tree, for regression diffing.

Writes {"<unit>::<fn>": "E"|"R"|"U"} to the given path. Take a snapshot before a
merge or a risky edit, then diff after: any E -> R/U transition is a regression
and must be resolved before committing.

usage: python tools/campaign/verdicts.py scratch/v_now.json
       python tools/campaign/verdicts.py --diff scratch/v_before.json scratch/v_now.json
"""
import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc


def dump(out_path):
    d = json.load(open('objdiff.json'))
    v = {}
    for u in d['units']:
        tp, bp = u.get('target_path'), u.get('base_path')
        if not tp or not os.path.exists(tp):
            continue
        target = cc.load(open(tp, 'rb').read())
        base = cc.load(open(bp, 'rb').read()) if (bp and os.path.exists(bp)) else None
        secs = target['sections']
        seen = set()
        for sym in target['symbols']:
            name = sym['name']
            if not name.startswith('_') or name in seen:
                continue
            if sym['section'] <= 0 or sym['storage'] not in (2, 3) or sym['value'] != 0:
                continue
            if secs[sym['section'] - 1]['name'] != '.text' or sym['section'] in seen:
                continue
            seen.add(sym['section'])
            try:
                ti = cc.section_info(target, name)
            except Exception:
                continue
            if ti is None:
                continue
            seen.add(name)
            st = 'U'
            if base is not None:
                try:
                    bi = cc.section_info(base, name)
                except Exception:
                    bi = None
                if bi is not None:
                    st = 'E' if cc.section_infos_equal(ti, bi) else 'R'
            v[f"{u['name']}::{name}"] = st
    json.dump(v, open(out_path, 'w'))
    n_e = sum(1 for x in v.values() if x == 'E')
    print(f'wrote {out_path}: {len(v)} functions, {n_e} exact')


def diff(before_path, after_path):
    a = json.load(open(before_path))
    b = json.load(open(after_path))
    gained = sorted(k for k in b if b[k] == 'E' and a.get(k) != 'E')
    lost = sorted(k for k in a if a[k] == 'E' and b.get(k, 'U') != 'E')
    for k in lost:
        print(f'REGRESSION  {k}  E -> {b.get(k, "GONE")}')
    for k in gained:
        print(f'gained      {k}  {a.get(k, "NEW")} -> E')
    print(f'== gained {len(gained)}   REGRESSIONS {len(lost)}')
    return 1 if lost else 0


if sys.argv[1] == '--diff':
    sys.exit(diff(sys.argv[2], sys.argv[3]))
dump(sys.argv[1])
