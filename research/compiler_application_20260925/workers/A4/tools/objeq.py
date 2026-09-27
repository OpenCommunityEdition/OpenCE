"""Compare two COFF objects ignoring only the file-header TimeDateStamp.
    python objeq.py <a.obj> <b.obj> [--quiet]
Prints IDENTICAL, or the list of differing sections/symbols (by section owner symbol name)."""
import struct, sys
NORM = '--norm-labels' in sys.argv
def parse(path):
    raw = open(path, 'rb').read()
    machine, nsec, ts, symptr, nsym, opt, ch = struct.unpack_from('<HHIIIHH', raw, 0)
    strtab = raw[symptr + nsym * 18:]
    def sname(b):
        if b[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', b, 4)[0]
            end = strtab.index(b'\0', off)
            return strtab[off:end].decode('latin-1')
        return b.rstrip(b'\0').decode('latin-1')
    syms = []
    i = 0
    while i < nsym:
        off = symptr + i * 18
        name, value, sec, typ, sclass, naux = struct.unpack_from('<8sIhHBB', raw, off)
        aux = raw[off + 18: off + 18 + 18 * naux]
        syms.append((i, sname(name), value, sec, typ, sclass, aux))
        i += 1 + naux
    byidx = {s[0]: s for s in syms}
    secs = []
    for k in range(nsec):
        off = 20 + opt + k * 40
        name, vsize, vaddr, rsize, rptr, relptr, lnptr, nrel, nln, flags = struct.unpack_from('<8sIIIIIIHHI', raw, off)
        nm = name.rstrip(b'\0').decode('latin-1')
        if nm.startswith('/'):
            nm = strtab[int(nm[1:]):strtab.index(b'\0', int(nm[1:]))].decode('latin-1')
        data = raw[rptr:rptr + rsize] if rptr else b''
        rels = []
        for r in range(nrel):
            va, si, rt = struct.unpack_from('<IIH', raw, relptr + r * 10)
            nm2 = byidx[si][1] if si in byidx else si
            if NORM and isinstance(nm2, str) and nm2.startswith('$L'):
                nm2 = ('$L', byidx[si][2], byidx[si][3])
            rels.append((va, nm2, rt))
        secs.append((nm, flags, data, rels))
    return secs, syms
def owner_keys(secs, syms):
    first = {}
    for s in syms:
        if s[3] > 0 and not s[1].startswith('.') and not s[1].startswith('$') and s[5] in (2, 3):
            first.setdefault(s[3], s[1])
    keys = []
    cnt = {}
    for k, sec in enumerate(secs, 1):
        if k in first:
            keys.append('sym:' + first[k])
        else:
            c = cnt.get(sec[0], 0); cnt[sec[0]] = c + 1
            keys.append('sec:%s#%d' % (sec[0], c))
    return keys
a_secs, a_syms = parse(sys.argv[1])
b_secs, b_syms = parse(sys.argv[2])
ak, bk = owner_keys(a_secs, a_syms), owner_keys(b_secs, b_syms)
amap = {k: s for k, s in zip(ak, a_secs)}
bmap = {k: s for k, s in zip(bk, b_secs)}
diffs = []
for k in sorted(set(amap) | set(bmap)):
    if '.debug$' in k:
        continue
    if k not in amap: diffs.append('ADDED   ' + k); continue
    if k not in bmap: diffs.append('REMOVED ' + k); continue
    x, y = amap[k], bmap[k]
    what = []
    if x[1] != y[1]: what.append('flags %08x/%08x' % (x[1], y[1]))
    if x[2] != y[2]: what.append('bytes(%d/%d)' % (len(x[2]), len(y[2])))
    if x[3] != y[3]: what.append('relocs')
    if what: diffs.append('CHANGED %s %s' % (k, ' '.join(what)))
def symset(syms):
    return sorted((s[1], s[2], s[5]) for s in syms if s[3] > 0 and not s[1].startswith('$'))
sa, sb = symset(a_syms), symset(b_syms)
if sa != sb:
    diffs.append('SYMBOLS differ: only-a %s only-b %s' % (sorted(set(sa) - set(sb))[:8], sorted(set(sb) - set(sa))[:8]))
if not diffs:
    print('IDENTICAL (%d sections)' % len(a_secs))
else:
    print('DIFFERENT: %d' % len(diffs))
    if '--quiet' not in sys.argv:
        for d in diffs[:60]:
            print('  ' + d)
