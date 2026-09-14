# Opus5 100K consolidated donor lane — handoff (2026-09-14)

## Verdict

The lane reaches **+104,809 meaningful strict-exact code bytes** versus canonical
`c3e257e9ab9a5d7e1c9b39862f87ac2abde4359c`, against a +100,000 objective. There are **zero regressions**.

| total | value |
| --- | ---: |
| accepted ledger, base | 6,972 functions / 1,140,161 meaningful |
| accepted ledger, final | **7,149 functions / 1,244,970 meaningful** (target 1,240,161) |
| accepted gain | **+177 functions / +104,809 meaningful** |
| strict stable-diff gain (by section identity) | **+182 functions / +107,712 padded / +106,222 meaningful** |
| regressions (stable diff from base snapshot `39557d49…`) | **0** |
| parks | 465 → **391** active / 0 stale / 0 invalid |
| retained fuzzy bodies, zero credit | see "Fuzzy (zero credit)" |

The stable-diff gain is larger than the accepted-ledger gain because a few functions were already credited in the semantic tier at base. The accepted ledger (`build/semantic_report.json`) is authoritative.

The machine-readable per-function record is `opus5_100k_consolidated_admission_manifest_20260914.json`. For every function it gives unit, section, padded and meaningful bytes, relocation count, normalized sha256 and the commits touching the unit. The per-object table is in `opus5_100k_checkpoint_20260914.md`.

## Identity

- Worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, branch `opus/100k-consolidated-20260914`.
- Base: `c3e257e9a`. Final: `32009865a`. Nothing was pushed, and the canonical checkout was not modified.
- Phase 1 merges:
  - `939c20eb8` merges `opus/30k-fresh-graphs-20260914` (`0a90aacb6`).
  - `1efcd3f05` merges `opus/pdb-ida-15k-20260914` (`70a5f0bbf`).
  - The merged total was verified at 7,002 / 1,157,255 (+17,094 meaningful / 17,312 padded / 33,599 fuzzy target bytes, 0 regressions).
  - Both lanes' manifests and handoffs are retained unchanged.

## How the gain was produced (after the merge: +87,715 meaningful)

| source | meaningful (approx.) | mechanism |
| --- | ---: | --- |
| Wave A/B/C worker landings | ~17,600 | one worker per `.c`, adversarial verifier per landing |
| Header/config packets (single coordinator) | ~24,000 | worker proposals → owner header → full build → bisect |
| Park re-investigation (waves C and D) | ~46,000 | new measured VC7 laws; every reopen adversarially verified, one dedicated commit per object |

Checkpoints: +40K (`3754eed8c`), +60K (`548451f6d`), and the final manifest in this commit. Every packet was verified by:
- full `ninja all_source progress semantic_progress`;
- `stable_verdicts` diff;
- `parked_functions`.

The admission audit, pytest and `git diff --check` ran at the checkpoints and at the end.

## Header and config packets (coordinator; consumer audits)

Every packet was a separate commit or a single measured packet with its only dependent source. Each was verified by a full build of all consumers with zero strict regressions.

