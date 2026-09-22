#!/usr/bin/env python3
"""Rank a directory's residuals by how close each one ACTUALLY is.

READ-ONLY DIAGNOSTIC. Recompiles each unit once through gate.py to get a current
candidate object, then reads only. Writes nothing into the source tree.

One row per residual:
  bytes         padded size of January's function
  frame         January `sub esp,N` against ours, flagged `!` when they differ -
                structural by campaign law
  refs          external reference multiset difference, with internal labels,
                `<fn>_jmptable` and the function's own symbol excluded
  real code     where real code ENDS on each side with padding stripped, and the
                delta - this is the number that survives the padding trap
  REGIONS       differing regions after removing relocation-spelling,
                relocation-site, branch-immediate and padding-only noise, with
                the raw alndiff count in parentheses

Ranked by REAL regions ascending, then bytes descending, so the top of the table
is the cheapest work with the most bytes behind it. That ordering matters: the
raw region count systematically OVERSTATES the closest functions (see
real_regions.py), so ranking by it buries the cheap work.

Read the `real code` delta together with the region count. `0 (12)` with a
non-zero delta means every instruction's TEXT matches and the gap is encoding
length or position - usually a short-versus-near branch, which is a cross-jump
binding question, not a missing instruction.

Usage:
    python tools/campaign/residual_triage.py                 # source/ai
    python tools/campaign/residual_triage.py source/units
"""
import collections
import os
import re
import struct
import subprocess
import sys

import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

DIR = sys.argv[1] if len(sys.argv) > 1 else 'source/ai'
UNITS = sorted(f[:-2] for f in os.listdir(os.path.join(ROOT, DIR)) if f.endswith('.c'))


def parse(path):
    d = open(path, 'rb').read()
    nsec = struct.unpack_from('<H', d, 2)[0]
    ptr = struct.unpack_from('<I', d, 8)[0]
    n = struct.unpack_from('<I', d, 12)[0]
    opt = struct.unpack_from('<H', d, 16)[0]
    strtab = ptr + n * 18
    syms = []
    i = 0
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
    base = 20 + opt
    bodies = {}
    for s in range(nsec):
        off = base + s * 40
        vsize, vaddr, rawsize, sptr, preloc, plnum, nreloc, nln, chars = \
            struct.unpack_from('<IIIIIIHHI', d, off + 8)
        if not (chars & 0x20):
            continue
        tg = []
        for r in range(nreloc):
            ro = preloc + r * 10
            ra, si, rt = struct.unpack_from('<IIH', d, ro)
            sy = syms[si] if si < len(syms) else None
            tg.append(sy[0] if sy else '?')
        bodies[s + 1] = (d[sptr:sptr + rawsize] if sptr else b'', tg, rawsize)
    out = {}
    for x in syms:
        if not x:
            continue
        nm, sn, cl, v = x
        if sn > 0 and v == 0 and sn in bodies and not nm.startswith('.') and nm not in out:
            body, tg, rawsize = bodies[sn]
            internal = (nm, nm + '_jmptable')
            out[nm] = (body, [t for t in tg if t not in internal
                              and not t.startswith('$L') and not t.startswith('$SG')],
                       rawsize)
    return out


def shape(body):
    end = len(body)
    while end and body[end - 1] in (0x90, 0xcc):
        end -= 1
    esp = None
    for k, ins in enumerate(MD.disasm(body, 0)):
        if ins.mnemonic == 'sub' and ins.op_str.replace(' ', '').startswith('esp,'):
            try:
                esp = int(ins.op_str.split(',')[1].strip(), 0)
            except ValueError:
                pass
            break
        if k > 10:
            break
    return esp, end


rows = []
for unit in UNITS:
    obj = os.path.join(ROOT, 'scratch', 'orch', 'triage_%s.obj' % unit)
    subprocess.run([sys.executable, 'tools/campaign/gate.py', DIR + '/' + unit,
                    '--all', '--out', os.path.relpath(obj, ROOT)],
                   capture_output=True, text=True, cwd=ROOT)
    out = subprocess.run([sys.executable, 'tools/campaign/gate.py',
                          DIR + '/' + unit, '--all'],
                         capture_output=True, text=True, cwd=ROOT).stdout
    residuals = [l.split()[2] for l in out.splitlines() if l.startswith('residual')]
    if not residuals:
        continue
    T = parse(os.path.join(ROOT, 'build/split/%s/%s.obj' % (DIR, unit)))
    O = parse(obj)
    for fn in residuals:
        tb, tr, tsize = T[fn]
        ob, orr, osize = O[fn]
        te, oe = shape(tb), shape(ob)
        tc, oc = collections.Counter(tr), collections.Counter(orr)
        refs = sum((tc - oc).values()) + sum((oc - tc).values())
        rr = subprocess.run([sys.executable, 'tools/campaign/real_regions.py',
                             DIR + '/' + unit, fn, '--ours-object',
                             os.path.relpath(obj, ROOT)],
                            capture_output=True, text=True, cwd=ROOT).stdout
        m = re.search(r'(\d+) reported, (\d+) REAL', rr)
        reported, real = (int(m.group(1)), int(m.group(2))) if m else (-1, -1)
        rows.append({'fn': fn, 'unit': unit, 'bytes': tsize,
                     'jesp': te[0], 'oesp': oe[0], 'jend': te[1], 'oend': oe[1],
                     'refs': refs, 'reported': reported, 'real': real})

rows.sort(key=lambda r: (r['real'], -r['bytes']))
print('%-44s %-20s %7s %-15s %5s %14s %8s'
      % ('function', 'unit', 'bytes', 'frame j/ours', 'refs', 'real code j/o', 'REGIONS'))
print('-' * 126)
for r in rows:
    frame = '%s/%s' % (hex(r['jesp']) if r['jesp'] is not None else '-',
                       hex(r['oesp']) if r['oesp'] is not None else '-')
    if r['jesp'] != r['oesp']:
        frame += ' !'
    code = '%d/%d' % (r['jend'], r['oend'])
    if r['jend'] != r['oend']:
        code += ' %+d' % (r['oend'] - r['jend'])
    print('%-44s %-20s %7d %-15s %5d %14s %4d (%d)'
          % (r['fn'][:44], r['unit'][:20], r['bytes'], frame, r['refs'], code,
             r['real'], r['reported']))
print()
print('%d residuals, %d padded bytes' % (len(rows), sum(r['bytes'] for r in rows)))
print('cheapest-first: the top rows are the fewest REAL differing regions with '
      'the most bytes behind them.')
