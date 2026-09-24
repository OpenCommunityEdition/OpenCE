# Fifty-object campaign lane (2026-09-25)

- **Worktree:** `C:\halo-worktrees\claude-fifty-objects-20260925`
- **Branch:** `claude/fifty-objects-20260925`
- **Base:** canonical `jonas/exact-pilots` **e9e62b78** (`git ls-remote` tip; the local
  `origin/jonas/exact-pilots` ref was stale at 5361eeae and was not used).
- **Scorer frozen:** objdiff-cli **3.3.1** (sha1 3130e428). A fresh worktree silently
  re-downloads the pinned 3.6.0 on its first ninja run. Under 3.6.0, this tip's `progress`
  fails on the 3.3.1-authored `editor_flying_camera` grouped data entry. That is a
  scorer-only effect and none of this lane's numbers use it.
- **Commits:** local only, never pushed. Canonical was not edited.

## Frozen baseline (e9e62b78, objdiff 3.3.1, full ninja)

| Measure | Value |
|---|---|
| Halo objects complete | 361 / 468 (All 479 / 833) |
| Halo meaningful code | 1,564,286 / 1,770,166 (7,425 / 7,574 functions) |
| Halo data | 2,572,828 |
| Board strict functions (stable snapshot) | 7,596 / 8,245 |
| Parks | 96 active, 0 stale, 0 invalid |
| Admission audit | 14 candidates, 0 contradicted, 7 rejected, 0 revoked |
| fake_match_scan | 25 review leads |
| pytest | 1,157 passed, 5 skipped, 26 subtests |

## Method

Eight-lane agent workflow. Each worker owned one `.c` (or one audit or provider task) and
worked only in `scratch/w/<unit>/` through `gate.py --source`. Each worker had to state an
evidence-backed hypothesis before each probe, citing January bytes and relocations, the
later /Od+RTC build, HCEX.pdb, cachebeta.pdb publics, or a decoded compiler law. Every
proposed function gain or object completion then went to an independent adversarial
reviewer, who re-ran the gates, the strip tests and the house-rule review. The integrator
applies only reviewed packets and runs the full chain for every batch: full ninja,
progress, the rename-stable 8,245-function diff, parks, admission audit, fake scan,
pytest and `git diff --check`.

## Batch 1

Result: Halo objects **361 -> 363**. Meaningful code **+2,769** (1,567,055).
Functions **+5** (7,430). Data **+1,530** (2,574,358). Stable diff +5 / 2,816 padded,
**0 regressions**. Parks 94 / 0 / 0. Admission 15 / 0 / 7 / 0. Fake leads 25 (unchanged).
pytest 1,157 / 5 / 26.

### Objects admitted (2)

| Object | Work | Evidence |
|---|---|---|
| tag_files/files_windows | new code: `_file_get_size` (224 padded) | /Od 0x8aef00 single-exit `result` and /Od 0x8ae8f0 named `GetLastError()` local in static `file_error`. Both are load-bearing; strip tests and a name-count sweep (0..40, plus unused-declaration controls) are negative. A C2 priority trace shows the inlined helper's extra web raises the error block's weight, which flips the ESI/EBX colouring to January's. Caveat: in the later build `error_code` is also used by FormatMessage. object_audit PASS; 0 PDB disagreements; provider link PASS. Park retired. |
| objects/objects | ownership: provider repair plus data entry | All 121 functions were already exact. The object was held by five hand-written NODUP helper copies in January's selected providers. **Layer 1 provider repair** removes the `#define x x_inline` renames and hand copies so the genuine real_math.h inline is used: `real_random` (action_charge), `signed_angular_difference` + `interpolate_scalar` (player_control), `valid_real_vector3d_axes2` + `valid_real_normal3d` (bored_camera), `point_in_sphere` (path_obstacles), and in decals `plane3d_from_point_and_normal`, `plane3d_negate`, `projection_sign_from_vector3d` and `triple_product3d`. Every provider row is unchanged, and each emitted COMDAT is byte-identical to January's selected section. Pair links pass in both orders. A board link census moves 250 -> 260 of 288 code-exact units to link-clean with 0 regressions, including six already-Matching cameras, items and scenario, which carried latent LNK2005s. The '$'-defect data entry pins the 1,530-byte `objects_update` section table. The `objects` rejection is retired. |

### Function gains, objects still incomplete (4)

| Function | Bytes | Source fact | Object blocker |
|---|---:|---|---|
| dead_camera `_dead_camera_new` | 288 | `dead_camera_constants` is file-static (absent from cachebeta publics). A static const aggregate's load is hoisted above pointer stores, and the /Od build fixes the pitch-before-timer statement order and the single-store unit phi. `static` is load-bearing and independently PDB-proven. | `_dead_camera_update` reproduces January's uninitialised `next_unit_index` read (the /Od build shows the later fix). Owner ruling, section 4. |
| actor_perception `_actor_perception_refresh_danger_zone` | 1,472 | The TU-private `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` rename is removed. The knowledge function uses the single-result form. Storage classes follow cachebeta publics. `_point_from_line3d` is emitted under ruling 20260921 #1 (January references it out of line here); the copy is byte-identical and passes the provider link. | blockage view-copy (owner packet section 5); `distance3d` / `distance_squared2d` provider |
| actor_looking `_actor_look_idle_find_prop` | 608 | `cos`/`sin` are spelled through the genuine real_math.h `cosine()`/`sine()` helpers. The newly emitted `_cosine`/`_sine` copies are identical to January's selected actor_combat copies and pair-link in both orders. | `_actor_look_update` register/slot residual |
| xbox_texture_cache `_texture_cache_steal_memory` | 224 | A named `stolen_address` local, used twice. An unused-local control stays residual. | texture_cache header / prototype / bitmap-schema debt (review items a-e) |

