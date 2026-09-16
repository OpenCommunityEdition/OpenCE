# `sound_manager.obj`: opus5 next-150K lane, wave n3a (structural park pass, 2026-09-15)

## 1. Scope and provenance

- Translation unit `source/sound/sound_manager.c`; target `build/split/source/sound/sound_manager.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
  `opus/next-150k-house-clean-20260915`, frozen baseline `ae10935da`. XDK VC7 13.00.9254 with the repository
  flags; no flag changed or passed.
- **Nothing was written to `sound_manager.c`, to any header, or to `config/*.json`.** No ninja, configure, pytest
  or git write command was run. `git diff --stat -- source/sound/sound_manager.c` is empty.
- Worker notes `scratch/workers/n3_sound_manager.md`; candidates, objects and listings in
  `scratch/workers/n3_sound_manager/`.
- Evidence: January split-object bytes, relocations and section order (authoritative); the in-TU EXACT donor
  `_refresh_sounds`; HCEA `reference/halocea/src/sound_refresh_looping.c` (later build, semantics only);
  `config/parked.json`; all eleven earlier `sound_manager_obj_*` ledgers; `scratch/w2/laws_w2.md` and
  `scratch/w3/laws_w3.md` applied as detectors.

## 2. Baseline and final state (identical)

`python -B tools/campaign/gate.py source/sound/sound_manager --all --forbid-emitted-symbol _point_from_line3d`
reports `== exact 58  residual 7  unwritten 0  (of 65 listed)` at both ends; the guard passed;
`scratch/parkcheck.py` on the real object reports `parks 5 drift 0`; `tools/fake_match_scan.py` reports 0 leads.
`branch_sweep` finds no historical blob above 58 exact.

| Function | Target padded / relocs | Real file | Tag | Park | n3a result |
|---|---:|---|---|---|---|
| `_refresh_listener` | 496 / 25 | 496 / 25 | sha | - | skipped-exhausted |
| `_sound_dispose_from_old_map` | 240 / 21 | 272 / 23 | size, relocs, sha | - | skipped-exhausted (census re-confirmed) |
| `_sound_find_like_channel` | 272 / 16 | 272 / 16 | sha | P | skipped-exhausted |
| `_sound_refresh_looping` | 1136 / 43 | 1088 / 42 | size, relocs, sha | P | **park re-baseline proposed: 1120 / 43** |
| `_sound_set_definition_end` | 208 / 9 | 208 / 9 | sha | P | owner-blocked (mechanism now proven) |
| `_update_channel_for_impulse_sound` | 528 / 29 | 528 / 29 | sha | P | skipped-exhausted |
| `_update_channel_for_looping_sound` | 1360 / 61 | 1376 / 61 | size, sha | P | **park re-baseline proposed: 1360 / 61** |

## 3. Result

Nothing landed in the real file (both live targets are parked bodies). The wave produced three new byte-proven
structural findings, two of which refute the recorded park premises outright, and a combined scratch candidate
`scratch/workers/n3_sound_manager/cand.c` that improves both parked rows with zero regressions.

## 4. `_update_channel_for_looping_sound` - park premise REFUTED, size and relocations now exact

The park entry says "No unique source-level mechanism explains the residual". Two do.

**4.1 If/else arm order (closes the whole 16-byte size gap).** January `0x452 mov ecx,[ebp-0x1c]; 0x455 test
byte [ecx],2; 0x458 jne 0x4e8` places the `_looping_sound_fake_impulse_sound_bit` arm FAR (0x4e8, after the
function tail, ending `jmp 0x482`) and makes the `sound_definition_next_permutation(definition, ..., NONE)` arm
the fall-through at 0x45e. The retained body emits the mirror image. VC7 lays if/else arms in source order
(laws_w3 A39/A44; A27 is the accepted precedent for reading polarity off arm layout), so January's source writes
the next_permutation arm first:

```c
if (!TEST_FLAG(
	looping_definition->flags,
	_looping_sound_fake_impulse_sound_bit))
{
	permutation_index = sound_definition_next_permutation(
		definition,
		sound->pitch_range_index,
		NONE);
}
else
{
	sound->type = _sound_stop_track;
	looping_sound->ordered_sounds_finished = TRUE;
}
```

Alone (`u1.c`) this takes the row from 1376/61 to **1360/61** - size and relocation count exact.

**4.2 `limited_pitch` is not a separate local (closes the whole frame permutation).** Frame census: both sides
`sub esp,0x40`, eight 4-byte slots, none missing or extra, but three rotate:

| slot | January | retained body |
|---|---|---|
| [ebp-4] | `sound` | `sound` |
| [ebp-8] | `pitch` / limit result (shared) | `channel` |
| [ebp-0xc] | `channel` | `scale` |
| [ebp-0x10] | `scale` | `pitch` / limit result |
| [ebp-0x14] .. [ebp-0x20] | primary_sound_index, track, looping_definition, looping_sound | same |

January `fstp [ebp-8]` @0xdd stores `pitch` and `fst [ebp-8]` @0x2b2 stores the `limit_pitch` result into the
SAME cell: one source variable with five references, which outranks `channel`. With two separate locals VC7 ranks
each alone and the cell falls to fourth. Clamping in place removes the local:

```c
pitch = limit_pitch(
	pitch,
	channel_get(sound->playing_channel_index)->pitch * pitch_range->natural_pitch,
	definition->maximum_bend_per_second);
properties.pitch = pitch * pitch_range->playback_rate;
```

With 4.1 (`u2.c`) every frame slot matches January: **1360 / 61 / `cb7925e4a26b9464`**, 418 vs 420 instructions.

**4.3 What is left (2 instructions, owner-blocked).** `@0x2ea` January `xor ecx,ecx; mov cx,word [edi+0x8e];
push ecx` vs our `movzx ecx, word [edi+0x8e]; push ecx`, and `@0x37e` January's extra `xor eax,eax` before
`call _channel_get_state`. Both are laws_w3 A34 zero-extension into a full push slot, the fingerprint of a 16-bit
PARAMETER. `u3.c` proves an inline `(word)` cast at the call site is byte-inert, so there is no in-file lever.
The size is already exact, so these four bytes are absorbed by padding.

## 5. `_sound_refresh_looping` - park premise REFUTED, relocations and block layout now exact

**5.1 The missing relocation is a source defect, not "a compiler-merged duplicate fade-call tail".** Confirmed
and adopted from the w3c finding: deleting the `real fade_in_time;` phi local and writing the two
`sound_start_fade` calls restores 43/43 with matching per-target counts and January's 0x14 frame.

**5.2 NEW: the `goto begin_stop;` spelling inverts the loop block layout.** The retained body emits the stop
block FIRST (ours @0x1e0 `cmp word [ebx+0x52],2`, main path jumped to @0x2d2); January falls through into the
main path @0x1db and jumps forward to the stop block @0x30a. Replacing the label and goto with the equivalent

```c
if (refresh_state != _looping_sound_refresh_stop && !loop->ordered_sounds_finished)
{
	...
}
else if (loop->state != _looping_sound_refresh_stop)
{
	...
}
```

reproduces January instruction-for-instruction from 0xaf to the end, and deletes a `goto`.

**5.3 NEW: the early exits do not return `result`.** January shares ONE frame cell `[ebp-1]` between `result`
(`sete byte [ebp-1]` @0x10) and `new_looping_sound` (`mov byte [ebp-1],0` @0x80, `,1` @0xa5), which forces
`source` into EBX (`mov ebx,[ebp+0x10]` @0xc, `push ebx` @0x93) - the retained body colours `result` into BL
instead and has to reload `[ebp+0x10]`. Measured: any early exit that names `result` (`a2`, `a6`, and w1's S1)
gives `result` its own cell `[ebp-2]` and loses the colouring; spelling both early exits
`return !new_looping_sound;` (`a1.c`) reproduces January's shared cell and EBX exactly. HCEA corroborates the
expression (`reference/halocea/src/sound_refresh_looping.c:117-125`, annotated as the original `return v16==0`).

`a1.c` = **1120 / 43 / `86e26bb3e59f7590`**, 365 vs 378 instructions, with only two facts left: January's
`mov cl,1` @0x84 (the last flag-safe slot before `jne 0xaf`) and its private `mov al,cl` epilogue @0x451 for the
stop-and-not-found return, where our build constant-folds that return and cross-jumps it into the `datum_delete`
`mov al,1` epilogue. That is one tail-merge decision, laws_w3 A43 class. `a5` and `a7` refute the merged-exit and
extra-local explanations.

## 6. The other rows

- **`_sound_set_definition_end`** (park, 208/9): 72 vs 73 instructions, the single difference being the same
  `xor eax,eax; mov ax,word [esi+0x8e]` vs `movzx`. `sd1.c` (a `word` local modelled on the EXACT in-TU donor
  `_refresh_sounds`, sound_manager.c:3233) yields `mov ax,word [...]` plus a compensating `movzx eax,ax`, so it
  is a clean negative. The mechanism is now proven: the January header declared a 16-bit third parameter for
  `sound_definition_find_pitch_range_by_pitch` while the exact `sound_definitions.c` definition is `long`. The
  owner already ruled that prototype an ABI lie (w1 4.4, w3c 5); per laws_w3 R13 the ruling binds and **no packet
  is proposed**. Recorded for the owner's original-bug adjudication only.
- **`_sound_dispose_from_old_map`** (active residual, 272/23 vs 240/21): fresh census re-confirms the w3c
  finding - January's wait loop is unrotated (`0xa6 jmp 0x82` back to a single header), ours duplicates the whole
  10-instruction header into the latch (+12 instructions, +32 bytes, +2 relocations). 11 loop spellings across 6
  lanes are measured inert and no w2/w3 law fires on header duplication. No shape spent.
- **`_refresh_listener`, `_sound_find_like_channel`, `_update_channel_for_impulse_sound`**: measured ties with
  their levers spent (w3c section 5). No shapes spent.

## 7. Proposals (orchestrator; zero strict credit)

Candidate `scratch/workers/n3_sound_manager/cand.c` = the current real file plus exactly the two reopened bodies
(`a1` + `u2`). Measured: `== exact 58 residual 7 unwritten 0`, row-for-row identical to baseline except the two
target rows; `_point_from_line3d` guard passed; `parkcheck` drift only on the two intended parks; owner census
identical to `build/base` (no new candidate-only owner, 22 candidate-only vs target on both sides); fakescan 0.

1. Re-baseline `config/parked.json` `_sound_refresh_looping` 1088/42/`4820b25c...` -> **1120/43/`86e26bb3e59f7590`**
   and land `a1`'s body. Supersedes the w3c `rl1` proposal (also 1120/43 but still block-inverted).
2. Re-baseline `_update_channel_for_looping_sound` 1376/61/`5fa29769...` -> **1360/61/`cb7925e4a26b9464`** and
   land `u2`'s body; reclassify the park from `unclassified` to the 16-bit-parameter owner-ruling class.

Both are strictly closer on every measured axis and both delete a local rather than adding anything.

## 8. Reopen criteria (replacing the recorded ones)

- `_sound_refresh_looping`: a source fact that stops VC7 constant-folding the stop-and-not-found
  `return !new_looping_sound;` and cross-jumping it into the `datum_delete` `return TRUE;` epilogue - i.e.
  anything that yields `mov cl,1` in the block ending at `jne` plus a private `mov al,cl` epilogue. Everything
  else in `a1.c` is byte-proven. Do not re-spend w1 S1-S8, waveC R1-R10, w3c rl1-rl3, or n3a a1/a2/a5/a6/a7.
- `_update_channel_for_looping_sound`: an owner ruling on the width of
  `sound_definition_find_pitch_range_by_pitch`'s third parameter, plus a form that zero-extends
  `sound->playing_channel_index` into the full EAX before `channel_get_state`. Everything else in `u2.c` is
  byte-proven. Do not re-spend the frame-permutation or block-order searches, or `u3`'s cast.
- `_sound_set_definition_end`: unchanged (same owner ruling).
- `_sound_dispose_from_old_map`: an EXACT January sibling whose `while (call() < real)` loop keeps a single header
  copy, with a nameable source cause.

## 9. Disposition

`sound_manager.obj` stays at 58/65 strict exact; this wave claims no bytes. It leaves two actionable zero-credit
park re-baselines that together take the two largest residual rows from 1088/42 and 1376/61 to 1120/43 and
1360/61, and it reduces both to a single named compiler decision each.
