# `physics.obj` opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No production change. `source/physics/physics.c` is byte-identical to the lane base
(`git diff --stat` empty). The final real-file gate equals the baseline:
`exact 13 / residual 4 / unwritten 0`, `_point_from_line3d` guard passes, both parks
unchanged (`scratch/parkcheck.py`: 0 drift), fake-match scan 0 leads.

| Function | Target padded / relocs | Real file | Best scratch candidate | Outcome |
|---|---:|---|---|---|
| `_physics_compute_vehicle_collision` | 1168 / 14 | 1152 / 14 | 1168 / 14, 355/355 insns, one byte store 2 insns early | not landed (store-placement tie) |
| `_compute_ground_plane` (PARKED) | 336 / 14 | 336 / 14 | unchanged | park kept (parameter-reload scheduling) |
| `_physics_compute_new` (PARKED) | 2944 / 49 | 3120 / 51 | 2944 / 49, frame and every slot exact, 911 vs 910 insns | not proposed (x87 commutative-order ties) |
| `_physics_update_old` | 5168 / 115 | 5280 / 114 | 4992 / 111 | not landed (axes2 inline decision + ties) |

Worker notes with per-function blocks: `scratch/workers/physics.md`; candidates and the
lab harness: `scratch/workers/physics/` (`cn_v6_arr.c`, `uo_v1.c`, `vc1.c`, `vc2.c`, `lab/`).

## Baseline and duplicate checks

- Gate at lane HEAD: `exact 13 residual 4 unwritten 0`; inherited exact
  `_physics_compute_biped_collision`, `_rotate_vectors3d_by_angular_velocity`,
  `_physics_update_new` untouched.
- Ledgers read: `physics_obj_opus5_100k_20260914.md` (do-not-repeat: 12 collision-flag
  positions, no local `matrix3x3_multiply` prototype, no `point_from_line3d` COMDAT),
  `physics_obj_jonas_*` (3), `physics_obj_update_new_reconciliation_20260908.md`,
  `claude_fable_opus_reconciliation_checkpoint_20260908.md`, the 100K handoff.
- `git log --all -- source/physics/physics.c` (22 commits) and `branch_sweep.py`: no blob
  better than the current file.

## `_physics_compute_new` (parked; scratch only)

January facts recovered from the target disassembly and verified by compilation
(`scratch/workers/physics/cn_v6_arr.c`):

1. The powered definition is tested non-NULL before the datum pointer is formed
   (`test eax,eax; je` after `tag_block_get_element_with_size`, NULL stores in the else
   arms). Ternaries and if/else produce identical bytes.
2. `fabs` (x87 `fabs` instruction), not the `ABS` macro; the lift factors form a
   double-typed chain (`fabs(dot)*...` without a `(real)` cast). VC7 canonicalises
   multiply chains: permuting factors in source is byte-inert (measured).
3. `depth_fraction` is an if/else with `water_depth < physics->water_depth` and an integer
   store of `1.0f` (`mov [ebp-0x18],0x3f800000`), which removes the two surplus
   literal relocations (51 -> 49).
4. Water/air friction scales are computed inside each branch after the powered velocity.
5. `PIN` is applied to a named `alignment` variable (single x87 evaluation), not to
   `PIN(dot_product3d(...))`, whose macro duplicates the dot product into the arms.
6. The powered-ground `weight` is computed eagerly (named local), the projected velocity is
   produced by a fused point-from-line expansion and added into the friction with a
   second fused expansion.
7. Antigrav ground effect is `height > 0.0f ? 1.0f - height/antigrav_height : 1.0f`
   (January `jne` polarity).
8. The force is accumulated with six `add_vectors3d(&force, &x, &force)` (January reads the
   zeroed `force.j` before `normal_force.j`).

Key lever: the TU-local `PHYSICS_POINT_FROM_LINE3D` do-while macro (pointer locals) does
not compile like the real `real_math.h` inline at these sites (`fstp [ebp-4]; fmul
[ebp-4]` versus January's `fst [ebp-4]; fmul st(1) ... fstp st(0)`, and every stack slot
shifts). A TU-local `static __inline` helper with the same body, typed or over `real`
component arrays with `.n` arguments (no casts), is byte-identical to the real inline in
`_physics_compute_new` (sha `bd31f258` in both), passes the emitted-symbol guard, and keeps
`_physics_update_new` exact when substituted there.

