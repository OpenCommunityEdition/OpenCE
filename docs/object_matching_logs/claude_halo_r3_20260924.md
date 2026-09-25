# Halo January campaign, round 3 (2026-09-24)

- **Worktree:** `C:\halo-worktrees\claude-fifty-objects-r3-20260924`.
- **Branch:** `claude/fifty-objects-r3-20260924`.
- **Base:** LOCAL canonical `jonas/exact-pilots` **193a6868** (verified in
  `worktrees/astra-reconcile-20260920`). The GitHub remote is behind and was not used. Rounds 1-2,
  including the 72 map-backed renames, are already canonical and are not counted here.
- **Scorer frozen:** objdiff-cli **3.3.1** (sha1 3130e428). The configure-pinned 3.6.0 was moved aside
  after the first build.
- **Commits:** local only; never pushed; canonical never edited.
- **Reserved:** these objects belong to active external Codex lanes (unmerged 2026-09-24 work) and are
  not duplicated: periodic_functions, connected_geometry, rasterizer_xbox_hardware_geometry,
  rasterizer_xbox_hardware_bitmaps, cinematics, dead_camera, xbox_sound_cache, lightning,
  path_structure_bsp.

## Frozen baseline (193a6868, full ninja, objdiff 3.3.1)

| Measure | Value |
|---|---|
| Halo objects complete | **383 / 468** (All 501) |
| Halo meaningful exact code | **1,577,237 / 1,770,166** (7,445 / 7,574 functions) |
| Halo data | 2,587,011 / 3,923,451 |
| Stable strict snapshot (board-wide) | 7,617 / 8,252 |
| Parks | 83 active, 0 stale, 0 invalid |
| Admission audit | 11 candidates, 0 contradicted, 2 rejected, 0 revoked |
| fake_match_scan | 26 leads |
| pytest | 1,161 passed, 5 skipped, 26 subtests |
| `git diff --check` | clean |

Baseline file hashes (sha256 prefix):

```
34928a93cbe91fac  *scratch/baseline/stable.json
b520ee5ed5695b6c  *scratch/baseline/report.json
4bdc62edab9c39f8  *scratch/baseline/semantic_report.json
82a3cb6f3e7bdd67  *config/config.json
4955eab155599ffb  *config/symbols.json
3682543bb0cd2401  *config/parked.json
```

## Board at baseline

84 incomplete Halo objects:

| Tier | Meaning | Objects |
|---|---|---|
| T1 | all functions exact | 15 |
| T2 | 1-2 small residuals | 36 |
| T3 | 1-2 larger residuals | 15 |
| T4 | 3+ residuals | 18 |

Residual functions:

| Class | Meaning | Functions |
|---|---|---|
| S1 | relocation multiset differs | 26 |
| S2 | instruction count differs | 59 |
| T | tie-shaped | 41 |
| UNWRITTEN | not yet written | 2 |

## Method (round 3)

- **Research policy:** every compiler experiment needs a falsifiable hypothesis with a control, and
  any proposed rule must first be validated on other functions' January sites with negative
  controls. No broad declaration-count sweeps or permutation ladders.
- **Operations:**
  - claim registry, with labels unique per lane;
  - per-function `records.json`;
  - adversarial review of every landing;
  - at most 8 concurrent agents;
  - full gates after every 5-15 KB wave.

## Wave R3-1 (priority lanes, authentic real_math.h, scout)