### Zero-credit ownership corrections (verified; no bytes move)

- **profile:** `compare_profile_sections`, `header_strings` and `format_strings` are external
  and `profile_globals` is static, per cachebeta publics.
- **transport_endpoint_winsock:** `connection_thread_list_maintenance` and
  `transport_endpoint_globals` are file-static.
- **actor_firing_position:** 14 file-static functions; the evaluator tables and sort globals are external.
- **game_engine:** 12 functions made static. The three hand-inlined copies of
  `game_engine_get_type` become calls, and the /Od-attested
  `game_engine_press_start_to_begin()` call is restored in `nonplayer_post_rasterize`
  (/Od 0x5a1600).
- **real_math:** the invented `real_math_constants` aggregate becomes four genuine file
  statics (HCEX real_math compiland).
- **model_animations:** five keyframe helpers are static.

These four objects (actor_firing_position, game_engine, real_math, model_animations) were function-
and data-exact after batch 1. The `distance3d` provider (fixed by Layer 2) and, for real_math, the
actor_combat `cross_product2d` copy kept them from admission. Batch 3 admits actor_firing_position
and model_animations. game_engine and real_math stay held (see below).

## Layer 2 (931ed8dc): shared real_math.h packet (owner ruling pending on one part)

**Landed and verified:**

- real_math.h `distance2d`/`distance3d` are `return square_root(distance_squared*(a, b));`, as in
  the /Od build (no local).
- `project_point2d` uses short mapping locals and one `?:` store. It is /Od-attested and equal to
  January's selected copy.
- action_vehicle, path_obstacles and decals drop their `#define x x_inline` renames and NODUP
  hand copies.
- object_lights and players use the /Od caller spellings.
- Every emitted helper copy is byte-identical to January's selected section. Every provider row is
  unchanged. The adversarial review (research `w/review_layer2/LEDGER.md`) reproduced the board
  sweep exactly.

**Pending owner ruling: the lock/unlock_global_random_seed prototype move.**

The move takes these two prototypes from random_math.h, a 2026-09-04 reconstruction header, into
real_math.h's existing RANDOM_MATH.C section. The evidence for that placement:

- real_math.h already has that section.
- The supplied genuine random_math.c includes real_math.h and not random_math.h.
- main.c shows C4013 for both functions without it.

But the move is also declared-name-count compensation. The corrected helper bodies remove 4
declared names from every real_math.h consumer, and the move adds 2 back. The bytes decide only
that count, not which prototypes move:

- **Strip test (R6 V1).** The landed header without the move loses `_bitmap_copy`,
  `_rasterizer_frame_statistics_draw` and `_get_edge_vertex` (7,376 B, all exact at base). It also
  changes `_collision_move_point` (already residual).
- **Count-only subset (R10 V5/V6).** Moving {lock, get_random_seed} or {unlock, get_random_seed}
  gives the same result.
- **The evidence-equal full move (R6 V2)** takes lock, unlock, get_random_seed and
  random_seed_debug_log, and regresses 4 other exact functions: `_bitmap_2d_alpha_bleed`,
  `_decals_delete_permanent_from_cluster`, `_rasterizer_frame_statistics_draw` and
  `__rasterizer_model_transparent_geometry_submit`.
- get_random_seed still raises C4013 in network_client_manager.c.

**If the owner rejects the move, reverting the two prototype lines alone is wrong,** because it
regresses the three functions above. Batch 3 admissions now depend on the Layer 2 bodies:

- leaf_map and biped_limp_noodle (their `_distance3d` rejections were retired);
- actor_firing_position, path and model_animations (distance3d provider).

The review's fallback diff (`w/review_layer2/fallback_revert_except_players.diff`) no longer
applies after batch 3. A rejection therefore means choosing between two costs:

- lose the three count-tied functions (7,376 B); or
- revert Layer 2 and revoke those admissions, `_convex_hull2d_perimeter` and
  `_player_set_action_result`.

**Reopen criterion:** move get_random_seed and random_seed_debug_log too, and retire
random_math.h, once the other declared-name gaps in real_math.h are recovered. Its
nonuniform_cubic_spline bodies are known decompiler output (v9..v20 locals), so the header's
count is already known to be off.

## Batch 3 (b807f9f6 header prerequisites, then the admission batch)

Result: Halo objects **363 -> 375** (+12). Stable diff: 4 new strict functions, 2,784 padded
(2,765 meaningful), **0 regressions**. Parks 89 / 0 / 0. Admission 11 / 0 / 5 / 0.

- **Fake scan: 26 leads.** The new lead is first_person_weapons' empty key-frame then-arm. It is
  kept because the /Od build has the empty-then `jne; jmp` and January's bytes have the `cmp ax,1`.
  It sits in a zero-credit, still-parked function.
- pytest 1,157 / 5 / 26; `git diff --check` clean.
- **Data +200** (path_structure_bsp tables) **+1,564** (units '$'-defect entry).
- **Scorer-only (not counted):** marking objects and units complete makes objdiff credit
  +147 code / +7 functions for their internal `$L` labels.

