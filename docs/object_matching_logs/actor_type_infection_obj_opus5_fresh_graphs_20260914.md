# `actor_type_infection.obj` opus5 fresh-graphs lane (2026-09-14)

## Result

`_infection_swarm_control` was structurally reconstructed and landed as a
**fuzzy, zero-credit** body under the unit-specific orchestrator allowance
(strictly closer in size and relocations, January-evidenced additions). It is
not strict exact. The object stays at 4/5 strict exact functions; no sibling
changed verdict.

| Function | January padded / relocs / normalized SHA-256 | Before (lane HEAD) | After (this ledger) | Disposition |
| --- | --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e...` | exact | exact | inherited |
| `_infection_wander_pause_time` | `96 / 5 / b4122501...` | exact | exact | inherited |
| `_infection_wander_move_time` | `96 / 5 / 2b2b9cfb...` | exact | exact | inherited |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e0...` | exact | exact | inherited |
| `_infection_swarm_control` | `3616 / 104 / 64eacc5c1e2a60caa70303a25621254f64fb070f72e5efa693892a0de1f19fee` | `3360 / 103 / 8efce2e0...` | `3600 / 104 / 62777de0dfcc6f5e14fb18d6ab1857ce53ec4b73a308f1d427d2aea595c325c9` | fuzzy landed, 0 credit |

Aligned instruction diff (normalized relocation spelling): 175 blocks / 810
changed instructions before, 31 blocks / 135 changed after (January 1127
instructions, candidate 1118). Frame `sub esp, 0xe8` now matches (was `0xe4`).
The relocation multiset is identical except the two jump-table base addends,
which shift with the 16-byte size gap; every jump-table case target aligns.

## Scope and provenance

