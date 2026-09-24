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

These four objects (actor_firing_position, game_engine, real_math, model_animations) are
function- and data-exact. Only the `distance3d` provider (and, for real_math, the
actor_combat `cross_product2d` copy) keeps them from admission.

## Held for owner rulings (not landed; evidence preserved)

See the research material listed at the end of this file. Each held item has its own reopen
criterion in its worker ledger.
