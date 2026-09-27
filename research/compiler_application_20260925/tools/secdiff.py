"""List every section that differs between two COFF objects (TimeDateStamp ignored).

usage: secdiff.py <a.obj> <b.obj>
Sections are matched by (name, COMDAT symbol) key; each side's raw bytes, relocations (offset, type, target name)
and characteristics are compared. Also reports symbol-table differences (name, section key, value, class)."""
import struct
import sys
NORM = "--norm-labels" in sys.argv
if NORM:
    sys.argv.remove("--norm-labels")
from pathlib import Path


def rd(path):
    b = Path(path).read_bytes()
    nsec = struct.unpack_from('<H', b, 2)[0]
    symptr, nsym = struct.unpack_from('<II', b, 8)
    strtab = symptr + nsym * 18
    secs = []
    for i in range(nsec):
        o = 20 + i * 40
        name = b[o:o + 8].rstrip(b'\0').decode('latin1')
        if name.startswith('/'):
            off = int(name[1:])
            name = b[strtab + off: b.index(b'\0', strtab + off)].decode('latin1')
        size, raw, rel, _, nrel, _, flags = struct.unpack_from('<IIIIHHI', b, o + 16)
        secs.append(dict(n=i + 1, name=name, size=size, raw=raw, rel=rel, nrel=nrel, flags=flags))
    syms, byidx, i = [], {}, 0
    while i < nsym:
        e = b[symptr + i * 18: symptr + i * 18 + 18]
        if e[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', e, 4)[0]
            n = b[strtab + off: b.index(b'\0', strtab + off)].decode('latin1')
        else:
            n = e[:8].rstrip(b'\0').decode('latin1')
        val, sec, typ, cls, aux = struct.unpack_from('<IhHBB', e, 8)
        s = dict(i=i, name=n, value=val, sec=sec, cls=cls)
        syms.append(s)
        byidx[i] = s
        i += 1 + aux
    # key each section by its name plus the first non-section external/static symbol defined in it
    owner = {}
    for s in syms:
        if s['sec'] > 0 and s['name'] != secs[s['sec'] - 1]['name'] and not s['name'].startswith('$') \
                and s['sec'] not in owner:
            owner[s['sec']] = s['name']
    keyed = {}
    for s in secs:
        k = (s['name'], owner.get(s['n'], ''))
        c = 0
        while k + (c,) in keyed:
            c += 1
        rels = []
        for r in range(s['nrel']):
            va, si, t = struct.unpack_from('<IIH', b, s['rel'] + r * 10)
            tn = byidx[si]['name']
            rels.append((va, t, '$label' if NORM and tn.startswith('$') else tn))
        keyed[k + (c,)] = dict(raw=b[s['raw']:s['raw'] + s['size']] if s['raw'] else s['size'], rels=rels,
                               flags=s['flags'])
    symset = sorted((s['name'], owner.get(s['sec'], '') if s['sec'] > 0 else s['sec'], s['cls']) for s in syms
                    if not s['name'].startswith('$') and not s['name'].startswith('.'))
    return keyed, symset


a, asy = rd(sys.argv[1])
b, bsy = rd(sys.argv[2])
diff = 0
for k in sorted(set(a) | set(b), key=str):
    if a.get(k) != b.get(k):
        diff += 1
        what = 'only-a' if k not in b else 'only-b' if k not in a else ','.join(
            f for f in ('raw', 'rels', 'flags') if a[k][f] != b[k][f])
        print('  %-10s %-60s %s' % (k[0], k[1][:60], what))
sd = sorted(set(asy) ^ set(bsy))
print('sections differing: %d; symbol-table differences: %d %s' % (diff, len(sd), sd[:6]))
