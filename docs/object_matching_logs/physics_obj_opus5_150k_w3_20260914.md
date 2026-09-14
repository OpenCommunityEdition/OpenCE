# physics.obj — opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/physics/physics.c`. Worker notes: `scratch/workers/w3_physics.md`; scratch: `scratch/workers/w3_physics/`.

## Baseline and final (HEAD 210aa8f81, real file unchanged)

| Gate | exact | residual | unwritten | guard |
| --- | ---: | ---: | ---: | --- |
| baseline | 13 | 4 | 0 | passed |
| final | 13 | 4 | 0 | passed |

`git diff --stat -- source/physics/physics.c` is empty. Park drift is 0 (`_compute_ground_plane` 336/14, `_physics_compute_new`
3120/51). The fake scan reports 0 leads. The owner census does not apply because nothing changed. Nothing newly exact.

Non-exact rows (target padded; unchanged at the end):

- `_compute_ground_plane` 336 [sha], PARKED
- `_physics_compute_new` 2944 [3120/51 vs 2944/49], PARKED
- `_physics_compute_vehicle_collision` 1168 [1152 vs 1168, sha]
- `_physics_update_old` 5168 [5280/114 vs 5168/115]

Ledgers read: `physics_obj_opus5_150k_w1`, `physics_obj_opus5_100k`, the three jonas ledgers,
`update_new_reconciliation`, `bored_camera_obj.md`, `breakable_surfaces_obj_opus5_150k_w2` (A22), `observer` w2,
`scratch/w2/laws_w2.md`, and the w1 worker notes.

## 1. `_physics_update_old`: the structural blocker is a wrong shared-header body

In w1, `valid_real_vector3d_axes2` stayed out of line at the final `match_assert_valid_real_vector3d_axes2`. That call
accounted for the size and relocation gap, and the mechanism was unknown. This wave found the mechanism.

- **January census** (`axes2_sites.py` over every split object): 9 January functions use the axes2 assert string.
  - Only `_physics_update_old` inlines the helper (3x `__real@3f50624de0000000`, no call).
  - The other 8 call it out of line: observer ×2, projectiles, objects ×2, units ×2, and `_rotate_vectors3d_by_angular_velocity`.
- **Header body is wrong.** `bored_camera_obj.md` records two trials:
  - A05: January's out-of-line COMDAT is strict EXACT with the direct conjunction
    `return valid_real_normal3d(forward) && valid_real_normal3d(up) && valid_realcmp(dot_product3d(forward, up), 0.f);`
  - A02: the `real_math.h` result-variable body was rejected (0x80).
  - `bored_camera.c` carries its own conjunction definition under a `#define` rename.
  - Lab `x8.c`: the conjunction body is instruction-identical to the January COMDAT (48/48); the header form has 64 instructions.
- **Inline behaviour** (lab `x1`-`x7`): the header body never inlines in any caller shape tried: loops, nested loops,
  in-loop sites, parentheses, a bound boolean, or axes3. `valid_real_normal3d` inlines only in callers with a real loop.
  A conjunction-form helper does inline completely at update_old's tail (`c/uo_andh.c`, measurement only): 5184/115.
- **Shadow header tree** (`scratch/workers/w3_physics/shadow`, `sgate.py` = `gate.py` with `/Isource` redirected), with
  only the axes2 body replaced:
  - Real physics.c: update_old goes 5280/114 -> 5456/118 and inlines. The 13 exact siblings are unchanged, and rotate
    still calls the helper out of line (as January).
  - w1 decoded body `uo_v1` (`c/uo1.c`): 4992/111 -> **5184/115**. Relocations and the 0x16c frame now equal the target.
  - Full sweep of all 447 source units (`sweep_axes2_all.txt`): no compile failures. Only four rows change:
    - update_old, as above;
    - `_observer_update_positions` (PARKED): 1280/28 -> 1648/32 against a 1568/40 target, so the park would drift;
    - `_bitmap_copy` (2784): EXACT -> sha;
    - `_bitmap_2d_uncompress_from_mipmap` (880): EXACT -> sha.
  - Both bitmap flips are one-instruction `mov reg,[ebp+0x10]` reload transpositions in TUs that never reference the helper.
  - Controls: a comment or `0.f`->`0.0f` edit does not flip the bitmap rows. The conjunction plus two unused locals, the
    conjunction padded to the same line count, and two other if/result spellings all flip them. The sensitivity is to the
    body's IL, not to local count or line numbers.
