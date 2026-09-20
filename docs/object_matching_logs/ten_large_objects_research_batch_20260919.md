# Ten-large-object research batch (2026-09-19)

## Result

This evidence-first batch gains **18 strict functions**, **10,729 meaningful
code bytes** (**10,819 padded bytes**), with **zero inherited regressions**.
The accepted ledger is now **7,548 functions / 1,452,824 meaningful bytes**.

The new strict functions are:

- `binkfile`: `_LockedAddFunc`
- `frnd`: `__frnd`
- `actors`: `_actor_unit_control`, `_actor_place`
- `encounters`: `_encounter_test_rule`
- `director`: `_director_process_variables`
- `profile`: `_profile_frame_get_value`
- `game_engine`: `_drawline`, `_default_starting_location_rate_function`,
  `_game_engine_get_starting_location_rating`,
  `_game_engine_post_rasterize_post_game`
- `ui_widget`: `_render_ui_widgets_postgame`, `_render_ui_widgets`
- `real_math`: `_sphere_intersects_triangle3d`,
  `_pill_intersects_triangle3d`
- `collisions`: `_object_test_vector`, `_collision_test_vector_exit`
- `render_cameras`: `_render_frustum_cube_visible`

`game_engine.obj` is 180/180 strict and `real_math.obj` is 84/84 strict.
Whole-object ownership accounting is 407/833 after the separately documented
ten-object admission packet.

## Evidence and source quality

The closures use January disassembly/relocations plus first-party or
same-revision debug evidence. Notable authentication includes the
same-revision two-arm source topology for `_profile_frame_get_value`, the
same-revision real-math local/branch topology and nested `triple_product3d`
shape, and XDK CodeView source-line records for the small `frnd` math helper.
No fuzzy function receives exact credit. The only new assembly findings are
the separately authenticated Bink locked-add primitive and `frnd` helper;
the Profile `rdtsc` and Decals math blocks are inherited and documented.

## Verification

- Full `ninja all_source progress semantic_progress`: pass.
- Stable diff from `scratch/ten-objects-before-20260919.json` to
  `scratch/ten-objects-final-20260919.json`: 18 gained / 10,819 padded bytes /
  0 regressions.
- Parks: 187 active / 0 stale / 0 invalid.
- Admission audit: 0 contradicted / 0 revoked.
- Tests: 1,152 passed / 5 skipped / 26 subtests passed.
- `git diff --check`: pass.
- Changed-source fake scan: only the four documented assembly families above.

Object-specific provenance and rejected alternatives are preserved in the
dated ledgers beside this file.
