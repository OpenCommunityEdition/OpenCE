# `bipeds.obj` opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/units/bipeds.c`, target `build/split/source/units/bipeds.obj`.
- Compiler: VC7 13.00.9254 (XDK 3911 toolchain), repository flags `/O2 /Oy-` (no per-TU override).
- Lane: `opus/100k-consolidated-20260914` at `b8cd50113`, worker `waveA:bipeds`.
- Evidence used: January target COFF (disassembly, relocations, literals); Ghidra decompilation
  `scratch/ghidra/out/bipeds.obj.decomp.c`; HCEA `src/blam/units/biped_start_landing.c`,
  `biped_update_jumping.c`, `biped_update_moving.c`, `biped_update_physics.c` (semantics only);
  Stian build-2276 `units/bipeds.c` `FUN_001a2b90` (jetpack topology corroboration only).
- Duplicate check: prior ledgers `bipeds_obj_batch7_exact_and_fuzzy_20260912.md` and
  `lane_reconciliation_10k_batch_7_20260912.md`; `git log --all -- source/units/bipeds.c` (21 refs);
  `branch_sweep.py source/units/bipeds` (18 historical blobs, none better than lane HEAD).

## 2. Validated baseline

Lane HEAD gate: `== exact 38 residual 13 unwritten 0` (guard passed). Nine residuals are parked and
locked (check_discard, fix_position, update_turning, update, find_nearby_support_surface,
adjust_placement, get_sight_position, get_autoaim_pill, accelerate) and were not edited; all nine keep
their `measurements.base` size/relocations/normalized SHA (verified on the final object).

| Residual | Target padded / meaningful | Relocs T | Baseline candidate |
|---|---:|---:|---|
| `_biped_start_landing` | 272 / 257 | 12 | 256 / 12 |
| `_biped_update_jumping` | 512 / 509 | 21 | 640 / 23 |
| `_biped_update_moving` | 4080 / 4071 | 97 | 4000 / 97 |
| `_biped_update_physics` | 5376 / 5366 | 124 | 4864 / 66 |

## 3. Result

Final real-file gate: `== exact 40 residual 11 unwritten 0`, `_point_from_line3d` guard passed,
object `scratch/workers/bipeds/final.obj`. No previously exact function changed state.

| Function | Disposition | Final size / relocs | Normalized SHA16 |
|---|---|---|---|
| `_biped_start_landing` | **strict EXACT** (272 padded / 257 meaningful) | 272 / 12 | `1eb5725913aec18c` |
| `_biped_update_jumping` | **strict EXACT** (512 padded / 509 meaningful) | 512 / 21 | `90bd42606540ced2` |
| `_biped_update_physics` | zero-credit fuzzy (structurally closer) | 5248 / 112 | `fccc72d132c21317` |
| `_biped_update_moving` | not landed (best candidate kept in scratch) | 4000 / 97 in file | — |

## 4. Accepted controls

### `biped_start_landing`

