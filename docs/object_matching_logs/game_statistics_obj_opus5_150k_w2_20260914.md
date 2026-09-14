# `game_statistics.obj` — Opus5 150K lane, wave w2 (2026-09-14)

## Baseline

The real file was unchanged at the start and is unchanged at the end. The focused gate reports **3 exact / 1 residual**, and the guard passes.

| Function | Target | Baseline |
| --- | --- | --- |
| `_game_statistics_record_kill` | 672 padded / 661 meaningful / 22 relocs / `faed51b584f65f1a` | 672 / 22 / `c83a7c0613ccce27`, parked `instruction-scheduling` (+0x1AE transpose) |

## History sweep

`branch_sweep` flags history blob `7a73a56760` as 4/4. A diff against the current file shows that its only closing construct is the one-use block-local `short *kills` alias. Ledger `game_statistics_obj.md` rejects that alias as an optimizer carrier, so it was **not ported**.

## Outcome: park reopen proposed

The scratch candidate is `scratch/workers/network_server_message_handler_game_allegiance_etc/gs_cand.c`. It gates at **4/4 exact**:

- The guard passes.
- parkcheck shows only this park drifting, to the target hash.
- The owner census is clean.
- The fake scan reports 0 leads.

Change versus the real file:

```c
long dead_player_index = player_index_from_unit_index(dead_unit_index);
long statistic_index;

if (dead_player_index == NONE)
	statistic_index = NONE;
else
	statistic_index = 0;

if (statistic_index != NONE)
{
	... unchanged body ...
	credited_player->statistics.kills[statistic_index]++;
	...
	assisting_player->statistics.assists[statistic_index]++;
```

The NonMatching comment is also removed.

### Why this is new evidence and not a repeat

Two earlier results, measured separately:

- **E47** (`game_statistics_obj_jonas_victim_flag_rejection_20260830.md`, 2026-08-30) tested the HCEA two-valued victim flag. It fixed the +0x1AE schedule. It was rejected only because the separate +0x67 ESI/EDX window was still open at that time.
- **Fable's loop-initialization order** (`79275995c`, retained 2026-08-31) later closed +0x67 on its own.

The combination was never measured. Measured today, it is strict exact. E47's do-not-repeat note depended on +0x67 still being open, which is no longer true.

### Authenticity

In the HCEA reconstruction (`halocea-review/src/blam/game/game_statistics_record_kill.c`), the binary keeps a NONE-or-0 victim flag:

- It returns when the flag is NONE.
- It indexes both `kills[victim_flag]` and `assists[victim_flag]` through that value, rather than folding them to constants.

This supports a real source variable rather than a carrier. The code is ordinary typed C89 with no barrier, alias or dead store: the variable guards the body and selects the statistics slot.

The criterion in the main ledger (reopen #3, "a defined-C dependency control not represented by E01-E46") is met.

### Shapes

| Shape | Result |
| --- | --- |
| Current body plus if/else flag | exact |
| Same flag as a declaration-initialized ternary | 688 bytes (rejected) |
| Name-only rename to `statistic_index` | exact |
| `static` `game_statistics_active` (linkage probe) | no effect |

## Reopen criteria (if rejected)

Reopen on an owner ruling on the HCEA NONE-or-zero statistics index.
