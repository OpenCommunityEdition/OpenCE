# `real_alpha_to_pixel32` owner reconciliation (2026-09-03)

## Result

The authenticated shared `real_alpha_to_pixel32` inline now lives in its
January-proven owner, `source/bitmaps/bitmaps_inlines.h`.  Every current direct
caller includes that owner instead of receiving a misplaced declaration from
`interface/hud_draw.h`.

This closes `_rasterizer_water_build_bumpmap` exactly and moves
`rasterizer_xbox_water.obj` from 12 exact / 2 residual to 13 exact / 1
residual.  The recovered function contributes 2,080 strict padded bytes and
138 relocations.  A full-tree stable-verdict sweep reports exactly one gained
target function, 2,080 gained padded bytes, and zero regressions across 8,245
tracked functions.

`_rasterizer_water_draw` remains an honest residual at 2,320 candidate bytes
versus 2,240 January bytes and 173 versus 171 relocations.  No source-shape
chase was made as part of this recovery.

## First-party authority and source shape

January retains `_real_alpha_to_pixel32` as a 112-byte, six-relocation owner
in `rasterizer_xbox_active_camouflage.obj`.  Its assertion records
`..\\bitmaps\\bitmaps_inlines.h:291`, and the byte-visible expression is
`alpha>=0.0f && alpha<=1.0f`.  The packet further proves a stack-materialized
`255.0f`, x87 multiply, 32-bit `FISTP` into a named local, in-memory shift by
24, and return of that top-byte value.

The earlier natural-C boundary is recorded in
`rasterizer_xbox_active_camouflage_obj_jonas_real_alpha_boundary_20260830.md`:
ordinary casts lower through `__ftol2`, while `/QIfist` uses a 64-bit
conversion.  The compact x87 block is therefore not register or optimizer
steering.  It is the smallest source form that directly expresses January's
authenticated helper packet.  This use is within the campaign rule permitting
sparse inline assembly in low-level math/helper functions.

The definition uses the canonical `real` and `pixel32` types, declares and
initializes the scale on one line, places its parameter on its own line, and
ends with an explicit return.  It introduces no raw address, raw structure
offset, type pun, volatile/register qualifier, pragma, compiler barrier,
dummy reachability, forced inline/noinline directive, or nonsensical branch.

## Header ownership and blast radius

`bitmaps_inlines.h` is now self-contained for the types and assertion macros
used by the helper.  The stale declaration was removed from `hud_draw.h`, and
the four live callers include the bitmap owner directly:

- `rasterizer_xbox_water.c`;
- `rasterizer_xbox_dynavobgeom.c`;
- `rasterizer_xbox_environment.c`;
- `rasterizer_xbox_screen_effect.c`.

Focused gates after the owner cleanup report 13/1/0 for water, 17/0/0 for
dynavobgeom, 33/2/9 for environment, and 0/2/11 for screen effect.  No target
function in those units regressed.  The subsequent complete build and stable
sweep cover the wider `hud_draw.h` include graph.

## Whole-object ownership boundary

Each calling translation unit emits a select-any `_real_alpha_to_pixel32`
COMDAT, which is the expected MSVC consequence of the genuine non-static
shared inline.  All four candidate copies are strictly identical to January's
selected active-camouflage packet: 112 bytes, six relocations, and normalized
SHA-256
`28913a006347a05e936678a44d230c0137795a8e427083b4ff0f1367ecd73596`.

The csplit active-camouflage owner records COMDAT selection 1 (no duplicates),
while each ordinary rebuilt header copy records selection 2 (select any).
The current active-camouflage source does not yet implement a function that
uses the helper, so it does not naturally emit the selected rebuilt owner.
Nothing artificial was added to force that emission.

`rasterizer_xbox_dynavobgeom.obj` has all 17 target functions semantically
exact, but it still has an independent one-byte target-data gap, so it is not
currently a whole-object admission candidate.  If that data gap is closed,
its select-any helper copy must be audited before the object is marked
complete.  Reopen that accounting boundary only after active camouflage
naturally emits the authenticated owner and an ordinary base/base link proves
the duplicate is discarded without a linked-image change.  The other three
callers are already incomplete for independent target-function reasons.

## Verification

- Complete `ninja all_source progress semantic_progress`: pass, zero semantic
  unit errors.
- Stable-verdict diff: one gained function / 2,080 padded bytes / zero
  regressions.
- Direct hardened COFF comparison: all four emitted helper copies equal the
  January owner.
- Defined-symbol scan over rebuilt Xbox rasterizer objects: no
  `point_from_line3d` COMDAT.
- Park audit after removing the now-stale bumpmap entry: 181 active / zero
  stale / zero invalid.
- Focused fake-match scan: one expected review lead, the authenticated helper
  assembly described above; no other findings.
- Object-admission audit: no unreviewed candidates or contradictions.
- `git diff --check`: no whitespace errors.