**Header prerequisite commit b807f9f6 (consumer sweep: full ninja, 0 moved rows):**

- **objects.h.** Both bounding-sphere inline assertions report `OBJECTS_H_FILE`. It defaults to
  January's relative `..\objects\objects.h`; object_lights defines the absolute spelling first.
  January's string census has the relative string defined by action_vehicle (and referenced by
  render_objects), and the absolute one defined by object_lights. action_vehicle loses its NODUP
  hand copy. render_objects' `lighting_storage` becomes HCEX's static local of
  `object_get_cached_render_lighting`.
- **path.h.** The opaque `__unknown140A0[0x5C]` span is HCEX's `struct path_result result`. The
  punning store in path.c is gone. The 53 path.h consumers show 0 section differences.

### Objects admitted (12)

| Object | Work | Evidence (research `w/<slug>/`) |
|---|---|---|
| units/units | ownership | 189/189 already exact. 25 `code_` placeholders get first-party names (January atlas exact tier, Sept-2001 maps, HCEX_Release). The worker's `unit_verify_inventory` was wrong: the reviewer found it is `unit_vectors_are_valid`, so the amended patch is used. 49 static flags from cachebeta publics. The '$'-defect data entry is `_magic_base_animation_seat_index` (1,564 B). The `_point_from_line3d` rejection is retired under ruling 20260921 #1. `units_ownership`, `review2_units` |
| physics/collision_debug | house-rule fix | The hand-expanded helpers become the /Od-attested `magnitude3d`/`arccosine` calls. `_collision_debug_cube_vertices` is static. 8 surplus COMDATs, 3 D3D tables and 11 literals are identical and pair-link. `collision_debug` |
| render/render_objects | provider packet (b807f9f6) | Already 22/22 and data 100%. The object is admitted once the bounding-sphere provider and the static-local storage are fixed. `bounding_sphere` |
| camera/editor_flying_camera | new code + storage | `_editor_camera_set_position_and_roll` (432) uses the /Od RTC locals in declaration order. The /Od-attested dead `left` pair is kept, and the in-source comment says it is never read. The reviewer ruled it admissible: the /Od build has the same RTC name and statement pair, the assert anchors show zero drift, the effect depends on content, and a count-only control (R4) is not exact. The first euler3d->euler2d view cast has per-site /Od pass-through, a byte-inert strip test and a strictly exact caller. `editor_camera_speed(_steps)` are static per cachebeta; the grouped entry is re-pinned. `editor_flying_camera`, `review2_editor_flying_camera` |
| rasterizer/xbox/rasterizer_xbox_shadows | new code + storage | `__rasterizer_environment_shadow_model_draw` (688). The model_draw bare block is load-bearing and independently attested: the camo precedent cfa20176 and the same scheduling signature at rasterizer_xbox_models +0x12bd. The gap statics have descriptive names (disclosed) and are split as the align-4 .bss and HCEX's loose statics suggest. `rasterizer_xbox_shadows` |
| cache/cache_files_windows | new code | `_cache_files_open_cache_files` (752). The trailing cleanup follows the CreateFile/resize idiom of /Od 0x857520. That is an analog from game_state_pc.c, disclosed as analog evidence. The plain spelling needs no decoration, and equivalent spellings also match. `cache_files_windows` |
| objects/widgets/light_volumes | new code + storage | `_light_volume_render` (912). This is the reviewer's amended st6b: its declaration order follows the /Od frame, and only `external_scale` (address reaches an out-of-line call) is block-scoped. `light_volumes`, `review2_light_volumes` |
| ai/actor_firing_position | admission | Already function- and data-exact. The batch-1 storage packet is verified against cachebeta publics. The two view casts are /Od-attested (0x45bd00, 0x456860). `review_admit2` |
| ai/path | admission + header | path.h HCEX field (b807f9f6). 32/32, audit PASS, 10/10 surplus identical. `review_admit2` |
| units/biped_limp_noodle | storage | The invented `biped_limp_noodle_globals` aggregate becomes HCEX's `features` and `last_positions`, as file-scope statics. Static locals were refuted: they put `last_positions` first in .bss. The `_distance3d` rejection is retired. Advisory, not added: move_relax returns early twice without popping a collision user (authentic; no BUG comment). `review_admit2` |
| structures/leaf_map | house-rule fix | `normalize_three_dee` calls `magnitude3d` + `scale_vector3d`, as /Od 0x89fc90 does. The `_distance3d` rejection is retired. `review_admit2` |
| models/model_animations | house-rule fix | `animation_get_root_velocity` calls `vector_from_points3d`, as /Od 0x708160 does. `review_admit2` |

### Verified zero-credit or data corrections landed with batch 3

- **path_structure_bsp:** HCEX-named file-static `quantized_pathfinding_surface_widths/heights`
  tables, +200 data. The object still needs the actor_combat `cross_product2d` ruling.
- **hud_nav_points:** `hud_update_nav_point_local_player` and `nav_point_data` are static per
  cachebeta. The bounding-sphere blocker is now resolved; only `_custom_render_nav_point` remains.
- **first_person_weapons patch A:**
  - `first_person_weapon_message` is static, with its prototype moved from the header into the
    TU's private block.
  - A genuine key-frame fix: the old `||` advanced the state on key frames.
  - The TU `animation_update_render_only` wrapper is used; `real_vector2d turning`;
    `sound_definition_index`.
  - The `_first_person_weapon_update` park is re-baselined (95.21506).

