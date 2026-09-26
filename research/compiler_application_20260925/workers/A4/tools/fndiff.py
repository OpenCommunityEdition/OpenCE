"""Instruction diff of one function between two objects (capstone), relocations annotated.
    python fndiff.py <a.obj> <b.obj> <symbol> [--all]"""
import sys
sys.path.insert(0, r'C:\halo-worktrees\claude-compiler-application-20260925\tools')
sys.path.insert(0, r'C:\tmp\halo-capstone')
import capstone
import coff_compare as cc
def dis(path, name):
    o = cc.load(open(path, 'rb').read())
    secs = o['sections']
    for s in o['symbols']:
        if s['name'] == name and s['section'] > 0 and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text':
            data = bytes(cc._section_bytes(o, secs[s['section'] - 1]))
            md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
            return ['%s %s' % (i.mnemonic, i.op_str) for i in md.disasm(data, 0)], [i.address for i in md.disasm(data, 0)]
    return [], []
a, aa = dis(sys.argv[1], sys.argv[3])
b, ba = dis(sys.argv[2], sys.argv[3])
n = 0
for k in range(max(len(a), len(b))):
    x = a[k] if k < len(a) else ''
    y = b[k] if k < len(b) else ''
    if x != y:
        n += 1
        if n <= 40 or '--all' in sys.argv:
            print('%4d %-5s %-40s | %s' % (k, hex(aa[k]) if k < len(aa) else '', x, y))
print('instructions %d/%d, differing positions %d' % (len(a), len(b), n))
