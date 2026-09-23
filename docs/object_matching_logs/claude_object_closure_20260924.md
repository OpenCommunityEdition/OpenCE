# Limited object-closure lane (2026-09-24)

Worktree `C:\halo-worktrees\claude-object-closure-20260924`, branch
`claude/object-closure-20260924`, base `5e81dabf` (jonas/exact-pilots). Two waves,
local commits only, never pushed.

## Baseline (verified before any edit)

Clean tree; full ninja; strict snapshot 7,574 / 8,245 functions; Halo meaningful
code 1,547,308 / 1,770,166; parks 103 active / 0 stale / 0 invalid; pytest 1,154
passed, 5 skipped, 26 subtests. All match the brief.

Admission audit: `tools/audit_object_admission.py` reports 42 candidates,
**0 contradicted**, 9 rejected, 0 revoked at the base. The brief's figure of 26
inherited contradictions is a different, raw-label view. It is not reproduced by
the shipped tool's classification, so the baseline is **not** claimed to have
zero raw-label contradictions.

The raw objdiff report lists complete-labelled units with raw objdiff gaps. Every
one of them is carried by the build's verified objdiff/semantic exception lines,
and those lines are identical before and after this lane (295 = 295, set-equal).

## Wave 1 — whole-object admission (commit c3277f6d)

Newly admitted **pre-existing exact** objects. None of this is new code; it earns
**zero** code-byte credit.

| object | functions | what was wrong | fix |
|---|---|---|---|
| source/ai/action_flee | 13/13 | three privates external in our split, static in January (cachebeta.pdb publics; no cross-TU refs) | symbols.json `"static": true` on `_action_flee_find_flee_position`, `_action_flee_current_position_exposed`, `_action_flee_at_flee_position` |
| source/ai/action_guard | 15/15 | `_action_guard_speak_post_combat` likewise | symbols.json `"static": true` |
| source/game/aim_assist | 16/16 | `compute_composite_attenuation` public in January, static in ours; selected-provider LNK2005 on `_arccosine` | drop `static`; repair the actor_moving provider (below) |
| source/ai/path_smoothing | 6/6 | LNK2005 on `_cross_product2d`, `_point_from_line2d`, `_set_real_point2d` | **not admitted**: blocked on `_cross_product2d` (see Blockers) |

Genuine helper-provider repairs. In each case the hand-written NODUP copy and its
`_inline` rename are removed, and the `real_math.h` `__inline` supplies the
January-owned COMDAT:

- **actor_moving / arccosine.** The single acos site becomes
  `angle = arccosine(facing_dot);`. It is doubly attested:
  - the /Od build calls an actor_moving-local helper chain at exactly this site
    (`0x465510 call -> 0x46d330 -> 0x4625b0 -> 0x46d3e0 acos`);
  - January's actor_moving owns `_arccosine`.

  All 36 actor_moving rows are unchanged, including the exact 2,752-byte
  `_actor_move_calculate_movement` and `_arccosine`.
- **path_obstacles / set_real_point2d.** Its `project_point3d` copy now calls
  `set_real_point2d`, exactly as the header inline does. All 16 rows are
  unchanged.

After Wave 1, no object on the board defines `_arccosine` or `_set_real_point2d`
NODUP.

## Wave 2 — `_action_vehicle_find_destination` (commit below)

**Newly matched code:**

- 1 function
- **1,232 padded / 1,225 meaningful bytes**
- action_vehicle closes: 17/17 exact, object audit PASS, and selected-provider
  link PASS in both orders for all 21 surplus symbols

It is admitted Matching.

Source: the /Od-attested statement list from Lane B's donor
(`claude-lane-b-ai-physics-units-20260920/scratch/lane_b/w/action_vehicle/diag_inline.c`).
It uses the genuine `real_math.h` helpers throughout, removing the hand
expansion (house rule 13). This lane made four changes on top of it:

