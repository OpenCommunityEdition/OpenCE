# `action_charge.obj`: perform reconstruction and setup frame residual (Opus 5 fresh-graphs lane, 2026-09-14)

## Result

- `_action_charge_perform` (3,248 padded, previously UNWRITTEN) is now a complete typed-C body, landed as
  **fuzzy** (exact credit 0): 3,216 / 84 relocations vs 3,248 / 84, identical relocation destination multiset.
- `_action_charge_setup` (944-byte residual) was reconstructed to **944 / 29 with identical instruction opcodes**;
  only frame-slot displacements differ. It is **not landed** (not strict exact).
- All 20 baseline-exact functions stay strict exact with unchanged normalized hashes.
- Object: `== exact 20  residual 2  unwritten 0  (of 22 listed)` (baseline `exact 20 residual 1 unwritten 1`).
  No newly exact function.

## 1. Scope and provenance

- Unit `source/ai/action_charge.c`, target `build/split/source/ai/action_charge.obj`, lane base `47d9dd0da`.
- XDK 3911 `CL 13.00.9254` with `/O2 /Oy- /DDEBUG /Dxbox` (unchanged; `/O2` auto-inlines, see section 6).
- Evidence used: January COFF disassembly (Capstone, relocation-annotated), Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/action_charge.obj.decomp.c` (perform 383-832, setup 891-1039), January map atlas
  `scratch/atlas/action_charge.obj.txt`, HCEA `src/blam/ai/action_charge_perform.c` / `action_charge_setup.c`
  (later build, semantics only), `source/ai/ai_debug.c` charge-decision and melee-check renderers (field meanings),
  `source/ai/actor_firing_position.c` weapon minimum-range idiom, `source/ai/action_fight.c`
  `actor_perception_unreachable` prototype.
- Prior work read in full: the seven existing action_charge ledgers and the archived frontier ledger
  `677defb9d:docs/object_matching_logs/action_charge_obj.md` (its perform/setup bodies were never committed;
  branch_sweep over 11 blobs found no body in any lane).

## 2. Validated baseline

| Function | Target padded | Relocs | Baseline | Now |
| --- | ---: | ---: | --- | --- |
| `_action_charge_perform` | 3,248 | 84 | UNWRITTEN | fuzzy 3,216 / 84, sha16 `2e9beea61ca0c704` (target `9ed9d44b1e9fd722`); first divergence +0x256 |
| `_action_charge_setup` | 944 | 29 | 688 / 20 | unchanged in production; best candidate 944 / 29 sha16 `31040fe5ab789ce3` (target `34c0c115890ecab8`); first divergence +0x3 (`sub esp,0x24` vs `0x20`) |
| 20 others | 1,984 | | EXACT | EXACT, hashes unchanged |

Data: production now also owns the target literals `__real@3f5db3d7`, `__real@3cbf258c`, `__real@3e99999a`,
`__real@3d4ccccd`, `__real@3f4ccccd` (perform). `__real@41f00000` and the `!actor->meta.swarm` string are
emitted for references January resolves as undefined externals. The setup-only
`actor %s melee animation has no damage keyframe` string stays absent until setup lands.

## 3. Accepted controls (production)

`action_charge_perform`, placed between the helper package and the private destination helper:

- declarations in January call order: actor, actor definition (`actr`), variant definition (`actv`), firing variant,
  charge state, `prop = NULL`, `result = FALSE` (January keeps `[ebp-3]` and returns it on the default path);
- prop-valid branch first with the "no target" arm last (`je` to +0x43c); a separate delayed-projectile arm
  (January lays `advancing = TRUE` right after that test and cross-jumps the vehicle-goal arm into it);
- stalking exposure assigned to `state_data->stalking_currently_exposed` and re-read (January materialises it at
  32-bit width, which a boolean local does not reproduce);
- close-range bounds with the `actor_firing_position.c` `MAX(minimum_range, weapon->weapon.ai_minimum_target_range)`
  idiom (weapon +0x40c);
- melee abort: `MIN(abort_range, maximum_abort_range)` with a named `maximum_abort_range = MAX(0.f, melee_attack_range) + 0.8f`
  (January stores it to a slot and compares with `fcom`), asserts at lines 288 and 487;
- melee check debug telemetry into `actor_debug_array` (`last_melee_time`, `field_108/114/120/12C/138/139/13C/148/14C`)
  exactly as `ai_debug.c` renders it;
- `dot < 0` if/else distance form (two separate `direction = prop->actor_to_prop` copies in January),
  leap arm `distance < lower && unreachable_ticks == 0 && !flying` aborts, ballistic leap with January's argument
  mapping (13th = vertical, 14th = horizontal);
- communication immediates as enums: `_ai_communication_melee` (43), `_comm_hostility_enemy` (3);
- tail: `target_prop` re-fetch with `out_of_range = FALSE; if (...) out_of_range = TRUE;` (January `xor cl,cl` form),
  single `return result;`.

Supporting source-local additions: includes `ai.h`, `ai_communication.h`, `items/projectiles.h`,
`items/weapon_definitions.h`, `ai_debug.h`; enum constants `_action_charge_prop_facing_central = 2`,
`_action_charge_prop_closing_speed_slow = 1`; TU-local `actor_perception_unreachable` prototype (same as action_fight.c).
The `(real_vector2d *)` casts on 3D vectors follow `actors.c` and January's call operands (the 3D aim vector's address
is passed to `normalize2d`).

## 4. Experiment matrix

Scores are differing instructions after alignment (`scratch/workers/action_charge/score.py`, relocation offsets normalised).

### perform

| Shape | Size / relocs | Diff insns | Kept |
| --- | --- | ---: | --- |
| first complete body | 3280 / 87 | - | no |
| prop-valid first, delayed arm split, exposure via state | 3248 / 87 | - | yes |
| named maximum_abort_range | 3216 / 84 | 144 | yes |
| dot<0 if/else, leap-abort polarity, order fixes (package helpers) | 3216 / 84 | 202 | yes (authentic form; package merges copies) |
| same, helpers marked `__inline` locally (not landed) | 3264 / 84 | 142 | lever only |
| check_range/berserk declaration orders (3 variants) | = | 140-141 | inert |
| target_range/time scopes (3 variants), unreachable scope | = | 140-141 | inert |
| `field_114` component copy | = | 135 but loses January's pointer copy | no |
| `target_prop` + if-form out_of_range, declaration order | = | 127 (`__inline` mode) / 182 (production) | yes |
| abort ternary as if/else, compare operand swap | = | 127 | inert |
| dot_product3d argument swaps, danger/attack sum order | = | 142 | inert (VC7 canonicalises) |

### setup (best candidate `scratch/workers/action_charge/setup_v12.txt`, not landed)

| Shape | Size / relocs | Notes |
| --- | --- | --- |
| single exit + debug decisions + assert + csmemset + keyframe error | 1040 / 29 | 9 tail-duplicated epilogues |
| + `result = FALSE` in inhibited/notarget arms (January stores at +0xbc/+0xe4/+0xea/+0x107) | 1040 / 29 | removing them later re-creates the duplication |
| + `move_range = MAX(minimum_move_range, target_range)` | 928 / 29 | duplication gone |
| + `state_data->leap_possible_if_at_range = real_random() < chance; leap = ...` | **944 / 29** | 32-bit compare width as January; only frame displacements differ |
| frame diagnostics: 16 declaration-order / scope / name / init / `valid` / variant-local variants, `/Fa` listings | 944 / 29 | `leap` and `end_tick` never share January's 4-byte cell |

## 5. Do-not-repeat

- perform: declaration order and block scope of `check_range`, `berserk_ranges`, `abort_range`, `time`,
  `target_range`, `unreachable`; dot_product3d argument order; `danger + attack` operand order; moving perform before
  the helper definitions (VC7 compiles bottom-up; identical output).
- setup: declaration order, block scope, names, dead initialisers of `leap` / tick outputs; removing `valid`; variant
  pointer vs name local; `leap = cmp ? TRUE : FALSE` / `(long)` casts (keep 8-bit width).
- Pure header helpers (removing the rename package) emit candidate-only `_magnitude_squared3d` (LNK2005 history in
  `action_charge_obj_jonas_math_helper_recovery_rejection_20260826.md`); do not land that.

## 6. Residual classification

- perform (measured): relocation multiset identical; remaining differences are register choices (melee-abort block
  `check_range` in BL vs `prop` in EBX; `field_114` and `field_12C` copy registers; argument registers of
  `unit_melee_attack_begin` / `ai_communication_event`), byte-cell packing of `berserk_ranges` / `check_range`, and
  x87 operand order of three inlined dot products. **Inference, with a measured lever:** the inherited
  NODUPLICATES helper package lets VC7 use normalize3d/normalize2d clobber sets (EDX kept live across the call);
  January's helpers were SELECT_ANY inline COMDATs (atlas `i` records), so its caller could not. Marking the local
  helper definitions `__inline` keeps all 11 helpers strict exact with no new owner and cuts perform's residual from
  182 to 127 instructions. Not landed because it changes exact siblings' linkage.
- setup (measured): instruction stream identical; residual is frame layout only (January frame 0x20 with `leap` at
  -8 and `end_tick` at -6 in one cell; candidate 0x24). No source lever found.

## 7. Reopen criteria

- Orchestrator decision on helper linkage (local `__inline` package, or header-owned helpers with January's
  `magnitude3d` body that does not reference `magnitude_squared3d`), then continue perform from
  `scratch/workers/action_charge/pbase3.txt` on that base.
- For setup: evidence of the original tick-output locals/types that makes `leap` and `end_tick` non-interfering for
  VC7's frame packer; land `setup_v12.txt` together with the destination-helper signature
  `(long actor_index, short goal, real_point3d const *melee_target_point)` only when strict exact.

## 8. Disposition

Object remains NonMatching: 20 strict exact, `_action_charge_perform` fuzzy (credit 0), `_action_charge_setup`
residual. No config, header, park, flag or comparator change.

## Lead admission notes (2026-09-14)

- The worker's TU-local `actor_perception_unreachable` prototype was replaced
  by the owning declaration added to `source/ai/actor_perception.h` in the
  separate prerequisite commit `a694cf1b2` (which also removed action_fight.c's
  consumer-local copy). `action_charge.c` now includes that header.
- The leap stores were reordered to the natural `leap_pending = TRUE` first,
  as measured by the adversarial reviewer; the emitted body is byte-identical
  (3216/84, sha16 `2e9beea61ca0c704`).
- The proposal to mark the 11 local math helpers `__inline` was NOT adopted:
  it changes COMDAT selection of 11 already-exact sections without an exact
  gain and needs its own whole-board review. `_action_charge_perform` remains a
  zero-credit fuzzy body; `_action_charge_setup` stays at its baseline residual.
