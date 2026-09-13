# `rasterizer_frame_statistics.obj` shared-owner fuzzy admission (2026-09-12)

## Result

The complete Opus frame-statistics renderer and its FPS/private-state support
are now retained as readable source, but receive **zero exact credit**.  The
admitted source uses the canonical 0x170-byte shared statistics definition and
the owning `rasterizer_debug_options.h` type; it does not restore either of the
donor's translation-unit-local duplicate layouts.

The fresh single-TU gate reports six inherited exact functions, four residuals,
and zero unwritten functions.  No inherited exact function changed.

| Function | Target/candidate padded bytes | Target/candidate relocations | Verdict |
| --- | ---: | ---: | --- |
| `_rasterizer_frame_statistics_draw` | 4,176/4,176 | 322/322 | coherent fuzzy, zero credit |
| `_rasterizer_frame_statistics_get_fps` | 384/384 | 21/19 | coherent fuzzy, zero credit |

The draw's target/candidate normalized SHA-256 values are respectively
`747b937d70b288778c244bc3e426d03294da76a99ea3afd31cc23abb5edcccec`
and `c69c5495940300ca808b4d6478ea9f3ca2c7c92b3dec16065d3ab9ea352186e9`.
The FPS values are
`c6275f2bf09ccb0363daeeff9afcab28656ef7f51f2a73b4f0780f5fdb095a92`
and `b2117c57a8ca3671a9c02317fedfe7da9f28f4dd42163ef5e7a92474f9f0864e`.

## Ownership and source review

The shared owner header names the FPS/object/model fields exposed by this
reader while preserving the established writer-authenticated names for the
same offsets.  In particular, the display now reads actual dynamic counts and
model shader-work accumulators instead of the donor's misleading `maximum` and
object-count aliases.  The complete 0x860 private record remains local as a
type and is referenced through the existing external symbol; no tentative BSS
owner was introduced.  January's `_profile_log_path` is emitted once as an
external four-byte data owner and keeps the natural C name `profile_log_path`.

The body uses project `real`, color, point, and rectangle types, the proper
debug-options owner, named statistics-mode constants, explicit returns, and
one parameter per line.  It introduces no inline assembly, forced inlining,
raw-address access, or `point_from_line3d` owner.  January's anomalous `0x48`
lit-dynamic-vertex allocation is explicitly marked `BUG (original)` with the
plausible safe correction documented, rather than silently presenting the
arithmetic as sensible.

## Why the donor exact verdict was not retained

With correct shared ownership, the draw has the same 1,178 instructions and
the same relocation count as January.  The first real differences are two
pairs of independent aggregate loads: January schedules statistics offsets
`+0xAC/+0x9C` and `+0xB0/+0xA0`, while the canonical-owner compile schedules
each pair in the opposite order.  The remainder of the stream aligns.

Closing those differences would require repeating the donor's declaration- or
expression-order search, whose artificial predecessor/layout dependency was
already rejected in
`rasterizer_frame_statistics_obj_opus_draw_source_admission_hold_20260906.md`.
No such compiler steering is admitted.  Reopen only with authentic January
source topology or a natural same-compiler donor.

## Verification

- `gate.py` for frame statistics: 6 exact / 4 residual / 0 unwritten;
- forbidden `_point_from_line3d` emission guard: passed;
- all ten direct shared-header consumer gates passed without losing their
  inherited exact sets (`rasterizer_debug`, `rasterizer_lights`, Xbox core,
  decals, draw primitives, dynavobgeom, environment, environment fog, shadows,
  and vertex-shader runtime);
- updated independent owner-layout/type fixture: 10/10 pytest tests passed;
- targeted fake-match scan: zero findings; and
- `git diff --check`: no whitespace errors (line-ending warnings only).

The parked ledger should be refreshed after the orchestrator's full objdiff
report so its measured percentages come from the canonical build rather than a
scratch single-TU estimate.