1. **Original local names from the /Od RTC descriptor at 0x4429b0:**
   - `perpendicular_vector_to_hint_line`, typed `real_vector2d` per its name;
     the donor had `closest_point` as `real_point2d`;
   - `hint_direction`, replacing `entry_to_hint`;
   - `vector_to_destination`, replacing `actor_to_destination`.

   The rest already matched the RTC names (modified_destination, vehicle_center,
   target_point, actor_to_vehicle, actor_to_target_point, target_point_to_vehicle,
   vehicle_radius, avoid_vehicle_direction, tangential_offset_vector, result,
   collision_origin, collision_vector).
2. **point_from_line2d provider repair.** Removed the hand-written NODUP copy and
   its rename. `_point_from_line2d` stays exact and is now emitted from the
   header, and no object on the board defines it NODUP.
3. **`REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` guard removed** for this TU.
4. **`scale_vector3d`** is used as the /Od call shows.

Owner-ruling compliance for the two newly emitted COMDATs:

- **`_point_from_line3d`.** Permitted by ruling 1: action_vehicle is one of the
  17 January objects whose own split object holds an UNDEF reference to it,
  which was re-verified here. The emitted section is `section_infos_equal` to
  January's selected copy in action_charge, and the pair link with action_charge
  (after 5e81dabf made it SELECT_ANY) passes in both orders.
- **`_scale_vector3d`.** Ruling 5, all-inlined class: a genuine shared-header
  `__inline`, a strict-exact caller, and a section that is `section_infos_equal`
  to January's action_charge copy. The pair link passes and the full build shows
  0 regressions.

Rule 24 / ruling 6. The function has 18 view casts: 16 are 3D->2D prefix views,
and 2 are point2d<->vector2d, which have identical layout.

- **Strip test.** All 18 casts were removed and the function recompiled:
  still EXACT 1,232, and `section_infos_equal` to the landed object. The casts
  are byte-inert type hygiene; without them CL emits C4133.
- **Per-site /Od attestation** (`halo_cache_symbols.exe` fn 0x4422a0). Each row
  gives the argument pushes, last push = first argument:

| source call | /Od call site | arguments pushed (arg1, arg2, ...) |
|---|---|---|
| vector_from_points2d((real_point2d const *)&actor->...body_position, (real_point2d const *)&vehicle_center, &actor_to_vehicle) | 0x44248b | actor+0xfc, [ebp-0x3c] vehicle_center (12 B), [ebp-0x60] |
| vector_from_points2d(body_position, (real_point2d const *)&target_point, &actor_to_target_point) | 0x4424a5 | actor+0xfc, [ebp-0x50] target_point (12 B), [ebp-0x70] |
| vector_from_points2d((real_point2d const *)&target_point, (real_point2d const *)&vehicle_center, &target_point_to_vehicle) | 0x4424b9 | [ebp-0x50], [ebp-0x3c], [ebp-0x80] |
| vector_from_points2d((real_point2d const *)entry_point, (real_point2d const *)hint_point, &hint_direction) | 0x4424dc | [ebp+0x10], [ebp+0x18], [ebp-0xb4] |
| point_from_line2d((real_point2d const *)&actor_to_target_point, &hint_direction, -dot, (real_point2d *)&perpendicular_vector_to_hint_line) | 0x442524 | [ebp-0x70] (vector2d), [ebp-0xb4], -dot, [ebp-0xa4] (vector2d, pushed at 0x4424ea) |
| point_from_line2d((real_point2d const *)&vehicle_center, &avoid_vehicle_direction, radius*1.1f, (real_point2d *)&modified_destination) | 0x442754 | [ebp-0x3c] (12 B), [ebp-0xc8], radius*1.1, [ebp-0x24] modified_destination (12 B) |
| perpendicular2d(&target_point_to_vehicle, (real_vector2d *)&tangential_offset_vector) | 0x4427de | [ebp-0x80], [ebp-0xf8] (12 B) |
| dot_product2d((real_vector2d const *)&tangential_offset_vector, (real_vector2d const *)&vector_to_destination) | 0x4427f4 | [ebp-0xf8] (12 B), [ebp-0xe0] (12 B) |
| negate_vector2d((real_vector2d const *)&tangential_offset_vector, (real_vector2d *)&tangential_offset_vector) | 0x44281c | [ebp-0xf8], [ebp-0xf8] |
| normalize2d((real_vector2d *)&tangential_offset_vector) | 0x442836 | [ebp-0xf8] |

