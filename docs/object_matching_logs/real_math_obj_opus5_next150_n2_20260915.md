# `real_math.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

## Result

**No function landed. `source/math/real_math.c` is unchanged (`git diff --stat` empty).** The final real-file gate is
identical to the baseline, row for row: `== exact 80 residual 4 unwritten 0`.

Two of the four residuals now have a **complete, admission-verified strict-EXACT candidate** blocked only on an owner
ruling; the other two are reduced, in scratch, to one and the same single 2-instruction x87 operand-order tie, and this
wave added two new measured negatives that close the remaining hypothesis class.

| Function | Target padded / relocs | Real file at HEAD | Best scratch candidate | Status |
| --- | --- | --- | --- | --- |
| `_point_in_triangle3d` | 432 / 9 | 432 / 9 `[sha]` | **strict EXACT** (`scratch/workers/n2_real_math/v3.c`) | OWNER-BLOCKED |
| `_pill_test_vector3d` | 448 / 9 | 464 / 9 `[size, sha]` | **strict EXACT** (`v3.c`) | OWNER-BLOCKED |
| `_sphere_intersects_triangle3d` | 608 / 6 | 624 / 6 `[size, sha]` | 608 / 6, one 2-insn diff (`v3.c`) | NOT-LANDED (tie) |
| `_pill_intersects_triangle3d` | 832 / 12 | 848 / 12 `[size, sha]` | 832 / 12, one 2-insn diff (`v3.c`) | NOT-LANDED (tie) |

Worker notes with the full per-function report blocks, frame censuses and relocation censuses:
`scratch/workers/n2_real_math.md`. All candidates are in `scratch/workers/n2_real_math/`.

## Baseline and guards

- **Emitted-symbol guard.** The `_point_from_line3d` guard fails at baseline and the failure is pre-existing: January's
  `real_math.obj` legitimately owns the math helpers, and the unchanged file already emits a 48-byte
  `_point_from_line3d` COMDAT alongside ~30 other surplus helper COMDATs. The unit was gated without the guard.
  Owner census of the best candidate against `build/base/source/math/real_math.obj`: **195 owners on each side, 0 new
  and 0 lost** — no additional candidate-only math owner appears and no `_point_from_line3d` copy was introduced.
- **Parks.** `scratch/parkcheck.py source/math/real_math` → "no parks for source/math/real_math". No drift possible.
- **Siblings.** The `v3.c` candidate leaves every baseline-EXACT row EXACT (full `--all` row diff; only the four target
  rows move).
- **Fake-match scan.** 0 review leads on the real file and 0 on `v3.c`.

## Duplicate prevention

