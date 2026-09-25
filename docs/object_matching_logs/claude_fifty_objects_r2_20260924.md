# Fifty-object campaign, round 2 (2026-09-24)

- **Worktree:** `C:\halo-worktrees\claude-fifty-objects-r2-20260924`.
- **Branch:** `claude/fifty-objects-r2-20260924`.
- **Base:** canonical `jonas/exact-pilots` **f6d00a8c**, verified with `git ls-remote`. This is the
  reconciliation merge of round 1 (`claude/fifty-objects-20260925` at 4e84c212) into canonical
  dbcea3d6.
- **Scorer frozen:** objdiff-cli **3.3.1** (sha1 3130e4288d483d259d1588092c8159f8e0230e08).
  - `configure.py` pins 3.6.0. A fresh worktree downloads 3.6.0 on its first ninja run, and
    `progress` then fails on canonical's 3.3.1-authored editor_flying_camera data group.
  - The 3.6.0 binary was moved aside and canonical's 3.3.1 restored before any measurement.
- **Commits:** local only; never pushed; canonical never edited.
- **Reserved:** the external Codex packet (the actor_combat `cross_product2d` provider repair plus
  path_obstacle_avoidance, path_smoothing, real_math, collision_features, path_structure_bsp) is
  reserved in the claim registry and not duplicated.

## Frozen baseline (f6d00a8c, full ninja, objdiff 3.3.1)

| Measure | Value |
|---|---|
| Halo objects complete | **381 / 468** (All 499) |
| Halo credited code | 1,573,109 / 1,770,166 (7,442 functions) |
| Halo data | 2,583,826 |
| Stable strict snapshot | **7,614 / 8,252** exact |
| Parks | 86 active, 0 stale, 0 invalid |
| Admission audit | 11 candidates, 0 contradicted, 2 rejected, 0 revoked |
| fake_match_scan | 26 review leads |
| pytest (`--basetemp` scratch) | 1,159 passed, 5 skipped, 26 subtests |
| `git diff --check` (config source docs tools) | clean |

These numbers equal canonical's own reconciliation record: 381 Matching objects, 7,614 strict
functions and 86 parks.

Baseline file hashes (sha256 prefix):

| File | Hash |
|---|---|
| stable.json | 0f28ba9f7781cb7b |
| report.json | be70986469d3d7e0 |
| semantic_report.json | a6fd07f17fd7b65e |
| config.json | 98844595bd6a0078 |
| symbols.json | b3a3902b7b546be8 |
| parked.json | f85efc3d5d01727a |
| object_admission_rejections.json | 56d7784cdbe407b7 |
| semantic_data_matches.json | f50858a7a5471a37 |
| semantic_matches.json | 0f6bde0bb6fefdfc |

cachebeta.exe sha256 is 4cc87b45f7212703…

## Opportunity board at baseline

There are 86 incomplete Halo objects, excluding `linker_common`. Board tiers:

| Tier | Meaning | Objects |
|---|---|---|
| T1 | every function exact | 16 |
| T2 | 1-2 residuals and at most 2,048 B | 36 |
| T3 | 1-2 larger residuals | 16 |
| T4 | 3 or more residuals | 18 |

Fuzzy percentage is not used for ranking. The structural census covers the residual functions
in these objects:

| Class | Meaning | Functions |
|---|---|---|
| S1 | reference multiset differs | 27 |
| S2 | instruction count differs | 61 |
| T | tie-shaped | 41 |
| UNWRITTEN | not yet written | 2 |

Held going in:

- the 6 Codex-reserved objects;
- dynavobgeom (canonical ruling);
- every January-bug candidate;
- the owner-queue picks and naming items, where first-party evidence has not removed the need
  for a ruling.

## Method

- A **claim registry** (`scratch/campaign/claim.py`, atomic lock file) makes every agent claim a
  unit before editing candidate source for it, including providers and header consumers found
  mid-task.
- **Wave structure:** family triage decides ATTACK, ADMISSION, RETIRE or OWNER per object from
  the ledgers and reopen criteria. An attack lane is spawned only for a NEW evidence-backed route.
  Every proposed landing gets an independent adversarial reviewer. The whole run is capped at 8
  concurrent agents.
