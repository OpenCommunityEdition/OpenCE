"""D1 strict whole-object comparison of two COFF objects (data only).

    python -B objcmp.py <a.obj> <b.obj> [--quiet]

Compares, per section in index order: name, size, characteristics, raw bytes, and every relocation as
(offset, type, target) where a target that is an external/static named symbol is its name, and a compiler label
($L..., $SG..., $S..., $$...) or section symbol is (section name, section ordinal-by-name, value). Then compares the
symbol table as a multiset (.debug$ sections are skipped: they hold the output path) of (name, storage, section name, value, type) with compiler labels replaced by
(section, value). Prints every difference; exit status 0 = identical under that normalisation.
"""
import collections
import re
import struct
import sys

sys.path.insert(0, r'C:\halo-worktrees\claude-compiler-application-20260925')
from tools import coff_compare as cc  # noqa: E402

LABEL = re.compile(r'^(\$L\d+|\$SG\d+|\$S\d+|\$\$\w+|\$[A-Za-z_]+\$\d*|\$state_not_allowed\$\d*)$')


def load(path):
    data = open(path, 'rb').read()
    o = cc.load(data)
    return data, o


def sec_key(o, secnum):
    if secnum <= 0:
        return ('abs' if secnum else 'undef',)
    s = o['sections'][secnum - 1]
    same = [x['index'] for x in o['sections'] if x['name'] == s['name']]
    return (s['name'], same.index(s['index']))


def sym_id(o, s):
    n = s.get('name') or ''
    if n.startswith('.') or LABEL.match(n) or (n.startswith('$') and s['storage'] == 3):
        return ('label',) + sec_key(o, s['section']) + (s['value'],)
    return ('sym', n)


def describe(path):
    data, o = load(path)
    secs = []
    for s in o['sections']:
        raw = cc._section_bytes(o, s) if hasattr(cc, '_section_bytes') else b''
        rel = []
        for ri in range(s['reloc_count']):
            a, ti, ty = struct.unpack_from('<IIH', data, s['reloc'] + ri * 10)
            rel.append((a, ty, sym_id(o, o['by_index'][ti]) if ti in o['by_index'] else ('?', ti)))
        secs.append(((s['name'],) + (s['size'], s['flags']), raw, tuple(rel)))
    syms = collections.Counter()
    for s in o['symbols']:
        sid = sym_id(o, s)
        syms[(sid, s['storage'], sec_key(o, s['section']), s['value'] if sid[0] == 'sym' else None, s['type'])] += 1
    return secs, syms


def main():
    a, b = sys.argv[1], sys.argv[2]
    quiet = '--quiet' in sys.argv
    sa, ya = describe(a)
    sb, yb = describe(b)
    diffs = []
    if len(sa) != len(sb):
        diffs.append('section count %d != %d' % (len(sa), len(sb)))
    for i, (x, y) in enumerate(zip(sa, sb)):
        if x[0][0] == y[0][0] and x[0][0].startswith('.debug$'):
            continue  # S_OBJNAME carries the output path; ignored (named in the docstring)
        if x[0] != y[0]:
            diffs.append('sec%d header %s != %s' % (i + 1, x[0], y[0]))
        if x[1] != y[1]:
            diffs.append('sec%d %s raw bytes differ' % (i + 1, x[0][0]))
        if x[2] != y[2]:
            diffs.append('sec%d %s relocations differ (%d vs %d)' % (i + 1, x[0][0], len(x[2]), len(y[2])))
    for k in sorted((ya - yb).elements(), key=str):
        diffs.append('symbol only in A: %s' % (k,))
    for k in sorted((yb - ya).elements(), key=str):
        diffs.append('symbol only in B: %s' % (k,))
    if not quiet:
        for d in diffs:
            print(d)
    print('objcmp %s: %d difference(s)' % ('IDENTICAL' if not diffs else 'DIFFERENT', len(diffs)))
    sys.exit(1 if diffs else 0)


if __name__ == '__main__':
    main()
