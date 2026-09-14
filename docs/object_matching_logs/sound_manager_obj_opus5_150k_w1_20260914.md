# `sound_manager.obj`: opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/sound/sound_manager.c`.
- Target: `build/split/source/sound/sound_manager.obj`.
- Lane worktree: `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
- Branch: `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Compiler: XDK VC7 13.00.9254 with the repository flags. No flag was changed.
- Only `sound_manager.c` was edited. No header, config, `symbols.json` or `parked.json` changed. No ninja, configure, pytest or commit was run.
- Evidence used:
  - January target bytes, relocations and section (emission) order;
  - January assert line anchors;
  - January PDB publics (`scratch/pdb200-current`);
  - map atlas `scratch/atlas/sound_manager.obj.txt`;
  - HCEA `sound/sound_new_impulse.c`, `cache/sound_refresh_looping.c`, `cache/sound_scale_random_value.c` (names and semantics only);
  - all nine earlier `sound_manager_obj_*` ledgers;
  - the 100K handoff.
- Worker notes: `scratch/workers/sound_manager.md`. Candidates and diagnostics are in `scratch/workers/sound_manager/`.

## 2. Baseline

Real-file gate: `== exact 57  residual 8  unwritten 0  (of 65 listed)`. The `_point_from_line3d` guard passed.

| Function | Target padded / relocs | Base | Status |
|---|---:|---|---|
| `_sound_new_impulse` | 976 / 48 | 976 / 48, sha | active residual |
| `_refresh_listener` | 496 / 25 | 496 / 25, sha | active residual (tie) |
| `_sound_dispose_from_old_map` | 240 / 21 | 272 / 23 | active residual (loop rotation) |
| `_sound_find_like_channel` | 272 / 16 | 272 / 16 | parked |
| `_sound_set_definition_end` | 208 / 9 | 208 / 9 | parked |
| `_update_channel_for_impulse_sound` | 528 / 29 | 528 / 29 | parked |
| `_sound_refresh_looping` | 1136 / 43 | 1088 / 42 | parked |
| `_update_channel_for_looping_sound` | 1360 / 61 | 1376 / 61 | parked |

## 3. Landed: `_sound_new_impulse` (strict EXACT)

- Size: 976 padded / 970 meaningful bytes, 48 relocations.
- Normalized sha: `7c5e4d796323cca5`.
- Final real-file gate: `== exact 58  residual 7`. Only this row changed.

### 3.1 The new lever: January's second caller of `sound_scale_random_value`

The `looping_sound_new` detail loop in January contains the complete inline body of `sound_scale_random_value`:
- the seed call;
- `real_seed_random_range`;
- `(upper - lower) * scale + lower`;
- `* base`.

The retained source reproduced that inline with six named locals (`scale`, `upper_scale`, `lower_scale`, `period_upper_bound`, `period_lower_bound`, `period`). These are the inliner's parameter copies.

The loop now uses the helper call itself:

```c
looping_sound->detail_play_times[detail_index] = (long)(
	sound_scale_random_value(
		detail->period_bounds.lower,
		detail->period_bounds.upper,
		definition->scale_lower_bound.detail_period,
		definition->scale_upper_bound.detail_period,
		source->scale) * 1000.f +
	sound_manager_globals.render_time);
```

Results of that change:
- `looping_sound_new` stays strict EXACT. It is an inherited row, not claimed here.
- VC7 now calls `sound_scale_random_value` out of line in `sound_new_impulse`, as January does at 0x245 (five pushes and a combined `add esp,0x1c`). The earlier +0x10 frame disappears.

Independent corroboration comes from emission order. January compiles `looping_sound_new` in a deferred pass, immediately before `sound_set_definition_end`. The emulated-locals body compiled in the first pass; the helper-call form reproduces January's deferred slot. The per-call-site decline itself is measured only in the real TU. A tiny two-caller harness always inlines, so the underlying VC7 mechanism is not isolated.

This supersedes the waveC classification "owner-level inline decision".

### 3.2 Remaining structural facts, each checked against January bytes

- **Single exit.** `sound_index` is initialized to NONE before the `tag_get` call. Every failure path returns it through one exit. The body is the nested single-exit reconstruction from waveC `impulse_d6`.
- **Dialog stop time.** January keeps `definition_index` in ebx from entry and homes the `game_time_get()` result in `[ebp-8]`. The expression is `long dialog_stop_time = game_time_get() + 30 * definition->longest_permutation_length / 1000 + 10;` with no temporary local. A named `game_time` local took ebx and forced a reload.
- **Dont-play arm.** The arm stores NONE explicitly (`mov [ebp-4],0xffffffff` at 0x3a1, on that path only). Without the store the function measures 960 bytes. HCEA's later build writes `return -1` here but `return result` elsewhere, which corroborates an explicit NONE.
- **Skip gate.** It keeps the waveC named `real random` local (random first, kept on the x87 stack).

