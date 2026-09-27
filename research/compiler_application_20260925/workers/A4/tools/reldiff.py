"""Show relocation/byte differences of one owner-keyed section between two objects.
    python reldiff.py <a.obj> <b.obj> <owner_symbol>"""
import struct, sys
sys.argv_saved = list(sys.argv)
def parse(path):
    raw = open(path, 'rb').read()
    machine, nsec, ts, symptr, nsym, opt, ch = struct.unpack_from('<HHIIIHH', raw, 0)
    strtab = raw[symptr + nsym * 18:]
    def sname(b):
        if b[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', b, 4)[0]
            return strtab[off:strtab.index(b'\0', off)].decode('latin-1')
        return b.rstrip(b'\0').decode('latin-1')
    syms = {}
    order = []
    i = 0
    while i < nsym:
        off = symptr + i * 18
        name, value, sec, typ, sclass, naux = struct.unpack_from('<8sIhHBB', raw, off)
        syms[i] = (sname(name), value, sec, sclass)
        order.append(i)
        i += 1 + naux
    secs = []
    for k in range(nsec):
        off = 20 + opt + k * 40
        name, vsize, vaddr, rsize, rptr, relptr, lnptr, nrel, nln, flags = struct.unpack_from('<8sIIIIIIHHI', raw, off)
        data = raw[rptr:rptr + rsize] if rptr else b''
        rels = [struct.unpack_from('<IIH', raw, relptr + r * 10) for r in range(nrel)]
        secs.append((data, [(va, syms[si][0], syms[si][1], rt) for va, si, rt in rels]))
    return secs, syms
a_secs, a_syms = parse(sys.argv[1]); b_secs, b_syms = parse(sys.argv[2])
def find(syms, name):
    for i, s in syms.items():
        if s[0] == name and s[2] > 0:
            return s[2]
na, nb = find(a_syms, sys.argv[3]), find(b_syms, sys.argv[3])
da, ra = a_secs[na - 1]; db, rb = b_secs[nb - 1]
print('bytes equal:', da == db, len(da), len(db))
for x, y in zip(ra, rb):
    if x != y:
        print('  a', x, '\n  b', y)
if len(ra) != len(rb):
    print('reloc count', len(ra), len(rb))
