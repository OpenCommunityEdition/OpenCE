import odpe, capstone, sys
sva, t = odpe.text()
lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
starts = []
for i in range(lo - sva, hi - sva):
    if t[i:i+3] == b'\x55\x8b\xec' and t[i-1] in (0xcc, 0xc3, 0xc2, 0x90):
        starts.append(sva + i)
starts.append(hi)
def cstr(va):
    off = odpe.va2off(va)
    if off is None: return None
    e = odpe.B.find(b'\0', off, off + 80)
    if e < 0: return None
    s = odpe.B[off:e]
    if len(s) >= 3 and all(32 <= c < 127 for c in s): return s.decode()
    return None
for a, b in zip(starts, starts[1:]):
    off = odpe.va2off(a)
    strs, calls = [], []
    for ins in odpe.MD.disasm(odpe.B[off:off + (b - a)], a):
        if ins.mnemonic == 'push' and ins.operands[0].type == capstone.x86.X86_OP_IMM:
            s = cstr(ins.operands[0].imm & 0xffffffff)
            if s: strs.append(s[:40])
        if ins.mnemonic == 'call' and ins.operands[0].type == capstone.x86.X86_OP_IMM:
            calls.append(ins.operands[0].imm)
    print('%x (%4x) strs=%s' % (a, b - a, strs[:4]))
