import xbe, capstone, collections, sys
X = xbe.Xbe(r"C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\work\astra-inputs\earlier-map-archives\2001-09-25\cachebeta.xbe")
T = 0x11000
fns = {"render_debug_add_cache_string": (0x175f70, 0xd0), "render_debug_add_cache_entry": (0x176310, 0x290), "render_debug": (0x1780a0, 0x240)}
refs = collections.defaultdict(list)
for name, (off, size) in fns.items():
    for ins in X.dis(T + off, size):
        for op in ins.operands:
            if op.type == capstone.x86.X86_OP_MEM and op.mem.disp and X.sec_of(op.mem.disp & 0xffffffff) in ('.data',):
                refs[op.mem.disp & 0xffffffff].append((name, ins.address - T - off, op.size, ins.mnemonic + ' ' + ins.op_str))
            if op.type == capstone.x86.X86_OP_IMM and X.sec_of(op.imm & 0xffffffff) in ('.data',):
                refs[op.imm & 0xffffffff].append((name, ins.address - T - off, 'imm', ins.mnemonic + ' ' + ins.op_str))
lo = min(refs)
for v in sorted(refs):
    print(hex(v), '+%04x' % (v - lo))
    for r in refs[v]:
        print('    %-32s +%04x w=%s %s' % r)