Ledgers read: `real_math_obj_jonas_safe_owner_restoration_20260826.md`,
`real_math_obj_jonas_global_typed_boundary_20260830.md`, `real_math_obj_jonas_helper_restoration_exact_20260831.md`,
`real_math_projection_current_context_recheck_20260904.md`, `real_math_obj_opus5_150k_w1_20260914.md`, plus
`scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, the 150K handoff rejection section and the 250K checkpoint.
`git log --all -- source/math/real_math.c` (15 commits) and `branch_sweep.py source/math/real_math` (95 unique blobs).
Every do-not-repeat family was honoured: no `outside[4]`/barrier spelling, no `_ReadWriteBarrier`, no forced-inline
third-edge helper, no direct third-edge scalar control, no declaration-order sweep of `_point_in_triangle3d`.

## The two structural rows are now a single shared x87 tie

Both `_sphere_intersects_triangle3d` and `_pill_intersects_triangle3d` reach target size **and** a 0-differing-row
relocation census in `v3.c` (6/6 and 12/12, `relocdiff --count-by-target`). Their frames are identical to January's
(`sub esp,0x30` and `sub esp,0x40`, every `[ebp±N]` slot equal — the slot tables are in the worker notes). What remains
is one instruction pair each, inside an inlined `cross_product3d(a, b, out)`, at the i-component's second product
`a->k * b->j`:

- sphere @0xdf: `T fld [ebp-0x28]; fmul [ebp-0x14]` vs `O fld [ebp-0x14]; fmul [ebp-0x28]`
- pill  @0x64: `T fld [ebp-0x14]; fmul [ebp-0x24]` vs `O fld [ebp-0x24]; fmul [ebp-0x14]`

**January-side law measured here (closes nothing, but explains the family).** In the inlined `cross_product3d` January
always `fld`s the `.k` operand of a commutative memory×memory product when one is present: i1 `fld b->k`,
i2 `fld a->k`, j1 `fld a->k`, j2 `fld b->k`, k1 `fld b->j`, k2 `fld b->i`. Our build reproduces five of the six and
picks `b->j` for i2. The divergence appears **only when both vector operands are fully homed in memory**; where `b`'s
`.k` component is still live on the x87 stack (the `fst` form) the two builds agree — which is why the same inlined
helper is EXACT in `_vector_intersects_triangle3d` (@0x44) and in `_sphere_intersects_triangle3d`'s own `normal` cross
(@0x5f). Both failing sites are byte-identical up to the differing instruction, so the machine state at the choice
point is the same in both builds and the difference must come from the expression IL, not from allocation history.

**New measured negatives (shadow tree only; the real header was never touched).** Using a copy of
`scratch/workers/path_obstacle_avoidance/shadow_gate.py` with a shadow `real_math.h`:

- **H1** — `cross_product3d` body reordered to declare `i, j, k` (the emission order) instead of `k, j, i`:
  fixes neither row and **loses 8 exact rows inside real_math alone** (82 → 72; `_point_from_planes3d`,
  `_refract_vector3d`, `_vector_intersects_pill3d`, `_vector_to_line_distance_squared3d`, `_yaw_vectors`, …).
- **H2** — the same body written as `real i; real j; real k;` plus three assignments: byte-inert (82 exact, same two
  residuals).

Together with w1's ~40-probe lab (helper text order, direct stores, swapped products, prototype/struct/static counts,
variable names) and w1's five call-site shapes, the header-body hypothesis is now closed. `branch_sweep`'s best
historical blob (`history:614690af`, exact 82 / residual 2) is also non-exact on these two rows, so no historical body
closes them either. Per the brief the tie budget is spent; the zero-credit 624→608 / 848→832 improvements were NOT
landed (laws_w2 R11).

## Owner ruling requested — 880 padded bytes, candidate ready

`_point_in_triangle3d` and `_pill_test_vector3d` are strict EXACT in `scratch/workers/n2_real_math/v3.c`
(whole-TU gate `== exact 82 residual 2 unwritten 0`). Both need one representation-pointer conversion at a math-helper
argument, which the 20260826 ledger's do-not-repeat list and the 20260831 rejection currently forbid. Per laws_w3 R13 a
recorded hold binds later waves, so nothing was landed; the ruling is requested with evidence those audits did not have.

**The conversions are forced by January's own relocations, not chosen for codegen.**

- `_point_in_triangle3d` carries **three `_project_point3d` relocations** (0x0f7, 0x106, 0x136). The projected objects
  are point differences — `real_vector3d` — and the only prototype is
  `real_point2d *project_point3d(real_point3d const *point, …)` (real_math.h:993/999). There is no `project_vector3d`.
  w1's pf1/pf2/pf3 bisection additionally proves January inlined `cross_product2d` on the projected `real_point2d`
  results (pf2, with the 2D crosses hand-written, returns to the 20-byte slot residual).
- `_pill_test_vector3d` carries **one `_fast_normalize2d` relocation** (0x18b) whose argument is EDI, the caller's
  `real_vector3d *normal` output; `fast_normalize2d` is declared `real_vector2d *fast_normalize2d(real_vector2d *v)`
  (real_math.h:540) and `real_vector3d` embeds no `real_vector2d` subobject (20260830 audit).
- The cast-free alternative is what is checked in today: `_point_in_triangle3d` hand-expands five real_math.h helpers
  (`vector_from_points3d` ×3, `cross_product3d`, `dot_product3d`, `magnitude_squared3d`, `cross_product2d` ×3), which
  is itself the prohibited hand-expanded-helper class (house rule 13, laws_w3 R15).

**Evidence that is new since the hold.**

1. **January's own object contains the cast as source text.** `build/split/source/ai/actors.obj` holds the assert
   literals `"(real_vector2d *) &actor->input.facing_vector"` (file offset 36169) and
   `"(real_vector2d *) &control_data.facing_vector"` (offset 36649). `actors.h:501` declares
   `real_vector3d facing_vector;` and `valid_real_normal2d` is a 2D helper, so Bungie's own source cast a
   `real_vector3d` to `real_vector2d *` for a 2D math routine. This is January-side proof, not an analogy, and it
   refutes the 20260826 premise that such a cast can only be layout punning.
2. **An exact, credited in-tree donor for the identical construct with the identical helper.**
   `_collision_prism_test_vector` (`source/physics/collision_features.c:1011-1012`) is strict EXACT at this HEAD
   (`gate.py source/physics/collision_features --all` → 20 exact / 0 residual) and contains
   `project_point3d((real_point3d const *)&v3d, prism->projection_axis, prism->projection_sign, (real_point2d *)&v2d);`
   with `real_vector3d v3d; real_vector2d v2d;`, directly above a `point_from_line3d` call that casts both an input and
   an output the same way.
3. **Established production practice.** 40 `(real_point2d …*)`, 44 `(real_vector2d …*)`, 64 `(real_point3d …*)` and 73
   `(real_vector3d …*)` casts already exist in `source/**.c`/`.h`; `normalize2d((real_vector2d *)&…)` on a
   `real_vector3d` appears in actors.c, action_charge.c, action_obey.c, action_vehicle.c, actor_combat.c,
   actor_firing_position.c and actor_looking.c; `real_math.c` itself defines `global_origin2d` … `global_forward3d`
   with the same cast.
4. **Lane verifier admissions postdating the hold.** laws_w2 section B ("Admitted, so don't avoid them: point-to-vector
   `(real_vector3d *)` casts") and laws_w3 section B ("Newly ADMITTED by verifiers: `(real_point2d const *)&point3d`
   casts into 2D helpers, 32 source sites"), landed EXACT in `_actor_move_avoidance_setup` (actor_moving w3),
   action_vehicle w3d and render_sprite w3.
5. **The 250K history-mining rejection of these two rows was mechanical.** Checkpoint §6.3 rejected the real_math blob
   "because it emits the forbidden `_point_from_line3d`" — a guard the unchanged baseline file already fails and which
   this lane does not apply to real_math. `branch_sweep`'s best blob `history:614690af` reaches the same 82/2 fixed
   point with the same cast class.

**Second recovered fact, independent of the ruling.** In `_pill_test_vector3d` January emits `mov dword [edi+8],0`
*after* `call _fast_normalize2d`. A store through a pointer cannot cross a call, so `normal->k = 0.0f;` follows the
normalisation in January's source; writing it before the call leaves exactly one insert/delete pair. This statement
order (not the cast alone) is what takes the row from 448-with-one-pair to strict EXACT.

**Scope.** Consumer: `source/math/real_math.c` only. No header, config, symbols.json or other translation unit changes.
If the ruling is refused, nothing in `v3.c` should be landed: with the two cast hunks reverted the file carries no
credit (the sphere/pill bodies inside it are zero-credit R11 improvements).

## Reopen criteria

- `_point_in_triangle3d`, `_pill_test_vector3d` (880 B): the owner ruling above. The hunks are already measured and
  clean; resubmit `scratch/workers/n2_real_math/v3.c` unchanged.
- `_sphere_intersects_triangle3d`, `_pill_intersects_triangle3d` (1,440 B): a measured, legal source control of the
  memory×memory `a->k * b->j` load order inside an inlined `cross_product3d` — a law from another January consumer, or
  an authenticated `real_math.h` body that is byte-neutral on the other 82 rows (H1 and H2 show the two natural
  spellings are not). Start from `v3.c`, never from the real file's bodies. The two close together.
