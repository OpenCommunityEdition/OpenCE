# `actor_moving.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254).
- All measurements through `tools/campaign/gate.py --source ... --forbid-emitted-symbol _point_from_line3d`,
  `tools.campaign.alndiff`, `tools.campaign.tinfo`, `scratch/parkcheck.py` and `tools/fake_match_scan.py`.
  Per-row normalized-sha comparisons use `scratch/workers/w3_actor_moving/rows.py`. Instruction diffs were
  counted after slot and label normalisation with `slotnorm.py`.
- Worker report with every shape: `scratch/workers/w3_actor_moving.md`. Candidates are in
  `scratch/workers/w3_actor_moving/`.
- Prior ledgers read: all fourteen `actor_moving_obj_*.md`, including opus5_150k_w1, opus5_100k waves B and C, and
  fresh_graphs. Also read: the w1 worker report, the 100K handoff, `scratch/w2/laws_w2.md` and `scratch/w3/c4013.txt`.
  The C4013 census has no actor_moving entries.
- `_actor_move_update` is on the DO-NOT-SPEND list and was not touched.

## 2. Baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (real file = HEAD 210aa8f81) | 26 | 10 | 0 |
| final real-file gate | **30** | 6 | 0 |

The guard passes. The unit has no parks. The per-row normalized-sha diff against the baseline object shows only the four
rows below. Every other row is byte-identical, including all six remaining residual hashes.

| newly strict EXACT | padded | relocs | sha16 |
|---|---:|---:|---|
| `_actor_move_avoidance_setup` | 576 | 18 | `3060b44f3fcbaf83` |
| `_actor_move_try_evasion_direction` | 416 | 18 | `a306a2cda400a435` |
| `_actor_move_try_evasion_vector` | 592 | 24 | `48da9be7024d1b05` |
| `_actor_move_initialize` | 336 | 15 | `4cb84b739dfc710c` |

Other checks:
- fake scan: 0 leads;
- `CL /W3 /Zs` warnings: 14 before and 14 after;
- CRLF line endings are preserved;
- `git diff --stat`: 51 lines added, 87 removed.

## 3. Laws applied and new measured facts

1. **Rename-import removal** (brief §9 normalize3d law and w2 A22 "visible inline declined"). The per-TU renames were
   removed:
   - `#define object_get_bounding_sphere/scale_vector3d/distance3d ..._inline`;
   - their external prototypes;
   - the five explicit `scale_vector3d_inline(` call spellings, which were forced inlining.

   VC7's per-site decisions for scale_vector3d and distance3d still equal January's, so those removals are byte-neutral.
   The object_get_bounding_sphere removal changes the caller's x87 schedule. It is required for avoidance_setup.
2. **Join-store law (NEW).** When the same store is written at the end of both arms of an if/else, VC7 merges it and
   hoists it before the branch as a register def, and the variable is enregistered. Writing the store once, after the
   if/else, has two effects:
   - VC7 tail-duplicates it into the arms as memory stores;
   - the variable spills.

   This is try_evasion_direction's `attempt_count`. It fixes the frame (8 → 0xc) and the random-arm cross-jump.
3. **Loop-increment placement.** A statement in a for-loop increment clause is emitted after the body. Examples are
   `attempt_index++, evade_direction ^= 1`. Statement order inside the body was inert across four orders.
4. **Two then-blocks prove an else-if split** (A16-style control-flow evidence). January's step-up failure falls
   through into its own `fstp st(0); jmp` block before the drop test. Consequences:
   - the `A || (B && C)` spelling lays out one then-block with a pop stub;
   - the split also moves `found` from `[ebp-1]` into BL.

   Initialising `found` before `evasion_is_ledge` reproduces the stores from BL.
5. **A5 alias removal on array-walking loops.** Pointer aliases over `sense_rays[i]` and `avoidance_rays[d][r]` changed
   the compiler's strength-reduced pointer bias (+8 vs January's +4). They also changed the hoisted zero pin and the
   induction-variable increment order. Indexed access reproduces January.
6. **A4 helper routing, diagnostic in destination_update.** January's three flying `moving_towards_point` rows use a
   uniform `body + vector` association. `add_vectors3d` with the admitted point/vector casts reproduces that
   association, but the function is not exact.

## 4. Per-function outcomes

### `_actor_move_avoidance_setup` — EXACT, landed

The distance is now `distance2d((real_point2d const *)&center, (real_point2d const *)&sphere_center)`, replacing a
magnitude2d of a hand-computed offset. The point2d cast idiom has 32 source sites. A15 fixes the role order:
`vector_from_points2d(a,b) = b-a`. The object_get_bounding_sphere rename import was also removed.

Measured shapes:
- as1, distance2d only: sha;
- as3, rename removal only: sha;
- as2 = as1 + as3: EXACT;
- r1, scale_vector3d removal: byte-neutral on all rows;
- r2, distance3d removal: byte-neutral on all rows;
- r4 = as2 + r1 + r2: EXACT. This version is landed.

### `_actor_move_try_evasion_direction` — EXACT, landed

Starting point: w1's ed10 (set_real_vector2d cases, a for loop, a success flag and a ternary write-back), which proves
January's exit tails. Changes applied on top of it:
- `attempt_count = 2;` is written once after the random-side if/else (law 2);
- the side toggle is in the for increment (law 3);
- set_real_vector2d is used in all six constructions;
- `actor_get(actor_index);` is a bare validation call (A17).