- **Per-object manifests** are written in each worker slug and carried into the research copy.
- **Integration:** reviewed packets only. Each batch gets:
  - a live-canonical recheck;
  - a full ninja build;
  - progress and semantic progress;
  - the stable whole-board diff;
  - parks, admission audit, fake-match scan and pytest;
  - `git diff --check`.

## Wave R2-1 (family triage -> attack -> review)

Eight family triages covered all 80 open, non-reserved objects. The verdicts split into:

- **3 ADMISSION:** hardware_geometry, xbox_texture_cache, hs_runtime storage.
- **3 ATTACK:** hs_runtime trigger volumes, actors (two functions), rasterizer_xbox_environment.
- **~30 OWNER**, with the ruling each needs recorded.
- **The rest RETIRE**, each citing the ledger negatives that exhaust it.

Per-object triage records are in `research/fifty_objects_r2_20260924/results/r2w1/TRIAGE__*.md`.

The key new evidence source was the Halo symbol atlas's **2001-09-25 linker-map tier**: MSVC map
ground truth for the retail cache exe and the cachebeta xbe. No earlier lane had used it. It names
static functions with January's exact spacing. The atlas carries functions only, so it cannot
name static data.

## Batch R2-1 (7979cf8f, 05255584)

Result: Halo objects **381 -> 384** (+3). Stable diff: +3 strict functions, **4,128 meaningful /
4,144 padded** code bytes, **0 regressions**. Parks 83/0/0. Admission 9/0/2/0. Fake scan 26.
pytest 1,159/5/26. `git diff --check` clean. Canonical was rechecked before integrating
(still f6d00a8c).

| Object | Kind of work | Evidence |
|---|---|---|
| cache/xbox_texture_cache | ownership + shared headers | Three D3D wrapper rows are static. Three first-party renames come from the 2001-09 map (`texture_cache_name_block_proc`, `compare`, `texture_cache_initialize_hardware_format`). `bitmap_group.h` `struct bitmap_data` takes HCEX field names and types (45 includers, 0 moved rows). The TU-local bitmap views are gone. `texture_cache_debug_render` goes in `texture_cache.h`; the Xbox-only public format mappers go in the object's own `xbox_texture_cache.h`, whose placement is disclosed as declaration-count-driven. The reviewer removed an invented pad member. `w/xbox_texture_cache`, `w/review_r2_xbox_texture_cache` |
| rasterizer/xbox/rasterizer_xbox_hardware_geometry | ownership (first-party name) | Round 1 held this as an owner pick, A vs B. The 2001-09 linker maps name `_D3DResource_MoveResourceMemory@8`/`_D3DVertexBuffer_MoveResourceMemory@8` with January's exact spacing, which attests B. The six placeholder stubs become XDK names, and 13 wrapper rows are static. The call's argument and position are byte-inert and disclosed. `w/rasterizer_xbox_hardware_geometry`, reviewer slug |
| ai/actors | new code (2 functions) | `_actor_input_update` (2,384) and `_actors_spawn_from_unit` (672) use the complete /Od named-local inventory in /Od declaration order. The reviewer notes a latent January swarm-member `actor_get(NONE)` path that the source reproduces unchanged. `w/actors`, `w/review_r2_actors` |

**Function gain, object still blocked:** hs_runtime `_render_debug_trigger_volumes` (1,088).

- It uses the /Od helper form, with the reviewer-amended /Od declaration order and the escape-law
  block.
- Its ruling-5 `_point_from_line3d` COMDAT is identical to January's copy and pair-links.
- 12 typecast converters are static, which fixes 12 provider-link failures.
- The `begin_random` tie still blocks the object.

**Process note:** the claim registry let two lanes that shared a label work the same unit (actors).
They converged on byte-identical objects. Labels are now unique per lane.

## Wave R2-2 (precedent audit, canary research, hygiene)

