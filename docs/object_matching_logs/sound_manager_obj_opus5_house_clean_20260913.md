# `sound_manager.obj` opus5 house-clean lane (2026-09-13)

## 1. Scope and provenance

- Translation unit: `source/sound/sound_manager.c`; target
  `build/split/source/sound/sound_manager.obj`.
- Lane worktree `C:\halo-worktrees\opus5-50k-house-clean-20260913`, branch
  `opus/50k-house-clean-20260913`, pinned base `73e97eca5`.
- Compiler: XDK VC7 13.00.9254, repository flags
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` (no flag was changed or passed).
- Evidence used: January target bytes/relocations (authoritative), January
  assert line anchors, HCEA reconstructions `cache/sound_definition_promote.c`,
  `sound/source_audible.c`, `cache/sound_dispose_from_old_map.c`,
  `sound/sound_new_impulse.c` (semantics only), and all seven earlier
  `sound_manager_obj*` ledgers.
- Only `sound_manager.c` changed. No header, config, symbols.json or
  parked.json edit. No ninja/configure/commit.

## 2. Validated baseline

Fresh real-file gate at the pinned base:
`== exact 51  residual 14  unwritten 0  (of 65 listed)`, `_point_from_line3d`
guard passed, fake-match scan 0 review leads.

Work pool (unparked residuals):

| Function | Target padded/meaningful | Relocs | Target hash | Base size / hash | Tag |
|---|---:|---:|---|---|---|
| `_sound_definition_promote` | 160 / 158 | 3 | `07c62c74ca8bb2f9` | 144 / `26a5dc289b3feb10` | size, sha |
| `_sound_dispose_from_old_map` | 240 / 236 | 21 | `80892c74c0d90b7a` | 272 / `c54fba50726ce6f7` | size, relocs 23, sha |
| `_source_audible` | 240 / 237 | 8 | `1da6136d5e94da86` | 240 / `719c0128a66a2647` | sha |
| `_refresh_listener` | 496 / 484 | 25 | `f5b03e22c4e86e1b` | 496 / `2ba59dbe4c46cda1` | sha |
| `_sound_new_impulse` | 976 / 970 | 48 | `7c5e4d796323cca5` | 976 / `37d885e8aab05d68` | sha (frame 0x20 vs 0x10) |

Parked and locked (not edited; measurements verified unchanged on the final
object): `_limit_pitch`, `_sound_set_definition_end`,
`_sound_find_like_channel`, `_looping_sound_new`,
`_update_channel_for_impulse_sound`, `_sound_refresh_looping`,
`_update_channel_for_looping_sound`.

## 3. Accepted controls (landed, strict EXACT)

Final real-file gate: `== exact 53  residual 12  unwritten 0  (of 65 listed)`.
Per-row diff against baseline: only `_sound_definition_promote` and
`_source_audible` changed (residual -> EXACT); no EXACT row was lost.

### `_sound_definition_promote` (160 padded / 158 meaningful / 3 relocs)

Retained source: one `short result = _sound_promotion_dont` declared before
the typed `sound_definition_get`, a positive `if (definition->promotion_count)`
block, and field compound assignments: decay the counter by elapsed render
time, clamp with `MAX(0, counter)`, stamp `promotion_time`, charge one
`longest_permutation_length`; over `promotion_count * length` either promote
(counter reset, `_sound_promotion_do`) or, with no promotion sound, refund the
charge (`-= longest_permutation_length`, `_sound_promotion_dont_play`).
Single `return result`.

Evidence: January `xor edi,edi` runs before the `tag_get` call (the result
is initialised first and lives in a callee-saved register); the non-default
returns are 32-bit constants `mov eax,1` / `mov eax,2` and the default is
`mov ax,di` (a result variable, per the single-exit law); three stores to
`+0x88` with a reload after the `+0x8C` store (field compound assignments,
not locals); the refund path computes `sub ecx,edx` from counter+length
rather than restoring a saved clamp. `sets dl` (not `setle`) fixes
`MAX(0, counter)` operand order.

### `_source_audible` (240 padded / 237 meaningful / 8 relocs)

Retained source: single-exit `nearest_listener_index` (initialised NONE) as
the only return value, with an `if / else if / else` over the spatialization
mode: unspatialized -> 0; relative and within range -> 0; absolute -> nearest
valid listener, obstruction computed for it, then NONE if out of range or
fully occluded.

Evidence: January returns 0 as `xor eax,eax` and NONE as `or eax,-1` (32-bit
constant forms of assignments to a result variable), every other exit is
`mov ax,si` from one variable initialised by `or esi,-1` /
`mov [ebp-8],esi` on entry, and the relative out-of-range path returns the
untouched NONE through the shared exit. First probe was exact.

Checks for both: whole-TU gate, `_point_from_line3d` guard, parked-sibling
drift (size/relocs/normalized SHA of all seven parked entries equal
`measurements.base`), owner census (candidate-only 26 / target-only 2 names,
identical to baseline), fake-match scan 0 leads.

## 4. Experiment matrix

| ID | Function | Source shape | Size T/B | Relocs T/B | Hash | First divergence | Siblings | Decision |
|---|---|---|---:|---:|---|---|---|---|
| E01 | promote | single-exit result declared after definition + field compound assignments, `MAX(0,x)` | 160/160 | 3/3 | `8633f7a3` | `+0x08`: result in eax, definition moved to ecx | 51/51 | reverted |
| E02 | promote | E01 with `result` declared before `definition` | 160/160 | 3/3 | `07c62c74` | none | 52 exact | **landed** |
| E02b | promote | E02 with `MAX(counter, 0)` | 160/160 | 3/3 | `42fbeefe` | `setle` vs `sets` | 51 | diagnostic, reverted |
| E03 | dispose_from_old_map | HCEA `any_sounds` flag, `for` fade loop, scoped `stop_time` | 240/272 | 21/23 | `c54fba50` | `+0xa6`: January `jmp 0x82` (top-tested wait loop); ours inverts and duplicates the test | 52 | reverted |
| E04 | dispose_from_old_map | E03 with deadline inline in the loop condition | 240/272 | 21/23 | `c54fba50` | same | 52 | reverted |
| E05 | dispose_from_old_map | E03 + HCEA `boolean paused` local re-read after the active block | 240/272 | 21/23 | `c54fba50` | same | 52 | reverted |
| E06 | source_audible | single-exit result, else-if chain | 240/240 | 8/8 | `1da6136d` | none | 53 exact | **landed** |
| E07 | refresh_listener | `listener` declared+initialised inside the loop | 496/496 | 25/25 | `2ba59dbe` | `+0x149`: esi/edi reload order swapped | 53 | reverted |
| E08 | refresh_listener | void single-exit `if (game_in_progress())` | 496/496 | 25/25 | `2ba59dbe` | same | 53 | reverted |
| E09 | refresh_listener | declaration order listener before index | 496/496 | 25/25 | `2ba59dbe` | same | 53 | reverted |
| E10 | new_impulse | full single-exit nested reconstruction (see §6) | 976/1024 | 48/49 | `e901350d` | `+0x03`: frame 0x20 vs 0x10; `_sound_scale_random_value` inlined instead of called | 53 | reverted |
| E11 | new_impulse | E10 moved to January assert-anchor position (after `sound_initialize`, before `sound_stop_impulse`) | 976/1024 | 48/49 | `e901350d` | identical to E10 | 53 | reverted |
| E12 | new_impulse | E10 with `sound_scale_value(...) < real_seed_random(...)` | 976/1024 | 48/49 | `e901350d` | identical to E10 | 53 | reverted |

## 5. Do-not-repeat list

- `_sound_dispose_from_old_map`: the 20260902 ledger attributed the wait-loop
  rotation to `_sound_idle` being unwritten. `_sound_idle` and
  `_update_channels` are now exact and the rotation persists, so that
  explanation is refuted. The `any_sounds` flag, inline deadline and captured
  `paused` local (E03-E05) are all byte-identical to the baseline.
- `_refresh_listener`: in-loop declaration, void single exit and
  declaration-order swap (E07-E09) do not move the esi/edi reload order. The
  20260909 ledger already rejected forcing it.
- `_sound_new_impulse`: definition position (E11) and skip-gate operand order
  (E12) do not change either the `sound_scale_random_value` inline decision
  or the random-versus-inline evaluation order.

## 6. Residual classification

- `_sound_dispose_from_old_map` - scheduling (measured). Every instruction
  outside the wait loop is identical. January keeps
  `while ((real)system_milliseconds() < stop_time) sound_idle();` top-tested
  with a back `jmp`; our build inverts it, adding one `system_milliseconds`
  call and one `__real@4f800000` load (the +2 relocations and +32 padded
  bytes). The cause of the inversion difference is unresolved (inference).
- `_refresh_listener` - register/spill-reload order tie (measured: 178/178
  instructions, all relocation identities equal, one swapped pair of stack
  reloads at the loop latch).
- `_sound_new_impulse` - structural plus inline schedule. Measured January
  facts: the result `[ebp-4]` is set to NONE before `tag_get` and every
  failure path returns it from one exit; the promotion `do` path is a real
  recursive call whose result is stored (the baseline `return
  sound_new_impulse(...)` becomes a tail-call `jmp`); `dont_play` stores NONE
  explicitly; the skip gate exits on `test ah,0x41; jne`, meaning
  `!(random > value)`; the format gate tests mono+sample_rate before stereo;
  datum stores follow store/call order (definition, playing channel, listener,
  type before the pitch call; flags and source identifier after it; fade
  times before loop track after `next_permutation`); `_sound_scale_random_value`
  is CALLED (5 pushes). E10 reproduces every one of these except the last two
  behaviours: our VC7 inlines `sound_scale_random_value` at this single call
  site (frame +0x10), and it evaluates the inlined `sound_scale_value` before
  `real_seed_random`, where January calls random first and keeps it on the x87
  stack (`fxch st(1); fxch st(2); fcompp`). Why January declines that inline
  is unresolved (inference: it is not definition position, per E11). The best
  structural candidate body is preserved at
  `scratch/workers/sound_manager/impulse_v1.txt` (not landed: a fuzzy rewrite
  of an already-written residual is inadmissible in this lane).

## 7. Reopen criteria

- `_sound_dispose_from_old_map`: a proven VC7 rule, or an exact January sibling,
  showing when a `while` loop whose condition is a call plus an
  unsigned-to-real conversion is left top-tested, traced to a nameable source
  form.
- `_refresh_listener`: a source-level fact (not a permutation) that changes the
  listener-pointer/index live-range split, such as an authenticated January
  body or an exact sibling whose identical spill order was closed by a
  nameable construct.
- `_sound_new_impulse`: evidence explaining January's declined inline of
  `sound_scale_random_value` and its call-first evaluation of the skip gate.
  Examples: a non-body attribute of those exact helpers, or a cross-object
  exact function with the same pattern and a known cause. Start from
  `impulse_v1.txt`.

## 8. Disposition

`sound_manager.obj` remains NonMatching: 53/65 strict exact. This lane adds
`_sound_definition_promote` and `_source_audible`: 2 functions, 395 meaningful
and 400 padded bytes, all strict gate EXACT. The three remaining pool
residuals stay active with the reopen criteria above. No fuzzy credit is
claimed.