- **Remaining under the corrected header:** 228 alignment hunks of 1-6 instructions spread over the whole function
  (header-local stack slots, counter store scheduling, powered-branch registers, tail normal3d order). This is the same
  tie family as compute_new and is not closable within the shape budget.

Classification: the structural cause is a wrong shared-header reconstruction (solved, orchestrator packet); the residual is tie-dominated.

## 2. `_physics_compute_vehicle_collision`: one-instruction scheduling tie

January `mov byte [ebp-1],1` (the collision flag) at 0x2ab follows `fmul [ebp-0x70]`, the in-place first product of
torque1.j, where radius1.k dies. Shapes this wave, all 1168/14 with the 13 siblings exact:

| Shape | Store | Note |
| --- | --- | --- |
| v1: real `point_from_line3d` inline, flag between the crosses | 0x1b7 | point guard fails (extern COMDAT) |
| v2: real inline, flag after the second cross | **0x2a8** | one instruction early |
| v3: TU-local `static __inline` typed helper, flag after | 0x2a8 | guard passes |
| p00-p16: flag at every statement boundary | 0x1b7 before the second cross, 0x2a8 after it | after the hit block: 896, different logic |
| v4: v3 + inner scope block for the point/radii | 0x2a8 | inert |

The prior lane's macro form placed the store two instructions early (0x2a2). The real-inline shape halves that to one
instruction. Class D: an integer-store vs x87 scheduling tie. Best scratch: `scratch/workers/w3_physics/vc/v3.c`.

## 3. Parks (scratch only; no reopen)

- `_physics_compute_new`: w1 exhausted the shape budget. The laws_w2 detectors do not control the remaining x87
  commutative operand orders: A22 younger-operand predicts January's loop-top order, but w1 measured a period-8
  dependence on local-symbol count. A11, A12, A15 and A20 do not apply. `cn_v6_arr.c` is still 2944/49 sha at HEAD.
- `_compute_ground_plane`: still one parameter-load placement (`mov eax,[ebp+0xc]` before the depths_of_hell copy).
  No law covers parameter-register load timing. Store order already follows declaration order (A20).

## 4. Orchestrator proposal (HOLD, information)

`source/math/real_math.h`: replace the `valid_real_vector3d_axes2` body with
```c
__inline boolean valid_real_vector3d_axes2(
	real_vector3d const *forward,
	real_vector3d const *up)
{
	return
		valid_real_normal3d(forward) &&
		valid_real_normal3d(up) &&
		valid_realcmp(dot_product3d(forward, up), 0.f);
}
```

**Evidence:** section 1 (bored_camera A05/A02, lab x8, January inline census, shadow-tree measurements).

**Consumers:** 188 files include `real_math.h`. The TUs naming the helper are bored_camera, camera_scripting, dead_camera,
editor_flying_camera, first_person_camera, flying_camera, following_camera, observer, orbiting_camera, static_camera,
projectiles, objects, physics, bipeds and units.

**Measured effect today:**

- Exact rows: -2 (`_bitmap_copy` 2784, `_bitmap_2d_uncompress_from_mipmap` 880).
- `_observer_update_positions`: the park needs a remeasure (1648/32).
- Gain: 0 until an update_old body closes.

Recommend holding until the packet can carry a closing update_old body and a resolution of the two bitmap ties.

## Reopen criteria

- update_old: the header packet above, plus closing the 228 tie hunks. Candidate start is `c/uo1.c` under the shadow header.
- vehicle_collision: a VC7 law for the hoist point of a constant store into an x87 expression, or an owner ruling on a
  1-instruction tie with the static helper.
- compute_new: a deterministic VC7 rule for x87 commutative operand order (the period-8 symbol-count mechanism).
- compute_ground_plane: a law for parameter-register load placement.
