"""Tree-wide naming-gap scan: target _code_* sections whose bytes exist in our
base object of the same unit under a real name, with the real name attested
nowhere else at a conflicting address requirement.  Reports (unit, code name,
our name, size, exact-if-renamed)."""
import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

d = json.load(open('objdiff.json'))
found = []
for u in d['units']:
    tp, bp = u.get('target_path'), u.get('base_path')
    if not tp or not os.path.exists(tp) or not bp or not os.path.exists(bp):
        continue
    try:
        t = cc.load(open(tp, 'rb').read())
        b = cc.load(open(bp, 'rb').read())
    except Exception:
        continue
    tsecs, bsecs = t['sections'], b['sections']
    # our base: real-named .text sections
    ours = {}
    for s in b['symbols']:
        if (s['name'].startswith('_') and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and bsecs[s['section'] - 1]['name'] == '.text'
                and not s['name'].startswith('_code_')):
            try:
                si = cc.section_info(b, s['name'])
            except Exception:
                continue
            ours.setdefault((si['size'], si['normalized_sha256']), []).append(s['name'])
    for s in t['symbols']:
        if not (s['name'].startswith('_code_') and s['section'] > 0 and s['storage'] in (2, 3) and s['value'] == 0):
            continue
        if tsecs[s['section'] - 1]['name'] != '.text':
            continue
        try:
            ti = cc.section_info(t, s['name'])
        except Exception:
            continue
        # skip if our base also defines this code_ name (already written under that name)
        try:
            if cc.section_info(b, s['name']) is not None:
                continue
        except Exception:
            pass
        key = (ti['size'], ti['normalized_sha256'])
        if key in ours:
            found.append((u['name'], s['name'], ours[key], ti['size']))

for f in found:
    print(f)
print(len(found), 'candidates')