**Precedent audit.** Round-1/R2-1 OWNER items were checked against canonical's actual rulings and
admitted landings. Result: 27 HELD_CLASS, 11 NEW_RULING_NEEDED, 11 COVERED_BY_PRECEDENT. Every
covered item is zero-credit hygiene or data; none completes an object. The audit found that
several OWNER items the triage thought precedent-covered are not:

- **projectiles' named macro** is held. It has the RB2 refusal shape, and
  `ai_debug_attractor_label_height` landed 46 minutes before the first-party-name condition existed.
- **actions' squaring macro** needs a new ruling. The 5186c5bd ruling was function-scoped, and here
  the plain spelling also matches.
- **structures' `double t` and rasterizer_geometry's staged temp** need new rulings. The later /Od
  build CONTRADICTS both temps: its frames have no slot for them.

**Canary research (lab only, real_math reserved).** `_rasterizer_frame_statistics_draw` is a
whole-TU record-count coincidence. The findings:

- C1 symbol records before the globals block, mod 64, window 1 wide.
- One record each for: names, params, locals, distinct string literals, `if`/`&&` labels, and
  distinct opened files.
- **Packet PA** loses nothing board-wide and makes collisions `_collision_move_point` (4,752 B)
  exact. It combines:
  - real_local_random's named local;
  - `valid_real_plane3d` as an `__inline`;
  - removal of the plain matrix_math definition;
  - the collisions loop-site call;
  - dropping one copy each of two campaign-added duplicate prototypes.
- The copy choice is count-selected, and for `bitmap_delete` it runs against ownership, so it is an
  owner decision.
- PA also needs COMDAT admission and a park re-baseline, and it must be re-solved after Codex's
  real_math change. It is held, with a rerun script
  (`research/fifty_objects_r2_20260924/w/canary_frame_statistics/`).

## Batch R2-2 (42125a52)

This batch has zero code credit and changes no object status. Data **+3,089**:

- projectiles `_projectile_update_section`: 1,548;
- object_lights `_lights_section`: 1,541.

Both are single-section '$'-defect entries with `allow_incomplete_unit`, following the leaf_map,
editor_flying_camera and race precedents.

Also in this batch, all reviewed:

- storage fixes for bink_playback, weapons, object_lights, vehicles, weather_particle_systems and
  rasterizer_xbox_debug (an already-Matching object whose audit goes FAIL(8) -> PASS);
- draw_string's invented aggregate replaced by HCEX file statics;
- render_debug's genuine helper calls, vehicle_datum and tentative debug toggles;
- vehicles' genuine point_from_line3d call;
- rasterizer_xbox_profile V1R: byte-inert ownership.

Gate: 0 regressions, parks 83/0/0, admission 10/0/2/0.

The breakable_surfaces sub-packet was **not** landed. It removes a per-unit `/Ow` override on
precedent alone, without image proof, and earns nothing.

## Owner escalation (round 2)

Each of these rulings would complete, or directly unblock, the listed object. Evidence and packets
are in `research/fifty_objects_r2_20260924/results/r2w2/PRECEDENT_AUDIT.md` and the round-1 owner
queue.

| Ruling | Completes | Cost / risk |
|---|---|---|
| Codex actor_combat `cross_product2d` packet (in flight) | path_obstacle_avoidance, path_smoothing, real_math, collision_features, path_structure_bsp | none known; unblocks vehicles' fighter (P1 class) |
| actor_perception Packet B: /Od aggregate view copy + empty then-arm + explicit else | actor_perception | the view copy is covered by the action_obey precedent; the empty then-arm and else need a ruling |
| bink_playback q3 else-if split | bink_playback | 26601453 rejected this exact function; only new evidence is cross-build (4/4 vs 1/4) |
| interface header variant | interface | A loses frame_statistics_draw by count; B is extern-over-static |
| structures `double t` / rasterizer_geometry staged temp | structures / rasterizer_geometry | the /Od frame contradicts both temps: likely NO |
| January-bug class (uninitialised reads, NULL deref, etc.) | king, glow, dead_camera, first_person_weapons, saved_game_files, player_profile, transport_endpoint_winsock, draw_string, … | canonical holds; unchanged |
| Canary PA duplicate-prototype copy choice + COMDAT admission | collisions (+4,752 B) after Codex | count-selected copy choice; re-solve after Codex |

