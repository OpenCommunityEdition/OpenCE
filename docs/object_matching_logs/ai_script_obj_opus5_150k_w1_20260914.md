# `ai_script.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/ai_script.c`; January target `build/split/source/ai/ai_script.obj`.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Baseline real-file gate (guard passes): **109 exact / 7 residual / 0 unwritten**.
- The residual rows are:
  - `_ai_index_from_string`: size 320 != 304.
  - `_ai_scripting_allegiance_broken`: size 64 != 80. **Parked.**
  - `_ai_scripting_follow_target_ai`: sha.
  - `_ai_scripting_migrate_and_speak`: sha.
  - `_ai_scripting_renew`: sha.
  - `_ai_scripting_set_current_state`: sha.
  - `_ai_scripting_teleport_starting_location_private`: sha. **Parked.**
- Prior records honoured:
  - `ai_script_obj_opus5_fresh_graphs_20260914.md` and its worker notes (`opus5-30k-fresh-graphs-20260914/scratch/workers/ai_script.md`);
  - `ai_script_obj_opus_coherent_reconciliation_20260909.md`;
  - `ai_flee_teleport_model_node_packet_20260912.md`;
  - `small_unwritten_threshold_wave_main_hs_ai_20260913.md`;
  - `config/parked.json` criteria.
- The eight functions inherited from the 100K lane were not touched.
- `branch_sweep` found no better blob.

## Result

- **The real file is unchanged**, and the final gate is still 109 / 7 / 0. `git diff --stat` is empty.
- **One park-reopen proposal** is strict EXACT in scratch.

### Park reopen: `_ai_scripting_allegiance_broken` (80 padded, 2 relocs, sha16 `08396f5b188d67da`)

- **Candidate:** `scratch/workers/ai_script_props_smoothing/park_reopen_ai_script_allegiance_broken.c`. It is the current real file with only this body replaced:

  ```c
  boolean broken = FALSE;

  if (team1_index != NONE && team2_index != NONE)
  {
  	broken =
  		game_team_is_ally(team1_index, team2_index) &&
  		game_team_is_enemy(team1_index, team2_index);
  }

  return broken;
  ```

- **Evidence (single-exit law).** January materialises FALSE in AL before the NONE guard. The guard-fail path jumps straight to the epilogue, and the call-fail join repeats `xor al,al`. That is how VC7 lowers a boolean result assigned from the `&&` predicate inside the guard: the value is never live across the calls.
- **Other forms measured:**
  - an if-statement `result = TRUE` form pins FALSE in BL;
  - an if/else FALSE arm collapses back to 64 bytes.
- **Park criterion refuted.** The park held that ordinary forms converge on 64 bytes and only artificial live ranges could reach January. An ordinary single-exit boolean reaches it, with no duplicate condition, forced register or dead store. The FALSE initialiser is live on the invalid-team path.
- **Semantics.** `ally_bitvector` and `friendly_bitvector` differ, so "allied but hostile" is a genuine predicate (HCEA agrees).
- **Checks:**
  - whole-TU 109 -> 110 exact, and only this row changes;
  - point guard passes;
  - the teleport park is unchanged, and the allegiance "drift" is the reopened body, now equal to the target;
  - owner census: no new owner;
  - fake scan: 0 leads.

## Residuals (not landed)

| function | classification | disposition / proposal |
| --- | --- | --- |
| `_ai_scripting_set_current_state` | header ownership (C4013, re-verified by `CL /Zs /W3`: the only C4013 in the TU) | ORCHESTRATOR-PROPOSAL, below |
| `_ai_scripting_teleport_starting_location_private` (park) | owner ABI | ORCHESTRATOR-PROPOSAL, below |
| `_ai_scripting_follow_target_ai` | January diagnostic defect: `ai_reference` is formatted twice (`push esi` at +0x43) | owner ruling only; not UB; previously rejected as misleading |
| `_ai_scripting_migrate_and_speak` | January omits a vararg for a 4-conversion format | undefined behaviour; forbidden by policy |
| `_ai_scripting_renew` | argument-load scheduling | exhausted (8 prior shapes); no new evidence |
| `_ai_index_from_string` | zero-register pinning (NONE kept in ESI) | exhausted (8 prior shapes); no new lever |

### Proposal 1: `actor_action_set_default_state` prototype

- **Change.** In `source/ai/actions.h`, section "prototypes/ACTIONS.C", after `actor_action_get_default_state`, add:

  ```c
  boolean actor_action_set_default_state(
  	long actor_index,
  	short override_state);
  ```

  Then remove the duplicate `.c`-local prototype at `source/ai/actions.c:416-418`.
- **Consumers.** 32 direct includers, all in `source/ai`, plus every `actors.h` consumer transitively. The list is in the worker notes.
- **Expected result.** `_ai_scripting_set_current_state` closes (160 B) with no body change. The previous lane measured this diagnostically.

### Proposal 2: `biped_approximate_surface_index` return type

- **Change.** Make `biped_approximate_surface_index` return `long` in `source/units/bipeds.h` and `source/units/bipeds.c`, and drop the `(short)` cast.
- **Evidence.**
  - January's caller compares the full EAX with `cmp eax,-1`, with no sign extension.
  - The exact callee returns `biped_find_ground_surface`'s long unchanged, so its bytes are identical for either return type.
  - HCEA declares an `int` return.
  - Declaration count is unchanged.
- **Consumers.** 23 includers of `bipeds.h`, including the protected `units.c`. The only C caller is `ai_script.c`.
- **Caveat.** The earlier packet reported consumer perturbation from this widening. It must be re-measured by a full build.

## Reopen criteria

- **set_current_state and teleport:** the corresponding header packet lands.
- **follow_target_ai:** an owner ruling on January diagnostic-argument defects.
- **renew and from_string:** a new scheduling or constant-pinning lever.
