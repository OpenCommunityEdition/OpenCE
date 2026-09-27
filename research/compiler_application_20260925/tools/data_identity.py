"""Independent data-section identity check (Q12). Own COFF reader; does not import tools/coff_compare.

usage: data_identity.py <unit> <section_name> [<owner_symbol>]
Compares January's split object (build/split/<unit>.obj) with ours (build/base/<unit>.obj) for the section that
holds <owner_symbol> (or the first section named <section_name>):
  - size, characteristics, payload with relocation fields zeroed, and every relocation (offset, type, target, addend)
  - owner symbols defined in the section: (name, value, storage class) on both sides
  - literal targets (??_C@...): our COMDAT bytes vs the bytes of January's linker-selected definition found by
    scanning every build/split object for a defined symbol of that name (must be exactly one distinct content)
Prints PASS/FAIL per item and the sha256 of both object files (evidence pins)."""
import hashlib
import struct
import sys
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
    return dict(b=b, secs=secs, syms=syms, byidx=byidx)


def sec_bytes(o, s):
    return bytearray(o['b'][s['raw']:s['raw'] + s['size']]) if s['raw'] else bytearray(s['size'])


def relocs(o, s):
    raw = sec_bytes(o, s)
    out = []
    for k in range(s['nrel']):
        va, si, t = struct.unpack_from('<IIH', o['b'], s['rel'] + k * 10)
        tgt = o['byidx'][si]
        add = struct.unpack_from('<i', raw, va)[0] if t in (6, 7, 20) else None
        # a target in the same section is spelled by section-relative position, not by name
        if tgt['sec'] == s['n']:
            key = ('self', tgt['value'] + (add or 0))
        else:
            key = ('sym', tgt['name'], add)
        out.append((va, t, key))
    for va, t, _ in out:
        raw[va:va + 4] = b'\0\0\0\0'
    return out, bytes(raw)


def find_section(o, sec_name, owner):
    if owner:
        for s in o['syms']:
            if s['name'] == owner and s['sec'] > 0:
                return o['secs'][s['sec'] - 1]
        raise SystemExit('owner %s not defined' % owner)
    return next(s for s in o['secs'] if s['name'] == sec_name)


def cstr(data):
    return data.split(b'\0', 1)[0] + b'\0'


def literal_bytes_ours(o, name):
    for s in o['syms']:
        if s['name'] == name and s['sec'] > 0:
            sec = o['secs'][s['sec'] - 1]
            return bytes(sec_bytes(o, sec)[s['value']:])
    return None


SPLIT_CACHE = None


def january_literal(name):
    global SPLIT_CACHE
    if SPLIT_CACHE is None:
        SPLIT_CACHE = [(p, p.read_bytes()) for p in Path('build/split').rglob('*.obj')]
    hits, owners = set(), []
    nb = name.encode('latin1')
    for p, b in SPLIT_CACHE:
        if nb not in b:
            continue
        o = rd(p)
        for s in o['syms']:
            if s['name'] == name and s['sec'] > 0:
                sec = o['secs'][s['sec'] - 1]
                hits.add(cstr(bytes(sec_bytes(o, sec)[s['value']:])))
                owners.append(p.as_posix())
    return hits, owners


def main():
    unit, sec_name = sys.argv[1], sys.argv[2]
    owner = sys.argv[3] if len(sys.argv) > 3 else None
    jp, op = Path('build/split/%s.obj' % unit), Path('build/base/%s.obj' % unit)
    J, O = rd(jp), rd(op)
    js, os_ = find_section(J, sec_name, owner), find_section(O, sec_name, owner)
    fails = [0]

    def check(label, ok, detail=''):
        fails[0] += 0 if ok else 1
        print('%-4s %s %s' % ('PASS' if ok else 'FAIL', label, detail))

    check('size', js['size'] == os_['size'], '%d vs %d' % (js['size'], os_['size']))
    check('characteristics', js['flags'] == os_['flags'], '%08x vs %08x' % (js['flags'], os_['flags']))
    jr, jraw = relocs(J, js)
    orr, oraw = relocs(O, os_)
    check('payload (relocation fields zeroed)', jraw == oraw, hashlib.sha256(oraw).hexdigest()[:16])
    check('relocation count', len(jr) == len(orr), '%d vs %d' % (len(jr), len(orr)))
    diffs = [(a, b) for a, b in zip(jr, orr) if a != b]
    check('relocations (offset,type,target,addend) identical', not diffs and len(jr) == len(orr),
          '' if not diffs else str(diffs[:3]))

    def owners(o, s):
        return sorted((x['name'], x['value'], x['cls']) for x in o['syms']
                      if x['sec'] == s['n'] and not x['name'].startswith('$') and x['name'] != s['name'])
    jo, oo = owners(J, js), owners(O, os_)
    check('owner symbols (name,offset,storage class)', jo == oo,
          '%d owners' % len(oo) if jo == oo else 'J-only %s O-only %s' % (
              sorted(set(jo) - set(oo))[:5], sorted(set(oo) - set(jo))[:5]))
    lits = sorted({k[1] for _, _, k in jr if k[0] == 'sym' and k[1].startswith('??_C@')})
    for name in lits:
        jdef = any(s['name'] == name and s['sec'] > 0 for s in J['syms'])
        odef = literal_bytes_ours(O, name)
        jhits, jown = january_literal(name)
        ours = cstr(odef) if odef is not None else None
        ok = len(jhits) == 1 and (ours is None or ours in jhits)
        check('literal %s' % name, ok, 'January %s (selected copy in %s); ours %s; %r' % (
            'defined here' if jdef else 'UNDEF here', ','.join(sorted(set(jown)))[:110] or '-',
            'defined (select-any)' if odef is not None else 'UNDEF', (ours or next(iter(jhits), b''))[:24]))
    print('pins: january %s %s | ours %s %s' % (
        jp.as_posix(), hashlib.sha256(jp.read_bytes()).hexdigest()[:16],
        op.as_posix(), hashlib.sha256(op.read_bytes()).hexdigest()[:16]))
    print('RESULT', 'PASS' if not fails[0] else 'FAIL (%d)' % fails[0])
    return 1 if fails[0] else 0


sys.exit(main())
