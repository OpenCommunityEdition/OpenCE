"""Drop parked entries whose function now matches exactly.

ninja's parked-function verification fails when a parked function turns out to be
byte-exact ("evidence is stale or invalid"), which is what happens every time a
branch sweep or a lane merge brings in a version that closes one. Run this after
adopting other lanes' work.

usage: python tools/campaign/unpark.py [--dry-run]
"""
import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

dry = '--dry-run' in sys.argv
d = json.load(open('objdiff.json'))
paths = {}
for u in d['units']:
    paths[u['name']] = (u.get('target_path'), u.get('base_path'))

# Classes that are byte-exact BY CONSTRUCTION, not because the gap closed: an
# assembly transcription always matches. Unparking one would silently re-credit
# it as a C reconstruction, which is the opposite of why it was parked. See
# research/asm_credited_functions_audit.md - the owner parked these deliberately.
NEVER_UNPARK = ('asm-implemented', 'vendored-assembly')

p = 'config/parked.json'
pj = json.load(open(p))
keep, drop = [], []
for e in pj['entries']:
    unit, fn = e['unit'].strip(), e['function']
    if e.get('class') in NEVER_UNPARK:
        keep.append(e)
        continue
    tp, bp = paths.get(unit, (None, None))
    exact = False
    if tp and bp and os.path.exists(tp) and os.path.exists(bp):
        try:
            ti = cc.section_info(cc.load(open(tp, 'rb').read()), fn)
            bi = cc.section_info(cc.load(open(bp, 'rb').read()), fn)
            exact = ti is not None and bi is not None and cc.section_infos_equal(ti, bi)
        except Exception:
            exact = False
    (drop if exact else keep).append(e)

for e in drop:
    print(f'now exact, unparking: {e["unit"].strip()}:{e["function"]}')
print(f'== {len(drop)} unparked, {len(keep)} still parked')
if drop and not dry:
    pj['entries'] = keep
    with open(p, 'w') as f:
        json.dump(pj, f, indent='\t')
        f.write('\n')
    print(f'wrote {p}')