Measured controls:
- the form without the law-2 store is 368;
- the form without the helper differs only in the forward-case argument-temporary signature;
- negate_vector2d is also exact, but was not chosen.

### `_actor_move_try_evasion_vector` — EXACT, landed

The height tests are split into `if (step up too high) found = FALSE; else if (maximum_ledge_height == 0.0f && drop too
deep) found = FALSE;`, and `found` is declared and initialised before `evasion_is_ledge` (law 4).

Shapes:
- x1, an if/else assignment of found: no change (A10 detector);
- y1, late initialisation: worse;
- y2, the split alone: 193 of 194 instructions;
- z1: EXACT.

The two conditions are distinct physical rejections and keep the original operand order. This is not an R2 both-arms
form.

### `_actor_move_initialize` — EXACT, landed

The first loop uses a `short` index with direct `sense_rays[sense_ray_index].field` stores. The second loop uses direct
`avoidance_rays[direction_index][avoidance_ray_index]` access (law 5).

Shapes:
- i1: short index only, inert;
- i2: indexed with a long index, 352/21;
- i3: short and indexed, 2 instruction-variable blocks left;
- i4: EXACT.

The body is independent of Fable's held S4. S4 differs: it writes explicit direction components, reads through a
`direction` pointer, and builds the offsets with set_real_vector3d. The data constants and COMMON externs were already
canonical, and no data owner changed.

### Not landed

- **`_actor_destination_update`.** The best scratch is `d6.c`, with 10 real blocks. It combines du1 order,
  point_from_line2d and add_vectors3d. Two differences remain:
  - the step-index EAX/ECX role versus the computation order;
  - the x87 lifetime of the negated projection.

  Shape d7 removed the next_step_point alias and was worse, so the step pointers are genuine. Five shapes were spent.
- **`_actor_get_stopping_distances`.** One commutative operand order remains, in the biped dot i-term. Previous waves
  already tried the obvious levers (do-not-repeat), and R3 rules out an operand swap. Skipped.
- **`_actor_path_refresh`.** There are 0 instruction differences. Three jcc choose a different identical epilogue copy.
  Six forms were already spent, including nesting. Skipped.
- **`_actor_move_vector_avoidance`.** The w1 va6 body re-measures at 4128/135 with 124 real blocks. The frame is
  0x60e4 vs January's 0x60e0, with a many-to-many slot remap, and site 2 contains an unevidenced written-out transform.
  No bounded shape exists. Not landed.
- **`_actor_move_test_avoidance_vector`.** Owner/policy-blocked. The only closer bodies are:
  - a twice-duplicated transform body, which is manual inlining under house rule 13;
  - forms that depend on `_point_from_line3d`, which lane rules forbid.
- **`_actor_move_update`.** DO-NOT-SPEND; untouched.

## 5. Owner census and proposals

These are candidate owners versus `build/base`. No header or config change is needed. They are disclosed for the
orchestrator's census acceptance:
- **Header-inline select-any COMDATs January references undefined:**
  - `_object_get_bounding_sphere`, with its assert literals `center`, `radius` and `..\objects\objects.h`; 8 and 11
    base objects already own these;
  - `_scale_vector3d` (60 base emitters);
  - `_distance3d` (17 base emitters).
- **real_math.h select-any surplus (A4 systemic family):**
  - `_distance2d` (2 base emitters);
  - `_vector_from_points2d` (10);
  - `_set_real_vector2d` (3).
- **Removed:** `_scale_vector3d_inline`, an invented non-January owner name.
- No data, BSS or COMMON change, and no target owner is missing.

The intake HOLD on `_actor_move_initialize` (fable_independent_intake_20260906) concerned two things: Fable's tuned S4
provenance, and the then-unowned data packet. The data is canonical now, and the landed body is independent of S4. The
orchestrator should confirm that the hold is lifted.

## 6. Do-not-repeat (added)

- try_evasion_direction and try_evasion_vector: closed.
- avoidance_setup and initialize: closed.
- try_evasion_direction statement order in the loop body is inert; the lever is the increment clause.
- try_evasion_vector: an if/else on found (A10) and late boolean initialisation do not move found into BL.
- destination_update: point_from_line2d does not change t's x87 lifetime; removing the step-pointer aliases is worse.
- scale_vector3d and distance3d rename removals are byte-neutral in this object.

## 7. Reopen criteria

- **destination_update:** start from `scratch/workers/w3_actor_moving/d6.c`. It needs a cause for the step-index
  register role and for t's x87 lifetime.
- **vector_avoidance:** start from w1 `va6.c`. It needs evidence for the site-2 expansion and a frame remap cause.
- **test_avoidance_vector:** an authenticated macro or helper for the written-out transform, or a change to the
  `_point_from_line3d` policy.
- **get_stopping_distances:** January evidence of an earlier by-name read of the biped velocity/forward i component.
- **path_refresh:** a same-compiler mechanism for epilogue-copy selection.

## 8. Disposition

Still active, not Matching: 30 of 36 strict exact. Four functions (1,920 padded bytes) receive new exact credit, pending
the orchestrator's full build. No fuzzy bodies were landed and no parks were touched.
