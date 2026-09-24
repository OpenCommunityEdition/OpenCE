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

## Running tallies (vs frozen base e9e62b78, objdiff 3.3.1)

| Tally | Value |
|---|---|
| 1. Net newly COMPLETE Halo objects | **14** (361 -> 375) |
| 2. New strict functions | **11** (stable diff; 0 regressions) |
| 3. New meaningful exact bytes | **5,864** |
| 4. New padded exact bytes | **5,936** |
| 5. New verified data bytes / admission-only closures | **3,294** data (1,530 + 1,564 + 200); 9 admission-only closures (objects, units, render_objects, actor_firing_position, path, biped_limp_noodle, leaf_map, model_animations, collision_debug) |
| 6. Fuzzy improvements at zero credit | first_person_weapons A (95.21506), plus the wave-2 FUZZY_IMPROVED rows in the research results |
| Scorer-only effects | +147 code / +7 functions (`$L` label credit from status flips); objdiff 3.6.0 not used |
| Regressions / revocations | 0 / 0 |

Contingent on the Layer 2 ruling: `_convex_hull2d_perimeter` (96), `_player_set_action_result`
(240), and the leaf_map, biped_limp_noodle, actor_firing_position, path and model_animations
admissions (see the Layer 2 section).

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

## Research material

`research/fifty_objects_20260925/` is regenerated from the ignored `scratch/` by
`scratch/campaign/curate.py`. It holds every worker and reviewer ledger, the patches, and the
best non-exact candidates as `*.vs_tree.diff` against this tree. `.obj` files, split copies and
symbols.json copies are excluded, and the folder is exempt from EOL and whitespace conversion so
the patches stay byte-exact.
