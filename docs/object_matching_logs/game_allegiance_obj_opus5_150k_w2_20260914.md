# `game_allegiance.obj` — Opus5 150K lane, wave w2 (2026-09-14)

## Baseline

The real file was unchanged at the start and is unchanged at the end, because both residual rows are parked. The focused gate reports **13 exact / 2 residual / 0 unwritten**, and the guard passes.

| Function | Target padded / relocs / sha16 | Baseline |
| --- | --- | --- |
| `_game_allegiance_broken` | 416 / 2 / `3eba7c13f76df6f6` | 416 / 2 / `abbb6b20…`, parked `register-allocation` |
| `_game_allegiance_create` | 224 / 5 / `47961c6c1d4c59a9` | 224 / 5 / `be3a5bd4…`, parked `register-allocation` |

## Outcome: park reopen proposed for both functions

This outcome requires a symbols.json linkage edit.

The scratch candidate is `scratch/workers/network_server_message_handler_game_allegiance_etc/ga_cand.c`. It gates at **15/15 exact**:

- The guard passes.
- Both parks drift to the target hashes.
- The owner census (kind and name) is clean.
- The fake scan reports 0 leads.

The candidate differs from the real file in two ways:

1. `static struct game_allegiance_globals *game_allegiance_globals = NULL;` (internal linkage).
2. `game_allegiance_create` reads `game_allegiance_globals->` directly. The copied `globals` local, and its re-store after `error()`, are removed. The post-loop test compares against `game_allegiance_globals->allegiance_count`.

The body of `game_allegiance_broken` is unchanged. Linkage alone closes it.

### Linkage evidence

1. **HCEX.pdb** (SHIP build, via `DIA2Dump -sym`) reports:
   - `game_allegiance_globals`: File Static, `struct struct_game_allegiance_globals *`.
   - `global_game_team_names`, from the same compiland: Global.

   The PDB therefore distinguishes the two linkages in this very TU.
2. **January split census:** no split object other than `game_allegiance.obj` references `_game_allegiance_globals`. No header in this repository declares it either.
3. **January codegen in `broken`:**
   - The pointer is loaded once, after the first `10*t1+t2` index computation.
   - It is never reloaded after the four `unsigned long` bit-vector stores.

   The measured alternatives fail:
   - External pointer plus direct access: 7 relocations, because VC7 assumes the stores may alias the pointer.
   - External pointer copied into a local: loaded at the assignment point, which occupies EDX. That forces the recorded CL/SI/DI allocation cycle.

   Only internal linkage reproduces January.
4. **January codegen in `create`:** with static linkage, the unchanged body differed only in `jle 0x51` versus `jle 0x4c`:
   - Comparing against the local count lets VC7 thread count <= 0 past the post-loop test.
   - January compares against the global field.
   - January also reloads the pointer after `error()`, which is natural for a direct global across a call.

   The HCEA donor also reads `game_allegiance_globals->allegiance_count` directly.

### New measured law

For VC7 13.00.9254, a never-address-taken static global:

- is not killed by stores through pointers;
- is loaded at first use;
- has its fields CSE'd into loop and branch compares, which changes jump threading.

Calls still kill it. An allocation cycle that involves a TU-private global is not exhausted until linkage has been checked, using a cross-object relocation census plus HCEX.pdb `DataKind`. None of the prior game_allegiance ledgers varied linkage.

### Shapes

| Shape | Result |
| --- | --- |
| b1: function-scope globals local (external) | load moves to entry (worse) |
| b2: direct global (external) | 7 relocations |
| Static plus unchanged `broken` body | exact |
| Static plus b2 | exact (not chosen) |
| `create`, static, unchanged body | 1 jump-target difference |
| c1 / c3: for or while loop | worse |
| c2: for loop with global bound | worse |
| c4: post-loop compare against `globals->allegiance_count` | exact |
| c6: all-direct, local-count post-loop compare | 1 difference |
| **c7: all-direct, no local** | **exact (chosen)** |

## ORCHESTRATOR-PROPOSAL (config)

Edit `config/symbols.json` line 22494 in place:

```
{ "file_offset": 4449424, "flags": 0, "name": "_game_allegiance_globals" },
->
{ "file_offset": 4449424, "flags": 0, "name": "_game_allegiance_globals", "static": true },
```

Then regenerate the csplit output for this object. The only consumer is `source/game/game_allegiance.c`, and no header declares the symbol. The candidate's symbol currently has storage class 3, while the split target has class 2. The data owner compares exact only after this regeneration.

The precedent is the objects.obj BSS statics in `docs/matching_methodology.md`.

## Reopen criteria (if rejected)

Reopen on an owner ruling on the internal-linkage proposal. Without it, both functions stay at the documented allocation cycles.