| commit | owner change | consumers | result |
| --- | --- | ---: | --- |
| `12e295220` | weapons.h: `weapon_aim`, `weapon_estimate_time_to_target` | 19 | prerequisite for actor_combat |
| `b8cd50113` | symbols.json: 12 atlas-authenticated private names | — | prerequisite for wave A |
| `5a9586851` | model_animation_definitions.h: `animation_update_internal` parameter widened to `long` | 14 | +304 |
| `6fda65bf8` | symbols.json: sound_manager PDB constants split; hud overlay `theta` function-static | — | +2,848 |
| `47fecc821` | symbols.json + frame_statistics.c: FPS count / profile accumulators as statics | — | +384 |
| `c5d7751bd` | ai_debug.h: typed `firing_position` records (names-only renames in ai_debug.c and actor_firing_position.c) | 27 | speech-timer park re-measured; +5,248 with `ff71ecfc2` |
| `a7d0d4388` | actors.h / aim_assist.h / projectiles.h / units.h / weapon_datum_flags.h prototypes; removes a duplicate `collision_surface_polygon` prototype | all | +3,280 with `f94f6d7db` |
| `9f3ea79c8` | actors.h: `actor_move_force_stop`, `actor_combat_currently_firing_burst`, `boolean actor_look_secondary`, 8-value primary look priority enum; removes a duplicate `profile_initialize(void)` | 45+ | +2,384 with `ff1054197` |
| `6c7ab19db` | new focused headers math/matrix_math.h (`matrix3x3_multiply`) and math/real_math_planes.h (`line_from_planes3d`); structure_fog_plane `region_index` / `runtime_material_type` | 2 / 15 | +4,496 with `a4a7fe8cf` and `4dd357580` |
| `8f41149c5` | players.h: signed switch nibble, `none = NONE` | 74 | +816 |
| `7a3bbb9c8` | bsp3d.h: designator sign-bit test `& LONG_MIN` | all bsp3d.h | +112 |
| `94f7bbb80` | bitmap_group.h owner types; bitmaps_mipmap.h / bitmap_group_internal.h prototypes; bitmap_compress_to_mipmap `transparent_color` | 45 | +4,544; bitmap_group_add_bitmap park re-baselined |
| `76efed8c6`, `32009865a` | symbols.json: `_lights_section`; observer tuning tables (static) | — | +10,480 |

Declaration-count sensitivity hit three times:
- weapons: units.h plus aim_assist.h;
- actors.h;
- real_math.h, at any position.

It was resolved without steering, either by removing a verbatim duplicate prototype from the sensitive TU's include set, or by declaring the prototype in a focused owner header. The bisection logs are in the commit messages and ledgers.

## Parks

74 parks were removed:
- wave C: 19;
- bitmap packet: 5;
- wave D: 50.

Each reopen went through an independent adversarial verifier that checked:
- strict exact;
- sibling and park preservation;
- owner census;
- the diff hunk by hunk against house rules;
- whether the park's criterion was met or its premise refuted.

Re-measured parks with appended evidence are listed below. Each is still fuzzy, and each change is either caused by a genuine owner or callee change or is a verifier-accepted closer body required by a reopen:

| park | cause |
| --- | --- |
| `_ai_communication_update_speech_timers` | ai_debug.h packet; flips to its second proven flag-neutral order |
| `__rasterizer_screen_effect` | callee EAX ABI |
| `_bitmap_group_add_bitmap` | owner types |
| `_get_edge_vertex` | calculate_vertex ABI |
| `_structure_visibility_build_surfaces_traverse_leaf` | planes_intersect_rectangle ABI |
| `_parse_string` | coupled prerequisite of the draw_string reopens |
| `_effect_generate_particles` | January per-instance loop behaviour |
| three collision_bsp bsp3d recursions | closer structure |

Parks intentionally not reopened, despite strict-exact scratch bodies:

| park | reason |
| --- | --- |
| `_extract_sequence` | reproduces January-proven redundant logic |
| `_ai_handle_editing` | owner commit `603f5b41c` parked the unsafe exact body |
| `_bitmap_draw_string` | reproduces a NULL dereference |
| `_biped_check_discard` | reproduces mismatched error() varargs |
| `_post_evaluator_global`, `_pre_evaluator_combatmove` | unused lookup locals |
| `_ai_test_line_of_fire` | parenthesis form without a genuine macro |
| `_convex_hull2d_perimeter`, `_build_torus` | expression-order steering |
| `__rasterizer_model_transparent_geometry_submit` | static names chosen to reproduce the name-hashed .bss layout |

## Rejected trials (all reverted or never landed)

- `_action_vehicle_desirable`: a redundant `desirable = FALSE` re-store. The single-assignment form is not exact.
- `_action_obey_command_begin`: a redundant `look_duration` re-store.
- Wave A actor_looking uncalled static bodies: unmeasurable; re-landed later with their header packet.
- screen_effect first form: shadowing local plus reordered products. Replaced by the verifier's fix.
- real_math.h placement of `matrix3x3_multiply` / `line_from_planes3d`: regressed decals and draw_primitives at any position.
- units.h + aim_assist.h without cleanup, and actors.h without cleanup: each regressed `_unit_preprocess_node_orientations`.
- `_point_from_line3d` emission outside action_charge.obj: requested by weapons, decals, actor_moving and action_vehicle workers; declined.
- Inline assembly, forced inline, 2D representation casts, `volatile`, unauthenticated static names, and ABI-mismatched prototypes (sound_definitions short/long): declined.