| Lane | Outcome |
|---|---|
| ai_communication | **`_ai_communication_finished` EXACT** (landed). `_ai_communication_event` is fuzzy, blocked by the held FALSE-initialised flag. `_actor_talk_weight` has no progress. `game_allegiance.h` enum owner prerequisite landed. |
| actor_perception | **Blockage function EXACT; the object completes (44/44, audit PASS, pdb 0, surplus identical, links PASS). OWNER-GATED** on the empty then-arm and the redundant `else blockage = 0`. New evidence: the HCEX.pdb line table for this function, read against the decoded 2011 PPC image, attests a separate `blockage = 0` statement (line 4812) and a 3-line comment-only arm (4819 -> 4823). The file is line-aligned with January to within 2 lines. A prior ruling rejects redundant equal-value stores, so the owner must decide. |
| actor_combat aim_grenade | **`_actor_aim_grenade` EXACT in a natural form** (no invented goto). Removing actor_combat's `#define normalize2d normalize2d_inline` redirect closes the 4-byte frame gap: the escape law plus the ownership oracle. **OWNER-GATED**: the function keeps January's authentic read of the never-assigned `aim_vector` on the vehicle path (attested in January, HCEX and /Od), which is the January-bug class. On a YES: path_smoothing, real_math and collision_features become admissible. path_obstacle_avoidance still has canonical's .bss naming hold, and path_structure_bsp is reserved. |
| collisions | **OWNER_RULING_PACKET.** Cross-build evidence shows `_collision_move_point`'s residual is a count-keyed site (Sept J / Oct O / Jan J reload swap), so no body spelling closes it. The duplicate-prototype copy choice cannot be decided by HCEX include topology. |
| real_math.h | **P1a landed** (zero credit, byte-inert on 273 consumers): January-string-attested assert macros and the January-exact spline body. **P2 (complete JAN tier) is OWNER-GATED**: it loses three count-coincident canaries, each one adjacent load swap that no body repair can move. |
| env spot light | Fuzzy. M8 +2 IL locals is unattested. pA (external `scale_vector3d` redirect) is rejected as P1 class. |
| ai_debug | Fuzzy. A new operand-order/spill census law. Context bisection shows head-marker spills depend on upstream inline expansions. |
| scout | New instruments: cross-build invariance (Aug/Sept/Oct 2001 bodies), and the confirmed count-site set (10 functions). |

## Batch R3-1 (4f236e9f prerequisites, 6f531fdd function)

**+1 strict function, +1,583 meaningful / 1,584 padded; 0 regressions.** Parks 82/0/0. Admission
11/0/2/0. Fake scan 26. pytest 1,161.

## Round-3 owner decisions (evidence under scratch/w/r3_*; curated at handoff)

| Decision | Unlocks | Evidence / cost |
|---|---|---|
| Credit `_actor_aim_grenade` with January's authentic uninitialised vehicle-path read (BUG comment) | +544 B; admits path_smoothing, real_math, collision_features (3 objects) | read attested in January, HCEX and /Od. New COMDATs listed in `w/review_aim_grenade_actor_combat/REVIEW.md` |
| actor_perception empty then-arm + explicit else | actor_perception (1 object, +385 B) | HCEX line-table evidence; prior ruling rejects redundant equal-value stores |
| collisions duplicate-prototype copy choice (stack_walk; bitmap_delete route) | collisions (1 object, +4,744 B) | count-keyed site; HCEX topology cannot decide ownership |
| real_math.h P2 (complete January tier) vs 3 count-coincident canaries | authenticity; no credit | each canary is one adjacent load swap keyed to TU count |
| transport_endpoint.h duplicate prototype (tn) | none (hygiene) | count-selected; byte-inert today |

## Wave R3-2 (three board-wide instruments + attacks)

- **Cross-build census (A):** covered 119 non-reserved residuals.
  - 106 are **class iii**: the Aug/Sept/Oct-2001 bodies equal January's, so our source has a local
    defect (not a late January edit and not a count site).
  - 6 are count-keyed; 2 are partial late edits; 2 are unwritten.
- **/Od statement-structure census (B):** covered all 128 residuals. Only a handful of structural
  differences were both new and usable.
- **Escape/redirect census (C):** confirmed law L2 on controls:
  - making a header-inline callee external grows the frame by exactly the January cells the
    escaped local shares (editor_camera +12, limp_noodle +24);
  - negative controls held;
  - E1 is per-address;
  - an external redirect can also masquerade as a "scheduler tie" through L5;
  - the frame census misses `__chkstk` prologues.

Attack outcomes (all reviewed):

| Function | Result |
|---|---|
| bipeds `_biped_find_nearby_support_surface` | **EXACT.** Real_math.h `plane3d_negate` body (P1) + render_objects /Od body (P2, approved) + genuine helper calls (P3, approved). P1 is under a dedicated shared-header review before landing. |
| decals `_decal_clip_to_surface` | **EXACT once the external project_point3d redirect is removed** (reverses a 2026-09-12 decision; disclosed). **OWNER-GATED:** the combined flag mask `!(flags & (a\|b\|c))` is supported only by January's inline decision; HCEX/2020 spell three tests. Reviewer amended the /Od declaration scope (P5b). |
| ai `_ai_test_line_of_sight` | EXACT frame/size only with both redirects removed; owner-gated (January uninitialised read). |
| first_person_weapons `_first_person_weapon_update` | owner packet. The same TU already has an identical landed construct (`weapon_play_first_person_weapon_sound`, 6d351b35), so the ruling is a consistency question. |
| structures, transport_endpoint_winsock, transport_initialize | owner packets |
| saved_game_files | zero-credit first-party static names (01) + storage (02r) approved; completion still needs owner patch 03 |
| geometry polygon3d, bitmap_group, bsp3d sphere, ballistic line of fire | fuzzy or blocked |

