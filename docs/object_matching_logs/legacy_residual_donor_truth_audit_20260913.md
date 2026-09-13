# Legacy donor-only exact residual audit (2026-09-13)

## Result

The apparent high-value tail reported by `tools/campaign/donor_delta.py` for
the old Claude/Fable trees is not transferable exact progress.  This audit
covered **6,240 meaningful donor-only bytes** across ten functions and admitted
**zero exact bytes**.  Every candidate is either a stale verdict for source
already present, an already integrated closure under a newer semantic name, or
an exact-looking construction that violates current ownership/source rules.

No production source, header, symbol map, compiler configuration, or parked
record was changed.  In particular, this pass did not trade correct private
linkage or typed ownership for an old donor's standalone emission.

| Unit / donor function | Donor bytes | Current focused state | Disposition |
| --- | ---: | --- | --- |
| `dead_camera::_dead_camera_update` | 1,235 | 1,248-byte residual | reject: the donor reads an uninitialized argument home slot to obtain the last one-byte schedule difference |
| `path_smoothing::_code_000516a0` | 896 | semantic `_find_turning_point`, 896-byte residual | reject: address name, foreign prototype, array-decay interfaces, hand-expanded normalization macro, and manual flag extraction; the retained typed body is already 99.993866% |
| `bitmap_utilities::_code_000684e0` | 866 | semantic `_bitmap_2d_uncompress_from_mipmap`, 880-byte residual in the live header context | stale: current and donor function sources are identical; an earlier correct owner declaration made it exact, while later declaration-position changes reopened the two independent terminal loads |
| `lrar_cache::_lrar_allocate` | 776 | 1,056/784 bytes and 47/27 relocations | reject: exactness requires `noinline` plus two selectively copied expansions of `lrar_get_block` |
| `game_engine::_code_0009ccf0` | 638 | semantic `_game_engine_verify_current_map`, 1,104/640 bytes and 60/52 relocations | reject: the old result prevents natural private-helper inlining through false external ownership / historical `noinline`; numeric engine and flag constants also replace recovered enums |
| `object_lights::_code_00129180` | 81 | semantic `_light_unmarked`, residual | reject: natural predicate and correct private ownership are already retained; donor spelling has no new source provenance |
| `object_lights::_code_00129470` | 551 | semantic `_find_point_lights_for_object_in_cluster`, 560-byte residual | reject: donor-only exactness depends on superseded declaration-position context; the live owner graph now closes 38 of 43 functions |
| `recorded_animations::_code_000839a0` | 630 | semantic `_recorded_animation_play_internal`, 640-byte residual | reject: the donor obtains exact function bytes by assigning the three zero-filled BSS owners the wrong names/linkage/order |
| `actor_combat::_code_0000fa40` | 346 | semantic `_actor_combat_find_nearby_target`, 352-byte residual | reject: code-address name, wrong use-site ownership, different collision API topology, and an emitted `_point_from_line3d` code definition in the preserved donor object |
| `rasterizer_lights::_rasterizer_lights_reset_for_new_map` | 48 | 48-byte residual | reject: the donor deliberately clears one whole record beyond the first array; this is undefined memory corruption, not admissible original-source recovery |
| **Total** | **6,240** | — | **0 admitted** |

The additional Fable-only 173-byte
`rasterizer_transparent_geometry::_code_00174120` verdict was also checked but
is not included in the 6,240-byte Claude table above.  It is the already
rejected packed-BSS/address-name donor.  Canonical retains the semantic
`rasterizer_sort_external` body as a zero-credit 176-byte residual with the
eight real globals rather than reconstructing a false packed aggregate.

## Focused reproduction

All focused compiles used the live worktree's XDK 3911 command through
`tools/campaign/gate.py` and rejected an emitted `_point_from_line3d` owner.

- `dead_camera`: 2 exact / 2 residual / 0 unwritten;
- `path_smoothing`: 4 exact / 2 residual / 0 unwritten;
- `bitmap_utilities`: 40 exact / 8 residual / 0 unwritten;
- `lrar_cache`: 11 exact / 1 residual / 0 unwritten;
- `game_engine_verify_current_map`: 1 residual, 1,104/640 padded bytes and
  60/52 relocations;
- `object_lights`: 38 exact / 5 residual / 0 unwritten;
- `recorded_animations`: 15 exact / 1 residual / 0 unwritten;
- `actor_combat`: 20 exact / 5 residual / 9 unwritten;
- `rasterizer_lights`: 9 exact / 4 residual / 0 unwritten;
- `rasterizer_transparent_geometry`: 17 exact / 2 residual / 0 unwritten.

The preserved Claude `actor_combat.obj` defines `_point_from_line3d`,
`_real_random_range`, and `_collision_test_line` as candidate code sections.
That object therefore fails the January inline-schedule guard independently of
the source-style problems in its exact address-named function.

## Controlling evidence

This is a consolidation audit, not a new spelling-search license.  The
candidate families and their reopen requirements are already established by:

- `dead_camera_obj_codex_checkpoint.md`;
- `path_smoothing_obj_jonas_natural_frontier_audit_20260830.md`;
- `bitmap_utilities_obj_selective_donor_reconciliation_20260831.md` and
  `claude_remaining_donor_batch_20260908.md`;
- `lrar_cache_obj_codex_small_closeout_20260903.md`;
- `game_engine_obj_jonas_no_fake_inline_cleanup_20260831.md`;
- `object_lights_obj_interrupted_opus_reaudit_20260904.md` and
  `object_lights_obj_preprocess_scene_packet_20260912.md`;
- `recorded_animations_obj_natural_recovery_20260830.md`;
- `opus_misc_small_admission_20260903.md`;
- `rasterizer_transparent_geometry_obj_jonas_fuzzy_closeout_20260831.md`.

Do not count these historical donor verdicts toward the next publication
checkpoint.  Reopen an individual residual only when new first-party/local
evidence explains its ownership and natural compiler context without
`noinline`, fake external linkage, wrong BSS identity, undefined behavior,
address names, raw type/offset overlays, or an unwanted math-helper COMDAT.
