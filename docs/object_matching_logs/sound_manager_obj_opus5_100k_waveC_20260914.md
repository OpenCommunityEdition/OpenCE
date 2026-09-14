# `sound_manager.obj` opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/sound/sound_manager.c`; target
  `build/split/source/sound/sound_manager.obj`.
- Lane worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, branch
  `opus/100k-consolidated-20260914`, lane HEAD `7a3bbb9c8`.
- Compiler: XDK VC7 13.00.9254, repository flags (none changed or passed).
- Evidence: January target bytes/relocations (authoritative), January PDB
  public symbols (`scratch/pdb200-current/pdb_symbols.json`), HCEA
  reconstructions (`sound/sound_new_impulse.c`, `game/refresh_listener.c`,
  `cache/sound_dispose_from_old_map.c`, `sound/limit_pitch.c`,
  `sound/looping_sound_new.c`, `cache/sound_set_definition_end.c`,
  `cache/sound_find_like_channel.c`, `game/update_channel_for_impulse_sound.c`),
  all eight earlier `sound_manager_obj_*` ledgers and the house-clean evidence
  worktree notes. No Ghidra or map-atlas file was prepared for this object.
- Nothing was written to `sound_manager.c`, headers, `config/symbols.json` or
  `config/parked.json`. No ninja/configure/commit. Worker notes:
  `scratch/workersC/sound_manager.md`; candidates in
  `scratch/workersC/sound_manager/`.

## 2. Validated baseline

Real-file gate at lane HEAD: `== exact 55  residual 10  unwritten 0  (of 65 listed)`,
`_point_from_line3d` guard passed, fake-match scan 0 leads, seven parked entries
equal `measurements.base`.

| Function | Target padded / relocs | Base size / relocs | Status |
|---|---:|---:|---|
| `_sound_new_impulse` | 976 / 48 | 976 / 48 (frame 0x20 vs 0x10) | active residual |
| `_refresh_listener` | 496 / 25 | 496 / 25 | active residual |
| `_sound_dispose_from_old_map` | 240 / 21 | 272 / 23 | active residual |
| `_limit_pitch` | 96 / 1 | 112 / 1 | parked |
| `_looping_sound_new` | 304 / 14 | 320 / 14 | parked |
| `_sound_set_definition_end` | 208 / 9 | 208 / 9 | parked |
| `_sound_find_like_channel` | 272 / 16 | 272 / 16 | parked |
| `_update_channel_for_impulse_sound` | 528 / 29 | 528 / 29 | parked |
| `_sound_refresh_looping` | 1136 / 43 | 1088 / 42 | parked |
| `_update_channel_for_looping_sound` | 1360 / 61 | 1376 / 61 | parked |

## 3. Accepted controls

None landed in the real file (final gate identical to baseline, row for row).
Three results are handed to the orchestrator because they touch parked bodies
or a shared header:

1. **`_limit_pitch` park-reopen (strict EXACT in scratch).** January sends the
   `bend == 0` and `desired == old` cases to the same `fld desired` block as the
   decreasing MAX arm, so the value is returned once. The body
   `real pitch; if (bend == 0 || desired == old) pitch = desired; else if (desired > old)
   pitch = MIN(desired, old * bend); else pitch = MAX(desired, old / bend); return pitch;`
   is strict EXACT (96/1). This follows the single-exit law.
2. **`_looping_sound_new` park-reopen (strict EXACT in scratch).** January's
   detail loop is a rotated top-tested loop. The guard `jg` enters the body past
   the loop-top reload, and the count <= 0 path has its own epilogue. The plain
   `for (detail_index = 0; detail_index < definition->details.count; detail_index++)`
   is strict EXACT (304/14). The park's `if (count > 0) do {} while` emits
   `jle; jmp body` (+16 B).
   Combined candidate for both: `scratch/workersC/sound_manager/combined_parks.c`
   gives 57 exact. Only those two rows change. No other park drifts, the owner
   census is unchanged and fakescan is clean.
3. **Header packet for `_sound_set_definition_end`.** January callers load
   `pitch_range_index` with `xor reg,reg; mov reg16,[m]`, which is the 16-bit
   parameter form. The same load appears in the parked looping-channel update.
   A `NONE` argument is pushed as `push -1`, which rules out `word`. January's
   definition loads the parameter as a dword (`mov esi,[ebp+0x10]`). Emulating a
   `short pitch_range_index` prototype makes `_sound_set_definition_end`
   code-identical to January, and `_update_potentially_audible_looping_sound`
   stays code-identical. `sound_definitions.c` compiled against a `short`
   prototype with its existing `long` definition shows no normalized instruction
   difference. Proposal: change only the `sound_definitions.h` prototype to
   `short pitch_range_index`. This is a genuine January prototype/definition
   mismatch and must be disclosed. After the change, unpark
   `_sound_set_definition_end` and re-measure the base of
   `_update_channel_for_looping_sound`, which drifts but stays non-exact.
   `sound_definitions.h` has 18 direct consumers; only `sound_definitions.c` and
   `sound_manager.c` reference the function.

## 4. Experiment matrix (scratch only)