- January frame `sub esp,8` holds both scaled thresholds; the soft threshold guard is
  `test ah,5; jnp exit` (jump-to-exit when `landing_velocity < minimum_soft`), so the source is an early
  `if (landing_velocity < minimum_soft_landing_velocity) return;` (also HCEA's shape).
- The landing velocity is loaded once before the hard test and kept on x87 in both arms: a working
  local `velocity` (`landing - soft` in the soft arm, the raw landing velocity in the hard arm — January
  and HCEA both scale the hard arm by the raw velocity).
- `fld recovery; fmul st(1); ftol; fstp st(0)` proves the normalised fraction is the same x87 local
  divided and pinned in place: `velocity /= velocity_range; velocity = PIN(velocity, 0.f, 1.f);`.

### `biped_update_jumping`

- Velocity is passed by address as point and result to three `_point_from_line3d` calls (no
  `set_real_point3d` copies), with the same `(real_point3d *)&...translational_velocity` idiom the exact
  `units.c` already uses.
- `MAX(0.f, dot_product3d(...))`: one inline dot product in the comparison and an out-of-line
  `_dot_product3d` in the kept arm; branch layout puts the `0.f` arm first.
- `push 0xbe4ccccc` is not `-0.2f` (`0xbe4ccccd`); it is the float constant `0.8f - 1.f`.
- The crouch brake calls `_scale_vector3d` out of line. **Measured VC7 law:** an `__inline` helper is
  not expanded in a low-frequency block. A copy in a `?:` arm, an `||`/`&&` right operand, or one extra
  nested `if` level stays a call; extra calls elsewhere in the function do not flip it and splitting
  `A && B` into nested ifs does not change it. The natural extra level is a separate
  `if (!impulse && crouch && airborne)` after the boost branch (jump-threaded away, no bytes).
- A crouch-block local `real_vector3d *velocity` bound for the scale call settles the remaining x87
  operand-order tie in the inline dot product.

### `biped_update_physics` (fuzzy)

Rewritten from the January x86 disassembly (not the HCEA PPC transplant): entry/exit
`match_assert_valid_real_point3d/vector3d` (lines 0xBCC/0xBCD/0xF10/0xF11), flying/absolute/airborne/
grounded integration with `cross_product3d`, `normalize3d`, `point_from_line3d` calls where January calls
them, `debug_biped_skip_collision`, `collision_move_pill`, support-surface edge walk with
`distance_squared3d`, contact selection, bumped object and elevator selection, stand-up feature test.
The relocation multiset distance to January drops from 60 to 12 with no relocation target absent from
January; the 12 missing relocations are exactly the `debug_biped_physics` block, which needs
`collision_debug_*` declarations (see §8). Deliberate deviation: the stick-surface test adds
`best_collision_index != NONE &&`; January indexes `collisions[-1]` on the first contact, an
out-of-bounds read the house rules forbid reproducing.

## 5. Experiment matrix

| ID | Function | Shape | Size T/C | Relocs T/C | Result |
|---|---|---|---:|---:|---|
| SL1 | start_landing | `!(landing < soft)` block | 272/256 | 12/12 | compare fixed |
| SL2 | start_landing | separate `velocity` local | 272/272 | 12/12 | only final fmul differs |
| SL3-8 | start_landing | `fraction*recovery`, drop `(long)`, `recovery_ticks` local, inner-scope fraction, inline PIN, PIN-in-store | 272/256-272 | 12/12-13 | no gain, reverted |
| SL9 | start_landing | `velocity /= range; velocity = PIN(velocity...)` | 272/272 | 12/12 | EXACT |
| SL12 | start_landing | + early `return` guard | 272/272 | 12/12 | **EXACT, landed** |
| UJ1 | update_jumping | pointer-cast point_from_line3d, MAX(0,dot) | 512/528 | 21/23 | scale inline |
| UJ2 | update_jumping | `0.8f - 1.f` | 512/528 | 21/23 | constant fixed |
| UJ3 | update_jumping | separate `if (!impulse && ...)` | 512/512 | 21/21 | dot i-term tie only |
| UJ4-6 | update_jumping | split initialisers, else-if + `!impulse`, extra outer local | 512/512 | 21/21 | tie unchanged |
| UJ7 | update_jumping | crouch-block `velocity` pointer local | 512/512 | 21/21 | **EXACT, landed** |
| UJX | update_jumping | learning probes: `?:` arm, `impulse ||`, nested ifs, extra calls | — | — | inline law measured, not landed |
| UM1 | update_moving | full rewrite from disassembly | 4080/4144 | 97/98 | 128→89 diff blocks |
| UM2-5 | update_moving | dy default / `.j = dy` fst, `_collision_user_melee`, `& LONG_MIN`, crouch transition arms | 4080/4112 | 97/96 | 98→95 |
| UMJ0 | update_moving | flying acceleration folded before `/30` | 4080/4112 | 97/96 | 81, frame slots equal |
| UMG3 | update_moving | biped flags local block | 4080/4080 | 97/96 | 77 |
| UMG6 | update_moving | `physics.velocity = physics.new_velocity` | 4080/4080 | 97/96 | 71 |
| UMN2 | update_moving | no `dy` local: cases write `.j` directly, `.j *= movement_scale`; alert `.j = 0` first | 4080/4096 | 97/96 | 67 (best, not landed); switch region byte-identical |
| UMX | update_moving | dy init/eager, default placement/case order, sidestep regroup, forward/sideways/crouch locals, out_flags local, `word` dead locals, `/Ow`, `/Oa` | — | — | no gain or worse; `/Ow`,`/Oa` byte-inert (flags never landed) |
| UP1 | update_physics | full rewrite | 5376/5280 | 124/112 | 245→176 blocks (with scratch debug block) |
| UP1D | update_physics | + debug block via scratch-only externs | 5376/5392 | 124/124 | relocation multiset equal; not landable |
| UPL | update_physics | landed: no debug block, component expressions, epsilon² | 5376/5248 | 124/112 | **fuzzy landed** |

## 6. Do-not-repeat list

- start_landing: `fraction * recovery`, removing `(long)`, `recovery_ticks` local, block-scoped
  `fraction`, PIN on the division, PIN inside the store — all measured inert.
- update_jumping: expecting a code-size or call-count budget to force out-of-line helpers (refuted);
  nested `if` vs `&&` (equivalent).
- update_moving: `/Ow` and `/Oa` (byte-identical, and flag changes are prohibited anyway); switch
  `default` position or case order (layout unchanged); `real dy = physics.movement_desired.j` at the
  declaration (eager x87 load); locals for forward/sideways/acceleration/crouch (spilled); `word out_flags`
  local (spilled or full-dword load); sidestep product regrouping (VC7 reassociates or CSEs).

## 7. Residual classification

- `_biped_update_moving` (unresolved, ~12 sites): January keeps CSEs of `physics` fields (e.g.
  `out_flags` low byte in `cl`, damage-flag mask in `ax`) alive across stores through `biped`; our compile
  kills or reloads eagerly. Remaining sites are x87 operand order and store scheduling. Not a flag
  (measured). **Measured VC7 law (switch region now byte-identical):** the last store of a struct field
  in a case block is forwarded into an x87 phi at the join and elided (a lazy `fld field` appears on the
  edges that did not write it); stores followed by further statements stay (`fst`). A `dy` local instead
  of writing the field directly produces an eager load or a spill.
- `_biped_update_physics`: missing debug block (header), then x87 operand-order and frame-slot ties.

## 8. Orchestrator proposal

Add to `source/physics/collision_debug.h` (globals section):

```c
extern real_point3d collision_debug_point;
extern boolean collision_debug_repeat;
extern real_vector3d collision_debug_vector;
extern real collision_debug_length;
extern real collision_debug_width;
extern real collision_debug_height;
```

Evidence: January `_biped_update_physics` relocations to `_collision_debug_point:0/4/8`,
`_collision_debug_repeat`, `_collision_debug_vector:0/4/8`, `_collision_debug_length`, `_width`,
`_height`. Direct consumers: `source/hs/hs_globals_external.c`, `source/main/main.c`,
`source/physics/collision_debug.c`, `source/physics/collision_usage.c` (gate each; declaration-count
sensitivity is real). With it, the scratch variant `scratch/workers/bipeds/up_body1d.c` measures
5392/124.

## 9. Reopen criteria

- update_moving: a same-compiler donor demonstrating lazy x87 reload of an assigned struct field
  across an `if`/`switch`, or the declaration shape that keeps physics-field CSEs alive across
  `biped->` stores. Start from `scratch/workers/bipeds/update_moving_best_n2.c` (67 diff blocks).
- update_physics: the §8 header proposal; then operand-order work from `up_body1d.c`.

## 10. Disposition

`bipeds.obj` is 40/51 strict exact. start_landing and update_jumping are eligible for `Matching` credit;
update_physics is zero-credit fuzzy; update_moving and the nine parks remain residual.
