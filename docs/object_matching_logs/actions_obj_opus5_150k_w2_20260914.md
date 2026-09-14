# `actions.obj` opus5 150K wave 2: park triage (2026-09-14)

## Scope

- Translation unit: `source/ai/actions.c`.
- Worker notes: `scratch/workers/actions_action_flee_etc.md`.
- Ledgers read:
  - `actions_obj.md`
  - `actions_obj_codex_closure.md`
  - `actions_obj_jonas_policy_reconciled_replay_20260827.md`
  - `actions_obj_jonas_large_natural_pair_no_fake_20260830.md`
  - `actions_obj_jonas_pursuit_name_recovery_20260831.md`
  - `actions_obj_semantic_private_names_20260831.md`
  - `actions_obj_large_fuzzy_closeout_20260901.md`
  - `actions_obj_opus5_100k_waveD_20260914.md`
- 100K lane worker notes also read: `scratch/workersD/actions.md` (shapes A-D, V1-V5, D2/D3/D3c/D3probe).
- Inherited exact functions, not touched: `_actor_action_handle_lost_contact` and `_actor_action_handle_danger_avoidance`.

## Baseline and result

The baseline and the final gate are identical: `== exact 60  residual 3  unwritten 0`. The guard passes and park drift is 0. The production file is **unchanged**.

| Function | Outcome | Reason |
| --- | --- | --- |
| `_actor_action_try_to_dive` (720) | OWNER-BLOCKED | January pushes a 16-bit `animation_impulse` (+0x185 `xor ecx,ecx; mov cx,[esi-2]`). HCEX.pdb gives `unit_test_animation_impulse(long, short)`, but `units.h:638` declares `long`, and the definition is in protected `units.c`. Any probe has to rename the callee symbol, so a strict measurement is impossible. The prerequisite is the 100K lane's units.h + units.c proposal. |
| `_actor_action_handle_vehicle_entry` (960) | NOT-LANDED (scratch closer body) | See the vehicle-entry section below. |
| `_actors_searching_same_position` (320) | SKIPPED-EXHAUSTED | HCEX.pdb has parameters only. The function is static in HCEX but public in the January PDB publics (rva 59216). Shapes A-D already cover the single-exit and result forms. |

## `_actor_action_handle_vehicle_entry`: single-exit re-investigation

Scratch only. The best candidate is `scratch/workers/actions_action_flee_etc/act_ve3.c` (diff: `act_ve3.diff`).

### Lever

The lever comes from this wave's flood measurements. VC7 splits a return join into constant epilogues where each path's value is a known constant. It keeps the live register copy (`dl`) only on paths where the initialiser is still enregistered.

### Shapes

**VE3.** A fully structured single exit (SINGLE-EXIT LAW), with no gotos or labels:

```c
if (!(flee && panic_type > 0) && action != obey && !(last_vehicle_check_time != NONE && last_vehicle_check_time + 45 >= current_time))
{
	...
	if (best_vehicle_index == NONE && certain_combat_timer >= 60)
	{
		for (slot = 0; ...) { ... }
	}
	if (best_vehicle_index != NONE && action_vehicle_setup_impromptu(...))
	{
		actor_action_change(...);
		result = TRUE;
	}
}
return result;
```

Result: 960/22, sha 9139bfc47fc7de8a.

- The success tail is now January's `mov al,1`. The previous build merged it as `mov dl,1 ... mov al,dl`.
- Three hunks remain. Two are the FALSE epilogue copies in swapped order: January places the late-exit `xor al,al` copy first and the guard `mov al,dl` copy last, while our build does the reverse. The third is the prop-loop `best_vehicle_index` store, emitted one x87 instruction before January's position (before `fmul st(1)` instead of after it).

**Other shapes:**

| Shape | Result |
| --- | --- |
| VE1: the late `return FALSE` statements jump to the common return | identical to VE3 |
| VE4: `result` declared first | worse |
| VE5: De Morgan'd guard | identical to VE3 |
| VE6: `best_vehicle_index` stored before the distance | identical to VE3 |

### Outcome

The function is not exact, so the park stays. The whole-TU rows are unchanged, the other parks are OK, the guard passes, and the fake scan reports 0 leads.

**Reopen criterion:** start from `act_ve3.c`. A source fact is still needed that orders the two FALSE epilogue copies and schedules the integer store after the x87 multiply.

## Checks

On the real file:

- owner census: no new or dropped owners;
- `fake_match_scan`: 0 leads;
- `git diff --stat -- source/ai/actions.c`: empty.
