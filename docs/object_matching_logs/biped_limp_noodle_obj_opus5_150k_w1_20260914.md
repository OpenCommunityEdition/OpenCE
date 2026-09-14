# `biped_limp_noodle.obj` park re-investigation (opus5 150K lane, wave w1, 2026-09-14)

Worker scope: `source/units/biped_limp_noodle.c` only. The three functions are parked
(`config/parked.json`), so all body work was scratch-only. **The real file is unchanged**
(`git diff --stat` empty). No exact bytes are claimed.

## Baseline (real file, whole-TU gate with `--forbid-emitted-symbol _point_from_line3d`)

| Function | Target padded / meaningful / relocs | Baseline | Status |
| --- | --- | --- | --- |
| `_biped_limp_noodle_get_max_relaxation_iterations` | 16 | exact | EXACT |
| `_biped_limp_noodle_valid_joint_rotation` | 1248 / 1244 / 34 | 1216 / 34 | residual (parked) |
| `_biped_limp_noodle_move_relax_and_constrain_positions` | 1968 / 1955 / 66 | 1888 / 66 | residual (parked) |
| `_validate_real_vector3d_axes3` | 496 | exact | EXACT |
| `_biped_limp_noodle_adjust_orientations` | 704 | exact | EXACT |
| `_biped_limp_noodle_relax_nodes_onto_environment` | 256 / 244 / 9 | 240 / 9 | residual (parked) |

Final gate: identical (`== exact 3 residual 3 unwritten 0`). The guard passes. Park drift is 0 of 3.
The fake-match scan finds 0 leads on the real file and on the scratch candidates `vj1.c`, `vj2.c` and `vm6.c`.

Ledgers read first:
- `biped_limp_noodle_codex_checkpoint.md` (E01-E05 and its do-not-repeat list);
- the bipeds batch7 and model_animations node-owner ledgers;
- the 100K consolidated handoff, which lists this object as not yet re-investigated with the wave C/D laws.

## Results

### `_biped_limp_noodle_valid_joint_rotation`: strict EXACT, but only with inline `point_from_line3d` (owner-blocked)

New January-side evidence (from alndiff against `build/split`):

1. **Target signature/ABI.** `new_world_position` arrives in `esi`, not `edi`.
   - This is the private register parameter January assigns once the body is right.
   - `node_index` is kept in `ebx`.
2. **No `parent_matrix` pointer local.** January re-derives `node_matrices[node->parent_node_index]` after each call: `movsx [node+0x24]; imul 0x34` at +0x1d7, +0x21b and +0x2e1.
3. **Frame 0xE4.** `rotate_to_direction` and `local_rotate_to_direction` own distinct slots.
4. **Compare polarity.**
   - The three cosine tests are `test ah,5; jnp`, which is `!realcmp(x, 1.f)`. The `realcmp` macro is proven by the assert literal.
   - The conical tail and the follow-parent test are `test ah,0x41; jne`, the positive forms `fabs(arccosine(c)) < parent_node->range` and `current->z > new->z`.
   - The baseline used the negated `>=`/`<=` forms, whose NaN behaviour differs.
5. **Projection.** It uses the copied `plane_normal` (up vector at `[ebp-0x38]`), not `plane.n` (`[ebp-0x48]`).
6. **Control flow.** `moved = TRUE` is set inside each constraint branch, the follow-parent test is one condition block, and there is a single return. The gotos are gone.

Shapes:

| id | shape | result |
| --- | --- | --- |
| j1 | structured body with items 2-6 and component projection | 1248/34, sha `33d319267db7ff51`. The only residual is the 5-instruction order of the assert's inline `plane3d_distance_to_point` terms (January y·j, x·i, then k·z; candidate k·z, j·y, i·x). |
| j3 | `new->x + n.i*t` | same as j1 |
| j4 | block-scoped `projection_distance` | same as j1 |
| j5 | `t*n.i` | same as j1 |
| j2 | j1 with `point_from_line3d(new_world_position, &plane_normal, projection_distance, &rotate_to_position)` | **strict EXACT 1248/34**. It emits the forbidden `_point_from_line3d` COMDAT, so the guard fails. |
| vx_static (probe only, inadmissible) | TU-local `static __inline` copy of the helper | EXACT and no COMDAT. This proves the inline expansion is January's spelling. |

Scratch files are under `scratch/workers/biped_limp_noodle/`:
- `j1.c` / `vj1.c`: best admissible candidate;
- `j2.c` / `vj2.c`: the exact candidate;
- `vx_static.c`: the probe.

### `_biped_limp_noodle_move_relax_and_constrain_positions`: 1968/66 size-exact in scratch; owner-blocked and a schedule residual remains