## Batch 4 (f7cd2e72 header packets, 6e3e2d35 admissions and storage)

Result: Halo objects **375 -> 378** (+3). Stable diff: 3 new strict functions, 2,928 padded
(2,911 meaningful), **0 regressions**. Parks 86 / 0 / 0. Admission 11 / 0 / 3 / 0. Fake scan 26
(unchanged). pytest 1,157 / 5 / 26. `git diff --check` clean.

### Objects admitted (3)

| Object | Work | Evidence (research `w/<slug>/`) |
|---|---|---|
| interface/player_ui | shared-header layout recovery | Two edits in the header commit f7cd2e72. (1) `game_engine.h`'s opaque `game_variant` view becomes the recovered January layout: universal/ctf/slayer/oddball/king/race variant members, with HCEX names only where January's offsets attest the field. The game engines, network managers and playlist_profile use the new member names. (2) `player_ui_get_edit_playlist_profile` returns `struct game_variant *`. The reviewer found two C4133 warnings in ui_widget_game_data_input_functions that the worker missed; the reviewer's patch 07 fixes them. Sweep: 46 header consumers, 0 moved rows. `player_ui`, `review3_player_ui` |
| sound/sound_manager | new code + shared header | `_update_channel_for_looping_sound` (1,360) and `_sound_set_definition_end` (208) are exact. The shared header `sound_definitions.h` now declares HCEX's `short old_range_index`, landed together with the /Od 0x877810 single-exit definition; this avoids the prototype/definition mismatch the owner rejected earlier. The redundant `channel_state == _sound_channel_queued &&` term is load-bearing and /Od-attested verbatim (0x89a5a1). 45 statics per cachebeta publics. Sweep: 18 consumers. `sound_manager`, `review3_sound_manager` |
| rasterizer/xbox/rasterizer_xbox_dynavobgeom | view removal + storage | Genuine shared declarations replace the caller-local debug-options, window and pixel-shader views. The unlit draw uses the /Od-attested `vector_from_points3d`/`dot_product3d` calls and HCEX's `forward`/`zero_plane` locals. The HCEX function-static `warned` latch is not public in cachebeta. `MAX()` reproduces the /Od `?:` store. Nine D3DINLINE wrappers are static. The surplus `real_*_to_pixel32` COMDATs and SDK tables are identical to January's selected copies and link in both orders. January's own object has UNDEF references to them, so its TU emitted them. `dynavobgeom`, `review3_rasterizer_xbox_dynavobgeom` |

### Function gains and zero-credit packets landed in batch 4

- **damage:** `_object_damage_body` is exact (1,360; reviewer patch B+). Patch A would newly emit
  the `collision_test_line`/`real_random` header COMDATs from a non-exact caller, so it is held (P1
  class).
- **players:**
  - Storage for three rows.
  - COMMON tentative definitions of `players_globals`/`team_data`. January's `linker_common`
    records 154-156 are players.c's, and HCEX has them as DataIsGlobal.
