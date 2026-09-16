# actor_perception.obj — opus5 next-150K lane, wave n2 (2026-09-15)

Worker scope: `source/ai/actor_perception.c` only. No header, config, build, test or other source file
was edited. Scratch evidence: `scratch/workers/n2_actor_perception/`; per-function notes:
`scratch/workers/n2_actor_perception.md`.

Prior ledgers read in full: `actor_perception_obj_opus5_150k_w1_20260914.md`,
`actor_perception_obj_opus5_150k_w3_20260914.md`, `actor_perception_obj_props_api_typed_followup_20260904.md`,
`actor_perception_obj_codex_checkpoint.md`, the two Jonas 2026-08-30 ledgers,
`actor_perception_emotion_situation_packet_20260913.md`,
`props_actor_perception_grenade_canonical_integration_20260904.md`,
`opus5_150k_house_clean_HANDOFF_20260914.md` and
`opus5_250k_house_clean_checkpoint_20260915.md`, plus `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md`
(including both do-not-repeat tables).

## Gate summary (real file, `--forbid-emitted-symbol _point_from_line3d`)

| state | exact | residual | unwritten | guard | parks |
| --- | ---: | ---: | ---: | --- | --- |
| baseline (HEAD `3083f800b`) | 34 | 9 | 1 | pass | 2 OK |
| final real file | **35** | 8 | 1 (`_code_00020990`) | pass | 2 OK, 0 drift |

Row-by-row diff of the two `--all` listings: exactly ONE row changed,
`_actor_perception_refresh_test_object` residual 1744 `[sha]` to **EXACT 1744**. No regression.
`tools/fake_match_scan.py`: 0 leads (HEAD 0). Owner census: the candidate's owner set is IDENTICAL to
`build/base/source/ai/actor_perception.obj` (58/58 code owners, none added or removed); versus
`build/split` it carries only the pre-existing systemic select-any real_math COMDATs plus the
`_actor_perception_refresh_danger_zone` alias, all inherited at HEAD (laws_w3 A30 class).
`scratch/parkcheck.py`: `parks 2 drift 0`.

Duplicate prevention: `git log --all -- source/ai/actor_perception.c` (40 commits) and
`tools/campaign/branch_sweep.py source/ai/actor_perception` (99 unique blobs) both confirm HEAD is the
best state (`exact 34`; best historical blob `e6e80d52` at `exact 33`). No donor exists.

## Per-function outcomes

| function | January | result | status |
| --- | --- | --- | --- |
| `_actor_perception_refresh_test_object` | 1744 / 46 | **1744 / 46 strict EXACT** | EXACT-LANDED (+1,744 padded) |
| `_actor_emotion_unopposable_retreat` | 1264 / 28 | 1264 / 28, 58 to 50 diff blocks in scratch | not landed (frame-slot permutation) |
| `_prop_status_refresh` | 3680 / 91 | unchanged | not landed (allocation/schedule tie; 0 shapes) |

## `_actor_perception_refresh_test_object` — what was wrong and why the fix is the original source

The HEAD body contained a **hand-expanded, constant-folded copy of `actor_perception_desire_prop`**
written as a goto soup (`choose_list` / `corpse_fallback` / `live_unit` / `create_prop`), two carrier
locals (`current_actor`, `unit_actor`) and a `pointers` struct hidden behind
`#define actor pointers.actor`. That is the R15 / house-rule-13 defect class.

January proves the helper is *called*, not copied: the function's 46 relocations contain no
`_actor_perception_desire_prop`, but they do contain the helper's own interior calls in helper order —
`_actor_data`/`_datum_get` twice at +0x1d6 and +0x1f0 (`actor_get(actor_index)` then
`actor_get(prop_actor_index)`), `_encounter_data`/`_datum_get` at +0x256, `_object_get_and_verify_type`
at +0x26a and `_actor_action_class` at +0x381 — i.e. VC7 inlined the real helper. Four sibling call
sites in this same TU already call it and are inlined with no relocation, and the helper's own
out-of-line body is EXACT (560/16). HCEA
(`research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_perception_refresh_test_object.c`)
independently shows the 13-argument call with `0` for the requested state.

