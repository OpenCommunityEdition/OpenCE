# `render_cameras.obj` sphere-diameter Opus reconciliation (2026-09-03)

## Scope

This packet selectively reconciles one small exact leaf from the frozen Opus
donor `opus/small-families-30k-20260902`, commit `8aed00b49`.  It does not
cherry-pick the donor wave.

The admitted function is:

- `_render_frustum_sphere_diameter_in_pixels`: 96 padded target bytes
  (82 meaningful code bytes)

## Source-quality review

The implementation is ordinary typed C.  It projects the sphere center onto
the frustum's forward depth, takes the absolute depth with `ABS`, clamps it to
`0.1f` with `MAX`, and scales the projected diameter by the supplied radius.
It uses the engine `real` and point/frustum types, contains no address-derived
identifier, raw offset, inline assembly, forced inline, volatile/register
steering, representation pun, artificial branch, or other matching-only
construct.

The translation unit now includes `render/render_cameras_internal.h`, which
already owns the cross-translation-unit prototype used by renderer clients.
No declaration was placed in a caller or unrelated source file.

## Verification

- Focused gate: exact, 96/96 padded bytes; all 43 normalized instructions and
  relocation identities agree.
- Object gate: 5 exact, 0 residual, 16 unwritten (previously 4/0/17).
- Rename-stable whole-tree sweep: +1 exact function / +96 padded bytes, zero
  regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,863 semantic exact, 5,880
  accepted exact, zero unit errors.
- Progress report: 830,151 / 2,198,102 matched code bytes and 5,812 / 11,060
  matched functions.
- Fake-match scan: zero review leads in the changed source.
- Park audit: 185 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions; the two inherited
  explicit rejections are unchanged.
- Tool tests: 261 passed.
- The January inline schedule remains intact; this packet emits no
  `point_from_line3d` COMDAT.

Stable snapshots:

- before: `scratch/after-draw-string-tab-20260903.json` (5,853 exact)
- after: `scratch/after-render-camera-diameter-20260903.json` (5,854 exact)