## Owner decisions after wave R3-2 (cumulative)

| # | Decision | Unlocks |
|---|---|---|
| 1 | Credit `_actor_aim_grenade` with January's authentic uninitialised vehicle-path read | +544 B; path_smoothing, real_math, collision_features (3 objects) |
| 2 | actor_perception empty then-arm + explicit else (HCEX line evidence) | actor_perception (+385 B, 1 object) |
| 3 | collisions duplicate-prototype copy choice (count-keyed site) | collisions (+4,744 B, 1 object) |
| 4 | decals clip combined flag mask (+ project_point3d internal) | +1,768 B (decals not complete) |
| 5 | first_person_weapons unguarded constant-index read (same class as a landed row) | first_person_weapons (1 object) |
| 6 | saved_game_files patch 03 (/Od double zeroing; January-bug class) | saved_game_files (1 object) |
| 7 | ai_test_line_of_sight uninitialised read | +996 B (ai not complete) |
| 8 | structures `double t`, rasterizer_geometry staged temp | structures, rasterizer_geometry (2 objects) |
| 9 | real_math.h P2 complete January tier vs 3 count-coincident canaries | authenticity |

## Wave R3-3 (two new instruments + attacks; zero credit)

- **Retail-build witness (calibrated):** compiles our source in a retail configuration and compares
  it against the Sept-25-2001 cache.exe (map bodies) and the Oct-12-2001 2276P.xbe. The retail
  configuration is VC7 13.00.9254, the production flags minus /DDEBUG and /Oy-, plus /Gr, with
  asserts compiled out and typed accessors inlined.
  - Controls: 18/18 positive in 10 TUs, 12/15 negative, 13/13 invariance.
  - Hit rates: 70.2% of source-stable exact bodies are verbatim in 2276P, against 2.8% of residuals.
  - Harness: `scratch/w/r3w3_retail/retail_gate.py`.
  - Blind spots: DEBUG-conditional code such as the saved_game_files mutex timeout, and register
    zero placement across a call.
- **HCEX line-table census:** covered 118 residuals. Its reliable evidence is aggregate `ld/std`
  copies and textual arm order. Return-site and line counts discriminate only weakly.
- **Attacks (11):**
  - game_sound: the typed-accessor respell is approved at zero credit. Reviewer-amended patch B is
    pending.
  - bitmap_group: research only.
  - Nine owner packets: saved_game_files, structures, bink q3, effects (x2), projectile_new, glow,
    actor_look_update, player_profile_new, render_sprite.
  - Pattern: every attack ended at an owner gate. In most cases the retail witness *confirmed*
    that a held construct is authentic.

## Owner rulings, 2026-09-24 (first ruling set)

| Q | Item | Ruling |
|---|---|---|
| 1 | aim_grenade | **YES** (04, not 04b). Admit only the natural no-goto body as a narrow, target-proven original-bug exception; keep the BUG comment. Do not retire the decals rejection or touch reserved path_structure_bsp. |
| 2 | actor_perception blockage | **HOLD.** The retail bytes and HCEX line records support the shape, but the prior empty-branch ruling requires authenticated original source. |
| 3a | collisions stack_walk copy | **HOLD.** Header ownership is unproved; do not select a copy by byte yield. |
| 3b | collisions bitmap_delete route | **HOLD.** Take neither the zero-regression route that contradicts HCEX nor an authentic route that loses exact functions. |
| 3c | collisions `_valid_real_plane3d` COMDATs | Allowed under the existing narrow rule, but collisions earns **no credit** until 3a/3b have a house-rule-compliant, zero-regression resolution. |
| 4 | first_person_weapons | **YES** (A2). Document the proved weapon-switch invariant. |
| 5 | saved_game_files | **NO.** Keep the explicit hold on writing an indeterminate count on mutex failure. |
| 6 | structures `double t` | **NO** (retail-contradicted). The genuine helper cleanup may land at zero credit after verification. |
| 7 | rasterizer_geometry | **YES.** Admit the meaningful `real` temp supported by the retail builds. Test declaration-time initialisation first; document separate assignment if it is required. |

