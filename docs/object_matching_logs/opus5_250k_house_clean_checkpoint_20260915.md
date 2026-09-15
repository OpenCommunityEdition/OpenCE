# Opus5 250K house-clean lane: checkpoint ledger (2026-09-15)

Continuation of `opus/150k-house-clean-20260914` (HEAD `0483bde89`, +89,085 strict meaningful) on branch
`opus/250k-house-clean-20260915`, in the same worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Lane base snapshot: `scratch/claude150k-before.json` (commit `12f7375d4`, 7,149 functions / 1,244,970 meaningful).
Target: +250,000 strict meaningful (`scratch/lane_manifest.py`: `TARGET = 1494970`, remaining computed against 250,000).

## 1. Startup proof (2026-09-15, at 0483bde89)

| check | expected | observed |
| --- | --- | --- |
| tracked tree / HEAD | clean / 0483bde89 | clean / 0483bde89 |
| `ninja` (incl. parked-function check) | pass | pass; "Validated parked compiler ties: 280" |
| stable diff `claude250k-start.json` vs `claude150k-before.json` | gained 144 90112; regressions 0; 8,245 / 7,263 | gained 144 90112 bytes; regressions 0; 8,245 functions, 7,263 exact |
| `tools.parked_functions` | 280 / 0 / 0 | 280 active / 0 stale / 0 invalid |
| `tools.audit_object_admission` | 0 contradicted / 7 rejected / 7 candidates | 0 / 7 / 7 |
| `pytest --basetemp scratch/pytest-250k-start` | 1151 passed, 5 skipped | 1151 passed, 5 skipped, 26 subtests passed |
| `lane_manifest.py` strict_new_meaningful | 89,085 | 89,085 (144 fns, 90,112 padded; 4 non-strict accepted / 2,195 excluded) |

## 2. Owner rulings (2026-09-15, asked at startup; strategy step A)

The owner took the strict option on every held item. Nothing below is landed or credited:

- collision_bsp `_collision_surface_test_sphere` (880; unit-local SSE `__asm` distance macro, p1.c): leave parked.
- env_fog 21-static .bss split (+800) and models 13-static .bss split (+1,296): neither lands.
- editor_flying_camera `is_scripted` linkage-anchor split (304; the w3 ledger says the split alone would also unlock
  `_editor_camera_flying_update` 1,408 and `_editor_camera_set_scripted` 432): held.
- The 7 whole-object admission candidates: not admitted. P7 `projection_from_vector3d`: stays reverted, no worker spend.

These are recorded in `scratch/WORKER_BRIEF.md` section 9b as final for this lane.

## 3. Pool at HEAD (scratch/gate4, regenerated 2026-09-15)

`gate.py <unit> --all --forbid-emitted-symbol _point_from_line3d` over the 151 units with non-exact rows (no guard for
action_charge / ai_debug / real_math): 239,536 residual + 192,176 parked + 6,088 unwritten padded bytes. The 90 units
that never got a w3 pass hold 110,832 residual + 100,768 parked + 3,144 unwritten. After removing protected units,
`rasterizer_xbox_transparent_geometry`, do-not-spend, owner-declined/standing holds and laws_w2 D do-not-repeat rows,
`scratch/build_wave3c_groups.py` leaves 157,832 spendable padded bytes in 53 groups / 81 units.

## 4. Strategy step C: C4013 census (zero credit, analysis only)

`CL /Zs /W3` over all 468 source units (`scratch/gate4/c4013.txt`, 112 C4013 + 23 C4133 lines;
`scratch/gate4/c4013_map.json` maps each site to its caller and gate status). Outside libtiff, only 7 sites are inside
non-exact functions (7,392 padded): rasterizer_xbox_models `_rasterizer_model_draw_environment_shader` (park),
action_vehicle `_action_vehicle_evaluate_seat`, light_volumes `_light_volume_render` (park), hud_weapon
`_render_grenade_hud` (`weapon_prevents_grenade_throwing` has no owner prototype), ai_communication
`_ai_communication_update_speech_timers` (park), action_uncover `_action_uncover_perform` (park), main
`_main_game_render` (park). All 7 were already in the w3 census; the un-run units carry them in their w3c group notes.
The other sites are in functions that are already exact (a prototype there is a regression risk, not a lever).

## 5. Strategy step D: static-linkage census (negative)

`scratch/gate4/static_census.py`: 53 external data symbols in January split objects with no January PDB public and no
cross-object relocation, referenced by at least one non-exact function at HEAD (74,880 B of referencing rows, double
counted). A scratch csplit with 50 of them `"static": true` (placeholder `_data_`/`_bss_` names excluded) changed
only the 33 owner objects (`scratch/static_probe/changed_objects.json`).

- Config only (HEAD objects vs the static split): 0 rows changed in all 33 units.
- C `static` added to the definition (tentative COMMON becomes a defined .bss symbol; storage 3 verified in the
  candidate objects) with the matching static split: **0 rows changed** in sound_manager, virtual_keyboard,
  stack_walk_windows, actor_perception, weather_particle_systems, rasterizer_xbox_active_camouflage, draw_string,
  dead_camera, render_particles, props, shader_transparent_chicago_preprocessor, debug_memory, key_agreement,
  bitmap_utilities (both tables), rasterizer_transparent_geometry (5 globals), rasterizer_xbox.
- game_engine_king `king_globals` static with the (single-consumer) header extern removed: gains nothing and loses
  `_find_next_hill` (112) and `_king_engine_initialize_for_new_map` (272). Same as the w2 static probe.
- Already `static` in C (config flip only, byte-inert): collision_debug x2, rasterizer_lights x4, player_effects x2,
  profile_globals, dsound_error_string.
- Not probed: `extern` layout-anchor declarations inside their own .c (ui_widget_globals_storage,
  dsound_effects_image, bink_playback x3, xbox_sound_cache_globals); editor_camera_* (owner-declined split); the
  header-extern globals lightning, light_volumes, race (w2 measured them as no effect), director x3 (rows are
  R3/A17 ruling-blocked) and game_globals (`_game_load` 208).

Conclusion: at HEAD the static-linkage law has no remaining closures for these residuals.