### 3.3 Admission checks

- Whole-TU gate: a row-by-row diff against baseline changes only `_sound_new_impulse`.
- `_point_from_line3d` guard: passed.
- `scratch/parkcheck.py`: 5/5 parks, drift 0.
- Owner census against `build/base` and the target: unchanged (19 candidate-only / 1 target-only).
- `tools/fake_match_scan.py`: 0 leads (HEAD also 0).
- Authenticity: an ordinary helper call replaces an emulated inline, and a single-exit body replaces early returns. January's store/exit layout and HCEA topology support both.

## 4. Not landed

### `_sound_refresh_looping` (parked; scratch only)

The 16-byte gap is now fully decoded:
- The entry `sete [ebp-1]` (result) is returned only on the disabled path. January reuses that slot for `new_looping_sound`.
- The not-found + stop exit returns a register boolean loaded before the found test: `mov cl,1` … `mov al,cl` in its own epilogue.
- The new-failed exit folds into the shared `xor al,al` epilogue.
- HCEA decompiles the new-failed return as `!new_looping_sound`.

Shapes tried on top of waveC rl10:

| Shape | Result |
|---|---|
| S1: early `return result` | 1120 |
| S3: nested block, no early return | 1120 |
| S5: `&&` guard | 1120 |
| S6: `if (index == NONE) { if (state != stop) new; if (index == NONE) return !new_looping_sound; }` | **size-exact 1136/43**, but materializes `sete` at runtime |
| S7: `return refresh_state == stop` | 1136, sha differs |
| S8: `return result` | 1120 |

Best size-exact scratch body: `scratch/workers/sound_manager/rlS6_body.txt`. Not exact, so no reopen.

### `_update_channel_for_impulse_sound` (parked; scratch only)

- Replacing the emulated `scale_upper_gain` / `scale_lower_gain` / `distance_gain` locals with `sound_scale_value(sound->source.gain, zero_gain_modifier, one_gain_modifier, scale) * master_gain * fade` gives byte-identical code. It is a proven cleanup for whenever the body is reopened.
- The push/x87-store scheduling tie remains.

### Skipped without new evidence

- `_refresh_listener` and `_sound_find_like_channel`: register ties.
- `_sound_dispose_from_old_map`: loop rotation.

### Owner-blocked (parked)

- `_sound_set_definition_end` and `_update_channel_for_looping_sound` still depend on the `short pitch_range_index` prototype that the owner ruled out.
- `sound_definitions.c`'s exact definition already spells every use as `(short)pitch_range_index` on a `long` parameter. Any reopen is an owner decision on that pair.

## 5. Compile-order diagnostic (do not repeat)

- **Emission rule.** VC7 13.00.9254 emits functions in multiple passes. Each pass walks the remaining definitions in source order and compiles a function once every TU-defined callee is compiled. `scratch/workers/sound_manager/callgraph.py` reproduces our emission order exactly.
- **January's passes.** January's section order decomposes into five passes consistent with the assert-line anchors. Every January function that uses a header `__inline` is compiled in pass 2 or later.
- **Reorder test.** Reordering all 65 definitions toward January's order (`ord1.c`) changes no residual sha. Compile order is inert for the `refresh_listener`, `dispose_from_old_map`, `find_like_channel`, `update_channel_*`, `set_definition_end` and `refresh_looping` residuals.
- **Other negatives for `sound_new_impulse`:**
  - moving `sound_scale_random_value` into a deferred position;
  - an inlined-code budget of +2/+8/+20 statements;
  - callee spellings with a seed local, the explicit formula or `real_local_random_range`.
  None of these changed the inline decision. `/Ob1` (diagnostic only) proves the TU is `/Ob2`: 24 rows break under `/Ob1`.
- **Tool caveat.** Git Bash rewrites `--cflag /ObN` into a path unless `MSYS_NO_PATHCONV=1` is set.

## 6. Reopen criteria

- **`_sound_refresh_looping`:** a source form that loads a pre-branch `mov cl,1` boolean, returns it through its own epilogue, and folds the new-failed path into the shared FALSE epilogue. Start from `rlS6_body.txt`.
- **`_update_channel_for_impulse_sound`:** a nameable construct that schedules a call's integer push ahead of the preceding x87 struct-field stores.
- **`_refresh_listener`, `_sound_dispose_from_old_map`, `_sound_find_like_channel`:** unchanged from the waveC ledger.
- **`_sound_set_definition_end`, `_update_channel_for_looping_sound`:** an owner ruling on the `pitch_range_index` prototype/definition pair.

## 7. Disposition

`sound_manager.obj` is 58/65 strict exact in the worker gate. This wave adds `_sound_new_impulse`: 970 meaningful / 976 padded bytes, 48 relocations. Only the orchestrator's full build counts.
