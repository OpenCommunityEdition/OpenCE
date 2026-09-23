# Breakable-surface effect: genuine inline helpers (2026-09-23)

Base: `f21e001f` on `jonas/exact-pilots`. This packet changes only
`source/physics/breakable_surfaces.c`, `source/effects/decals.c`, and the
retired `_breakable_surface_effect` park. No math header or concurrent Claude
scope file changes.

The prior breakable body manually expanded `point_from_line3d` to suppress a
COMDAT, and manually expanded `projection_from_vector3d`. The January shape,
HCEX line 290, and the four `/Od` point-helper calls were documented in the
earlier breakable-surface and structural-lane ledgers. This packet restores
the existing `real_math.h` helpers in the caller. Decals previously renamed
the header's projection helper and hand-wrote an identical ordinary external
definition. Removing that shadow provider lets the genuine shared-header
inline supply its 80-byte SELECT_ANY copy without changing any decals code
owner bytes.

Proof on the isolated build:

- Full `ninja` passes. `gate.py` reports `_breakable_surface_effect` EXACT
  4,032 padded bytes (target/candidate normalized SHA-256
  `510486a7d0a1f2fc1dfb8f0c47990c57f83ce1030ef5e4db8dfc42943dc56bfa`)
  and `_projection_from_vector3d` EXACT 80 bytes.
- Stable whole-board snapshots from identical `f21e001f` source trees:
  8,245 rows / 7,586 exact to 7,587 exact; only the breakable effect gained,
  zero regressions. The Halo meaningful-code report rises by 4,020 bytes;
  padded strict gain is 4,032 bytes.
- COFF owner census: decals retains the same 55 code owners with identical
  normalized bytes and relocation counts. Breakable changes only the effect
  owner and adds `_point_from_line3d` and `_projection_from_vector3d`.
  January-selected-provider pair links for these two newly emitted helpers
  pass in both input orders. Four unrelated decals duplicate-provider failures
  are inherited and do not qualify as new admissible ownership.
- Parks: 103 to 102 active, zero stale/invalid. Admission audit has zero
  contradicted. `pytest`: 1,154 passed, 5 skipped, 26 subtests. The fake scan
  has no new leads; its one decals assembly finding was present at base.

This is function credit, not automatic whole-object admission. Although the
breakable unit now reports 12/12 functions and 685/685 data, its other
pre-existing provider conflicts still require a separate ownership audit
before adding `metadata.complete` to `objdiff.json`.
