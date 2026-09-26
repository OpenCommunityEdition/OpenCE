"""Raw (non-resolved) disclosure diff of two COFF objects: section order, symbol-table differences and relocation
target SPELLINGS per section (coff_compare's hardened destination resolution can call two spellings EXACT; this
lists them so they can be disclosed).

    python rawdiff.py <before.obj> <after.obj>
"""
import re
import sys

sys.path.insert(0, r'C:\halo-worktrees\claude-compiler-application-20260925')
from tools import coff_compare as cc  # noqa: E402

LBL = re.compile(r'^\$L\d+$|^\$label\$\d+$')


def load(p):
    return cc.load(open(p, 'rb').read())


def owners(o):
    first = {}
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] in (2, 3) and not s['name'].startswith(('.', '$')):
            first.setdefault(s['section'], s['name'])
    return first


def relocs(o, secno):
    import struct
    sec = o['sections'][secno - 1]
    out = []
    for ri in range(sec['reloc_count']):
        roff = sec['reloc'] + ri * 10
        address, target_index, rtype = struct.unpack_from('<LLH', o['data'], roff)
        t = o['by_index'][target_index]
        name = t['name']
        if LBL.match(name):
            name = '$L'
        addend = struct.unpack_from('<i', o['data'], sec['raw'] + address)[0] if 'raw' in sec else None
        out.append((address, rtype, name, addend))
    return out


A, B = load(sys.argv[1]), load(sys.argv[2])
oa, ob = owners(A), owners(B)
order_a = [oa[n] for n in sorted(oa) if not A['sections'][n - 1]['name'].startswith('.debug')]
order_b = [ob[n] for n in sorted(ob) if not B['sections'][n - 1]['name'].startswith('.debug')]
print('== section order: %s' % ('identical' if order_a == order_b else 'DIFFERS'))
if order_a != order_b:
    moved = [n for i, n in enumerate(order_b) if i >= len(order_a) or order_a[i] != n]
    print('   first differing positions (after):', moved[:6])
sa = {s['name']: (s['storage'], s['value']) for s in A['symbols'] if s['section'] > 0 and not LBL.match(s['name'])}
sb = {s['name']: (s['storage'], s['value']) for s in B['symbols'] if s['section'] > 0 and not LBL.match(s['name'])}
print('== symbols only in before:', sorted(set(sa) - set(sb)))
print('== symbols only in after :', sorted(set(sb) - set(sa)))
chg = sorted(n for n in set(sa) & set(sb) if sa[n] != sb[n] and not n.startswith('.'))
print('== symbols with changed storage/value:', chg[:20])
inv_b = {v: k for k, v in ob.items()}
nspell = 0
for secno, owner in sorted(oa.items()):
    if owner not in inv_b:
        continue
    ra, rb = relocs(A, secno), relocs(B, inv_b[owner])
    if ra != rb:
        nspell += 1
        diffs = [(x, y) for x, y in zip(ra, rb) if x != y]
        print('   reloc spelling differs in %-50s %d of %d rows, e.g. %s' % (owner, len(diffs), len(ra), diffs[:2]))
print('== sections with raw relocation-spelling differences: %d' % nspell)
