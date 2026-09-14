# `path_obstacle_avoidance.obj` opus5 150K wave 3: relabel-blocked rows re-measured (2026-09-14)

## Scope

- Translation unit `source/ai/path_obstacle_avoidance.c`; target `build/split/source/ai/path_obstacle_avoidance.obj`.
- Worker notes: `scratch/workers/w3_actions_path_obstacle_avoidance_etc.md`.
- Ledgers read:
  - `path_obstacle_avoidance_obj_jonas_two_leaf_20260821.md`
  - `path_obstacle_avoidance_obj_jonas_valid_point2d_20260826.md`
  - `path_obstacle_avoidance_obj_jonas_disc_heap_wave_20260829.md`
  - `path_obstacle_avoidance_obj_fable_owner_reconciliation_20260908.md`
  - `path_obstacle_avoidance_obj_opus5_150k_w1_20260914.md`
  - w1 worker notes `scratch/workers/path_obstacle_avoidance.md`
  - w1/w2 orchestrator summaries (`scratch/w1/w1-summary.txt`)
- The w1 reopens (`_render_debug_path`, `_path_new`, `_path_add_steps`) are already landed and exact; they were
  not touched.

## Baseline and result

The baseline and the final real-file gate are identical: `== exact 22  residual 2  unwritten 0`. The guard passes,
park drift is 0, the owner census is clean, and `fake_match_scan` reports 0 leads. The file is **unchanged**.

| Function | Padded / relocs | Gate | Outcome |
| --- | --- | --- | --- |
| `_path_avoid_obstacles` (parked) | 1360 / 49 | `[size 1312!=1360, sha]` in the real file | NOT-LANDED: the w1 v5 body is still normalized-sha equal at HEAD; blocked only by the held relabel |
| `_render_debug_obstacle_path` | 224 / 28 | `[reloc-identity]` | NOT-LANDED: same blocker |

## Re-measurement at HEAD 403108e28

Candidate: `scratch/workers/w3_actions_path_obstacle_avoidance_etc/pao_v5.c`, which is the current real file plus
the w1 v5 body.

- `_path_avoid_obstacles` measures 1360/49 with normalized sha `ec47d37a5ccdd19d`, equal to the target. Its gate
  tag is `[reloc-identity]` only.
- `relocdiff` matches 49 of 49 relocations. The 2 differing rows are the `.bss` label identity: January uses
  `_current_traverse_index` +0/+5432, while the source uses the statics `debug_path`/`debug_obstacles`.
- All 22 baseline-EXACT rows are unchanged, and parkcheck drift is limited to the function itself.

## Evidence search for authentic static names

Authentic names would lift the owner hold on invented descriptive `.bss` names. None were found:

- The January PDB publics in the 0x319D08 region are only `_debug_obstacle_path_on_failure` (+0x2140) and
  `_debug_obstacle_path` (+0x2141). There are no statics.
- punpckhdq `path_obstacle_avoidance.c` names the region `_bss_00319d08`.
- HCEA `path_avoid_obstacles.c` uses a different debug-storage design (`state->debug` arrays).
- The h1_tags exports only contain the hs global name strings.

So there is nothing new to propose. The w1 symbols.json relabel proposal stands as the owner decision.

## Reopen criteria

- Either an owner ruling on the w1 relabel (`_debug_path`/`_debug_obstacles` static at 3251464/3256896) or
  authentic names.
- After that, unpark `_path_avoid_obstacles` with the v5 body and retire the `_render_debug_obstacle_path` veto.
