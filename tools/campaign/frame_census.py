#!/usr/bin/env python3
"""Board-wide frame-size census: every function whose `sub esp,N` differs.

READ-ONLY DIAGNOSTIC. Reads build/split (January) and build/base (ours) and
writes nothing; it recompiles nothing and is not part of any build. Added by the
Lane A AI-core lane on 2026-09-20 alongside
docs/object_matching_logs/claude_lane_a_frame_mismatch_census_20260920.md, which
explains what the output means and what it does not prove.

A differing `sub esp,N` is STRUCTURAL - the set of homed locals differs, a
local's scope or type - so every row is a source-reachable lead, not a tie. At
the time of writing it flagged 56 functions worth 101,056 padded bytes, 35 of
them currently parked as compiler ties.

Usage:  python tools/campaign/frame_census.py
"""
import collections
import os
import struct
import sys

import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPLIT = os.path.join(ROOT, 'build', 'split')
BASE = os.path.join(ROOT, 'build', 'base')

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def sections(path):
    """{function name: (body bytes, padded size)} for every code COMDAT."""
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
        if not (chars & 0x20):          # IMAGE_SCN_CNT_CODE
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


def esp(body):
    """The prologue's `sub esp,N`, or None."""
    for k, ins in enumerate(MD.disasm(body, 0)):
        if k > 12:
            return None
        if ins.mnemonic == 'sub' and ins.op_str.replace(' ', '').startswith('esp,'):
            try:
                return int(ins.op_str.split(',')[1].strip(), 0)
            except ValueError:
                return None
        if ins.mnemonic == 'call':
            return None
    return None


rows = []
scanned = 0
for dirpath, _dirs, files in os.walk(SPLIT):
    for f in files:
        if not f.endswith('.obj'):
            continue
        tpath = os.path.join(dirpath, f)
        rel = os.path.relpath(tpath, SPLIT)
        opath = os.path.join(BASE, rel)
        if not os.path.exists(opath):
            continue
        ts = sections(tpath)
        if not ts:
            continue
        os_ = sections(opath)
        scanned += 1
        for fn, tbody in ts.items():
            obody = os_.get(fn)
            if obody is None or tbody == obody:
                continue
            a, b = esp(tbody), esp(obody)
            if a is None or b is None or a == b:
                continue
            rows.append((b - a, fn, rel.replace('\\', '/'), a, b, len(tbody)))

import json
_parked = json.load(open(os.path.join(ROOT, 'config', 'parked.json'), encoding='utf-8'))
PARKED = {(e['unit'], e['function']): e['class'] for e in _parked['entries']}


def parked_of(unit_obj, fn):
    return PARKED.get((unit_obj[:-4] if unit_obj.endswith('.obj') else unit_obj, fn))


rows.sort(key=lambda r: (-r[5]))
print('objects scanned: %d      functions with a FRAME MISMATCH: %d' % (scanned, len(rows)))
print()
hist = collections.Counter(r[0] for r in rows)
print('delta histogram (ours minus January, bytes):')
for delta, count in sorted(hist.items(), key=lambda kv: -kv[1])[:25]:
    bar = '#' * min(count, 60)
    print('  %+6d  x%-4d %s' % (delta, count, bar))
print()
tot12 = sum(r[5] for r in rows if r[0] == 12)
tot = sum(r[5] for r in rows)
print('padded bytes behind ALL frame mismatches : %d' % tot)
print('padded bytes behind the +12 class alone  : %d' % tot12)
print()
pk = [r for r in rows if parked_of(r[2], r[1])]
print('PARKED functions carrying a frame mismatch : %d  (%d padded bytes)'
      % (len(pk), sum(r[5] for r in pk)))
print('  ...of which class=instruction-scheduling : %d'
      % sum(1 for r in pk if parked_of(r[2], r[1]) == 'instruction-scheduling'))
print('  ...of which class=register-allocation    : %d'
      % sum(1 for r in pk if parked_of(r[2], r[1]) == 'register-allocation'))
print('  ...of which class=unclassified           : %d'
      % sum(1 for r in pk if parked_of(r[2], r[1]) == 'unclassified'))
print()
print('every frame-mismatched function, largest first:')
print('%-50s %-40s %7s %7s %8s %6s  %s'
      % ('function', 'unit', 'jan', 'ours', 'bytes', 'delta', 'park class'))
for delta, fn, unit, a, b, size in rows:
    cls = parked_of(unit, fn)
    print('%-50s %-40s %7s %7s %8d %+6d  %s'
          % (fn[:50], unit[:-4][:40], hex(a), hex(b), size, delta, cls or '-'))