- Unit `source/ai/actor_type_infection.c`; target `build/split/source/ai/actor_type_infection.obj`.
- Compiler: VC7 13.00.9254 `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja flags, no per-unit override).
- Evidence: January COFF disassembly (authority); Ghidra decompilation
  `scratch/ghidra/out/actor_type_infection.obj.decomp.c` (hypotheses only);
  HCEA `halocea-review/src/blam/units/infection_swarm_control.c` (semantics,
  explicit `primary_trigger`/throttle zero stores, explicit turn-clamp arms);
  existing `_ai_atom_move_facing_*` enum (`ai/ai_scenario_definitions.h`,
  already used by `action_obey.c`).
- Prior ledgers read: `_jonas_helper_pair_20260825`, `_jonas_swarm_jump_recovery_20260827`,
  `_jonas_exhaustive_census_20260830`, `_aim_jump_owner_reconciliation_20260909`.

## New evidence (satisfies the census reopen criterion)

1. **VC7 inlines `real_math.h` helpers by static block frequency.** Tiny-TU
   calibration with the unit's flags: `normalize3d` never inlines at function
   entry frequency; inside one loop it inlines under at most one nested
   conditional and not under two; joins restore frequency; two nested loops
   allow three conditionals. January inlines `normalize3d` at the first and
   third `has_direction` sites and calls it out of line at the second (inside
   the `dot < -0.9` else arm). That is only reachable when the
   `if (has_direction)` block sits at member-loop level, not nested inside
   `if (parent == NONE) { if (!airborne) ... }`. Hoisting it supplied the missing
   relocation (104/104) and January's exact inline/out-of-line split.
2. **VC7 pins a constant register globally once its IL use count crosses a
   threshold.** Removing any one of the six constant-3 uses killed the candidate's
   loop-wide `mov edi, 3` pin. January's search case has two byte-level stores of
   `animation_state = 3` but they are one statement after the if/else,
   tail-duplicated by VC7; that restores January's charge-local `mov ecx, 3`,
   switch layout, and the animation/separation frame slots.

## Accepted controls (retained source shapes and their evidence)

- Timer: `if (timer > 0) timer--; else switch (action) { case search: case charge: ... }` — `test ax,ax; jle`, `movsx eax; cmp eax,7/0xa` compare chain; `MAX(cooldown, 6.f)` gives January's `test ah,0x41; je`.
- No early return before the member loop (single loop guard).
- Member accessor `(struct biped_datum *)unit_get(unit_index)` — January mask 3.
- `short` movement/animation/aiming locals (dword constant stores, `movsx word`).
- Combat `>= definite` arm first; `target_prop` set in both arms after the melee test; `best_prop`/`best_prop_index` declared before the scores (`xor ebx,ebx` zero reuse).
- Action switch case order sleep, alert, guard, search, flee, charge/obey (jump-table body layout); charge `!= NONE` arm before the wander arm; `attack_delay_ticks > 0` (unsigned `sbb` form).
- Attach: dead-parent arm first; `< 255` / `> 0` unsigned guards (`jae`/`jbe`).
- Airborne arm first; wander `move_ticks > 0` arm first; `distance_squared < 0.25f` near arm first; each arm calls `rotate_vector_about_axis` (January reuses the `real_random_range` outgoing arguments, no stack pop between); jitter literal `0.020943951f` (`0x3cab92a6`) with a named damping term (`__real@bd888889`, not folded).
- Obey: `has_direction = TRUE` first; `switch (facing) { case _ai_atom_move_facing_left: case _ai_atom_move_facing_right: ... default: ... }` reproduces January's `movsx eax,word; cmp eax,2; jl; cmp eax,3; jg`.
- `has_direction` block at loop level; `dot < -0.9f` forward arm first; neighbor `dot(delta,right) > 0` subtract arm first with `real distance = square_root(distance_squared)` (January's `fxch` pair); explicit if/else turn clamp (only form producing pre-multiplied ±π/2 constants).
- Packet: weapon/grenade/zoom, flags, trigger, animation, aiming store order; explicit `primary_trigger = 0` and throttle `{1,0,0}`; `control.facing_vector` assigned in each arm and copied to aiming/looking (restores January's `to_center`/`control` frame order); `target_prop == NULL ? NONE : unit_index` arm order.

## Experiment matrix (candidate copies under `scratch/workers/actor_type_infection/`)

| Shape | size/relocs | changed insns | kept |
| --- | --- | --- | --- |
| v0 lane HEAD | 3360/103 | 810 | baseline |
| f1 structural rewrite | 3456/102 | 363 | yes |
| f2 has_direction hoist | 3584/104 | 284 | yes |
| f4 per-arm rotate | 3584/104 | 277 | yes |
| f6 case order | 3584/104 | 264 | yes |
| pH explicit clamp | 3584/104 | 254 | yes |
| oA facing switch | 3584/104 | 250 | yes |
| q3 declaration order | 3584/104 | 240 | yes |
| t1 packet store order | 3584/104 | 238 | yes |
| F2 named neighbor distance | 3584/104 | 238 (fxch fixed) | yes |
| y1 search animation after if/else | 3600/104 | 191 | yes |
| P1 facing_vector in arms | 3600/104 | 135 | yes |
| H1 enum constants + unit_get cast | 3600/104 | 135 (byte-identical to P1) | **landed** |

Siblings stayed exact in every whole-TU gate; `_point_from_line3d` guard passed.

## Do-not-repeat list

Measured inert or worse: scope promotion of `up`/`direction`/`control`/`to_center`;
`up` declaration order or declare-init; `long` animation/aiming types (`char` is
worse); `distance3d()` (identical code but emits a candidate-only `_distance3d`
COMDAT, rejected); `PIN`, `MIN(MAX)`, `MAX(MIN)`, `FLOOR(CEILING)`,
`CEILING(FLOOR)`, `DEGREES_TO_RADIANS` clamp spellings (none distribute the
multiply); split or else-if jump decision (worse); `target_prop ? ... : NONE`
(wrong arm order); `== 2 || == 3` and `> 1 && < 4` facing tests; hiding the
`actions.h` `normalize3d` prototype (not the inlining cause).

## Residual classification (measured facts; remaining causes are inference)

- x87 scheduling: prop-scan magnitude add order (`jj+kk+ii` vs `kk+jj+ii`);
  inline `normalize3d` #1 keeps `1/mag` on the stack until the join;
  `cross_product3d(&up, &direction, ...)` commutes `up.i` products at three sites.
- Register allocation, end of loop body: January keeps `should_jump` in `al`,
  precomputes `control_flags` into `esi` before `csmemset`, loads the unit in
  `edi` and flags in `cx`, and sinks the final forward-copy else arm; the
  candidate uses `bl`/`esi`/`ax`. This cluster carries the 16-byte size gap.

## Owner and data census

Against the lane-HEAD compile: only literal COMDAT deltas — added
`__real@bd888889` and `__real@bfc90fdb` (both January-owned, listed in the
object's January symbol table), removed non-January `__real@3f6eeeef`. No new
code, helper COMDAT, BSS or COMMON owner. `fake_match_scan`: 0 leads (baseline 0).

## Reopen criteria

Reopen only with a same-compiler donor or natural spelling that moves
`should_jump` to a caller-saved register (January's pre-call `control_flags`
computation), or evidence explaining the `up.i` operand commutation. Do not
re-spend the structural or do-not-repeat lists above.

## Disposition

Still `NonMatching`; `_infection_swarm_control` is fuzzy and receives zero exact
credit. No header or configuration change; no orchestrator proposal.

## Lead admission correction (2026-09-14)

The adversarial review REJECTED landing the improved `_infection_swarm_control`
body (3600/104 against 3616/104, 135 changed instructions) because the function
was already a written residual and the change does not make it strict exact.
The lead fully reverted `source/ai/actor_type_infection.c` to the lane base
(3360/103 residual). The improved candidate is preserved as research at
`scratch/workers/actor_type_infection/rejected_realfile_3600.c`, together with the
worker's scratch variants. Exact credit: 0. No production source changed.
Reopen from that candidate only with a source reason for the remaining 16-byte
register cluster (should_jump/control_flags/unit pointer homes).
