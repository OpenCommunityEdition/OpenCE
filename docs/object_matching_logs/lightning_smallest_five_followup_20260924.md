# Lightning smallest-object follow-up (2026-09-24)

Base: `4d1ebf179023f46efb77929fb599e514601152fb`. This packet corrects
storage ownership; it does **not** claim a new exact function or a completed
object. `lightning.obj` remains 8/9 strict exact, with `_lightning_submit`
parked at zero credit (2,576 padded bytes, 80 relocations, 756 instructions).

The 2026-09-25 Lane C research packet independently identified
`_lightning_globals` as file-static in the historical PDB (DataKind 5), and
the symbol is absent from the historical public-symbol list. The previous
external declaration in `lightning.h` had no consumer outside `lightning.c`.
The source now defines it `static`, the unused header declaration is removed,
and the single `symbols.json` entry is marked static. The rebuilt object
records the global with COFF storage class 3. No function body, data bytes,
or section size changed.

I read the current September 2025 Lightning worker ledger before probing.
It already tested more than 100 source shapes and identified the last
`_lightning_submit` discrepancy as a C2 x87-store/index-reload scheduling
transpose after a separately owner-gated 3D point-to-vector view copy. Two
new ordinary-source probes were negative and fully reverted:

- A named shared `real texture_x = u + u_offset` produced a different
  normalized body hash (`bc07e3e1…`), not January's hash.
- Writing the second vertex as `vertices[1]` and advancing by two altered
  the pointer-offset instruction stream and left the reload after the x87
  store. It was not a byte-close alternative.

The historical `/Od` evidence favours genuine `point_from_line3d` calls in
the vertex pair, while the current body hand-expands that helper. The
folded-inline COMDAT policy admits a new copy only with a strict-exact caller,
so this correction remains held with the fuzzy function; it was not silently
landed. Likewise, the owner-gated view cast and any fabricated scheduling
dependency remain unlanded.

Verification after the storage correction: `ninja progress` passed; the
whole-unit gate stayed 8 exact / 1 residual / 0 unwritten; parks were 83
active / 0 stale / 0 invalid; admission audit had 0 contradictions; the
Lightning fake-match scan reported 0 findings; and pytest passed 1,161 tests,
5 skipped, 26 subtests. `git diff --check` was clean.

Reopen `_lightning_submit` only with a demonstrated natural VC7 source form
that detaches the second vertex's `texture.x` x87 store from the adjacent
`fstp st(0)` without moving color or texture-y stores, or a first-party
January-era source fragment. The complete do-not-repeat inventory remains in
`research/fifty_objects_20260925/w/lightning/LEDGER.md`.