## Batch R3-2 (owner rulings)

- **aim_grenade** (+544 padded / +543 meaningful): r3_aim_grenade patch 01 only, i.e. the /Od
  statement list with no goto.
  - The vehicle-path read of the unassigned `aim_vector` keeps its BUG comment.
  - Config: 03A (collision_features, real_math and path_smoothing set to Matching) and 04 (retires
    the stale collision_features `_projection_from_vector3d` rejection).
  - Not applied:
    - 02 (the reaim /Od shape, whose local renames come only from a later build: held name class);
    - 03b/03C (path_structure_bsp is reserved; path_obstacle_avoidance's .bss relabel is held);
    - 04b (the decals rejection is kept);
    - 05 (optional).
  - Retail: the body is verbatim in Oct 2276P; the held goto candidate is not.
  - Strip tests (reviewer):
    - Byte-inert: the five argument view casts, the local names, and the leftover
      `real_math_declarations.h` normalize2d prototype.
    - Load-bearing, all inside the admitted natural /Od body: the unassigned read, the /Od
      statement order, removal of the normalize2d redirect, and the `h` view copy
      `*(real_vector2d const *)&aim_vector`.
    - The `h` copy is not byte-inert (a field copy is non-exact). It is attested as an integer mov
      pair in /Od (0x44ecc7) and as ld/std in HCEX; precedent `action_obey.c:1259`.
  - Superseded: Codex hold probe aa2d8a96 (`codex/five-more-halo-20260925`).
- **first_person_weapons** (+1,536 padded), patch A2:
  - `VALID_INDEX` at all five /Od-attested constant-index sites.
  - The two update-site NULL guards are removed.
  - The three `REAL_MATH_EXTERNAL` magnitude/square-root redirects are removed; the resulting
    surplus is identical to January's action_charge/action_alert copies.
  - Invariant proof:
    - Every write to `weapon_index` stores NONE except the one at first_person_weapons.c:1521.
      That write is nested in `if (animation_graph->first_person_weapon_animations.count)` for the
      graph taken from `weapon_definition->weapon.interface_definition.first_person_animations`.
    - `first_person_weapon_update` re-derives the same graph after its `unit_index!=NONE`,
      `weapon_index!=NONE` and `weapon_try_and_get` guards.
    - The new-map `csmemset` leaves `weapon_index` at 0, but it sets `unit_index` to NONE.
      `unit_index` is written only by `first_person_weapon_new_unit`, which immediately runs
      `switch_weapons`, and that stores NONE first.
    - So the NULL arm cannot reach either read. The comment at the moving-animation site states
      this, and the jitter site refers to it.
    - The completeness gap (the csmemset path) was found by the semantics reviewer.
  - The earlier `weapon_play_first_person_weapon_sound` (6d351b35) has the same unguarded shape.
    It is NOT covered by this invariant, because it takes an arbitrary weapon. It supports only
    that the construct is authentic, not that it is safe (reviewer note; owner follow-up).
  - /W3 goes from 11 to 12 warnings: one C4244 inside `real_math.h`'s `square_root` inline, which
    is now compiled in this TU. The three new COMDATs fall in the all-inlined COMDAT class admitted
    on 2026-09-21 (#5).
- **rasterizer_geometry** (+144 padded): `real value = (real)(long)(compressed<<21);`.
  - The declaration-time initialiser was tested first. It is debug EXACT and verbatim in 2276P
    (0x129d10), and it MATCHes the Sept cache.exe body.
  - The temp is load-bearing only at its first (i) use.
    - Reusing it for j and k is byte-inert in debug and in both retail witnesses. Three separate
      temps, and a separate assignment in place of the initialiser, are also inert.
    - So the j/k staging is neither required nor attested. It is kept so the three components are
      written the same way, which is disclosed here.
    - A declaration-time initialiser can serve only the first use; the later uses have to be
      assignments.
  - Negatives that miss both retail witnesses: no temp (production), the /Od mask+division form,
    and staging through `long`.
- **structures** (zero credit): the helper cleanup only, i.e. a single shared FALSE and the
  genuine `point_from_line3d` call, replacing production's hand-expanded rows (rule 6).
  - Production's names, types and declarations are kept (`scratch/w/r3b2/structures_min/`).
  - The residual moves from `[size 416!=432, sha]` to `[sha]` at January's 432 B.
  - A keyed diff against HEAD shows only the target function changed. /W3 stays at 12.
  - Not landed: round-2 patch A's extra elements.
    - The HCEX parameter name `point` and local renames.
    - `short projection_sign`: January-attested by `movzx ax,al` at +0xed, but it adds two C4244
      warnings.
    - The plane lookup inlined into the distance call, plus inner-block declarations.
    - These move the body closer to 2276P (retail ratio 0.979 against 0.636 for the minimal
      cleanup), but the ruling admitted only the helper cleanup. They are recorded for a future
      ruling.
- **Full gate** (`scratch/campaign/gate_r3b2/summary.json`):
  - ninja exit 0 and progress OK.
  - Halo objects **383 -> 388 / 468**.
  - Meaningful code 1,578,820 -> **1,581,031** (+2,211); functions 7,446 -> **7,449**; data
    2,587,011 (unchanged).
  - Stable diff against the frozen baseline: gained 4 functions / 3,808 padded, including
    batch 1's 1,584 B. **0 regressions.**
  - Parks 82 -> 80 (0 stale, 0 invalid). Admission: 0 contradicted, 0 revoked.
  - Fake-match leads: 26 (unchanged).
  - pytest: 1,161 passed, 5 skipped, 26 subtests. `git diff --check` clean.
- **Whole-object checks** on collision_features, real_math, path_smoothing, first_person_weapons
  and rasterizer_geometry:
  - object_audit PASS;
  - pdb_storage 0 disagreements;
  - surplus_identity 0 non-identical;
  - selected-provider link PASS in both orders.
- Per-function records: `scratch/w/r3b2/records.json`.

## Wave R3-3b (retail-witness follow-ups; 5 attacks + reviews)

- rasterizer_lights `_rasterizer_lens_flare_submit_for_cluster`: **EXACT**, reviewer-approved.
  - `direction`/`up` and their uses move into an inner block, following the /Od declaration order.
  - The earlier "scheduler tie, five shapes exhausted" entry is retired.
  - The object stays incomplete: `_rasterizer_lens_flares_draw` is a parked residual, and
    `_rasterizer_lights_reset_for_new_map` is the owner-held overrun.
- bipeds file-statics: a storage packet, approved with amendment 04 (the parked
  `_ai_test_ballistic_line_of_fire` rebaseline). Zero credit.
  - The retail private ABI and the Sept map STATIC list attest that `biped_falling_danger`,
    `update_airborne`, `snap_facing`, `update_moving` and `fudge_vectors` are file-static.
- vehicles `_update_alien_fighter_physics_new`:
  - Exact through the genuine `cross_product2d` call. Verbatim in 2276P and matches the Sept map.
  - Stays under the owner hold of 2026-09-21 ("the fighter stays held").
  - Its provider-link dependency on actor_combat patch 01 is now satisfied.
  - F1: the /Od form of the attested statement is a post-January reassociation.
- Owner packets (unchanged class):
  - hs_parse_boolean (LAW U: the uninitialised read is confirmed in both retail builds);
  - players examine_nearby_device (RB2 parenthesis).

## Batch R3-3 (approved packets that need no owner ruling)

| Packet | Credit | Notes |
|---|---|---|
| r3w2 bipeds P1+P2+P3 | `_biped_find_nearby_support_surface` +544 padded | See below. |
| bipeds storage 01+02+03+04 (review_r3w3b amended set) | 0 | bipeds pdb_storage now has 0 disagreements. `_ai_test_ballistic_line_of_fire` fuzzy score drifts 99.70022 -> 99.69588 through the declaration count; it is parked and no exact row is lost. |
| lens_flare P1 scope block | `_rasterizer_lens_flare_submit_for_cluster` +400 padded | rasterizer_lights 11/13. |
| game_sound typed accessor, patch A | 0 | `game_looping_sound_get` is January's own name (line-282 assert string). Retail OCT verbatim goes 19 -> 28/32. The optional amended AB scope patch is not landed. |
| saved_game_files 01+02r+04a+04b | 0 | First-party static names plus storage. pdb_storage 4 -> 0. 45/46: only the owner-held (Q5) enumerate function remains residual. |

Notes on the bipeds P1+P2+P3 row:
- P1: attested `real_math.h` `plane3d_negate` body, i.e. `negate_vector3d` + d (/Od 0x56d630 and
  the HCEX_Release line table).
- P2: render_objects /Od shadow-volume plane pair.
- P3: bipeds genuine helper calls.
- Dedicated 467-unit header review: 0 exact rows lost.
- The bipeds park is retired. The render_cameras `_render_camera_build_frustum` park is refreshed to
  3408 / 112 / d42e420f / 88.8082; the target is unchanged, and that site is not a
  `plane3d_negate` caller in /Od.

Full gate (`scratch/campaign/gate_r3b3/summary.json`):
- ninja exit 0.
- Halo objects **388 / 468**; no new object completes in this batch.
- Meaningful code **1,581,962** (+931); functions **7,451** (+2); data 2,587,011.
- Stable diff against the frozen baseline: gained 6 functions / 4,752 padded, **0 regressions**.
- Parks 80 -> 79 (0 stale, 0 invalid). Admission: 0 contradicted, 0 revoked.
- Fake-match leads 26. pytest 1,161 passed. `git diff --check` clean.

Complete objects that consume the new `_negate_vector3d` surplus, re-audited:
- game_sound, units, leaf_map, render_objects, collision_features: object_audit PASS, surplus
  identical, provider link PASS, pdb 0.
- items: object_audit PASS and provider link PASS, but pdb 3 (see follow-ups).
- bsp3d: see follow-ups.

**Follow-ups found during the re-audit (pre-existing; identical on the pre-batch objects):**
- bsp3d, admitted Matching, has object_audit FAIL(1) and provider link FAIL(1) on
  `_bsp3d_clip_polygon_to_leaves_recursive`. January's split and symbols.json mark it static
  (not a PDB public), but bsp3d.c defines it external. This is a zero-credit storage fix.
- items, Matching, has pdb_storage 3: `_item_maximum_impact_velocity`, `_item_update_section` and
  `_verify_item_location` are not cachebeta publics, but ours and symbols.json keep them external.

## Owner rulings, 2026-09-25 (ruling list 2)

- **vehicles fighter: narrowly REOPENED** on the new first-party and link evidence (lifts the
  2026-09-21 "the fighter stays held" for this function only).
- **bink split arms:** to get a separate ruling; presented as its own question and not landed.
- **Held:** the parentheses-only matches (projectile_new O1, players RB2) and the other
  bug-dependent candidates (glow, player_profile R1-R3, hs_parse_boolean, effects,
  render_sprite, ai_test_line_of_sight). "Matching bytes alone does not lift those house-rule
  holds."
- **Not ruled; still pending:** actor_looking (s5 view cast + `can_look` head) and the decals
  flag mask.

## Batch R3-4 (vehicles fighter reopen)

- `_update_alien_fighter_physics_new` is exact (+1,088 padded): the hand-written 2D yaw
  expression becomes the genuine
  `cross_product2d((real_vector2d const *)&desired_rotation.forward, (real_vector2d const *)&vehicle->object.translational_velocity)`.
- Evidence:
  - Debug EXACT.
  - Verbatim in Oct-2001 2276P at 0x1529a0, and a Sept-25 cache.exe map MATCH. Production matches
    neither.
  - The `@cross_product2d@8` copy also matches Oct/Sept.
  - /Od 0x8f6cbf calls the helper with the two direct 3D pointers.
  - The view casts meet ruling 6 (2026-09-21): attested site, byte-inert cast, layout prefix,
    exact caller.
- Link: the new SELECT_ANY `_cross_product2d` passes the selected-provider link against
  actor_combat. This depends on batch R3-2's NODUP -> ANY change; before R3-2 it failed with
  LNK2005.
  - vehicles provider link PASS in full and on the baseline.
  - object_audit goes FAIL(2) -> FAIL(1): only the pre-existing `_update_alien_scout_physics`
    residual remains.
  - vehicles 38/39, so the object is not complete.
- **Disclosure (reviewer F1):** the /Od statement at this site is a post-January revision.
  - /Od computes `cp*unknown308/unknown2f8` (mulss, then divss).
  - January and Oct retail compute `cp/unknown2f8*unknown308` (fdiv, then fmul), which is what the
    landed source spells.
  - /Od attests the helper call and its two arguments, not the arithmetic order.
- Full gate (`scratch/campaign/gate_r3b4/summary.json`):
  - ninja exit 0.
  - Halo objects 388 / 468.
  - Meaningful code **1,583,050** (+1,088); functions **7,452**.
  - Stable diff against the frozen baseline: gained 7 functions / 5,840 padded, **0 regressions**.
  - Parks 79. Admission: 0 contradicted, 0 revoked.
  - Fake-match leads 26. pytest 1,161 passed. `git diff --check` clean.
