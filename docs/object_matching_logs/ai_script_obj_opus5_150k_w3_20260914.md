# `ai_script.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/ai_script.c`; target `build/split/source/ai/ai_script.obj`.
- Worker notes: `scratch/workers/w3_path_smoothing_actor_firing_position_etc.md`.
- Baseline real-file gate (guard passes): `== exact 112 residual 4 unwritten 0`:
  `_ai_index_from_string` [size 320!=304], `_ai_scripting_follow_target_ai` [sha], `_ai_scripting_migrate_and_speak` [sha],
  `_ai_scripting_renew` [sha]. No parks.
- Ledgers read: `ai_script_obj_opus5_150k_w1_20260914.md`, `ai_script_obj_opus5_fresh_graphs_20260914.md`,
  `ai_script_obj_opus_coherent_reconciliation_20260909.md` and the older integration ledgers; HCEA
  `blam/ai/ai_index_from_string.c`, `blam/ai/ai_scripting_renew.c`.
- `branch_sweep.py`: best historical blob 111/5/0 (below the current file).

## Result

**Real file unchanged** (final 112/4/0, rows identical, guard passes, fake scan 0 leads). One owner-ruling proposal with a
strict-exact scratch candidate.

| function | w3 finding | disposition |
| --- | --- | --- |
| `_ai_index_from_string` | alndiff: January `push esi` at entry, `or esi,-1; mov [ebp-4],esi`, none-path `mov [ecx],esi`; ours shrink-wraps ESI and stores immediates; the -16 B also includes the no-separator store that January cross-jumps with the squad/platoon store (possible only because ESI is saved at entry). Laws A3c/A10/A18 give no admissible spelling. | exhausted (zero-register pin); 8 prior shapes not repeated |
| `_ai_scripting_renew` | only argument-load placement around the grenade calls | exhausted (scheduling) |
| `_ai_scripting_follow_target_ai` | January +0x43 `push esi` (= ai_reference) for the target_ai_name conversion: a copy/paste diagnostic defect | owner ruling (below) |
| `_ai_scripting_migrate_and_speak` | January omits a vararg for a 4-conversion format | UB (R12), not spent |

### ORCHESTRATOR-PROPOSAL (owner ruling, original-bug policy): `_ai_scripting_follow_target_ai` 176

Candidate `scratch/workers/w3_path_smoothing_actor_firing_position_etc/as_fta1.c` = current real file with the second
`ai_index_to_string` passing `ai_reference`, preceded by
`/* BUG (preserved for exact matching): ... A corrected build should pass target_ai_reference. */`.
Whole-TU gate 113/3/0, only this row changes, guard passes. Target proof: the pushed register is ESI loaded from `[ebp+8]`.
Not UB (valid reference, correct buffer and size); the effect is a debug print showing the source encounter twice. Not landed
because of the recorded hold (opus_coherent 20260909 rejection; fresh-graphs "withheld pending owner ruling").

## Reopen criteria

- follow_target_ai: owner admits the diagnostic defect (candidate ready).
- index_from_string: a demonstrated constant-register pinning lever. renew: a per-argument reload scheduling lever.
- migrate_and_speak: none under the UB policy.