| ID | Function | Shape | Size T/C | Relocs T/C | Result |
|---|---|---|---:|---:|---|
| N1 | new_impulse | impulse_v1 + promotion `switch` | 976/1024 | 48/49 | jump-chain dispatch (January if-chain); reverted |
| N2 | new_impulse | gate via `real_local_random()` | 976/1024 | 48/49 | still hoisted; adds `_real_local_random` owner; reverted |
| N3 | new_impulse | `real random = real_seed_random(...)` block local, `if (random > sound_scale_value(...))` | 976/1024 | 48/49 | skip gate now identical to January (random first, fcompp); scale_random_value still inlined; best = `impulse_d6.txt` |
| N-d | new_impulse | diagnostics: explicit formula; no pitch call; pitch call under 1/2 extra `if`; scale_random_value defined after caller; nested-arg callee; recursion removed; +8 statements / +30 calls | - | - | inline of static `sound_scale_random_value` never declined |
| L1 | refresh_listener | HCEA inverted `== NONE` arm first | 496/496 | 25/25 | layout flips, reload order unchanged; reverted |
| D1 | dispose_from_old_map | definition before sound_render / before refresh_listener | 240/272 | 21/23 | unchanged |
| D2 | dispose_from_old_map | `for(;;){if(now>=stop)break;}`, `while{continue;}`, `while(TRUE)` + real local | 240/272 | 21/23 | all rotated |
| P1 | limit_pitch | nested positive guard, result local | 96/112 | 1/1 | still duplicated exit |
| P2 | limit_pitch | if/else-if/else result local | 96/96 | 1/1 | **EXACT** (proposal) |
| P3 | limit_pitch | reuse parameter as result | 96/96 | 1/1 | sha |
| S1 | looping_sound_new | `for` loop | 304/304 | 14/14 | **EXACT** (proposal) |
| S2 | looping_sound_new | `while` with `= 0` init | 304/320 | 14/14 | unchanged |
| E1 | set_definition_end | prototype `word` (emulated) | 208/208 | 9/9 | breaks update_potentially_audible (`push 0xffff`) |
| E2 | set_definition_end | prototype `short` (emulated), casts kept or removed | 208/208 | 9/9 | code-identical (proposal) |
| F1-3 | find_like_channel | declaration order / named channel pointer | 272/272 | 16/16 | byte-identical to base |
| I1-2 | update_channel_for_impulse | operand swap; HCEA pitch local | 528/528, 544 | 29/29 | identical / worse |
| R1-10 | refresh_looping | two-call fade-in tail; stop block as else arm; stop_sound test first; explicit returns; result-variable variants | 1136/1120 | 43/43 | best `park_refresh_looping_best.c`: one early `return TRUE` block merged (-16 B) |
| U1-7 | update_channel_for_looping | (on E2) fake-impulse else arm; `next_def != NONE && (...)` else-if; `pitch = limit_pitch(pitch, ...)`; channel_get_state arg casts/param types | 1360/1360 | 61/61 | best `park_update_channel_for_looping_best.c`: one missing `xor eax,eax` |

## 5. Do-not-repeat list

- `_sound_new_impulse`: promotion as switch; `real_local_random`; caller
  nesting, caller size, definition order, recursion and callee spelling do not
  change the `sound_scale_random_value` inline decision. The named `random`
  local already fixes the gate. Do not re-derive it.
- `_refresh_listener`: the inverted if arm is refuted by January's fallthrough
  layout. This adds to the house-clean E07-E09 attempts.
- `_sound_dispose_from_old_map`: definition position and every loop spelling
  above still rotate.
- `_sound_find_like_channel`: declaration order is inert.
- `_update_channel_for_impulse_sound`: commutative operand swaps are
  normalized away.
- `sound_definition_find_pitch_range_by_pitch`: a `word` prototype is refuted.

## 6. Residual classification

- `_sound_new_impulse`: owner-level inline decision, measured. January calls
  the static 48-byte `sound_scale_random_value` (the PDB publics confirm it is
  static) at its only call site. Our VC7 always inlines it, which adds +0x10
  frame. The cause is unresolved (inference).
- `_refresh_listener`: spill-reload order tie at the loop latch.
- `_sound_dispose_from_old_map`: loop-rotation decision.
- `_sound_find_like_channel`: esi/ebx coloring tie.
- `_update_channel_for_impulse_sound`: argument pre-push / frame-store
  scheduling tie.
- `_sound_refresh_looping`: return-value materialization for one early return
  (January `mov cl,1` pre-branch phi).
- `_update_channel_for_looping_sound`: one zero-extension idiom before a
  register-passed short argument. Decoding also found that the parked body
  nests the next-permutation test and so skips the permutation advance. This is
  a real behavioural difference in the parked source.

## 7. Reopen criteria

- `_sound_new_impulse`: a proven VC7 cause for declining a static auto-inline
  at a sole unconditional call site. Start from `impulse_d6.txt`.
- `_refresh_listener`: an exact sibling that reloads a strength-reduced pointer
  IV before its loop index, with a nameable source cause.
- `_sound_dispose_from_old_map`: a measured rule for a non-rotated while loop
  whose condition is a call plus an unsigned-to-real conversion.
- `_sound_refresh_looping`: a source form that materializes an early TRUE
  return as a register phi. Start from `park_refresh_looping_best.c`.
- `_update_channel_for_looping_sound`: a source form that zero-extends a short
  field into a register short argument. Start from
  `park_update_channel_for_looping_best.c` with the header packet.
- `_limit_pitch`, `_looping_sound_new`: criteria met, see §3.
- `_sound_set_definition_end`: met by the header packet in §3.

## 8. Disposition

`sound_manager.obj` remains NonMatching at 55/65 strict exact. This wave landed
nothing in production and claims no credit. Two parks (`_limit_pitch` 96 B,
`_looping_sound_new` 304 B) have strict-EXACT scratch candidates for the
orchestrator to reopen. `_sound_set_definition_end` (208 B) becomes exact with a
one-token `sound_definitions.h` prototype packet.
