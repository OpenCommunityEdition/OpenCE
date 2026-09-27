"""Read January's linked image (cachebeta.exe, data only, never executed): the reset body, the parameter getter's
lea displacement, submit's csmemcpy size, and the base-relocation entries that cover each absolute address."""
import struct
b = open('cachebeta.exe', 'rb').read()
pe = struct.unpack_from('<I', b, 0x3c)[0]
base = struct.unpack_from('<I', b, pe + 24 + 28)[0]
nsec = struct.unpack_from('<H', b, pe + 6)[0]; opt = struct.unpack_from('<H', b, pe + 20)[0]
secs = {}
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    name = b[o:o+8].rstrip(b'\0').decode()
    vsize, va, rsize, raw = struct.unpack_from('<IIII', b, o + 8)
    secs[name] = (va, vsize, raw, rsize)
def rd(rva, n):
    for name, (va, vs, raw, rs) in secs.items():
        if va <= rva < va + max(vs, rs):
            return b[raw + rva - va: raw + rva - va + n]
# base relocations
rva_reloc, size_reloc = struct.unpack_from('<II', b, pe + 24 + 96 + 5 * 8)
relocs = set()
p = secs['.reloc'][2]; end = p + size_reloc
while p < end:
    page, blk = struct.unpack_from('<II', b, p)
    if blk == 0: break
    for k in range((blk - 8) // 2):
        e = struct.unpack_from('<H', b, p + 8 + 2 * k)[0]
        if e >> 12 == 3:
            relocs.add(page + (e & 0xfff))
    p += blk
print('image base 0x%X, HIGHLOW base relocations %d' % (base, len(relocs)))
reset = 1509792; getter = 1509488; submit = 1511104
body = rd(reset, 0x30)
print('reset @0x%X: %s' % (base + reset, body.hex()))
imm = struct.unpack_from('<I', body, 1)[0]
res = struct.unpack_from('<I', body, 8)[0]
res2 = struct.unpack_from('<I', body, 0x19)[0]
cnt = struct.unpack_from('<I', body, 0x27)[0]
print('  push 0x%X (reloc@+1: %s); results VA 0x%X (reloc@+8: %s); results2 VA 0x%X; count VA 0x%X (reloc: %s)' % (
    imm, (reset + 1) in relocs, res, (reset + 8) in relocs, res2, cnt, (reset + 0x27) in relocs))
g = rd(getter, 0x3d)
i = g.find(bytes.fromhex('8d04808d04c5'))
par = struct.unpack_from('<I', g, i + 6)[0]
print('getter @0x%X: lea eax,[eax+eax*4]; lea eax,[eax*8+0x%X] at +0x%X (reloc: %s) => stride 0x28' % (
    base + getter, par, i + 3, (getter + i + 6) in relocs))
print('  params VA - results VA = 0x%X; overrun end = results + 0x%X = params + 0x%X' % (par - res, imm, res + imm - par))
s = rd(submit, 0x160)
j = s.find(bytes.fromhex('6a28'), 0x140)
print('submit @0x%X +0x%X: %s' % (base + submit, j, s[j:j+12].hex()))

# every base-relocated absolute address in the whole image that points into [results, count+4)
import json, bisect
syms = json.load(open('config/symbols.json'))
code = sorted((e['file_offset'], e['name']) for e in syms if e.get('flags') == 32)
offs = [c[0] for c in code]
LO, HI = res, cnt + 4
hits = []
for site in sorted(relocs):
    v = struct.unpack('<I', rd(site, 4))[0]
    if LO <= v < HI:
        k = bisect.bisect_right(offs, site) - 1
        fn = code[k][1] if k >= 0 else '?'
        what = 'results' if v < par else ('params' if v < cnt else 'count')
        hits.append((site, v, fn, what))
print('image-wide relocated references into [0x%X, 0x%X): %d' % (LO, HI, len(hits)))
from collections import Counter
print(' by function:', dict(Counter(h[2] for h in hits)))
span = [h for h in hits if par <= h[1] < par + (res + imm - par)]
print(' references landing in the overwritten span [0x%X, 0x%X):' % (par, res + imm))
for site, v, fn, what in span:
    print('   site 0x%X (%s+0x%X) -> 0x%X = params+0x%X' % (base + site, fn, site - offs[bisect.bisect_right(offs, site) - 1], v, v - par))
