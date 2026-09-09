# `render_debug.obj` point-string reconciliation (2026-09-09)

## Result

`render_debug_string_at_point` is reconstructed as natural typed C and is a
strict exact match: **296 meaningful / 304 padded code bytes**, with the same
ordered relocation identity as January. `render_debug.obj` advances from 22
to 23 exact target functions without changing an inherited exact function.

The source validates its three pointer arguments, projects the world point,
constructs terminal-text bounds relative to the current viewport, and either
draws immediately or stores the string/point/color in the existing debug
cache. It uses project `real` geometry types, named UI constants, the existing
cache enum, and an explicit terminal `return;`. No register/volatile steering,
inline assembly, raw offset access, artificial condition, or new global is
present.

## Ownership correction

The earlier isolated candidate required `render_camera_world_to_screen`, but
placing that declaration in broad `render_cameras.h` regressed four unrelated
exact functions. The declaration now lives in the established narrow
`render_cameras_internal.h` cross-translation-unit interface already consumed
by `render_debug.c`. This is the real Render Cameras owner, not a caller-local
prototype, and avoids changing unrelated public-header consumers.

## Verification

- focused gate: 304 padded bytes exact;
- emitted `_point_from_line3d` guard: passed;
- full `ninja semantic_progress`: passed;
- global ledger relative to published `6a9bc341e`: 23 gains / zero losses /
  +5,269 meaningful exact bytes including the preceding AI packet;
- global state: 6,709 accepted functions, 1,064,098 meaningful accepted bytes,
  zero unit errors.

No whole-object completion or fuzzy-body credit is claimed.
