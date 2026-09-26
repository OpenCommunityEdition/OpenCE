"""E1 helper (read-only): list /Od functions that CALL a target VA (E8 rel32 scan within Ghidra function bodies),
and the callees of a function.  python -B od_callers.py callers <va_hex> | callees <va_hex>"""
import struct, sys, re, bisect
from pathlib import Path
import capstone
D = Path(r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906')
EXE = D / 'halo_cache_symbols.exe'
b = EXE.read_bytes()
pe = struct.unpack_from('<I', b, 0x3c)[0]
nsec = struct.unpack_from('<H', b, pe + 6)[0]
opt = struct.unpack_from('<H', b, pe + 20)[0]
base = struct.unpack_from('<I', b, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    vsize, va, rsize, raw = struct.unpack_from('<IIII', b, o + 8)
    secs.append((base + va, rsize, raw))
def va2off(va):
    for sva, rs, raw in secs:
        if sva <= va < sva + rs:
            return raw + va - sva
funcs = {}
for line in open(str(EXE) + '.functions.tsv'):
    p = line.rstrip('\n').split('\t')
    if p[0] == 'entry':
        continue
    funcs[int(p[0], 16)] = [(int(s, 16), int(e, 16)) for s, e in re.findall(r'\[([0-9a-f]+), ([0-9a-f]+)\]', p[3])]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
def thunk_target(va):
    off = va2off(va)
    if b[off] == 0xE9:
        return va + 5 + struct.unpack_from('<i', b, off + 1)[0]
    return va
def calls_in(f):
    out = []
    for s, e in funcs[f]:
        off = va2off(s)
        for ins in md.disasm(b[off:off + (e - s + 1)], s):
            if ins.mnemonic == 'call' and ins.op_str.startswith('0x'):
                out.append((ins.address, thunk_target(int(ins.op_str, 16))))
    return out
mode, tgt = sys.argv[1], int(sys.argv[2], 16)
if mode == 'callees':
    for a, t in calls_in(tgt):
        print('%08x call %08x' % (a, t))
else:
    for f in sorted(funcs):
        for a, t in calls_in(f):
            if t == tgt:
                print('fn %08x  site %08x' % (f, a))