Six evidence-led shapes, each measured (`scratch/workers/n2_actor_perception/r1..r7.c`):

| # | change | result |
| --- | --- | ---: |
| r1 | replace the hand expansion with `actor_perception_desire_prop(actor_index, _prop_state_unacknowledged, unit_index, unit_actor_index, FALSE, player, enemy, dead, dead_ticks, suicide_radius, distance_squared, 0, &optional)` and `if (optional) {queue} else {prop_new_unacknowledged}`; delete `current_actor`/`unit_actor` | 1760/46, 83 to 68 blocks |
| r2 | positive `dead` test (inert) plus **`distance_squared3d(&position.body_position, &origin)`** | 70 blocks, whole x87 block fixed |
| r3a | **`long unit_index = object_index;`** at the declaration; the else arm keeps only `unit_actor_index = unit->unit.actor_index;` | 70 to **27** blocks |
| r4 | `distance3d(&position.body_position, &origin)` in the projectile branch plus `owner_unit_index = NONE;` moved after the danger-zone stores | **size 1744**, 538 == 538 insns, 12 blocks (3 real) |
| r5 | `if (unit->unit.swarm_actor_index != NONE) { unit_actor_index = unit->unit.swarm_actor_index; ... }` | last register web closed |
| r6 | `dead = TEST_FLAG(...) && feign_death_timer == 0;` as its own statement, then `if (!dead) dead_ticks = 0; else if (time_of_death == NONE) dead_ticks = 0x7FFF; else dead_ticks = (short)game_time_get() - (short)unit->unit.time_of_death;` | **STRICT EXACT** |

Byte proofs for each: (r3a) January stores `[ebp-0xc]` at +0x61 before `object_get_origin` and RELOADS
it at +0xc9, which also frees EDI for `actor_index` across the swarm call; (r4) January emits
`or esi,0xffffffff` at +0x61c after the whole danger-zone store block, where the declaration-initializer
form spills `NONE` into the dead parameter home (`mov [ebp+0xc],-1` at +0x5ab); (r5) January loads the
swarm field into EAX for the compare and CSEs the assignment into `mov ebx,eax`; (r6) the last remaining
relocation-address difference was `_game_time_get` 0x14e versus 0x141, i.e. two swapped tail blocks.
Control `r7.c` (the same three arms as an if/else-if that duplicates `dead = TRUE`) is still `[sha]`, so
the closing lever is the SPLIT, not statement duplication (this is not R2).

Meaning: walk the object sibling/child tree once per perception pass; for a biped resolve the swarm root
into a unit, assess suicide danger, then ask `actor_perception_desire_prop` whether the unit is worth a
prop — a "too far" unit is queued as an optional candidate, otherwise a new unacknowledged prop is
allocated and position-refreshed.

## Laws measured this wave (for the orchestrator's law table)

**n2-1. TWO-CONSTANT ARM KEEPS THE INLINE RANGE.** A helper arm written `x = (cond) + K;` or
`x = cond ? K+1 : K;` lowers to the identical `xor r,r; cmp; setcc rb; add r,K` bytes, but only the
two-constant `if/else` form keeps the constant-arm value range alive across inline substitution.
Detect: January has NO `test r,r; jcc` at the join after an inlined helper whose other arms fold, and
January's last else-if arm falls through into the join while ours is exiled past the join with a
back-jump. Measured on `_actor_emotion_assess_unopposable_danger` inlined into
`_actor_emotion_unopposable_retreat` (the standalone helper stays EXACT in all three spellings); it
removed `test si,si; jle` and re-laid `[0xc4, 0x199)` onto January. Closed nothing on its own.