## Wave R2-3 (record-count census, January real_math.h recovery, synthesis; lab only)

**Census.** The run inserted K = 0..63 dummy records at four positions for all 86 incomplete units.
Totals: 5,504 compiles per position, a 64x64 grid for the sensitive rows, and a 28,992-compile
whole-board fragility map. The harness was validated against gate.py on 600 rows.

- **Only 1 of 126 residual functions closes by record count**: ui_widget_game_data_input_functions
  `_solo_level_select_list_update_displayed_items` (704 B, k = +2, window 2).
- 7 residuals flip between two forms, neither of them January's.
- 118 are completely count-inert.
- There is no shared-header pattern, and no object completes by count alone. **This route is
  retired.**
- Fragility map: 22 of 7,250 exact functions are count-sensitive, and 7 break within |3| records.
  These are the canaries any header-authenticity work must respect:
  - `_rasterizer_frame_statistics_draw`, `_bitmap_copy`;
  - race_engine_player_update and race_touch_flag;
  - `_unit_preprocess_node_orientations`, `_bitmap_2d_alpha_bleed`, `_get_edge_vertex`.

**January real_math.h recovery (first-party).**

- January has 10 real_math.h assert anchors, on 8 lines (848, 859/860, 879/880, 1508/1509, 1530).
- HCEX_Release has line records for 96 out-of-line helpers.
- Publics joined with contribs give 93 January-emitted real_math-family helpers.
- New law, checked with 0 inversions on 34 batches: VC7 emits pending inline callees just before
  their first user, in header definition order.
- Missing from our header but January-public: `vector_intersect_plane2d`, `valid_real_vector2d` and
  `valid_real_normal2d`. There are also 16 January-public prototypes; our focused math headers are
  campaign inventions, and HCEX has none of them.
- The record costs are calibrated. Adding the genuine January content breaks about 5 count-coincident
  canaries, so several of today's exact functions depend on the header staying inauthentic by a
  specific count. This is an owner-level finding. Details:
  `research/fifty_objects_r2_20260924/w/real_math_h_recovery/`.

**Synthesis.** Only packet PA closes anything with zero loss: collisions `_collision_move_point`,
making collisions 20/20 with an audit PASS. It still needs the owner's duplicate-copy decision. PU
(a duplicate `player_ui.h` prototype) and MN (a parameter name) are zero-loss genuine hygiene.
They are held, because landing them now would shift the post-Codex solo lookup (`codex_lookup.txt`
in `research/fifty_objects_r2_20260924/w/record_synthesis/`):

| Codex net N in the gdif TU | Solo outcome |
|---|---|
| +2 or +3 | closes with nothing added |
| +1, +4, +5 | closes with MN, PU or both |
| anything else | needs owner-decision compensators |

## Canonical checkpoint 4d1ebf17 (merged as cf3a1a6a)

Canonical cherry-picked this lane's f6d00a8c..5d3ca708 range and **held the
rasterizer_xbox_hardware_geometry admission**. The 2001 maps support the wrapper names, but the
added `D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO)` call has an unattested
position and argument. Canonical also added an isolated Xbox link probe (`tools/link_probe.py`) and
private-helper ownership repairs (20dd67c0).

This lane merged 4d1ebf17 with canonical's exact tree, so the hold is respected. **The corrected
object count is 383, not 384.**

## Completeness critic and batch R2-3 (b9930521)

An independent critic found five routes with concrete evidence. The main one was that the
2001-09-25 linker maps name static functions that still carried placeholder or invented names in
21 incomplete objects. Twenty-two adversarial reviews approved 22 of 22 packets. The last 10 ran
concurrently at the user's direction, and one interrupted review was re-run from scratch.

Landed, all zero credit:

- **72 first-party static-function renames across 20 objects.** Each has map alignment,
  relocation-masked body identity and matching callers, and each row is absent from cachebeta
  publics. The hs radiosity pairing is proven by the Sept `.rdata` definition pointers.
- **hs_compile storage:** three tables external, `hs_compile_finish` static with a TU-local
  prototype.