Remaining differences (all tie class): velocity `cross_product3d` k-component load order
(2 products), alignment dot i-term, water-pressure chain head, antigrav magnitude chain
order, force-add `.i` terms (normal, air, powered), torque cross first product, and one
allocator choice (powered datum kept in `ecx` across `pin_fraction` versus January's reload
into `edx`).

### Measured commutative-order mechanism (lab, `scratch/workers/physics/lab/`)

- The three-statement velocity block compiled alone reproduces January's load order.
- Adding N used scalar locals cycles the six cross-product load orders with period 8 in N;
  locals before or after the block both shift it (roughly total count mod 8, with some
  register-driven exceptions).
- Inline helper calls, plain calls, extra file-scope extern object or function
  declarations (1-16), identifier renames and a dead constant-initialised local are inert
  (identical normalized SHA).
- In the full function, two invisible named scalar locals fix the loop-top site but the
  tail sites respond to a different window; none of the 64 combinations of six natural
  invisible-local options closes every site.

Interpretation: VC7's operand order for two x87 memory loads depends on local-symbol
numbering modulo 8, so reproducing January requires January's exact local inventory for
the whole function. This is a measured partial law, not a complete rule.

## `_physics_update_old`

- Relocation multiset: January inlines the whole `valid_real_vector3d_axes2 ->
  valid_real_normal3d -> valid_realcmp -> valid_real` chain at the final
  `match_assert_valid_real_vector3d_axes2` (3x `__real@3f50624de0000000`, `fst [ebp+0xc]`
  parameter-slot temporary). Ours calls `_valid_real_vector3d_axes2`.
- The January out-of-line copy (bored_camera.obj COMDAT) is strict exact against our header
  body. In the same January TU `_rotate_vectors3d_by_angular_velocity` calls it out of line.
- In our physics.c context the helper never inlines at a match_assert site: not with the
  loop or the tail deleted, not in a one-statement function, not after `physics_update`,
  not in a units.c copy; spelling the condition as an explicit `&&` still calls
  `valid_real_normal3d` out of line. The January decision is not a caller-size budget.
- Scratch candidate `uo_v1.c` (prior decoded rewrite + compute_new findings): 4992/111
  (the axes2 inline accounts for the gap), slot permutations in the header locals and the
  same commutative ties.

## `_physics_compute_vehicle_collision`

With the fused collision point (macro or real inline) the body is 355/355 instructions
identical except `mov byte [ebp-1],1`: January places it inside the second inlined
`cross_product3d` (after `torque1.i` is stored); ours at 0x1b7 or 0x2a2 depending on the
statement position. Placement is insensitive to local count (8 variants). Not landed.

## `_compute_ground_plane` (parked)

Only January's early `mov eax,[ebp+0xc]` (definition parameter) before the plane copy
differs. A named `radius` local (HCEA) and `center`/`ground_plane` pointer locals did not
move it. Park kept.

## Rejected / not landed shapes

- `real radius` local in compute_ground_plane (homes radius into the parameter slot).
- Direct-member macro variant of `PHYSICS_POINT_FROM_LINE3D` in the real file: breaks
  `_physics_update_new` (operand-order flip); not proposed.
- Named `water_scale` at block top or in the powered branch, scale+add powered velocity
  (all measured, none January's form).
- Invisible-local count combinations (64) for compute_new: no closure; these would in any
  case need independent authenticity review before use.

## Proposals

- Information for the orchestrator: a TU-local `static __inline` point-from-line helper
  (component-array parameters) reproduces the real inline's code where the do-while macro
  does not, and passes the emitted-symbol guard. It is only worth adopting with a closure
  that needs it; no production change was made.
- Optional: re-baseline the `_physics_compute_new` park measurement on the closer scratch
  body (2944/49) once the helper spelling is accepted.

## Reopen criteria

- `_physics_compute_new`, `_physics_update_old`: a complete VC7 rule for the commutative
  x87 operand order (local-symbol numbering), or authentic January local inventories; for
  update_old additionally the condition under which `valid_real_vector3d_axes2` inlines.
- `_physics_compute_vehicle_collision`: a VC7 law for integer constant store placement
  inside inline x87 expansions, or source evidence for a different collision-flag
  construct.
- `_compute_ground_plane`: evidence of the first statement that uses
  `mass_point_definition`.
