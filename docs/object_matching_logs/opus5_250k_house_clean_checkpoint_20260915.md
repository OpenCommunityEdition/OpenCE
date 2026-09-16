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

## 6. Local (token-free) levers run on 2026-09-15, after the 150K operator's guidance

The 150K lane operator confirmed A1 and the C4013 sweep are exhausted, reported that the C2.Dll 13.00.9254 decode
(Aug 2026) found NO instruction scheduler and NO spill-cost field - allocation is driven by DEFINITION POSITION, which
is already reachable from source - so there is no compiler-side lever left for the `[sha]` tie pool, and named three
retired lever classes worth a local re-run. All three were run here, at zero agent cost.

### 6.1 objdiff under-credit sweep - NEGATIVE (0 recoverable bytes)

`scratch/undercredit.py` (written here; the other lane's `add_semantic_matches.py` is not in this worktree).
114 functions / 88,624 padded bytes are strict-exact (`section_infos_equal`) yet scored below 100% by objdiff. Every
one already carries internal-label relocation parity, and **all 114 are already in `build/semantic_report.json`
accepted_ledger** - so there are no uncredited bytes on the floor at this HEAD. 100 of the 114 were already `E` in the
frozen lane-base snapshot and would not have been claimable for this lane in any case.

### 6.2 Naming-gap attestation - NEGATIVE (0 proposals)

`python -B -m tools.campaign.namegap --json` over all configured units: `proposal_count 0`, `proposal_bytes 0`,
`ambiguities_rejected 0`. The proposal set that grew after earlier waves is empty at this HEAD.

### 6.3 Donor/history mining - 13 candidates, ALL REJECTED on authenticity

`scratch/hist_sweep.py` over 142 units (every unit with a non-exact row except the nine batch-4 units), gating every
unique historical blob of each `.c`: 13 functions / 2,032 padded bytes gate strict EXACT in some blob but not at HEAD
(`scratch/hist/candidates.json`). Verified individually (`scratch/hist/verify.sh`, gate + row diff + parkcheck):

| function | B | verdict |
| --- | ---: | --- |
| `__rasterizer_widget_draw_sprite2d` | 528 | REJECT: the blob loses the HEAD-exact `__rasterizer_widget_submit_occlusion_test`; net zero and a regression |
| `__rasterizer_widget_begin` | 0 | REJECT: same loss, no gain |
| `_rasterizer_lens_flares_submit_occlusion_tests` | 400 | OUT: `_point_from_line3d` COMDAT (lane rule) |
| `_hud_play_sound` | 352 | REJECT: the blob body is `word *volatile played_flags` + a consumer-local prototype + `goto mark_played` + hand `|= FLAG(...)`. The park evidence records HEAD's body as the deliberate policy replacement of exactly this one (R18, R8) |
| `_hs_parse_boolean` | 304 | REJECT: costs 48 exact rows |
| `_wind_variance_get` | 224 | HANDED TO THE w3d_wind WORKER: union bit-mask replacing `fabs` (laws_w2 B admits a local union bit transfer) but bundled with a `static` linkage change and a second park drift; must be decided from January bytes, minimum hunk only |
| `_point_in_triangle3d`, `_pill_test_vector3d` | 0 | REJECT: the real_math blob emits the forbidden `_point_from_line3d` |
| `_convex_hull2d_perimeter` | 96 | REJECT: costs 6 exact rows |
| `_get_mutex_from_pool` | 64 | REJECT (fake-match): the blob declares `extern struct mutex_reference transport_address_string;` - another TU's *string* symbol, given a wrong type - and walks `(long)&transport_address_string.in_use` as the pool loop bound. An invented extern used for a link-time address |
| `_rasterizer_lights_reset_for_new_map` | 48 | REJECT: laws_w2 R12 buffer overrun |
| `_code_000d16d0` | 16 | REJECT: costs 9 exact rows |

**Finding (new do-not-repeat):** at this HEAD, history mining returns only bodies that earlier lanes already rejected
on policy - the tree has absorbed every admissible historical body. Re-run it only after a source-shape law lands that
did not exist when those blobs were written.
