#!/usr/bin/env python3
"""Board-wide census of a machine-code IDIOM, January against ours, per function.

READ-ONLY DIAGNOSTIC. Reads build/split (January) and build/base (ours), writes
nothing, recompiles nothing, is imported by nothing and runs in no build step.

The idea generalizes a finding from the Lane A wave of 2026-09-20. Once you have
decoded a source-level lever, you usually know the byte pattern it emits. Count
that pattern per function in both trees and every disagreement is another
instance of the same lever - a precise worklist, found without recompiling
anything.

Worked example, the built-in `square` set. VC7 has two shapes for "multiply a
value by itself" depending on which x87 stack slot the value is in:

    d9 c0 d8 c9     fld st(0) ; fmul st(1), st      square the top of stack
    d9 c1 d8 ca     fld st(1) ; fmul st(2), st      square the deeper operand

Which one January emits is decided by the source's expression form. Counting
them found 18 functions where our counts disagree with January's, across AI,
physics, bitmaps, interface, structures and units - one decoded lever, an
eighteen-function worklist.

Usage:
    python tools/campaign/idiom_census.py                 # the `square` set
    python tools/campaign/idiom_census.py --set fdiv      # another built-in set
    python tools/campaign/idiom_census.py --bytes d9c0d8c9,d9c1d8ca
    python tools/campaign/idiom_census.py --set square --unit source/ai

CAVEAT that matters. Our build carries out-of-line COMDAT copies of header
`__inline` helpers that January's split objects do not have, so "only in ours"
rows are dominated by that known extra-COMDAT gap and are NOT a signal. Only the
present-in-both-but-different rows are. The script separates them for you.

A byte pattern can also occur inside an embedded jump table or other data, so
treat a single-count disagreement on a function with a jump table with care.
"""
import argparse
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPLIT = os.path.join(ROOT, 'build', 'split')
BASE = os.path.join(ROOT, 'build', 'base')

SETS = {
    # "x * x" - which x87 slot the value sits in is decided by the source form.
    'square': ['d9c0d8c9', 'd9c1d8ca'],
    # reciprocal-then-multiply versus repeated divide: January averages a point
    # with one `fdivr 1.0f` and three `fmul`, never three `fdiv`.
    'fdiv': ['d838', 'd83d', 'd87d'],
    # integer-to-float conversion of a loop counter
    'fild': ['db45', 'db04', 'db44'],
    # the 32-bit inline fistp that is never compiler output (fast_ftol's shape)
    'fistp': ['db5d', 'db1d'],
}


def parse(path):
    try:
        d = open(path, 'rb').read()
    except OSError:
        return {}
    if len(d) < 20:
        return {}
    nsec = struct.unpack_from('<H', d, 2)[0]
    ptr = struct.unpack_from('<I', d, 8)[0]
    n = struct.unpack_from('<I', d, 12)[0]
    opt = struct.unpack_from('<H', d, 16)[0]
    strtab = ptr + n * 18
    syms = []
    i = 0
    try:
        while i < n:
            off = ptr + i * 18
            raw = d[off:off + 8]
            value, secnum, typ, sclass, naux = struct.unpack_from('<IhHBB', d, off + 8)
            if raw[:4] == b'\x00\x00\x00\x00':
                so = struct.unpack_from('<I', raw, 4)[0]
                end = d.index(b'\x00', strtab + so)
                nm = d[strtab + so:end].decode('ascii', 'replace')
            else:
                nm = raw.rstrip(b'\x00').decode('ascii', 'replace')
            syms.append((nm, secnum, sclass, value))
            for _ in range(naux):
                syms.append(None)
            i += 1 + naux
    except Exception:
        return {}
    base = 20 + opt
    bodies = {}
    for s in range(nsec):
        off = base + s * 40
        if off + 40 > len(d):
            break
        vsize, vaddr, rawsize, sptr, preloc, plnum, nreloc, nln, chars = \
            struct.unpack_from('<IIIIIIHHI', d, off + 8)
        if not (chars & 0x20):
            continue
        bodies[s + 1] = d[sptr:sptr + rawsize] if sptr else b''
    out = {}
    for x in syms:
        if not x:
            continue
        nm, sn, cl, v = x
        if sn > 0 and v == 0 and sn in bodies and not nm.startswith('.') and nm not in out:
            out[nm] = bodies[sn]
    return out


def count(body, pats):
    c = []
    for p in pats:
        n = 0
        i = body.find(p)
        while i != -1:
            n += 1
            i = body.find(p, i + 1)
        c.append(n)
    return tuple(c)


def census(root, pats, unit_filter):
    d = {}
    for dp, _dirs, fs in os.walk(root):
        for f in fs:
            if not f.endswith('.obj'):
                continue
            path = os.path.join(dp, f)
            rel = os.path.relpath(path, root).replace('\\', '/')[:-4]
            if unit_filter and not rel.startswith(unit_filter):
                continue
            for fn, body in parse(path).items():
                c = count(body, pats)
                if any(c):
                    d[(rel, fn)] = c
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--set', default='square', choices=sorted(SETS),
                    help='built-in idiom set (default: square)')
    ap.add_argument('--bytes', help='comma-separated hex patterns, overrides --set')
    ap.add_argument('--unit', default='', help='restrict to units under this prefix')
    a = ap.parse_args()

    hexpats = a.bytes.split(',') if a.bytes else SETS[a.set]
    try:
        pats = [bytes.fromhex(h.strip()) for h in hexpats]
    except ValueError:
        sys.exit('patterns must be hex, e.g. --bytes d9c0d8c9,d9c1d8ca')

    print('idiom: %s' % ', '.join(h.strip() for h in hexpats))
    j = census(SPLIT, pats, a.unit)
    o = census(BASE, pats, a.unit)
    common = set(j) & set(o)
    diff = sorted(k for k in common if j[k] != o[k])
    only_j = sorted(k for k in j if k not in o)
    only_o = sorted(k for k in o if k not in j)

    print('functions carrying it: January %d, ours %d, in both %d'
          % (len(j), len(o), len(common)))
    print('  present in both and AGREEING  : %d' % (len(common) - len(diff)))
    print('  present in both and DIFFERING : %d   <- the worklist' % len(diff))
    print('  only January                  : %d' % len(only_j))
    print('  only ours                     : %d   (mostly the known extra-COMDAT'
          ' copies of header inlines - NOT a signal)' % len(only_o))
    print()
    print('PRESENT IN BOTH, COUNTS DIFFER:')
    print('%-44s %-48s %-14s %s' % ('unit', 'function', 'January', 'ours'))
    for k in diff:
        print('%-44s %-48s %-14s %s' % (k[0][:44], k[1][:48], j[k], o[k]))
    if only_j:
        print()
        print('ONLY IN JANUARY (we never emit it here):')
        for k in only_j[:40]:
            print('%-44s %-48s %s' % (k[0][:44], k[1][:48], j[k]))


if __name__ == '__main__':
    main()
