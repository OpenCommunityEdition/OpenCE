#!/usr/bin/env python3
"""Where is the unmatched DATA?

READ-ONLY DIAGNOSTIC. Reads build/split and build/base, writes nothing,
recompiles nothing, is imported by nothing and runs in no build step. Added by
the Lane A AI-core lane on 2026-09-20 alongside
docs/object_matching_logs/claude_lane_a_data_census_20260920.md.

The board's data gap is far larger than its code gap (halobetacache: 60.2% of
data bytes matched, about 1.56 MB outstanding, against roughly 326 KB of code).
This answers whether that is a pool of cheap near-misses or work not yet done.

It is work not yet done. Over 4,010,867 data-COMDAT bytes in 524 objects,
50.2% is symbols ABSENT from our build (1,681 symbols) and only 0.5% (13
symbols) is present-but-not-identical - and ALL THIRTEEN are SIZE mismatches.
Not one data symbol on the board differs only in its contents.

Usage:  python tools/campaign/data_census.py
"""
import collections
import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPLIT = os.path.join(ROOT, 'build', 'split')
BASE = os.path.join(ROOT, 'build', 'base')

DATA_SECTIONS = ('.data', '.rdata', '.bss')


def datasecs(path):
    """{symbol: (section name, normalized bytes, size)} for data COMDATs."""
    try:
        d = open(path, 'rb').read()
    except OSError:
        return None
    if len(d) < 20:
        return None
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
        return None
    base = 20 + opt
    bodies = {}
    for s in range(nsec):
        off = base + s * 40
        if off + 40 > len(d):
            break
        nm = d[off:off + 8].rstrip(b'\x00').decode('ascii', 'replace')
        if nm.startswith('/'):
            try:
                so = int(nm[1:])
                end = d.index(b'\x00', strtab + so)
                nm = d[strtab + so:end].decode('ascii', 'replace')
            except Exception:
                pass
        vsize, vaddr, rawsize, sptr, preloc, plnum, nreloc, nln, chars = \
            struct.unpack_from('<IIIIIIHHI', d, off + 8)
        short = nm.split('$')[0]
        if short not in DATA_SECTIONS:
            continue
        body = d[sptr:sptr + rawsize] if sptr else b'\x00' * rawsize
        norm = bytearray(body)
        for r in range(nreloc):
            ro = preloc + r * 10
            if ro + 10 > len(d):
                break
            ra, si, rt = struct.unpack_from('<IIH', d, ro)
            if ra + 4 <= len(norm):
                norm[ra:ra + 4] = bytes(4)
        bodies[s + 1] = (short, bytes(norm), rawsize)
    out = {}
    for x in syms:
        if not x:
            continue
        nm, sn, cl, v = x
        if sn > 0 and v == 0 and sn in bodies and not nm.startswith('.') and nm not in out:
            out[nm] = bodies[sn]
    return out


units = []
for dirpath, _dirs, files in os.walk(SPLIT):
    for f in files:
        if not f.endswith('.obj'):
            continue
        tpath = os.path.join(dirpath, f)
        rel = os.path.relpath(tpath, SPLIT).replace('\\', '/')
        opath = os.path.join(BASE, rel)
        ts = datasecs(tpath)
        if ts is None:
            continue
        have = os.path.exists(opath)
        os_ = datasecs(opath) if have else {}
        if os_ is None:
            os_ = {}
        total = miss_absent = miss_diff = match = 0
        absent_syms = diff_syms = 0
        for sym, (sec, tnorm, size) in ts.items():
            total += size
            got = os_.get(sym)
            if got is None:
                miss_absent += size
                absent_syms += 1
            elif got[1] != tnorm or got[2] != size:
                miss_diff += size
                diff_syms += 1
            else:
                match += size
        if total:
            units.append({'unit': rel[:-4], 'built': have, 'total': total,
                          'match': match, 'absent': miss_absent, 'diff': miss_diff,
                          'absent_syms': absent_syms, 'diff_syms': diff_syms})

T = sum(u['total'] for u in units)
M = sum(u['match'] for u in units)
A = sum(u['absent'] for u in units)
D = sum(u['diff'] for u in units)
print('data COMDAT bytes over %d objects: %d total' % (len(units), T))
print('  byte-identical (normalized) : %9d  (%.1f%%)' % (M, 100.0 * M / T))
print('  symbol ABSENT from our build : %9d  (%.1f%%)   %d symbols'
      % (A, 100.0 * A / T, sum(u['absent_syms'] for u in units)))
print('  present but DIFFERENT        : %9d  (%.1f%%)   %d symbols'
      % (D, 100.0 * D / T, sum(u['diff_syms'] for u in units)))
print()
built = [u for u in units if u['built']]
print('restricted to objects we actually build (%d of %d):' % (len(built), len(units)))
bt = sum(u['total'] for u in built)
print('  total %d   identical %d (%.1f%%)   absent %d   different %d'
      % (bt, sum(u['match'] for u in built),
         100.0 * sum(u['match'] for u in built) / bt,
         sum(u['absent'] for u in built), sum(u['diff'] for u in built)))
print()
print('objects we build whose data is PRESENT BUT DIFFERENT, largest first')
print('(this is the cheap pool: the symbol exists, only its contents differ)')
print('%-56s %9s %9s %7s' % ('unit', 'diff B', 'total B', 'symbols'))
for u in sorted([u for u in built if u['diff']], key=lambda u: -u['diff'])[:30]:
    print('%-56s %9d %9d %7d' % (u['unit'][:56], u['diff'], u['total'], u['diff_syms']))
print()
print('objects we build with data symbols ABSENT, largest first')
print('%-56s %9s %9s %7s' % ('unit', 'absent B', 'total B', 'symbols'))
for u in sorted([u for u in built if u['absent']], key=lambda u: -u['absent'])[:20]:
    print('%-56s %9d %9d %7d' % (u['unit'][:56], u['absent'], u['total'], u['absent_syms']))
