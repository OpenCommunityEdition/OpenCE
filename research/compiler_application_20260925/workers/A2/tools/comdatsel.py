"""A2 lab tool (read-only): print the COMDAT selection of named .text sections in a COFF object.

    python -B comdatsel.py <obj> <symbol> [<symbol> ...]

Reads the section-definition auxiliary record of each section symbol (IMAGE_AUX_SYMBOL.Section.Selection):
1 NODUPLICATES, 2 ANY, 3 SAME_SIZE, 4 EXACT_MATCH, 5 ASSOCIATIVE, 6 LARGEST.
"""
import struct
import sys

SEL = {0: 'none', 1: 'NODUPLICATES', 2: 'ANY', 3: 'SAME_SIZE', 4: 'EXACT_MATCH', 5: 'ASSOCIATIVE', 6: 'LARGEST'}


def main():
    b = open(sys.argv[1], 'rb').read()
    nsec = struct.unpack_from('<H', b, 2)[0]
    symoff, nsym = struct.unpack_from('<II', b, 8)
    stroff = symoff + 18 * nsym

    def name(raw):
        if raw[:4] == b'\0\0\0\0':
            o = struct.unpack_from('<I', raw, 4)[0]
            e = b.index(b'\0', stroff + o)
            return b[stroff + o:e].decode('latin-1')
        return raw.rstrip(b'\0').decode('latin-1')

    sec_sel = {}
    first_name = {}
    i = 0
    while i < nsym:
        o = symoff + 18 * i
        raw = b[o:o + 8]
        value, secnum, typ, cls, naux = struct.unpack_from('<IhHBB', b, o + 8)
        n = name(raw)
        if naux and cls == 3 and value == 0 and n.startswith('.') and secnum > 0:
            ao = o + 18
            length, nrel, nline, chk, num, sel = struct.unpack_from('<IHHIHB', b, ao)
            sec_sel[secnum] = sel
        elif secnum > 0 and not n.startswith('.') and not n.startswith('$') and cls in (2, 3):
            first_name.setdefault(n, (secnum, cls))
        i += 1 + naux
    for want in sys.argv[2:]:
        if want not in first_name:
            print('%-40s not defined' % want)
            continue
        secnum, cls = first_name[want]
        print('%-40s section %3d storage %d selection %s' % (want, secnum, cls, SEL.get(sec_sel.get(secnum, 0), sec_sel.get(secnum))))


if __name__ == '__main__':
    main()
