# actor_perception.obj — opus5 150K house-clean lane, wave w3 (2026-09-14)

Worker scope: `source/ai/actor_perception.c` only. No header, config, build or other source file was edited.
Scratch evidence: `scratch/workers/w3_actor_perception/`; per-function notes: `scratch/workers/w3_actor_perception.md`.
Prior ledgers read: w1 (`actor_perception_obj_opus5_150k_w1_20260914.md`), the 2026-09-13 emotion/situation packet, the
Codex checkpoint, the two Jonas 2026-08-30 ledgers, the 2026-09-04 props/grenade ledgers, the 2026-09-08 and 2026-09-13
reconciliation checkpoints, the w1 verifier verdicts (`scratch/w1/w1-results.json`) and `scratch/w2/laws_w2.md`.
There was no w2 work on this object.

## Gate summary (real file, `--forbid-emitted-symbol _point_from_line3d`)

| state | exact | residual | unwritten | guard | parks |
| --- | ---: | ---: | ---: | --- | --- |
| baseline (HEAD) | 33 | 9 | 2 | pass | 3 OK |
| final real file | 33 | 10 | 1 (`_code_00020990`) | pass | 3 OK, 0 drift |

All 33 baseline EXACT rows are unchanged (per-function size/relocation/sha diff: only `_prop_status_refresh` (new),
`_actor_situation_update` (rewritten), `_prop_acknowledged` (candidate-only COMDAT removed) and `_subtract_vectors3d`
(header `__inline` COMDAT, systemic real_math class) differ). `tools/fake_match_scan.py`: 0 leads (HEAD 0).
`CL /Zs /W3`: the HEAD C4013 for `prop_status_refresh` is gone (the definition now precedes both callers).
No new strict-exact credit is claimed for the real file.

## Per-function outcomes

| function | January | result | status |
| --- | --- | --- | --- |
| `_actor_situation_combat_status_update` (parked) | 320/7 | scratch EXACT `b68099052dd13bf2` | park reopen proposed |
| `_prop_status_refresh` | 3680/91 | 3680/91, 157 aligned diff lines | fuzzy landed (was UNWRITTEN) |
| `_actor_situation_update` | 1264/28 | 1264/28 (was 1232/28), one misplaced 6-insn block | fuzzy landed (structural correction) |
| `_actor_perception_aiming_vector_test_blockage` (parked) | 400/10 | scratch 400/10 (park 384/10) | not landed; 5 shapes |
| `_actor_perception_friend_prop_is_attacking` (parked) | 432/16 | AL/CL register tie | not landed; 3 shapes |
| `_code_00020990` | 1472/50 | unchanged | owner-blocked (COMDAT ruling OUT for the lane) |
| `_actor_emotion_update` | 1664/38 | unchanged | policy-blocked (specific_threats[9] read) |
| `_actor_perception_update` | 4720/170 | unchanged | 1 shape inert |
| `_actor_perception_refresh` | 2416/86 | unchanged | w1-exhausted binding schedule; distance local genuine |
| `_actor_emotion_unopposable_retreat`, `_actor_perception_refresh_test_object` | — | unchanged | not attempted (sha rows, no specific law) |

## Park reopen: `_actor_situation_combat_status_update`

