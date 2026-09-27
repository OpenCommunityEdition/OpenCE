import odpe, capstone, collections, sys
sva, t = odpe.text()
lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
dlo, dhi = int(sys.argv[3], 16), int(sys.argv[4], 16)
starts = []
for i in range(lo - sva, hi - sva):
    if t[i:i+3] == b'\x55\x8b\xec' and t[i-1] in (0xcc, 0xc3, 0xc2, 0x90):
        starts.append(sva + i)
starts.append(hi)
refs = collections.defaultdict(list)
for a, b in zip(starts, starts[1:]):
    off = odpe.va2off(a)
    for ins in odpe.MD.disasm(odpe.B[off:off + (b - a)], a):
        for op in ins.operands:
            if op.type == capstone.x86.X86_OP_MEM and op.mem.base == 0 and dlo <= (op.mem.disp & 0xffffffff) <= dhi:
                refs[op.mem.disp & 0xffffffff].append((a, ins.address, op.size, '%s %s' % (ins.mnemonic, ins.op_str)))
            elif op.type == capstone.x86.X86_OP_MEM and dlo <= (op.mem.disp & 0xffffffff) <= dhi:
                refs[op.mem.disp & 0xffffffff].append((a, ins.address, op.size, '%s %s' % (ins.mnemonic, ins.op_str)))
            elif op.type == capstone.x86.X86_OP_IMM and dlo <= (op.imm & 0xffffffff) <= dhi:
                refs[op.imm & 0xffffffff].append((a, ins.address, 'imm', '%s %s' % (ins.mnemonic, ins.op_str)))
print('functions:', len(starts) - 1, 'first', hex(starts[0]), 'last', hex(starts[-2]))
for v in sorted(refs):
    print(hex(v))
    for f, at, w, txt in refs[v]:
        print('    fn %x  @%x  w=%s  %s' % (f, at, w, txt))
