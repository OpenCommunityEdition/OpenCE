# `action_flee.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/action_flee.c`; target `build/split/source/ai/action_flee.obj` (January 2002 Xbox debug, VC7 13.00.9254).
- Worker notes: `scratch/workers/w3_path_smoothing_actor_firing_position_etc.md`; scratch dir `scratch/workers/w3_path_smoothing_actor_firing_position_etc/`.
- Baseline real-file gate (guard passes): `== exact 12 residual 1 unwritten 0`. The only residual, `_action_flee_perform` (864/30), is parked
  (park base 864/30 `cefb8e42bcdf2d49`, target `d62f05304574f88e`).
- Ledgers read: `action_flee_obj_jonas_two_leaf_20260821.md`, `action_flee_obj_update_semantic_owner_boundary_20260905.md`,
  `action_flee_obj_threshold_leaf_reconciliation_20260908.md`, `action_flee_obj_position_helpers_exact_20260909.md`,
  `action_flee_obj_opus5_150k_w2_20260914.md`; laws `scratch/w2/laws_w2.md` (C2, A7, R8, R9).
- `branch_sweep.py`: best historical blob 10/3/0 (below the current file).

## Result

**The real file is unchanged** (final gate 12/1/0, guard passes, park drift 0/1, fake scan 0 leads). One header packet is proposed,
measured with a shadow-header consumer census.

## `_action_flee_perform` - park reopen via a genuine units.h packet (no body change)

Law A7 (missing prototype via the genuine owner). `CL /Zs /W3` (`scratch/w3/c4013.txt`) reports two implicit declarations:
`unit_start_running_blindly` (line 414) and `unit_is_speaking` (line 689). Their definitions are `units.c:3897`
(`void unit_start_running_blindly(long unit_index)`; only a units.c-local prototype exists) and `unit_dialogue.c:389`
(`boolean unit_is_speaking(long unit_index)`). No header declares either; `units.h` owns both prototype sections (UNITS.C and
UNIT_DIALOGUE.C). January +0x1ea `test al,al` proves the boolean return; punpckhdq `units.h:557` declares `unit_is_speaking`
right after `unit_notify_impulse_sound`.

Shadow-header method: `shadow_gate.py` compiles each unit inside a full copy of the `source/` headers (so every quoted include
resolves in the shadow tree) and dumps per-function size/relocations/normalized sha. 76 units transitively include `units.h`.

| trial | units.h change | action_flee | 76-consumer deltas vs pristine |
| --- | --- | --- | --- |
| P | none | 12/1, perform = park base | - |
| H1 | + `unit_is_speaking` only | 13/0 | perform -> target; speech_timers park load pair; `_physics_update_old` (non-exact); **`_unit_preprocess_node_orientations` EXACT -> residual** |
| H2 | + both prototypes | **13/0** | **exactly two**: perform -> target; `_ai_communication_update_speech_timers` park flips to its second documented flag-neutral load order (672/43 `885fdb81966284ac`) |

Sum of strict-exact rows over the 76 units: P 2402 -> H2 2403, zero losses. H1 is rejected (declaration-count victim).

ORCHESTRATOR-PROPOSAL (exact diff `scratch/workers/w3_path_smoothing_actor_firing_position_etc/units_h_packet_H2.diff`, CRLF):

```c
/* prototypes/UNITS.C, immediately before unit_stop_running_blindly */
void unit_start_running_blindly(
	long unit_index);

/* prototypes/UNIT_DIALOGUE.C, immediately after unit_notify_impulse_sound */
boolean unit_is_speaking(
	long unit_index);
```

Consumers: 66 direct includers (`units_h_direct_includers.txt`), 76 compiled transitive consumers (`units_h_consumers.txt`).
Same commit: unpark `_action_flee_perform` (864, no source change) and re-baseline the `_ai_communication_update_speech_timers`
park base to `885fdb81966284acf5ba7138a4bbbd2221e7c61c89114fdd6f53b96ba89b81a0`. A full ninja plus stable sweep is required.

## Reopen criteria

- `_action_flee_perform`: met when the H2 packet lands (not the single-prototype variant).
