#!/usr/bin/env python3
"""Board-wide structural-versus-tie split for every non-exact function.

READ-ONLY DIAGNOSTIC. Reads build/split and build/base and writes nothing; it
recompiles nothing and is not part of any build. Added by the Lane A AI-core
lane on 2026-09-20 alongside
docs/object_matching_logs/claude_lane_a_structural_vs_tie_census_20260920.md,
which explains the three tiers, what each does and does not prove, and the three
traps that make a census like this lie.

Tiers: S1 the emitted reference multiset differs; S2 references agree but the
real instruction count differs; T both already agree. Only T makes "tie" the
default reading, and even S1 is not proof of source-reachability - the backend
can delete a reference by itself (see the cross-jump on
_ai_communication_finished).

Usage:  python tools/campaign/structural_split.py
"""
import collections
import json
import os
import struct

import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPLIT = os.path.join(ROOT, 'build', 'split')
BASE = os.path.join(ROOT, 'build', 'base')

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

_parked = json.load(open(os.path.join(ROOT, 'config', 'parked.json'), encoding='utf-8'))
PARKED = {(e['unit'], e['function']): e['class'] for e in _parked['entries']}


def read(path):
    """{function: (body, [relocation target names])} for every code COMDAT."""
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
        tgts = []
        sites = []
        for r in range(nreloc):
            ro = preloc + r * 10
            if ro + 10 > len(d):
                break
            ra, si, rt = struct.unpack_from('<IIH', d, ro)
            sy = syms[si] if si < len(syms) else None
            nm = sy[0] if sy else '?'
            tgts.append(nm)
            sites.append(ra)
        body = d[sptr:sptr + rawsize] if sptr else b''
        # NORMALIZE exactly as the comparator does: zero the relocated field at
        # every site. Comparing RAW bodies calls a function non-exact whenever a
        # relocated dword happens to hold a different link-time placeholder, which
        # silently pulls already-EXACT functions into the tie tier.
        norm = bytearray(body)
        for ra in sites:
            if ra + 4 <= len(norm):
                norm[ra:ra + 4] = bytes(4)
        bodies[s + 1] = (body, tgts, bytes(norm))
    out = {}
    for x in syms:
        if not x:
            continue
        nm, sn, cl, v = x
        if sn > 0 and v == 0 and sn in bodies and not nm.startswith('.') and nm not in out:
            body, tgts, norm = bodies[sn]
            # INTERNAL references are named differently by the two builds and are
            # NOT a program difference: ours emits `$L#####` jump-table labels
            # where January's split object relocates the same entries against the
            # enclosing function symbol itself. Counting either as a reference
            # inflates the structural tier - it put _ai_debug_render_actor at a
            # false 109-row deficit when its relocation count is in fact 1905 on
            # both sides.
            internal = (nm, nm + '_jmptable')
            out[nm] = (body, [t for t in tgts
                              if t not in internal and not t.startswith('$L')
                              and not t.startswith('$SG')], norm)
    return out


def real_code(body):
    """(instruction count, real code length) with trailing padding stripped.

    CAVEAT: a body with an EMBEDDED JUMP TABLE is partly data, and a linear
    disassembler counts those dwords as instructions. Treat insn_delta as
    unreliable wherever the function relocates against itself or a _jmptable
    symbol - read the frame and the reference multiset instead.
    """
    end = len(body)
    while end and body[end - 1] in (0x90, 0xcc):
        end -= 1
    b = body[:end]
    return sum(1 for _ in MD.disasm(b, 0)), end


