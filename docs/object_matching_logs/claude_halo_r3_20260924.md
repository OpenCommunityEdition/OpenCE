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