Candidate: `scratch/workers/w3_actor_perception/cs4.c` (current real file plus only this body). It gates exact 34 /
residual 9 / unwritten 1 with the guard passing; the only sha change is this function (to January's hash); the other
two parks are OK; census unchanged; fakescan 0. The body alone on HEAD is also EXACT (`cs_head.c`, exact 34).

Evidence:

1. **Width of the combat-status phi (A18 value facts).** January's outer `MAX` else arm re-evaluates the inner `MAX`
   with `movsx ecx,cx` / `movsx ecx,dx`, an int-width phi. The parked body stored the result in a `short combat_status`
   local, which narrows the phi (no movsx). Writing the field directly,
   `actor->state.combat_status = MAX(suspicion, MAX(artificial, global_combat_status_table[target_type]))`, and reading
   the field afterwards restores the int-width phi.
2. **Control flow.** The status==none path falls into the shared `certain_combat_timer = 0` block, and the >= certain
   path stores `certain_combat_timer++` and then `uncertain_combat_timer = 0` (tail-merged with the `uncertain++` store).
   That is two ordinary if/else statements, the same topology as HCEA. The parked `goto combat_status_timers_updated`
   was a decompiler artifact.

The park's criterion ("authentic source or compiler-context evidence") is met, and its "pure scheduling" premise is
refuted: the residual came from a local's width and an artificial goto.

## Fuzzy landing: `_prop_status_refresh` (previously UNWRITTEN)

- This is the w1 body with the w1 verifier's four required spelling fixes (named LOS/target enums, `DATUM_INDEX_TO_ABSOLUTE_INDEX`).
- **`prop_acknowledged` is a macro.** January inlines the acknowledged-band test at the enemy `use_maximum_distance`
  site, although that block is laid out cold. A `static __inline` stays out of line there under every w1 spelling.
  January's object also owns no `_prop_acknowledged` section. The TU now uses the same macro as `actor_stimulus.c`,
  with a disclosure comment. Because of the two-level `assert` macro, `actor_perception_acknowledge`'s assert is
  spelled `vassert(prop_acknowledged(prop), "prop_acknowledged(prop)")`; that function stays EXACT.
- The acknowledged path shares the driver/mounted `use = FALSE` block (tail merge), which gives
  `else if (prop_acknowledged(prop)) use_maximum_distance = FALSE;`.
- The non-enemy path stores FALSE before the mode test and TRUE in the dead/in-combat path.
- Result: 3680/91 (the w1 body was 3680/92) with 157 aligned diff lines (w1: 218). What remains is argument-load
  order at the visibility/audibility calls, plus register ties.
- Rejected shapes: an if/else spelling (184); enemy-first ordering (200); inline lighting ternaries (261).

## Fuzzy landing: `_actor_situation_update` (structural correction)

- **The baseline had two logic defects.**
  - It incremented `cumulative_threats[N]` only when `priority <= N`. January increments unconditionally, then raises
    the priority with `MAX`.
  - It read `dangerous_vehicle_driver` (+0x136) for fire support. January reads +0x135, `vehicle_gunner` (A19).
- **What the rewrite adds.** A typed body with no padded flag struct and no goto:
  - a separate `if (visible)` block, so the zero priority register is used for the `unreachable_ticks` compare;
  - the fighting re-test for fire support;
  - area/visible/close stores placed after the actor-type computation (declare+init moves them: 1280 bytes);
  - the actor's target prop fetched before the friend's (locals in the else-if arm);
  - `prop->target_weight > best_target_weight`, with the index stored first.
- **Result.** 1264/28. Every instruction, stack slot and relocation agrees except one 6-instruction block, the
  `shooting && line_of_sight == clear` arm, which January lays out after `specific_threats[priority]++`.
- **Inert or rejected shapes.** A threatening flag, a goto form and `||` operand order made no difference.

## Not landed

- **`_actor_perception_aiming_vector_test_blockage` (park).** Structured source reaches 400/10 in scratch
  (`avb_body3b.c`: positive conditions, `normalize2d(&h) > 0.0f`, the result used as the blockage level). Two things
  remain:
  - January's constant-0 result store on the out-of-range path. It needs `else result = 0`, which is redundant with the
    initializer (R12), so it is not admissible.
  - January keeps the 1/magnitude scale temporary alive until after the j store. This is the same in all 20
    normalize2d sites tree-wide.
  - Five shapes spent. Reopen with a non-redundant spelling of the zero store.
- **`_actor_perception_friend_prop_is_attacking` (park).** January loads `known_enemies` into CL, keeping AL=attacking
  live. Single-exit (fpa1, fpa2) and nested early exits (fpa3) are worse or inert. The in-loop `return TRUE` is proven by
  the `mov al,1` epilogue.
- **`_actor_emotion_update`.** Difference 1 is January scanning `specific_threats[9]`, one past the array. The existing
  ledger deliberately corrected it, and preserving it would be an out-of-bounds read (policy), so no shapes were spent.
- **`_code_00020990`.** Still needs header proposal A (w1) plus the `_point_from_line3d` COMDAT ruling, which is OUT for
  this lane.
- **`_actor_perception_update`.** Inverting the danger-timer condition polarity is byte-identical after normalization.
  The w1 reopen criteria stand.
- **`_actor_perception_refresh`.** The `distance_squared` local is used again for the list priority (genuine), and the
  w1 binding-schedule verdict stands.

## Laws measured this wave (for the orchestrator's law table)

- **MAX-PHI WIDTH.** A nested `MAX` assigned to a `short` local narrows the ?: phi, so the else arm is not sign-extended.
  Assigning it to the `short` field (then reading the field) keeps VC7's int-width phi (`movsx` in both arms).
  Closed `_actor_situation_combat_status_update`.
- **MACRO vs STATIC INLINE.** An acknowledged-band predicate inlined in a cold block where a `static __inline` stays out
  of line, in an object with no helper section, is a macro. The two-level `assert` then needs the explicit-string `vassert`.
- **STATEMENT-POSITION ZERO STORES.** `boolean a = FALSE` declare+init versus assignment after the preceding calls is
  not inert (1264 vs 1280). January's store position after the calls is evidence for the assignment form.

## Reopen criteria

- `_prop_status_refresh`: a natural lever for the argument-load order at the visibility calls (w1 measured proposal A,
  short knowledge, at -14 diffs).
- `_actor_situation_update`: a spelling that lays out the non-visible shooting arm after the specific-threat increment.
- `_actor_perception_aiming_vector_test_blockage`: see above.
- `_code_00020990`: header proposal A plus a lane/owner ruling on the COMDAT.