rows = []
for dirpath, _dirs, files in os.walk(SPLIT):
    for f in files:
        if not f.endswith('.obj'):
            continue
        tpath = os.path.join(dirpath, f)
        rel = os.path.relpath(tpath, SPLIT).replace('\\', '/')
        opath = os.path.join(BASE, rel)
        if not os.path.exists(opath):
            continue
        ts = read(tpath)
        if not ts:
            continue
        os_ = read(opath)
        for fn, (tbody, trel, tnorm) in ts.items():
            got = os_.get(fn)
            if got is None:
                continue
            obody, orel, onorm = got
            if tnorm == onorm and collections.Counter(trel) == collections.Counter(orel):
                continue          # strictly exact on normalized bytes - skip
            ti, tlen = real_code(tbody)
            oi, olen = real_code(obody)
            tc, oc = collections.Counter(trel), collections.Counter(orel)
            reloc_delta = sum((tc - oc).values()) + sum((oc - tc).values())
            rows.append({
                'fn': fn, 'unit': rel[:-4], 'bytes': len(tbody),
                'insn_delta': oi - ti, 'len_delta': olen - tlen,
                'reloc_delta': reloc_delta,
                'park': PARKED.get((rel[:-4], fn)),
            })

ALIAS = 'csplit-relocation-alias'

# S1: the relocation MULTISET differs - a missing or extra program reference.
#     Near-proof of a structural gap. The csplit one-past-the-end alias class is
#     DEFINED by a relocation naming difference, so those parks are excluded:
#     they are expected here and are not evidence of anything.
# S2: relocations agree but the real instruction COUNT differs. A strong prior,
#     NOT proof: a pure backend decision can change instruction count on its own.
#     _ai_communication_finished is the campaign's own counterexample - nineteen
#     source shapes proved its -4 instructions is a cross-jump selection, not a
#     source difference.
# T : both already agree. Only here is "tie" the default reading.
s1 = [r for r in rows if r['reloc_delta'] and r['park'] != ALIAS]
s2 = [r for r in rows if (not r['reloc_delta'] or r['park'] == ALIAS) and r['insn_delta']]
ties = [r for r in rows if not r['insn_delta'] and (not r['reloc_delta'] or r['park'] == ALIAS)]
structural = s1 + s2


def tot(rs):
    return sum(r['bytes'] for r in rs)


print('non-exact functions compared : %d   (%d padded bytes)' % (len(rows), tot(rows)))
print()
print('S1  relocation multiset differs (missing/extra program reference)')
print('    %4d functions   %8d bytes' % (len(s1), tot(s1)))
print('S2  relocations agree, real instruction COUNT differs (strong prior, not proof)')
print('    %4d functions   %8d bytes' % (len(s2), tot(s2)))
print('T   both already agree - only here is "tie" the default reading')
print('    %4d functions   %8d bytes' % (len(ties), tot(ties)))
print()
for tier, rs in (('S1', s1), ('S2', s2), ('T', ties)):
    pk = [r for r in rs if r['park']]
    print('%s parked: %4d functions %8d bytes   %s'
          % (tier, len(pk), tot(pk), dict(collections.Counter(r['park'] for r in pk))))
print()
ps = [r for r in s1 + s2 if r['park']]
print('PARKED but not a tie by this evidence: %d functions, %d bytes'
      % (len(ps), tot(ps)))
print()
print('largest PARKED S1 rows - a park over a MISSING OR EXTRA REFERENCE:')
print('%-50s %-38s %8s %7s %7s %7s  %s'
      % ('function', 'unit', 'bytes', 'd-insn', 'd-len', 'd-reloc', 'park class'))
for r in sorted([r for r in s1 if r['park']], key=lambda r: -r['bytes'])[:30]:
    print('%-50s %-38s %8d %+7d %+7d %7d  %s'
          % (r['fn'][:50], r['unit'][:38], r['bytes'], r['insn_delta'],
             r['len_delta'], r['reloc_delta'], r['park']))
print()
print('largest UNPARKED S1 rows:')
for r in sorted([r for r in s1 if not r['park']], key=lambda r: -r['bytes'])[:25]:
    print('%-50s %-38s %8d %+7d %+7d %7d'
          % (r['fn'][:50], r['unit'][:38], r['bytes'], r['insn_delta'],
             r['len_delta'], r['reloc_delta']))
print()
print('largest TIE-CANDIDATE rows (same instructions, same references):')
for r in sorted(ties, key=lambda r: -r['bytes'])[:25]:
    print('%-50s %-38s %8d %+7d %+7d %7d  %s'
          % (r['fn'][:50], r['unit'][:38], r['bytes'], r['insn_delta'],
             r['len_delta'], r['reloc_delta'], r['park'] or '-'))
