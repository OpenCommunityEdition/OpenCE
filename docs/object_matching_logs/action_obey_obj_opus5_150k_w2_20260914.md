# ActionObey (opus5 150K house-clean lane, wave w2, 2026-09-14): command_begin blocker proved, control left exhausted

Worker lane `opus/150k-house-clean-20260914` (base `12f7375d4`). Only `source/ai/action_obey.c` was in scope.
**No production source changed.** `git diff --stat -- source/ai/action_obey.c` is empty, and the final real-file gate
matches the baseline row for row.

## 1. Baseline (real file, whole-TU gate, `_point_from_line3d` guard passed)

`== exact 25  residual 2  unwritten 0  (of 27 listed)`. This unit has no `config/parked.json` entries.

| Row | January (padded / relocs / sha16) | Real file |
| --- | --- | --- |
| `_action_obey_command_begin` | 4320 / 167 / `5b56fdd8b88a4456` | 4336 / 167, frame `0x184` vs `0x174` |
| `_action_obey_control` | 992 / equal | 992, [sha] only |

Inherited exact rows from earlier lanes were not touched: `_vehicle_possibility_qsort`, `_action_obey_describe_command`,
`_action_obey_command_perform`, `_action_obey_individual_perform`, `_action_obey_perform`.

## 2. `_action_obey_command_begin`: NOT LANDED (policy-blocked, reopen criterion proved unsatisfiable)

Prior state: the 100K lane's exact file (`opus5-100k-consolidated-20260914/scratch/orch/action_obey_waveB.c`) differs from
the real file in four hunks. The orchestrator rejected it twice because one hunk restores
`look_duration = command->parameter1;` inside the `_ai_atom_look` valid arm, a store that repeats the declaration
initializer. The admission note asked for a non-redundant January statement shape.

### January evidence (target COFF via `tools.campaign.alndiff`)

- Look prologue `+0x1ca..0x1e2`: `mov ecx,[ebp-4]` (command), `mov edx,[ecx+4]` (parameter1). It then stores the three
  NONE indices and `mov [ebp-0x28],edx` (duration).
- Look valid arm `+0x205..0x213`: `mov eax,[ebp-4]; mov edi,ecx; mov ecx,[eax+4]; mov [ebp-0x14],edi; mov [ebp-0x28],ecx; jmp +0x418`.
- Across the whole function, `[ebp-0x28]` has exactly six uses:
  - writes: `+0x1e2` (prologue), `+0x210` (look valid), `+0x2a4` and `+0x2b9` (look_random valid, `fstp`);
  - reads: `+0x418` (the join, `fld` then compare against 0.0) and `+0x4fa` (pause_timer).
- Neither look_player nor look_object writes it. All six invalid look/look_random exits (`+0x1ee`, `+0x1ff`, `+0x229`,
  `+0x23a`, `+0x24a`, `+0x255`) jump straight to `+0x418` without a store.

### Dominance argument

The join reads the duration first on every path. On the invalid edges and on the look_player/look_object paths, the only
reaching definition is the prologue store of `command->parameter1`. The source must therefore contain a dominating
`look_duration = command->parameter1` (the declaration initializer or an equivalent statement). The only alternative is
reading an uninitialized local, which is undefined behaviour and prohibited, and VC7 would not synthesize the prologue store
in that case anyway.

The look-valid store copies the same value, from the same memory, into the same variable. Nothing writes `[ebp-0x28]` or
`command->parameter1` between the two stores. Every C statement that emits these three instructions is therefore
value-redundant.

HCEA (`halocea-review/src/blam/ai/action_obey_command_begin.c:261-267`, a later build) has no such store, so it offers no
alternative January spelling. **The requested reopen criterion cannot be met on January's bytes.**

### Measurements (reference only, whole-TU with guard)

| Candidate | Change | Result |
| --- | --- | --- |
| `b4_waveB.c` | 100K exact form (includes the re-store) | 4320/167 EXACT; all other rows unchanged. Not landable. |
| `n1_no_restore.c` | b4 minus only the re-store | 4320 [sha]. The three look-arm instructions are missing, and the -0x14/-0x28/-0x2c slot permutation cascades through the look_random, look_player and look_object arms. |

No further shapes were tried: the dominance argument covers the remaining space. Classification: policy-blocked. January
contains a value-redundant store, so this is not a register or scheduling tie.

### Disposition and reopen

- Stays a zero-credit fuzzy genuine caller in the real file.
- The row can close only through an explicit owner decision to admit a January-proven redundant store. The evidence for
  that decision is the 100K exact file plus the proof above. Do not requeue it for workers.

## 3. `_action_obey_control`: SKIPPED (exhausted, on the brief's do-not-spend list)

No new source or type evidence was found, so no shapes were spent.

Re-reading the four differing instruction pairs (`+0x33e..0x391`) gives one refinement to the 100K classification:

- In both builds, the pair member held in ECX is loaded first in the facing arm and stored first at the join.
- The `global_forward2d` arm loads in memory order in both builds.

So the "j-first copy, +0x448 before +0x444" ordering follows from the register choice (January: i in EDX, j in ECX; ours:
the reverse). It is not a separate statement-order fact. The residual is a pure ECX/EDX permutation of an enregistered
`real_vector2d` (class C). HCEA `action_obey_control.c:170-180` uses the same ternary-copy semantics that were already tried.

Reopen criterion is unchanged from the 100K ledger: a verified January idiom that copies an enregistered 2D pair with the
second member in ECX, or new type evidence for `orders.move.jump_alignment_vector`.

## 4. Final checks

- Final real-file gate equals the baseline (`diff` of the `--all` listings is empty); guard passed.
- No parks for this unit, so no park drift; owner census unaffected (no edit); `tools/fake_match_scan.py` reports 0 leads.
- Worker notes: `scratch/workers/action_obey.md`; candidates and objects: `scratch/workers/action_obey/`.
