# `draw_string.obj` tab-stop Opus reconciliation (2026-09-03)

## Result

`_draw_string_set_tab_stops` is now strict exact: 112 padded bytes, eight
relocations with matching identities, and matching normalized code.  The
whole-object gate advances from 18 to 19 exact functions; its nine existing
residuals are unchanged.

The interrupted Opus lineage exposed the source shape, but its old snapshot
still called the owned storage `bss_004c1908`.  That address-derived identifier
was not imported.  Canonical had already recovered the correct semantic
`draw_string_globals` owner and the target split now carries the same name, so
the exact body was reconciled directly onto that owner.

## Retained source

The setter validates the short count against
`MAXIMUM_NUMBER_OF_TAB_STOPS`, assigns the bounded value with the project
`MIN` macro, and copies the requested short entries when the stored count is
positive.  Reading the stored count for the copy length naturally preserves
January's second globals access; no dummy read or optimizer-only alias is
needed.  This is the same coherent bounded-array behavior already supported by
the later HCEA reconstruction and the existing object ledger.

The function retains one parameter per line and an explicit final `return;`.
It introduces no header, prototype, type, global, linkage, owned-data, or
inline-schedule change.  It contains no raw address/offset, volatile or
register forcing, pragma, barrier, assembly, undefined behavior, fake
dependency, or nonsensical control path.  The rebuilt object contains no
`point_from_line3d` symbol or COMDAT.

## Verification

- focused gate: `_draw_string_set_tab_stops` strict exact;
- whole-object gate: 19 exact / 9 residual / 0 unwritten;
- rename-stable full-tree comparison: +1 exact / +112 padded bytes / zero
  regressions across 8,245 functions;
- full `ninja` build: success and zero semantic unit errors;
- progress: 830,069 / 2,198,102 meaningful code bytes and 5,811 / 11,060 code
  functions;
- the now-stale park was removed with the repository unpark tool: 185 active /
  0 stale / 0 invalid;
- admission: 0 candidates / 0 contradicted; two inherited reviewed rejections
  unchanged;
- fake-match scan: zero review leads;
- tooling tests: 261 passed;
- protected bitmap working-file hashes unchanged;
- `git diff --check`: clean.

Stable comparison:

- before: `scratch/after-object-lights-combined-20260903.json`
- after: `scratch/after-draw-string-tab-20260903.json`
