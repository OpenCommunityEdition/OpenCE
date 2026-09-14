# `actor_combat.obj`: burst caller graph reconstructed diagnostically; owner-blocked (2026-09-14)

## Scope and provenance

- Lane `opus/pdb-ida-15k-20260914`, pinned base `c3e257e9ab9a5d7e1c9b39862f87ac2abde4359c`; worker owned only
  `source/ai/actor_combat.c`. The real file is **unchanged** by this lane (`git diff --stat` empty).
- Compiler: XDK VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox` (uniform build flags; `/O2` implies `/Ob2`, which is why
  January inlines the same-TU `actor_get_weapon_definition` into `actor_combat_update`).
- Target: `build/split/source/ai/actor_combat.obj`, SHA-256 `6030e4891bf64d0cb0c41028a6c816ef07c4442f6c573ac70f37fb1e049ca45b`.
- Names: January debug map atlas (`scratch/atlas/actor_combat.obj.txt`, exact tier): `f590 actor_combat_enable_special_fire_situation`,
  `f880 actor_combat_get_firing_parameters`, `fba0 actor_start_pause`, `fc50 actor_start_first_burst_delay`,
  `10620 actor_combat_allow_special_fire_situation`, `109a0 actor_start_burst`; PDB public `_actor_combat_update`.
  Signatures/linkage from the HCEA DIA table already recorded in `actor_combat_obj_caller_closure_api_boundary_20260905.md`.
- Semantics only (never layout): HCEA `actor_combat_update.c` / `actor_start_burst.c`, Stian 2276 `FUN_00022390`,
  canonical `ai_debug.c` firing-decision renderer (`_firing_*` enum, `actor_debug_info` burst fields).

## Validated baseline (unchanged at finish)

`python -B tools/campaign/gate.py source/ai/actor_combat --all --forbid-emitted-symbol _point_from_line3d`
=> `== exact 20  residual 5  unwritten 9  (of 34 listed)`; point guard passed; park drift PASS (4/4).

| Row | January padded / relocs / sha16 | State |
| --- | --- | --- |
| `_actor_combat_update` | 4672 / 146 / `e37ad38594fffad1` | UNWRITTEN |
| `_code_000109a0` (`actor_start_burst`) | 1968 / 67 / `cf16e6e39069e23e` | UNWRITTEN |
| `_code_0000fc50` (`actor_start_first_burst_delay`) | 208 / 8 / `5de9a157692b5702` | UNWRITTEN |
| `_code_0000fba0` (`actor_start_pause`) | 176 / 11 / `eeb7cce55b1efa7c` | UNWRITTEN |
| `_code_00010620` (`actor_combat_allow_special_fire_situation`) | 176 / 9 / `1bddf744667a501a` | UNWRITTEN |
| `_code_0000f880` (`actor_combat_get_firing_parameters`) | 160 / 6 / `7866fc29f8cfd7c3` | UNWRITTEN |
| `_code_0000f590` (`actor_combat_enable_special_fire_situation`) | 144 / 2 / `5a477b5450f82b33` | UNWRITTEN |
| `_fast_ftol`, `_random_boolean` | 32 / 0, 32 / 2 | UNWRITTEN (natural COMDATs of the graph) |
| `_actor_aim_grenade` | 544 (ours 576) / 22 | residual, semantic-withdrawn park |
| four parked residuals | unchanged | PARKED-LOCKED |

## Why nothing landed

All six private statics are reachable only from `actor_combat_update` (VC7 static COMDAT law), and the parent cannot be written
admissibly in this lane:

1. `source/items/weapons.h` declares neither `weapon_aim` (called at `+0x666`, `+0xF79`) nor `weapon_estimate_time_to_target`
   (`+0xD3A`, defined `weapons.c:1132`).
2. January's parent calls real-math helpers out of line (`_real_random_range`, `_distance_squared3d`, `_distance3d`,
   `_vector_from_points3d`, `_point_from_line3d` x2, `_magnitude_squared3d` x3) while the same TU inlines `real_random_range`,
   `magnitude_squared3d`, `vector_from_points3d` and `distance_squared3d` elsewhere (the known per-caller inline boundary). No
   `REAL_MATH_EXTERNAL_*` variant exists for three of them, `real_math.h` is outside this lane's proposal authority, and a post-import
   redeclaration in `actor_combat.c` is a consumer-local prototype (inadmissible under the lane brief, although the TU already carries
   that pattern for `normalize2d`/`normalize3d`).
3. `struct actor_debug_info` (`ai_debug.h`) still has `char __unknown1C[40]` over `+0x34..+0x5B`; the parent writes a long, two
   booleans, two vectors and two reals there.
4. `actor_definition.flags2` bits 1 and 2 (must-stand / must-stop to fire) have no owner enumerators; the special-fire mode/situation
   domains and the combat-status ladder have no shared owner (TU-local complete copies would be tolerated with disclosure).

## Diagnostic reconstruction (scratch only; zero credit)

Complete graph TU: `scratch/workers/actor_combat/u3.c` (SHA-256 `ba465d38db705d61a1e9c2aef5ce257916fcaa9a65417bb0a538e8617e63a7e7`),
object `u3.obj` (`58fe5f4eaea90861591220bba2c9ae0630b3f60e800bc8e05a8f4ac0fa5ca6f7`), built with authentic names, the scratch-only
declarations of blocker 1-2, a scratch-only debug view struct for blocker 3 and scratch-only flags2 enumerators for blocker 4.
Gated with `--alias <authentic>=code_<addr>` for the six privates.

| Function | Result in whole-graph TU |
| --- | --- |
| `actor_combat_enable_special_fire_situation` | EXACT 144/2 |
| `actor_combat_get_firing_parameters` | EXACT 160/6 |
| `actor_start_pause` | EXACT 176/11 |
| `actor_start_first_burst_delay` | EXACT 208/8 |
| `actor_combat_allow_special_fire_situation` | EXACT 176/9 |
| `_fast_ftol`, `_random_boolean` | EXACT (natural COMDATs; no wrapper, no rename) |
| `_sine`, `_cosine`, `_tangent` | stay EXACT with their import aliases and wrappers removed (natural inline use) |
| `actor_start_burst` | 1968/67, sha-only; 53 of 618 instructions differ |
| `actor_combat_update` | 4672/146, sha-only; **2 of 1334 instructions differ** |

Whole TU: 27 exact / 7 residual / 0 unwritten; every baseline-EXACT row stays EXACT; 4/4 parks unchanged; point guard passes; new data
owners are exclusively January-attested literal COMDATs; fake-match scan 0 leads.

## Accepted source shapes (evidence)

- Helpers: `.inc` handoff forms were right first time except `actor_start_first_burst_delay`, which needs the orphan flag latched
  before `fire_immediately = TRUE` (otherwise `bl` is copy-propagated into the flag store). The pause helper's nested
  `get_firing_parameters(actor, actor_combat_get_firing_variant_definition(actor), ...)` is proven by the argument pushes preceding
  the inner call. `allow_special_fire_situation` resolves the actor before the strafing test (single-exit result flag).
- `actor_start_burst`: `new_target_time = mem; new_target_time *= difficulty()` and the same two-step for the projectile error and
  return radius (January spills the tag value before the call); `firing_while_moving` stored per branch with the `== NONE` branch
  first; `cross_product3d(target - head, {0,0,1})` (literal 0.0 multiplies retained, so a local constant vector, not `global_up3d`);
  `tangent(MIN(ticks*velocity*(1.0f/TICKS_PER_SECOND), _pi/4.0f))`; `fast_ftol(ticks*(origin/limit))`; console literals from `.rdata`.
- `actor_combat_update`: assert anchors 133..785 with exact literals; decision switch case order wild, holding, bursting, pausing and
  transition switch order none, bursting, holding|pausing, wild (jump-table layout); `target.target_prop_index !=
  control.current_fire_target_prop_index`; `x = fraction; x += difficulty()` for tracking/leading and `MIN(1.0f, x) > 0.0f`;
  inline `vector_from_points3d` for the estimate facing and the line-of-fire vector; `weapon_aim` out-parameter `linear` block-scoped
  at each use site (fixes the `[ebp-2]/[ebp-3]` byte-slot order); `short trigger_delay = (short)fast_ftol(30/rate)` then
  `MAX(2, trigger_delay)`. The two dead difficulty reads and the discarded `TAG_BLOCK_GET_ELEMENT(&triggers, 0)` are January
  instructions (`fstp st(0)`, unused `eax`), not inventions.

## Experiment matrix (do not repeat)

- start_burst ties (no effect or worse): operand-order flips of the rotation products; cos/sin as function-scope locals, declared
  before `aim_vector`, or via a named `up` vector; CSE'd trig calls (worse); declaration order of `burst_geometry`/`firing_pattern`;
  weapon-block locals at the `else if` level (none) or function scope (+4 frame, worse); `return_radius *= limit/origin`; named
  `maximum_burst_radius` local.
- update last 2 instructions (burst_target.i add operand order; January `rel.i + origin.x` but `origin + rel` for j,k):
  `add_vectors3d(&rel, &origin)` (4), manual `+=` first add (2), swapped first add (10), hand-written asymmetric components (4688 B).

## Residual classification

- `actor_start_burst`: frame-slot coloring permutation across `[ebp-0x10/-0x14/-0x18]`, x87 last-use folding of the origin
  cos/sin register values (same family as the parked `actor_combat_find_nearby_target`), and the capped-branch limit store. Backend ties.
- `actor_combat_update`: one commutative x87 load order. Backend tie.
- Both are measured inside the diagnostic TU; the real owner mechanisms may move them.

## Owner census, fake scan, checks (real file)

Real file unchanged: final gate identical to baseline; code owners 31/31 identical to `build/base`; data owners identical except the
`.debug$S` object-path artifact; `tools/fake_match_scan.py source/ai/actor_combat.c` 0 leads.

## weapons.h declaration simulation

All 19 direct `weapons.h` consumers (no header includes it) were gated with the two proposed prototypes injected after their
`weapons.h` include versus unmodified scratch copies: 0 changed rows in every TU, including `units.obj` 189/189.

## Reopen criteria / disposition

Disposition: active, owner-blocked, zero credit. Reopen when (1) `weapons.h` owns the two prototypes, (2) an owner-approved mechanism
exposes the six out-of-line real-math declarations to this TU, (3) `actor_debug_info +0x34..+0x5B` has authenticated names and
(4) the flags2 / special-fire / combat-status enumerators have owners. Then replay `scratch/workers/actor_combat/u3.c` (replace the
scratch-only declarations with the owners), apply the six `symbols.json` renames, and re-gate; expect 7 new exact rows immediately
(5 helpers + `_fast_ftol` + `_random_boolean`, 928 padded bytes) with `actor_start_burst` and `actor_combat_update` as near ties.