The RTC descriptor sizes (12 B for the 3D objects, 8 B for the 2D ones) prove
that each 2D helper received a 3D object's address at that exact site.

## Measurements

| | base 5e81dabf | after Wave 1 | after Wave 2 |
|---|---|---|---|
| strict exact functions (of 8,245) | 7,574 | 7,574 | **7,575** |
| Halo meaningful code bytes (of 1,770,166) | 1,547,308 | 1,547,308 | **1,548,533** (+1,225) |
| halobetacache linked files (of 468) | 332 | 335 | **336** |
| parks (active/stale/invalid) | 103/0/0 | 103/0/0 | 103/0/0 |
| admission audit (cand/contra/rej/revoked) | 42/0/9/0 | 39/0/9/0 | 39/0/9/0 |
| pytest | 1,154 passed | 1,154 passed | 1,154 passed |

Stable diff: Wave 1 gained 0 with 0 regressions. Wave 2 gained 1 function /
1,232 padded bytes with 0 regressions.

Both waves passed the same checks:

- fake-match scan: unchanged;
- castcheck: unchanged, apart from Wave 2's +16 counted lines admitted above;
- `git diff --check`: clean.

## Rejected trials

- **Naive provider unhanding on four providers at once.** It removed the rename
  and copy without restoring a genuine use, so each provider lost its
  January-owned helper row (UNWRITTEN). path_obstacles also lost 2 functions to
  the dangling `set_real_point2d_inline` call. Reverted, and redone per provider
  with /Od-attested uses.
- **actor_combat `_actor_aim_grenade` restoration** (the only genuine
  `cross_product2d` user; /Od 0x44ebb0 calls `cross_product2d(&facing,
  &horizontal_aim_vector)` at 0x44ed45).
  - The /Od shape is dot_product2d, cross_product2d, scale_vector2d(magnitude2d)
    and a scale_vector3d tail, with RTC names `aim_vector`, `new_desired_target`,
    `horizontal_aim_vector` and `new_aim_vector`. It reached the same size
    (576 -> 544) with an identical 177-instruction stream.
  - The remainder is frame packing: `sub esp,0x28` vs January's `0x24`, plus one
    eax/edx swap.
  - The live frame table (Lane C `target.py`) shows ours ranking `aim_vector`
    (17 refs) ahead of `new_aim_vector` (15 refs). The fild temp `tv309` does not
    share `horizontal_aim_vector`'s entry, and `new_desired_target` merges into
    `new_aim_vector`. January ranks `new_aim_vector` first, overlays the fild temp
    on `horizontal_aim_vector.j`, and overlays `new_desired_target` on
    `aim_vector`.
  - Measured inert:
    - declaration order and local scope (13 variants);
    - 1..23 file-scope names before the function (the name-count oracle);
    - pure in-function names;
    - statement order of the k copy.
  - **Not landed.** It adds 5 view casts and 5 COMDATs (`_dot_product2d`,
    `_magnitude2d`, `_magnitude_squared2d`, `_scale_vector2d`, `_scale_vector3d`),
    which rulings 5 and 6 admit only in a strict-exact function. The candidate is
    kept at `scratch/lane/actor_combat_aag_candidate.c`.

## Blockers

- **path_smoothing admission (6/6 exact)** is blocked only by `_cross_product2d`:
  actor_combat still hand-writes it NODUP, and it is the sole NODUP definer on
  the board.
  - **Reopen** when `_actor_aim_grenade` is strict exact with the /Od-attested
    helper calls.
  - Then:
    1. remove actor_combat's `cross_product2d` rename and hand copy;
    2. re-run `provider_link.py source/ai/path_smoothing`;
    3. mark path_smoothing Matching.
  - The residual is frame-slot packing, not instruction selection. Start from
    the frame-table readout above.

No shared-header edit was needed or made in either wave.
