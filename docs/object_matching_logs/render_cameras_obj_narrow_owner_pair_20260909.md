# `render_cameras.obj` narrow-owner pair (2026-09-09)

## Result

Two named, ordinary-C Render Cameras functions are admitted as strict exact:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_render_camera_screen_to_world` | 304 / 304 | 26 | `305959df963d41995dd91517b5a6d60f3779404b1db6a0a1c6836c62e12d058b` |
| `_render_frustum_build_point_flags` | 248 / 256 | 4 | `084004dce70432c599a3cc9e31aaddfdf602446456b4dacbee33c709cee4e70e` |
| **Gain** | **552 / 560** | **30** | |

The isolated universal gate reports 7 exact, 0 residual, and 14 unwritten
functions for `source/render/render_cameras.obj`. The full semantic build
advances the unpublished batch from 6,709 to 6,711 accepted functions and
from 1,064,098 to 1,064,650 meaningful accepted bytes. Relative to published
commit `6a9bc341e`, the cumulative unpublished result is +25 functions and
+5,821 meaningful bytes, with zero losses and zero unit errors.

## Source and ownership

The source topology and January-exact bodies are preserved by Fable commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`; the implementation also agrees
with the previously archived HCEA/PC reconstruction evidence documented in
the Render Cameras ledgers. The four frustum side constants use semantic enum
names and `FLAG`, rather than manual bit masks.

The earlier broad-header trial in
`render_cameras_obj_fable_owner_trial_rejected_20260906.md` was correctly
rejected because editing `render_cameras.h` changed three unrelated exact
functions through C2 declaration-position sensitivity. This retry uses the
existing narrow true-owner header `render_cameras_internal.h`, which is owned
by `render_cameras.c`; it does not place prototypes in consumer C files or an
unrelated subsystem. The full build rebuilt all actual narrow-header
consumers and found no accepted-function loss.

## Policy and validation

Both bodies use project types and named structure fields, place each parameter
on its own line, avoid address-derived names and compiler steering, and end
the void body with an explicit `return;`. No assembly, `register`, `volatile`,
pragma, alias trick, raw offset, synthetic branch, or configuration change is
present.

Validation:

- `python tools/campaign/gate.py source/render/render_cameras --all
  --forbid-emitted-symbol _point_from_line3d`
- `ninja semantic_progress`
- published-baseline accepted-ledger comparison: zero losses
- `python tools/fake_match_scan.py source/render/render_cameras.c
  source/render/render_cameras_internal.h`: zero review leads
- `git diff --check`