- **bink_playback:** `bink_playback_update__internal`.

Held: bink `bink_decompress_audio_frame`. Landing it needs an added call with an unattested
position, which is the class canonical held for hardware_geometry.

Gate: objects 383, code and data unchanged, 0 regressions, parks 83/0/0, pytest 1,161.

## Round-2 handoff

**Base / tip.**

- Base: canonical f6d00a8c.
- Merged canonical 4d1ebf17 at cf3a1a6a.
- Tip: this commit, on branch `claude/fifty-objects-r2-20260924`.
- Local only; never pushed.
- Scorer: objdiff 3.3.1 (sha1 3130e428) throughout.

**Target vs result.**

- **Target: 50 new complete Halo objects. Verified: 2** (381 -> 383): ai/actors (new code) and
  cache/xbox_texture_cache (first-party ownership plus shared-header fixes).
- A third, rasterizer_xbox_hardware_geometry, is held by canonical.

The evidence does not support 50. Every one of the 86 open objects was triaged:

- the retirements cite exhausting negatives;
- the precedent audit found no uncovered admission;
- the record-count census closes 1 of 126 residuals, and no object;
- the remaining pool is owner-gated (see the owner escalation table), Codex-reserved, or a
  recorded tie.

| Tally (vs f6d00a8c) | Value |
|---|---|
| 1. Net newly complete Halo objects | **2** (actors, xbox_texture_cache) |
| 2. New strict functions | **3**: `_actor_input_update` 2,384; `_actors_spawn_from_unit` 672; `_render_debug_trigger_volumes` 1,088 |
| 3. New meaningful exact code bytes | **4,128** |
| 4. New padded exact code bytes | **4,144** |
| 5. New verified data bytes / admission-only closures | **3,089** (projectiles 1,548; object_lights 1,541) / **1** (xbox_texture_cache) |
| 6. Fuzzy improvements at zero credit | none landed; zero-credit candidates are preserved in the research copy |
| Scorer-only effects | none (objdiff 3.6.0 would additionally credit hs 54,780 and actions 2,404 data bytes; not taken) |
| Regressions / revocations | 0 / 0 (hardware_geometry was quarantined before canonical credit, not revoked) |

**Landed zero-credit hygiene** (makes objects' admission state truthful):

- storage and ownership for bink_playback, weapons, object_lights, vehicles,
  weather_particle_systems, rasterizer_xbox_debug, draw_string, render_debug and
  rasterizer_xbox_profile;
- 72 first-party renames in 20 objects;
- hs_compile storage;
- hs_runtime converter storage.

**Held, with reopen criteria** (evidence in `research/fifty_objects_r2_20260924/`):

| Item | Reopen when |
|---|---|
| Codex-reserved actor_combat + 5 dependents | Codex reports; then re-gate P1 dependents (vehicles fighter) |
| Canary packet PA (collisions `_collision_move_point` +4,752, collisions complete) | Codex's real_math change lands; `rerun_packet.py --refresh` still 0 lost; owner rules on the duplicate-prototype copy choice (bdB vs ownership-natural bdI) and COMDAT admission |
| ui_widget_game_data_input_functions solo (704 B) | Codex's record net N is known; consult `codex_lookup.txt` (N = +2/+3 closes it; +1/+4/+5 closes it with the PU/MN hygiene) |
| hardware_geometry, bink `bink_decompress_audio_frame` | first-party evidence of the call site/argument, or an owner ruling on byte-inert unattested calls |
| owner escalation table (above) | the listed rulings |
| real_math.h January content (3 January-public helpers, 16 prototypes) | owner accepts losing the count-coincident canaries (fragility map), or their genuine repairs are found |

**Retired routes:**

- the record-count oracle as a primary route (1/126);
- the Sept-map route for static DATA names (the atlas holds functions only);
- the family-triage RETIRE list (`results/r2w1/TRIAGE__*`).

**Process incidents:** none this round. One review was interrupted by the integrator and re-run;
its stale claim was released.

**Expected integration conflicts:** none known. The branch contains canonical 4d1ebf17
exactly, plus b9930521 and the docs and research commits on top.