## Fuzzy (zero credit) bodies retained in the real files

The merged lanes carry 33,599 fuzzy target bytes. This lane adds complete, coherent, strictly-closer bodies at zero credit (January padded target sizes):

| function | bytes |
| --- | ---: |
| `_ai_communication_event` | 8,064 |
| `_biped_update_physics` | 5,376 |
| `_collision_move_point` | 4,752 |
| `_actor_look_update` | 4,720 |
| `_actor_combat_update` | 4,672 |
| `_weapon_update` | 2,800 |
| `_actor_start_burst` | 1,968 |
| `_trigger_create_projectiles` | 1,824 |
| `_object_test_vector` | 656 |
| `_actor_look_idle_find_prop` | 608 |
| `_actor_look_find_random_vector` | 608 |
| `_actor_look_valid_look_vector` | 240 |
| `_actor_look_valid_aim_vector` | 128 |

Total: about 36,400 target bytes.

## Evidence used

- January target COFF objects (build/split) for code, relocations, .rdata literals and data layout.
- January debug map atlas (exact tier) for private names and the `_code_` renames.
- January PDB public symbols: public linkage (e.g. `extract_build_debug_plate`). No Halo types or locals exist in it.
- Ghidra 12.1.2 headless decompilation of the split objects (hypotheses only).
- HCEA/HCEX reconstructions and DB-derived types for names, semantics and topology; HCEX local scope grouping for actions lost-contact. Never for addresses or layouts.
- IDA: not used for automation (IDA Free lacks `-S` and IDAPython). No supplied binary was executed and nothing was uploaded.

New measured VC7 13.00.9254 laws, recorded in scratch/WORKER_BRIEF.md sections 9-10 and in the wave ledgers:
- **inline frequency law:** helpers stay out of line in cold blocks; the argument-kind clause refines this;
- **paren/FPU argument law:** a parenthesised whole argument is staged through the x87;
- **x87 operand-order law** and **x87 phi store forwarding**;
- **.bss name-hash ordering;**
- **movzx/movsx element-type proof;**
- frame gap = missing locals;
- tail-recursion scope.

## Verification (final tree `32009865a`)

| gate | result |
| --- | --- |
| `ninja all_source progress semantic_progress` | pass; accepted_exact 7,149 |
| stable diff `scratch/before.json` → `scratch/final-snapshot.json` | 182 gained / 107,712 padded / **0 regressions** |
| parked functions | 391 active / 0 stale / 0 invalid |
| object admission audit | 0 candidates / 0 contradictions / 7 rejections (unchanged from base) |
| pytest tools | 1,151 passed / 5 skipped / 26 subtests |
| `git diff --check` base..HEAD (source, config) | pass |
| fake-match scan, all changed `.c` | 58 changed files: 7 review leads at base, 7 at final — **no new leads** |
| `_point_from_line3d` guard, all changed units except action_charge | pass on all 57 changed units except ai_debug, whose `_point_from_line3d` COMDAT is **pre-existing** (identical in the base-equivalent pdb-ida lane object; this lane only renamed fields there); action_charge is the genuine owner |

## Protected and overlap notes

- `actor_perception.c` was not touched. It holds the largest remaining unwritten graph, but `opus5-hud-reconcile-20260913` had it dirty today.
- `hud_weapon.c`, `devices.c`, `units.c`, `vehicles.c` and `matrix_math.c` were not touched.
- `ai_debug.c` received names-only field renames required by the ai_debug.h packet.
- `hud_draw.c` and `interface.c` received mechanical `registration_point` respellings from the bitmap packet.
- The merged lanes had already touched `interface.c` / actor_perception.h per their handoffs.
- symbols.json and parked.json edits are in-place line edits, never re-serialized.

