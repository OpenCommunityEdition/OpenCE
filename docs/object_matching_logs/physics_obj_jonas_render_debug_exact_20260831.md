# `physics.obj` render-debug recovery (2026-08-31)

## Result

Starting from canonical commit `ee24686f1`, this bounded follow-up admits the
single previously unwritten `_render_debug_physics` body recovered by the
Claude/Fable campaign at `e02971dfd`.  The hardened comparison is strict:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_render_debug_physics` | `262 / 272` | `12` | `d22fc648c16f41c42b5753553f7589f2dbf74f7322cb83137f38d65ee67e4926` |

`physics.obj` advances from **5 exact / 0 residual / 12 unwritten** to
**6 exact / 0 residual / 11 unwritten**.  No inherited exact function or data
owner changes.  The whole-board increase is exactly one function and 262
meaningful code bytes.

## Semantic and source-authenticity review

The function resolves the owning object and definition, transforms the
physics center of mass into world space, renders its basis at the object's
bounding radius, then iterates the typed mass-point block.  Each mass point's
position and orientation vectors are transformed through the instance matrix
before the point sphere and local basis are rendered.  The half-radius basis
scale is the target-owned `0.5f` constant.

The body uses the established `physics_instance`, `physics_definition`,
`mass_point_definition`, `object_datum`, and `object_definition` owners.  It
contains no guessed vehicle/runtime layout, raw offset, address-named owner,
representation cast, assembly, pragma, forced inline, volatile/barrier,
synthetic anchor, dead condition, or hand-written bit logic.  The function
name and public signature were already present in January-derived symbols and
`physics.h`.

The exact relocation sequence independently attests the program topology:

- object and tag resolution;
- center-of-mass and per-mass-point transforms;
- two calls to `_render_debug_vectors`;
- one `_render_debug_sphere` call with `_global_real_argb_white`;
- the typed tag-block element helper; and
- the target `__real@3f000000` half constant.

This packet deliberately excludes Fable's larger vehicle-collision body.  It
is a register-allocation residual and depends on guessed private vehicle
layouts, as recorded in
`physics_obj_jonas_claude_natural_prefix_exact_20260831.md`.

## Owner-header repair

The Fable campaign proved the function bytes with a translation-unit-local
declaration of `render_debug_vectors`.  That declaration was not retained:
the function is defined by `render_debug.c`, so its prototype now lives in
the owning `render_debug.h`.  `physics.c` includes that header normally.

All pre-existing direct `render_debug.h` consumers plus the new physics
consumer were recompiled with the PID-isolated campaign gate:

| Unit | Exact / residual / unwritten after the header edit |
|---|---:|
| `source/cache/xbox_sound_cache` | `14 / 3 / 1` |
| `source/cutscene/recorded_animations` | `15 / 1 / 0` |
| `source/units/units` | `189 / 0 / 0` |
| `source/effects/material_effects` | `3 / 0 / 0` |
| `source/game/players` | `52 / 17 / 1` |
| `source/render/render_sprite` | `1 / 0 / 9` |
| `source/scenario/scenario` | `46 / 0 / 0` |
| `source/ai/ai_debug` | `56 / 4 / 0` |
| `source/physics/collision_features` | `20 / 0 / 0` |
| `source/physics/point_physics` | `6 / 0 / 0` |
| `source/physics/collision_bsp` | `16 / 0 / 14` |
| `source/render/render_debug` | `2 / 0 / 34` |
| `source/render/render` | `13 / 0 / 0` |
| `source/objects/objects` | `121 / 0 / 0` |
| `source/rasterizer/rasterizer` | `131 / 0 / 0` |
| `source/physics/physics` | `6 / 0 / 11` |

The full production rebuild recompiled all sixteen units and reported zero
semantic unit errors.  The declaration-count sentinel `units.obj` remains
189/189.  No new `point_from_line3d` or other unwanted helper COMDAT is
emitted by `physics.obj`.

## Validation

- `tools/campaign/gate.py source/physics/physics --all`:
  `6 exact / 0 residual / 11 unwritten`;
- direct `tools.coff_compare` replay: `all_equal: true`, 272 bytes and all 12
  relocation identities exact;
- full configure/Ninja build, ordinary progress, and semantic progress: pass;
- semantic audit: 470 units, 5,027 functions evaluated, 4,916 semantic exact,
  4,926 accepted exact, zero unit errors;
- campaign total: 627,905 / 2,198,102 meaningful code bytes and
  4,883 / 11,060 accepted functions;
- Halo total: 614,991 meaningful code bytes and 4,716 functions;
- changed-file fake-match scan: zero review leads;
- `git diff --check`: clean apart from informational LF/CRLF warnings.

No configuration, semantic exception, parked record, generated report,
completion label, commit, or push is part of the recovery itself.
