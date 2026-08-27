# -*- coding: utf-8 -*-
"""Static map of C2.Dll's optimization-diagnostic facility (VC7 13.00.9254).

Derived by PE-aware string/xref analysis 2026-08-09. Documents WHY the
compiler's own optimization narration cannot be used to expose the x87
instruction-selection numbering behind ai_debug's residual ties.

Findings (addresses are preferred-base VAs, imagebase 0x10700000):

  logger              0x1082c892  formats into a 0x2084 stack buffer, then
                                  calls the sink 0x1082c8fb
  sink gate           0x1082c8fb  reads global [0x1088c2c0]; if null -> return
                                  -1 (NOTHING printed). This is the master gate.
  diag-file opener    0x1082c790  fopen([0x1088c8d4] name, mode "a") -> stored
                                  in [0x1088c2c0]. Only reached on the
                                  PGO/LTCG report path (mode global
                                  [0x1088c304]==2, alt name "ltcg.obj").
  message pool        0x1085dc00..0x1085e420  the OPT:/PEEP:/INL:/CNT:/INF:
                                  format strings (pogocg.c, P2 pass).
  cmp-reorder peephole 0x1083b5e0..0x1083b6ab  emits
                                  "PEEP: Comparisons reordered near line %lu
                                  (%I64u vs %I64u)". The two %I64u values are
                                  64-bit block execution frequencies (struct
                                  +0x28/+0x2c, propagated by add into edi-block
                                  and sub from esi-block) -- PGO PROFILE COUNTS.

Conclusions:
  1. The whole narration facility is welded to PGO/LTCG whole-program mode
     (report file opened only there). Per-TU /O2 + /FAcs emits ZERO narration
     (verified empirically).
  2. The one reorder peephole it narrates is profile-frequency branch
     ordering -- a DIFFERENT axis from the x87 consume-vs-memory / operand-
     load-order instruction-selection tie behind code_0003a910 and
     ai_debug_render_actor.
  3. Therefore c2.dll exposes no built-in readout of the numbering that
     decides our ties. Reading that numbering requires a live debugger inside
     the x87 selection/scheduling pass, not any diagnostic switch.

Run this script to re-verify the addresses against the shipped C2.Dll.
"""
import struct, sys, os

C2 = os.path.join(os.path.dirname(__file__), '..', 'xbox', 'bin', 'vc7', 'C2.Dll')


def main():
    data = open(C2, 'rb').read()
    ib = struct.unpack_from('<I', data, struct.unpack_from('<I', data, 0x3c)[0] + 24 + 28)[0]
    checks = {
        'PEEP: Comparisons reordered': b'Comparisons reordered near line',
        'INL: Inlining ... but can\'t': b"%s but can't: %s",
        'OPT: Before/After': b'%s  Before: %I64d  After: %I64d',
        'pogocg.c path': b'pogocg.c',
        'ltcg.obj alt name': b'ltcg.obj',
    }
    print('imagebase %#x' % ib)
    for label, needle in checks.items():
        fo = data.find(needle)
        print('  %-32s file=%#08x va=%#010x %s'
              % (label, fo, ib + fo, 'OK' if fo > 0 else 'MISSING'))
    # gate global present in the sink
    gate = struct.pack('<I', 0x1088c2c0)
    n = data.count(gate)
    print('  gate global [0x1088c2c0] referenced %d times' % n)


if __name__ == '__main__':
    main()
