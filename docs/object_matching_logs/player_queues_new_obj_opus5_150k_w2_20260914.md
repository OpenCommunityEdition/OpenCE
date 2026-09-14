# `player_queues_new.obj` — Opus5 150K lane, wave w2 (2026-09-14)

## Baseline

The real file was unchanged at the start and is unchanged at the end, because all three residuals are parked. The focused gate reports **20 exact / 3 residual**, and the guard passes.

| Function | Target | Baseline |
| --- | --- | --- |
| `_update_client_get_maximum_possible_server_time` | 80 / 3 / `30ffd0838c5ff2ed` | 80 / 3 / `1e89386c…` (compare/load transpose) |
| `_update_queues_reset_and_fill_with_lies` | 272 / 24 / `786c6c80f9616932` | 272 / 24 / `f76f7cbc…` (store around a push) |
| `_update_client_local_ticks` | 192 / 15 / `006837ab68e64068` | 352 / 31 (callee inlined) |

## Outcomes

### `_update_queues_reset_and_fill_with_lies`: park reopen proposed, no config needed

The one-use alias `struct server_update *update = &update_client_globals.updates[update_index].update;` is replaced by direct indexed member access:

```c
update_client_globals.updates[update_index].update_number = update_number;
update_client_globals.updates[update_index].update.action_count = 1;
csmemset(
	update_client_globals.updates[update_index].update.actions,
	0,
	sizeof(update_client_globals.updates[update_index].update.actions));
```

This applies the accessor-binding law: binding the element to a local changes the schedule, and direct access reproduces January. The rejected alternatives were the HCEA `struct update *` element alias and assigning the alias after the update_number store.

It is exact both on the unchanged file (`scratch/workers/network_server_message_handler_game_allegiance_etc/pq_f1_nonstatic.c`, 21/23) and in the combined candidate `pq_cand.c` (22/23):

- The guard passes.
- Only the reopened parks drift; the `local_ticks` park is unchanged.
- The census is clean.
- The fake scan reports 0 leads.

### `_update_client_get_maximum_possible_server_time`: park reopen proposed, needs symbols.json

The body is unchanged. Internal linkage of `update_client_globals` closes it.

Evidence:
- HCEX.pdb records `update_client_globals` and `update_server_globals` as **File Static**.
- In the January split census, both symbols are referenced only by player_queues_new.obj.
- No header declares either symbol.

Making `update_server_globals` static changes no code (tested alone: 20/3). It is proposed for fidelity to the PDB linkage.

The .bss layout is unchanged: server at 0, client at 0x4110, in one 0x145a4-byte section with flags 0xc0400080. The combined candidate is `pq_cand.c`. The mechanism is the static-global aliasing law recorded in `game_allegiance_obj_opus5_150k_w2_20260914.md`.

**ORCHESTRATOR-PROPOSAL.** Edit `config/symbols.json` in place, then regenerate the csplit output for this object:

```
line 22517: { "file_offset": 4451936, "flags": 0, "name": "_update_server_globals", "static": true },
line 22518: { "file_offset": 4468592, "flags": 0, "name": "_update_client_globals", "static": true },
```

The only consumer is `source/game/player_queues_new.c`.

### `_update_client_local_ticks`: not landed

In January, `update_server_build_server_update` is called, not inlined, and `&[ebp+8]` is passed as `update_number`. That address is the dead `ticks` parameter slot: VC7 overlaid the `long update_number` local there after the loop counter moved to ESI.

HCEX.pdb locals are `short ticks_elapsed`, `client_update`, `update` and `long update_number`, with no loop index. The original loop therefore decrements the parameter.

Shapes tried:

| Shape | Result |
| --- | --- |
| l1: `while (ticks-- > 0)` | 352 / 31, callee still inlined |
| l2: `for (; ticks>0; ticks--)` | 352 / 31, callee still inlined |
| l3: while loop with trailing decrement | 352 / 31, callee still inlined |
| `local_ticks` defined before the callee | still inlined (VC7 inlines later-defined callees) |
| Static globals | no effect |

Classification is unchanged: TU-context inliner decision. Reopen with evidence of what blocked January's auto-inlining, such as a callee IL size or inline budget fact.

## Checks

- Real-file final gate: 20/3.
- Park drift: 0.
- Census clean.
- Fake scan: 0.
- `git diff` of the file: empty.