**n2-2. SPLIT BOOLEAN versus FUSED IF/ELSE FIXES THE JOIN FALL-THROUGH.** When a boolean and a value
derived from it are computed by one fused `if (C) { b = TRUE; inner if/else } else { b = FALSE; v = 0; }`,
VC7 orders the tails [inner-then][inner-else][outer-else to join]. Writing `b = C;` as its own statement
and the value as a flat `if (!b) ... else if ... else ...` chain emits the SAME threaded code (the
`b = TRUE` store still lands between the inner compare and its jcc) but orders the tails
[inner-then][outer-else][inner-else to join], which is January's. Detect: equal instruction counts,
equal blocks, and ONE relocation whose ADDRESS differs because two tail blocks are swapped.
**Closed `_actor_perception_refresh_test_object` (1,744 B).**

**n2-3. A15 ROLE ORDER FOR distance3d / distance_squared3d.** Both inline
`vector_from_points3d(a,b) = b - a`, so January's `fld [X]; fsub [Y]` proves the source wrote
`distance*3d(&Y, &X)`. Two sites in this function were argument-reversed; fixing them fixed the whole
x87 subtraction block and the later `magnitude*` accumulation order.

**n2-4 (diagnostic). A `struct {...} pointers;` plus `#define name pointers.name` carrier, together with
a `goto`-labelled arm chain inside a large function, is a reliable fingerprint of a hand-expanded
callee.** Confirm it by looking for the callee's own interior relocations, in callee order, in January's
relocation list with no relocation to the callee itself.

## Not landed

- **`_actor_emotion_unopposable_retreat` (1264/28).** Best scratch body
  `scratch/workers/n2_actor_perception/v6.c` (50 diff blocks, down from 58): helper arm as a
  two-constant if/else (n2-1), `friend_actor->emotion_target_time >= actor->last_emotion_target_time`
  and `threshold > definition->player_threshold` (both proven by January's `cmp` operand direction and
  corroborated by HCEA), and the three decompiler `goto`s replaced by `if / else if` (byte-inert).
  Named `real upper`/`real lower` locals at the two `real_seed_random_range` sites (HCEA has exactly
  those) are WORSE — 1296 `[size]` — because VC7 gives them fresh frame cells while January's two dead
  stores are packed into the dead `target_prop_index` / `iterator` cells. Residual: January's scalar
  frame order is exactly the REVERSE DECLARATION ORDER of the current source and ours is a permutation
  of it (full slot table in the notes); no source-legitimate lever for that permutation was found.
  Zero strict credit; brief section 8 forbids landing a fuzzy body over an already-written coherent one.
- **`_prop_status_refresh` (3680/91).** Full frame and relocation census in the notes: frame identical,
  91/91 relocations with identical targets, relocations 0 to 62 at identical addresses, instructions
  0 to 528 byte-identical. Every later difference is a store/load interleave or the argument
  materialisation schedule at the visibility/audibility calls (January re-reads the `position` parameter
  at each push, ours loads it at the head of the argument group), with the EAX/ECX/EDX roles rotated as
  a consequence. No structural defect exists: no `goto`, no carrier, no hand-expanded callee, no missing
  or extra call. HCEA's argument-local order for the first visibility call is REFUTED by January
  (`lighting` is stored at +0x780 before the knowledge call, which is our current spelling). 0 shapes
  spent; classified an allocation/schedule tie.

## Reopen criteria

- `_actor_emotion_unopposable_retreat`: a source lever that permutes VC7's scalar frame-slot order in a
  function whose declaration order already equals January's reverse-slot order. Start from `v6.c`.
- `_prop_status_refresh`: a measured VC7 law that moves a parameter RE-READ to the push site rather than
  the head of the argument-evaluation group. The w1 header proposal A (`short`
  `actor_get_perception_knowledge`, consumers action_charge.c, action_fight.c, action_flee.c,
  actor_firing_position.c, ai_communication.c) was measured at only -14 diffs and does not close it.
- `_code_00020990`: unchanged — header proposal A plus the `_point_from_line3d` COMDAT ruling (OUT).