New evidence (candidates built on `vj2.c`, so the callee ABI is January's):

- **Push distance.** It is the inline `vector_intersect_plane3d(endpoint, &collision.plane.n, &collision.plane)`. The dot order j,k,i and the `fdiv; fchs` (num / -nn) appear only with the helper.
- **Embedded count.** `embedded[0] + embedded[1]` is computed once, then tested against 0 and 2.
- **Endpoint selector.** It is `endpoint_index == 0 ? position : parent_position`: January loads `position` first.
- **Rest-distance range test.** It is `!(rest_distance <= 0.f || rest_distance > 10.f)` (`jnp`/`je`). The distance bounds are positive `>= 0.f`/`< 20.f`, and the epsilon tests are `realcmp` forms.
- **The apply loop needs inline `point_from_line3d`.** January loads `endpoint` into two registers (`p` and `result`). The ray origin is `point_from_line3d(parent_position, &segment, -0.015f, &ray_origin)`. So this body is blocked by the same COMDAT policy.
- **Two register-distinct copies of the enqueue-children statements.**
  - Range failures #1-#5 reach the copy at 0x6d4..0x70f, which jumps to the while test.
  - All other failures and the correction tails reach 0x713.
  - CFG-equivalent spellings (m3 nested ifs, m4 early goto, m5 goto-free else) are byte-identical to m2b, so VC7 does not duplicate the block itself.
  - m6, which enqueues and then `continue`s on the range-rejection path, reproduces the block, the frame slot ranking and the 1968 size.

Remaining difference in m6 (sha `1a77a1762511c233`): about 12 instructions of scheduling in the first node block.
- January stores gravity velocity k, then i and j, then loads the parent index and computes segment x/y with indexed addressing, with the `lea` late.
- Our build hoists the parent-index load and the `lea`s and sinks the zero stores.
- Loop-top padding compensates for the 4-byte difference.

| shape | change | result |
| --- | --- | --- |
| m7 | pointer assignments moved after the velocity set | identical to m6 |
| m8 | indexed `vector_from_points3d` | identical to m6 |
| m9 | parent pointer declared first | identical to m6 |
| m10 | component velocity stores | identical to m6 |
| m14 | pointers in a nested block | identical to m6 |
| m11 | pointers at do-body scope | frame 0x304 (worse) |
| m12 | separate `correction_velocity` local | frame 0x30c (worse) |
| m13 | segment before velocity | a different, non-January order |

Census: m6 adds candidate-only `_point_from_line3d` and `_vector_intersect_plane3d` COMDATs.

### `_biped_limp_noodle_relax_nodes_onto_environment`: register tie with a proven weight lever; no authentic spelling

- The frames are equal (0x10).
- January keeps `biped` in `esi` across the snapshot loop and both calls. It spills `node_index` to `[ebp-8]` and emits no loop-top alignment.
- The baseline spills `biped` and enregisters `node_index`.

| shape | change | result |
| --- | --- | --- |
| rA | early `return relaxation_complete` | worse (frame 0xC) |
| rB | function-scope index | inert |
| rC | `last_positions` alias | inert |
| rD | `while` loop | inert |
| rE | `if` + do-while | inert |
| rH2 | `= x + 1` increment | inert |
| rH3 | `MIN` saturation | worse |
| rF | `if (current < max)` instead of `if (!relaxation_complete)` | reproduces January's allocation (biped in `esi`, index in memory, no pad), but emits a second compare/`jae` instead of branching on the stored boolean. Diagnostic only. |

## Proposals

**ORCHESTRATOR-PROPOSAL (owner ruling): `_point_from_line3d` COMDAT policy.**
- This object adds two more consumers to handoff item 1.
- `valid_joint_rotation` (1248 padded / 1244 meaningful) is strict EXACT in `scratch/workers/biped_limp_noodle/vj2.c` once inline `point_from_line3d` may be used here.
- `move_relax_and_constrain_positions` needs it too.
- The measured mechanism: a `static __inline` form of the helper is exact and emits no COMDAT.
- Park coupling (measured with `scratch/parkcheck.py` on `vj2.obj`): reopening `valid_joint_rotation` drifts the parked `move_relax_and_constrain_positions` body from 1888/66 sha `34b872cd` to 1888/66 sha `683135cea08a8197`, because its call site follows the new `esi` ABI. The orchestrator must re-baseline that park in the same commit. `relax_nodes` does not drift. Moving to it in `real_math.h` would need a full-tree measurement (header-inline linkage law: a blanket `static` costs 21 objects), limited to `point_from_line3d` only.
- `real_math.h` consumers: effectively the whole tree.
- If the owner admits a non-COMDAT inline form, re-gate `vj2.c` whole-TU. It must not drift the other two parks: the ABI of `move_relax` changes with the new callee, so re-baseline or re-measure it. Then land it as a park reopen.

## Reopen criteria

- **`valid_joint_rotation`:** owner admits inline `point_from_line3d` without an emitted COMDAT, or waives the COMDAT for this TU. The j2 body is ready.
- **`move_relax_and_constrain_positions`:**
  - the same owner ruling;
  - an authentic spelling for the first-node-block schedule;
  - review of the credibility of the m6 enqueue-and-continue rejection path.
- **`relax_nodes_onto_environment`:** an authentic spelling that gives `biped` more allocation weight than the loop index while still branching on `relaxation_complete`. Do not re-spend rA-rH3.
