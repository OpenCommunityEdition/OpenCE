"""E1 helper (read-only, data only): locate /Od functions in halo_cache_symbols.exe by string xref.
    python -B od_find.py <substring> [--exe cache|tool|sapien]
Prints each string occurrence VA, then every function (from the Ghidra functions.tsv) whose body contains an
imm32 equal to that VA (push/mov/lea imm)."""
import struct, sys, re, bisect
from pathlib import Path
D = Path(r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906')
which = 'cache'
args = [a for a in sys.argv[1:]]
if '--exe' in args:
    which = args[args.index('--exe') + 1]
    del args[args.index('--exe'):args.index('--exe') + 2]
EXE = D / {'cache': 'halo_cache_symbols.exe', 'tool': 'tool_symbols.exe', 'sapien': 'sapien_symbols.exe'}[which]
b = EXE.read_bytes()
pe = struct.unpack_from('<I', b, 0x3c)[0]
nsec = struct.unpack_from('<H', b, pe + 6)[0]
opt = struct.unpack_from('<H', b, pe + 20)[0]
base = struct.unpack_from('<I', b, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    name = b[o:o + 8].rstrip(b'\0').decode('latin-1')
    vsize, va, rsize, raw = struct.unpack_from('<IIII', b, o + 8)
    secs.append((name, base + va, rsize, raw))
def off2va(off):
    for n, va, rs, raw in secs:
        if raw <= off < raw + rs:
            return va + off - raw
funcs = []
for line in open(str(EXE) + '.functions.tsv'):
    p = line.rstrip('\n').split('\t')
    if p[0] == 'entry':
        continue
    rngs = re.findall(r'\[([0-9a-f]+), ([0-9a-f]+)\]', p[3])
    for s, e in rngs:
        funcs.append((int(s, 16), int(e, 16), int(p[0], 16)))
funcs.sort()
starts = [f[0] for f in funcs]
def fn_of(va):
    i = bisect.bisect_right(starts, va) - 1
    if i >= 0 and funcs[i][0] <= va <= funcs[i][1]:
        return funcs[i][2]
needle = args[0].encode('latin-1')
pos = 0
hits = []
while True:
    k = b.find(needle, pos)
    if k < 0:
        break
    # walk back to string start
    s = k
    while s > 0 and b[s - 1] != 0:
        s -= 1
    e = b.find(b'\0', k)
    hits.append((off2va(s), b[s:e].decode('latin-1')))
    pos = k + 1
seen = set()
for va, txt in hits:
    if va in seen or va is None:
        continue
    seen.add(va)
    print('STRING %08x %r' % (va, txt[:120]))
    imm = struct.pack('<I', va)
    q = 0
    fs = {}
    while True:
        k = b.find(imm, q)
        if k < 0:
            break
        cva = off2va(k)
        f = fn_of(cva) if cva else None
        if f:
            fs.setdefault(f, []).append(cva)
        q = k + 1
    for f, sites in sorted(fs.items()):
        print('   fn %08x  xrefs %s' % (f, ' '.join('%08x' % x for x in sites)))