## Remaining ranked work

1. **Owner rulings that unlock exact bodies already measured:**
   - `_point_from_line3d` COMDAT policy (weapons `trigger_create_projectiles` 1,824, decals `decal_new_from_collision` 6,176, actor_moving `move_update` 3,136);
   - units.c `short animation_impulse` ABI (ai_communication `find_actor_to_reply` 1,104, actions try_to_dive, actors `unit_control` 784);
   - reproduction of original January bugs (bitmap_draw_string 304, biped_check_discard 176, ai_handle_editing 1,296);
   - unused-lookup locals (actor_firing_position 800).
2. **Header packets proposed but not landed:**
   - units.h / weapons.h animation prototypes for `_biped_update` (1,120; declaration-count risk);
   - collision_debug.h debug externs (biped update_physics);
   - ai_debug.h actor_debug_info fields (actor_combat_update 2-instruction tie);
   - shader_definitions.h radiosity/physics split (transparent_geometry assert literals);
   - `_pi` float literal in real_math.h (render_camera_build_frustum).
3. **Names-only config batch (no credit):**
   - dsound static renames;
   - `_rasterizer_environment_fog_screen_is_active`;
   - `decal_check`;
   - particle_systems rdata split;
   - collision_debug spray split;
   - draw_string BSS statics.
4. **Remaining park objects not yet re-investigated with the wave C/D laws:** s3tc, network_connection, transport_endpoint_winsock, hs_runtime, motion_sensor, editor_flying_camera, encounters, hud_unit, biped_limp_noodle, path_obstacle_avoidance, cache_files_decompress_windows, glow, hud_messaging, bitmap_utilities, rasterizer_xbox_models (model_draw_environment_shader), sound_manager (5).
5. **Residual ties recorded exhausted** (do not re-spend without new evidence): transparent_geometry group_draw (58 tie blocks), actor_move_update, weapon_update, collision_move_point, action_obey control, ui_widget IV strength-reduction pair.

## Integration / cherry-pick order

The branch is linear on top of the two merges. Integrate by merging `opus/100k-consolidated-20260914` at `32009865a`. Alternatively, cherry-pick in commit order (`git log --reverse c3e257e9a..32009865a`), respecting these dependencies:

1. The fresh-graphs and PDB/IDA lane commits, as merged (`939c20eb8`, `1efcd3f05`).
2. `12e295220`, `b8cd50113` (weapons.h and name prerequisites).
3. Wave A TU commits `ffc0dec8f` … `9becd0577`, then `5a9586851`.
4. Wave B TU commits `0fc3f380b` … `abc6b0021`, then config `6fda65bf8`, `47fecc821`.
5. Header `c5d7751bd` **before** `ff71ecfc2`. Header `a7d0d4388` **before** `f94f6d7db`.
6. `3754eed8c` (docs), header `9f3ea79c8` **before** `ff1054197`, header `6c7ab19db` **before** `a4a7fe8cf` and `4dd357580`, `8f41149c5`, `7a3bbb9c8`.
7. Wave C `a389e0035` … `8da27ccae`, then the reopens `071c5c045` … `d6cbc58ea`, then `94f7bbb80`, `548451f6d`.
8. Wave D `ddf4ba163` … `e7c706a81`, then `76efed8c6`, `32009865a` (symbols.json and parked.json edits are per object and must stay in order), then this docs commit.

Before integrating, re-check canonical for new commits touching:
- the 46 changed objects;
- the 22 changed headers;
- config/symbols.json and config/parked.json.

Then rebuild and re-run the stable diff, because declaration-count sensitivity means any canonical header change can shift the listed ties.

Scratch evidence stays untracked under `scratch/`:
- worker notes (workers, workersC, workersD);
- verifier probes (verify*, verifyP, verifyD2, verifyBM);
- orchestration scripts (waveA.js template, embed_args.py, lane_totals.py, checkpoint_manifest.py, orch/render_parked.py, orch/remeasure_parks.py, orch/declcensus.py);
- snapshots, gate rows, Ghidra and atlas dumps.