- **hs:**
  - 839 January-static storage flags.
  - The /Od-attested static helper `hs_enumerate_scenario_data` replaces 11 hand expansions (the
    reviewer's amendment; its PDB storage disagreement goes to 0).
  - The worker's `real_random_range` cleanup is held (P1 class).
- **hs_scenario_definitions:** first-party `static byte_swap_script_syntax_data` (the reviewer's
  fallback). The 9-name data split stays held.
- **player_profile:** HCEX `profile_color_table`; the write-thread park is re-baselined
  (88.40708 -> 88.451324).
- **main:**
  - `main_exit` in the /Od 0x6aff90 shape, and 25 statics.
  - January-resolved corrections: strict `>`, dead `requested_rate` store dropped, and
    `best_interval` as the `" des %d"` argument.
  - Parks re-baselined: `_main_update_time` 62.637814 -> 98.986336, `_main_frame_rate_debug`
    94.51923 -> 98.65385.

### Process incidents in wave 3 (repaired, no tracked change)

Both happened inside reviewer agents, and both were repaired byte-exact before integration:

- A broken `&&` chain ran `rm -rf .git` in the worktree root. The reviewer restored the
  worktree pointer file.
- `tools/campaign/volatile_scan.py --help` ran its hard-coded scan, because the script has no
  argparse. The scan rewrote `source/hs/hs.c` to LF; the reviewer restored it from backup.

The integrator verified that the tree was clean and hs.c all-CRLF before batch 4. The worker
brief now forbids destructive commands outside a worker's slug and running unread repo scripts.

## Batch 5 (466698b8 hud_unit, cdc8ebd3 hygiene)

Result: Halo objects **378 -> 379** (+1). The stable diff is unchanged (0 moved rows) and there are
**0 regressions**. Parks 86 / 0 / 0. Admission 11 / 0 / 2 / 0. pytest 1,157 / 5 / 26.

| Object | Work | Evidence |
|---|---|---|
| interface/hud_unit | header inline + house-rule fix | January's 26 stack-sentinel sites across hud_draw, hud_unit, hud_nav_points, hud_weapon and motion_sensor share one inlined shape. Both asserts at each site carry one line literal, so it is a macro. January's 48-byte `_check_stack_buffer` has no relocations anywhere. The Sept-2001 map flags the helper as an inline COMDAT (`i hud_draw.obj`). `hud_draw.h` now carries the unchanged body as a header `__inline` plus the sentinel macro (`match_assert_stack_frame`; descriptive name, since neither /Od nor HCEX has the sentinel). That removes hud_unit's invented gotos. hud_nav_points and hud_weapon use it too; their callers are exact. motion_sensor keeps its copy, because its sentinel callers are residual (rule i). Also: the reviewer-amended names and storage, and retirement of the `_fast_ftol` rejection (identical copy; pair links pass both ways). `hud_stack_check`, `review4_hud_unit`, `review4_hud_draw` |

**Disclosed, pending owner ruling (P7).** The packet drops rasterizer_xbox_models' `#include
"interface/hud_draw.h"`:

- The include is genuinely dead: it was added by project commit 635bd83d, and the file uses no name
  from the header. Removing it is raw- and warning-inert.
- It is load-bearing only as declared-name shielding. The inline adds about 9 names, which would
  flip `__rasterizer_model_transparent_geometry_submit` (1,296 B).
- It is not the held add-declarations class (Layer 2, main header_swap), but it is a count effect
  and is disclosed as one.
- If the owner rejects it, revert 466698b8. That loses the hud_unit admission; nothing else was
  exact because of it.

Zero-credit hygiene in cdc8ebd3:

- geometry drops its NODUP `plane2d_distance_to_point` copy, which removes a latent
  collision_bsp<->geometry LNK2005;
- collision_bsp `collision_bsp_usage_times` is static;
- rasterizer_lights P1: HCEX file statics with January's .bss layout;
- motion_sensor .bss symbol layout is fixed;
- rasterizer_xbox_models: five D3D wrapper rows are static.

Wave 4 found no new strict function. Every remaining residual in those lanes falls into one of
three groups:

- the authentic-bug owner class: crosshairs_draw, hud_update_weapon_local_player,
  motion_sensor_update, rasterizer_lights_reset_for_new_map;
- the P1 class, where a new header COMDAT would come from a non-exact caller;
- scheduler/allocator ties where the stop rule fired (evidence in `results/wave4/`).

## Running tallies (vs frozen base e9e62b78, objdiff 3.3.1; at cdc8ebd3)

| Tally | Value |
|---|---|
| 1. Net newly COMPLETE Halo objects | **18** (361 -> 379); checkpoints: 10 at b9a8d587 |
| 2. New strict functions | **14** (stable diff; 0 regressions) |
| 3. New meaningful exact bytes | **8,775** |
| 4. New padded exact bytes | **8,864** |
| 5. New verified data bytes / admission-only closures | **3,294** data (1,530 + 1,564 + 200); 12 admission-only closures (objects, units, render_objects, actor_firing_position, path, biped_limp_noodle, leaf_map, model_animations, collision_debug, player_ui, rasterizer_xbox_dynavobgeom, hud_unit) |
| 6. Fuzzy improvements at zero credit | `_main_update_time` 62.64 -> 98.99; `_main_frame_rate_debug` 94.52 -> 98.65; `_player_profile_write_thread_proc@4` 88.41 -> 88.45; first_person_weapons A (95.21506); plus the FUZZY_IMPROVED rows in the research results |
| Scorer-only effects | +147 code / +7 functions (`$L` label credit from status flips). NOT taken: objdiff 3.6.0 would credit hs's 54,780 data bytes and actions' 2,404 at zero source cost |
| Regressions / revocations | 0 / 0 |

Contingent on the Layer 2 ruling: `_convex_hull2d_perimeter` (96), `_player_set_action_result`
(240), and the leaf_map, biped_limp_noodle, actor_firing_position, path and model_animations
admissions (see the Layer 2 section). Contingent on the P7 ruling: the hud_unit admission (see batch 5).

## Held for owner rulings (not landed; evidence preserved)

Every item's evidence is under `research/fifty_objects_20260925/` (`w/<slug>/` for worker and
review ledgers and patches, `results/wave*/` for the structured results).

| Item | Would unlock | Ruling needed | Reopen criterion | Evidence |
|---|---|---|---|---|
| actor_combat P1: remove the NODUP `cross_product2d` hand copy and restore the /Od-attested call in `_actor_aim_grenade` | **5 objects**: path_obstacle_avoidance, path_smoothing, real_math, collision_features, path_structure_bsp | Admit a January-owned header COMDAT, and a byte-inert /Od-attested view cast, whose caller is not yet strictly exact. The edit is byte-inert on all 81 sections (NODUP -> ANY only). | Owner admits P1, or `_actor_aim_grenade` goes strict-exact with the /Od helper calls | `w/cross_product2d/` |
| Layer 2 prototype move (landed) | keeps 3 count-tied functions | name-count compensation in a genuine owner header | see the Layer 2 section | `w/review_layer2/` |
| game_engine_king | 1 object | reproduces January's uninitialised `[ebp-4]` read in `_find_next_hill` (authentic bug class) | owner admits the BUG-commented read | `w/king/`, `results/wave2/REVIEW__source_game_game_engine_king.md` |
| glow `_get_particle_world_position` | 1 object | two authentic uninitialised array reads (also in the /Od build); ruling 20260920 may already allow them | owner admits the BUG comments | `w/glow/` |
| first_person_weapons patch B | 1 object (34/34) | authentic unguarded NULL dereference (January, /Od, HCEA 2011) | owner admits the BUG comment | `w/first_person_weapons/owner_gated_exact.patch` |
| dead_camera `_dead_camera_update` | 1 object | authentic uninitialised `next_unit_index` read | owner admits the BUG comment | batch-1 ledger |
| object_lights | 1 object | a surplus `_object_get_bounding_sphere` copy that differs from the selected copy only in its January-proven per-TU `__FILE__` literal; also needs data_gap's packet | owner ruling | `w/bounding_sphere/`, `w/data_gap/` |
| rasterizer_xbox | 1 object | a .bss split that adds 9 descriptive (non-first-party) static names | owner policy ruling | `w/rasterizer_xbox_audit/` |
| rasterizer_xbox_hardware_geometry | 1 object | choose `IDirect3DResource8_MoveResourceMemory` or `D3DVertexBuffer_MoveResourceMemory` for the no-op wrapper at 0x158460 (byte-identical) | owner picks A or B, or first-party evidence | `w/hardware_geometry/` |
| network_connection connect form | 2 functions | already under owner review; the reviewer recommends rejecting it (`success = TRUE;` steering) | owner packet | `w/network_connection/` |
| breakable_surfaces B1 | 1 object | the genuine `real_local_random` body adds one declared name board-wide and flips two fragile ties | declared-name gap recovery, or owner ruling | `w/breakable_surfaces_audit2/` |
| actions grouped data entry | +2,404 data | tools change to `semantic_progress.py` for grouped extents (scorer change; not taken in this lane) | owner or integrator accepts the verifier fix, or the scorer moves to 3.6.0 | `w/actions/verifier_combined_extent.diff` |
| render_debug | 1 object | an invented `render_debug_globals_definition` aggregate whose separate-static names are unrecoverable, plus six consumer-local `extern boolean debug_*` with no definer | owner ruling or new naming evidence; the helper/vehicle patch (a)-(c) is ready at zero credit | `w/review_admit2/patches/render_debug_genuine_helpers_and_vehicle.patch` |
| bitmap_drawing | 1 object | the unnamed 16-byte static stays held under the 2026-09-20 ruling | first-party name and type | `results/wave1/` |
| hud_unit | 1 object (+ hud_weapon/motion_sensor/hud_nav_points hygiene) | the stack-sentinel code is a hand-copied `check_stack_buffer` with two invented gotos (held class); January's asserts share one line (a macro), and the Sept-2001 map tags the helpers as inlines | a `hud_draw.h` `__inline check_stack_buffer` packet (wave 4), or an owner ruling; the reviewer-amended 01R + 02-04 are ready | `w/review3_hud_unit/` |
| rasterizer_xbox_profile | 1 object | invented TU-private aggregates (pad members; loose statics need new descriptive .bss names = held class), a /Od-attested `%s`-with-NONE crash path (BUG class), three load-bearing `volatile`s | owner rulings on names, BUG and volatile | `w/rasterizer_xbox_profile/`, `w/review3_rasterizer_xbox_profile/` |
| hs_scenario_definitions data split | 1 object | 9 file-static data names with no first-party attestation (1 invented) | owner ruling on descriptive names | `w/hs_scenario_definitions/`, `w/review3_hs_scenario_definitions/` |
| models .bss | 1 object | the only exact form is file-scope `= {0}` statics in reverse use order, contradicting the first-party static-local form; 3 of 4 names are lost | owner ruling or first-party names | `w/models_bss/` |
| transport_endpoint_set_winsock `net_startup_debug();` | storage only | a call needed only to make VC7 emit the static; its arity contradicts the 2001 `@net_startup_debug@4` evidence | owner ruling (+ signature) | `w/transport_endpoint_set_winsock/`, reviewer slug |
| player_profile R1-R3 | 3 functions | authentic defects: uninitialised serialised block, file left open, close after delete (BUG comments ready) | owner ruling (authentic-bug class) | `w/player_profile/production_owner_gated.patch` |
| main header_swap / main_crash | 1 function | declared-name-count compensation (Layer 2 class) / authentic NULL write | owner ruling | `w/review3_main/` |
| damage patch A / hs real_random_range cleanup | 0 (house-clean) | new header COMDAT emitted from a non-exact caller (P1 class) | same as P1 | `w/damage/`, `w/review3_storage_packet_/` |

## Research material

`research/fifty_objects_20260925/` is regenerated from the ignored `scratch/` by
`scratch/campaign/curate.py`. It holds every worker and reviewer ledger, the patches, and the
best non-exact candidates as `*.vs_tree.diff` against this tree. `.obj` files, split copies and
symbols.json copies are excluded, and the folder is exempt from EOL and whitespace conversion so
the patches stay byte-exact.

## Final handoff

- **Branch:** `claude/fifty-objects-20260925`. Local only; never pushed; canonical never edited.
- **Base:** e9e62b78, the `git ls-remote` tip of `jonas/exact-pilots` at lane start.
- **Tip:** the commit that adds this section. The last code commit is cdc8ebd3.
- **Scorer:** objdiff-cli **3.3.1** (sha1 3130e428) for every number. Not upgraded; 3.6.0 figures appear only where marked scorer-only.

### Commit order

| Commit | Content |
|---|---|
| bf937aac | Batch 1: files_windows, objects; 5 exact functions; ownership corrections |
| 931ed8dc | Layer 2: real_math.h helper bodies, provider hand-copy removal (**lock/unlock prototype move pending owner ruling**) |
| b807f9f6 | Header prerequisites: objects.h `OBJECTS_H_FILE`, path.h HCEX field |
| b9a8d587 | Batch 3: 12 objects, 4 exact functions |
| 954eebd2 | Research evidence (waves 1-2) |
| f7cd2e72 | Batch 4a: player_ui (game_engine.h/player_ui.h), sound_manager (sound_definitions.h) |
| 6e3e2d35 | Batch 4b: dynavobgeom; damage `_object_damage_body`; storage packets |
| 16542e46 | Ledger + wave-3 evidence |
| 466698b8 | Batch 5a: hud_unit via the hud_draw.h `check_stack_buffer` inline (**P7 pending owner ruling**) |
| cdc8ebd3 | Batch 5b: zero-credit storage and provider hygiene |
| eb33baec | Ledger: batch 5 |
| (this commit) | Owner queue, final handoff, wave-4/5 evidence |

Changed paths, excluding `research/`, as of eb33baec: 75 files, +3,717 / -4,304 lines. This commit adds the owner-queue doc.

- **config (5):** config.json, object_admission_rejections.json, parked.json,
  semantic_data_matches.json, symbols.json.
- **Shared headers (10):** real_math.h, random_math.h, objects.h, path.h, game_engine.h,
  player_ui.h, sound_definitions.h, hud_draw.h, first_person_weapons.h, light_volumes.h.
- **Source (59 .c files)** and **docs (2):** this ledger and the owner queue.

### Target vs result

**Target: 50 new complete Halo objects. Verified: 18** (361 -> 379). The evidence cannot support
50 without owner rulings. The measured owner queue
(`claude_fifty_objects_20260925_owner_queue.md`) adds 16 more on an all-yes, excluding the two
items canonical already superseded. That gives 34, still short of 50. Every remaining
self-landable residual in the pool is a documented tie, where the stop rule fired, or an
owner-gated form.

| # | Object | Kind of work |
|---:|---|---|
| 1 | tag_files/files_windows | new code (file_get_size) |
| 2 | objects/objects | provider repair (Layer 1) + data entry |
| 3 | units/units | ownership: first-party names, storage, data entry |
| 4 | physics/collision_debug | house-rule fix (/Od helper calls) + storage |
| 5 | render/render_objects | provider packet (objects.h) + static-local storage |
| 6 | camera/editor_flying_camera | new code + storage |
| 7 | rasterizer/xbox/rasterizer_xbox_shadows | new code + storage |
| 8 | cache/cache_files_windows | new code |
| 9 | objects/widgets/light_volumes | new code + storage |
| 10 | ai/actor_firing_position | admission (storage packet verified) |
| 11 | ai/path | admission + path.h HCEX field |
| 12 | units/biped_limp_noodle | storage (HCEX file statics) |
| 13 | structures/leaf_map | house-rule fix (/Od helper calls) |
| 14 | models/model_animations | house-rule fix (/Od helper call) |
| 15 | interface/player_ui | shared-header layout recovery (game_variant) |
| 16 | sound/sound_manager | new code + shared header |
| 17 | rasterizer/xbox/rasterizer_xbox_dynavobgeom | view removal + storage (**see policy conflict below**) |
| 18 | interface/hud_unit | header inline + house-rule fix (**P7 disclosure**) |

Checkpoints: 10 objects at b9a8d587. 20/30/40/50 were not reached. No 10,000-meaningful-byte
checkpoint was reached either: the total is 8,775.

### Tallies (vs e9e62b78, objdiff 3.3.1)

| Tally | Value |
|---|---|
| 1. Net newly complete Halo objects | **18** |
| 2. New strict functions | **14** (stable 8,245-function diff; 0 regressions) |
| 3. New meaningful exact bytes | **8,775** |
| 4. New padded exact bytes | **8,864** |
| 5. New verified data bytes / admission-only closures | **3,294** / **12** |
| 6. Fuzzy improvements at zero credit | `_main_update_time` 62.64 -> 98.99, `_main_frame_rate_debug` 94.52 -> 98.65, `_player_profile_write_thread_proc@4` 88.41 -> 88.45, first_person_weapons A 95.21506; more in `research/.../results/` |
| Scorer-only (not counted) | +147 code / +7 functions (`$L` label credit on status flips); objdiff 3.6.0 would add hs 54,780 and actions 2,404 data bytes at zero source cost |
| Regressions / revocations | **0 / 0** |

**Contingent credits:**

- Layer 2 ruling: `_convex_hull2d_perimeter`, `_player_set_action_result`, and the leaf_map,
  biped_limp_noodle, actor_firing_position, path and model_animations admissions.
- P7 ruling: the hud_unit admission.

### House-rule review (source, headers, COMDATs, providers)

**Source.**

- Every landed packet passed an independent adversarial reviewer. Each reviewer checked house
  rules 1-22 and the strip test, ran /Od declaration-order checks for any new scope, and verified
  storage claims against cachebeta publics.
- Where a reviewer issued an amendment, the amendment is what landed:
  - units (unit_vectors_are_valid);
  - light_volumes (/Od declaration order);
  - dynavobgeom (MAX);
  - hs (`hs_enumerate_scenario_data`, no cleanup COMDAT);
  - main (`main_exit` /Od shape);
  - damage (B+, not A);
  - player_ui (patch 07);
  - hud_unit (01RS; P6 dropped).
- The fake-match lead count went 25 -> 26. The new lead is the /Od-attested empty then-arm in
  the still-parked first_person_weapons update.

**Headers.** Every shared-header edit landed with a full ninja and stable diff, with 0 regressions:

- real_math.h/random_math.h;
- objects.h, path.h;
- game_engine.h, player_ui.h, sound_definitions.h;
- hud_draw.h;
- first_person_weapons.h.

Count-sensitive edits are disclosed (Layer 2 move, P7). No filler declarations landed.

**COMDATs.** Newly emitted header-inline COMDATs come only from strictly exact callers (rule i).
Each is identical to January's selected copy and passes provider links in both orders.

**Providers.** Four sets of NODUP hand copies were removed:

- Layers 1 and 2: 17 helpers;
- action_vehicle `object_get_bounding_sphere`;
- geometry `plane2d_distance_to_point`;
- hud_draw `check_stack_buffer`.

Known remaining NODUP conflicts:

- actor_combat `cross_product2d` (owner item P1);
- effects `real_local_random`;
- player_control `limit2d`;
- decals `plane2d_from_points`;
- items `object_get_type`.

**Disclosed judgement calls:**

- the Layer 2 prototype move and P7 (count effects, pending rulings);
- editor_flying_camera's /Od-attested dead `left` pair;
- the descriptive names of rasterizer_xbox_shadows' gap statics and of the
  `match_assert_stack_frame` macro;
- cache_files_windows' analog evidence;
- rasterizer_lights' load-bearing `= {0}`. Canonical a595bbc2 later landed the same idiom for
  models.

### Held and rejected items

Every held item has an apply-ready packet, a ruling question and measured unlocks in
`claude_fifty_objects_20260925_owner_queue.md`. The items with no object at stake are in the
held-items table above. Evidence for each is under `research/fifty_objects_20260925/`.

Items that are not to be retried without new evidence:

- the brief's do-not-reopen list (decals/physics, path_obstacles parenthesis, aim_grenade goto, hud_draw pragma, s3tc, render_sprite, incompatible providers, flags `x * height`);
- periodic_functions' DAG tie (~150 respellings);
- the residuals whose stop rule fired, listed in `results/wave3` and `results/wave4`.

### Process incidents

Both happened in wave 3, inside reviewer agents, and both were repaired byte-exact before
integration:

- `rm -rf .git` in the worktree root;
- a hs.c rewrite by `volatile_scan.py`.

No tracked change resulted. The worker brief now forbids both patterns.

### Expected integration conflicts (trial `git merge-tree` against canonical dbcea3d6)

Canonical gained 9 commits after e9e62b78. The two lanes' new Matching sets are disjoint:
canonical added models, hs_scenario_definitions and hs_globals_external. A resolved merge
therefore expects **382** Halo objects, subject to a full gate. Five files conflict:

| File | Resolution guidance |
|---|---|
| `source/ai/path_structure_bsp.c` | Both lanes added the same HCEX tables (canonical e208c87b). Take canonical's spelling `static real const ...[8]`, drop this lane's comment, and re-gate (bytes expected identical). |
| `source/hs/hs_scenario_definitions.c` | Take canonical's separate data definitions (Matching upstream). This lane's static `byte_swap_script_syntax_data` rename (6e3e2d35) is subsumed if canonical has it; keep canonical's symbols.json rows. |
| `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c` | Both lanes replaced the local views with genuine owner types. This lane also has the /Od helper calls, HCEX locals, the `warned` latch and MAX, and admits the object. Canonical 641e1466 keeps a narrowed rejection instead. **Policy conflict, needs an owner decision:** see below. |
| `config/symbols.json` | Line-level overlaps on the dynavobgeom wrapper rows, the path_structure_bsp tables and the hs rows. Take the union; the rows are identical where both edited. Never re-serialise. |
| `config/object_admission_rejections.json` | Take the union of retirements. This lane retired objects, leaf_map, biped_limp_noodle, units, the hud_unit `_fast_ftol` rejection and dynavobgeom. Canonical retired hs_scenario_definitions, hs_globals_external and models. Canonical's new narrowed dynavobgeom entry follows the policy decision below. |

Canonical also changed `tools/campaign/gate.py`, `stable_verdicts.py` and `verdicts.py`, which
now identify functions by COFF type 0x20, so fastcall functions count. Re-take the stable
snapshot after merging before comparing function counts.

**Dynavobgeom and SDK-table policy conflict.** Canonical 641e1466 declined to admit dynavobgeom
for two reasons:

- the unreconciled `global_window_parameters` extern;
- selected-provider links "not a complete ordinary program link" for the three stock D3D SDK
  tables every D3D8.h TU emits.

This lane admitted objects under the brief's criterion: identical to January's selected copy,
plus pair links passing in both orders. A census on this tree (built objects vs build/split) finds:

- **55 objects canonical already lists as Matching emit the same three surplus D3D SDK tables.**
- **9 of this lane's 18 admissions emit them too:** files_windows, objects, units,
  collision_debug, rasterizer_xbox_shadows, cache_files_windows, biped_limp_noodle,
  sound_manager and rasterizer_xbox_dynavobgeom.
- Seven Matching objects declare `global_window_parameters` the same consumer-local way as
  dynavobgeom.

Two outcomes:

- If the owner applies canonical's stricter standard uniformly, those 64 objects need revisiting,
  not only this lane's.
- If the owner keeps the brief's standard, canonical's narrowed dynavobgeom rejection should be
  retired at reconciliation.

This conflict was found at handoff, after all admissions landed.
