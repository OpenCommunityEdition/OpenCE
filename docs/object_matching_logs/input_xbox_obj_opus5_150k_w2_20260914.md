# `input_xbox.obj` park re-investigation, opus5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and baseline

- Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Owned source: `source/input/input_xbox.c` only. No header, config, build, or other source was edited. Nothing was committed.
- Baseline real-file gate (with the `_point_from_line3d` guard): **22 exact / 5 residual / 0 unwritten**.
- All five residuals are parked in `config/parked.json`. They are locked in the real file, so every body was investigated in scratch only:
  - `_input_flush_rumble`
  - `_input_get_device_states`
  - `_input_get_raw_data_string`
  - `_input_update_analog_button_state`
  - `_input_update_keyboard_devices`

Ledgers read before any work:
- all eleven `input_xbox_obj_*.md` ledgers;
- the handoff `opus5_100k_consolidated_HANDOFF_20260914.md`;
- the off-ancestor ledger `76bdaf763:docs/object_matching_logs/progress_bar_and_input_xbox_obj_opus_20260906.md` (branch `opus/30k-ui-script-profiles-20260906`). Its do-not-repeat list covers `_input_get_device_states` shapes 1-5 and the key-tick pointer fix.

`branch_sweep.py` found no historical blob better than the current file.

## Outcome

| function | January padded / relocs | real file (unchanged) | best scratch | disposition |
| --- | ---: | --- | --- | --- |
| `_input_flush_rumble` | 176 / 8 | 160 / 7 | **EXACT** (`fn/r10.c`) | PARK-REOPEN proposed |
| `_input_get_raw_data_string` | 240 / 14 | 240 / 14, sha differs | **EXACT** (`fn/s3.c`) | PARK-REOPEN proposed |
| `_input_get_device_states` | 1232 / 28 | 1280 / 32 | 1216 / 28, correct frame (`fn/d2.c`) | not landed; blocked on the helper spelling below |
| `_input_update_keyboard_devices` | 640 / 49 | 640 / 51 | 640 / 49, sha differs (`fn/k3.c`) | not landed; constant-pin tie |
| `_input_update_analog_button_state` | 48 / 0 | 48 / 0, sha differs | unchanged | not landed; `sub al,0x20` vs `add al,0xe0` |

The reopen candidate is `scratch/workers/input_xbox/candidate.c` (sha256 `c544db919186146715e93e7296771e4a025647bac284c793c6f751fe906117ab`). It is the current real file plus only the two reopened bodies. Its checks:

- Gate: **24 / 3 / 0**. Diffing the full row listing against the real-file gate shows exactly the two rows `residual -> EXACT`; every other row is unchanged.
- The guard passes.
- `scratch/parkcheck.py`: the three remaining parks are unchanged (1280/32, 48/0, 640/51).
- Owner census against `build/base` and `build/split`: no candidate-only owner, no base-only owner, and no owner absent from the target.
- `tools/fake_match_scan.py`: 0 leads.
- CRLF line endings are preserved.

## New TU law (evidence for both reopens)

**In `input_xbox.c`, January iterates the four gamepads with a `short gamepad_index` `for` loop and indexes `input_globals.<array>[gamepad_index]` directly.** No pointer walks or pointer locals are used. Corroboration:

1. The exact sibling `input_dispose` uses exactly this form.
2. January's own assertion literals in this TU spell `input_globals.gamepad_handles[gamepad_index]`.
3. The `short` index type matches every public gamepad API in `input.h`.

The same spelling with a `long` index measures differently. VC7 then uses an end-pointer compare or a different IV base. The `short` index keeps a separate down-counter, as January does.

### `_input_flush_rumble` (176 / 8, sha `f473cb2d…`)

The park premise was "natural body remains 160 vs 176". January proves a different source shape:
- January emits **two** `XInputSetState` calls: the call is tail-duplicated into both arms of the right-motor `?:`.
- The handle is loaded once into ECX and never reloaded.
- The induction register is based on `gamepad_handles` (+316), with the rumbler read at `[edi+0xd8]`.

Shapes tried:

| shape | spelling | result |
| --- | --- | --- |
| r1 | `long` index `for` loop, `HANDLE` local, feedback pointer | 160 / 9 |
| r2 | pointer walk plus `HANDLE` local copy | 192 / 8 (IV based on the rumbler) |
| r3, r4 | r2 with increment order or declaration order swapped | inert |
| r5 | index for handle and rumbler, feedback pointer | 192 / 8 |
| r6 | `long` `for` loop, direct globals except feedback | 160 / 9 |
| r7 | r6 with `while` count | 160 / 7 |
| r8 | `long` index + count, all fields through direct globals | EXACT |
| r9 | r8 as a `long` `for` loop | 160 / 9 |
| **r10** | r8 as a `short gamepad_index` `for` loop | **EXACT** (proposed body) |

### `_input_get_raw_data_string` (240 / 14, sha `3a95b06c…`)

The park attributed the residual to "equivalent pointer setup / register scheduling". That was a symptom of the pointer-walk spelling.

Shapes tried:

| shape | spelling | result |
| --- | --- | --- |
| s1 | `long` index + count, direct globals | 224 / 17 |
| s2 | `long` `for` loop | 208 / 17 |
| **s3** | `short gamepad_index` `for` loop, direct `input_globals.gamepad_handles[...]` / `raw_gamepad_states[...]` | **EXACT** (proposed body) |

The first s3 run showed reloc-identity only, because a bash heredoc had eaten the `__FILE__` backslashes. After writing the paths with `chr(92)` the row is strict EXACT.

## Not landed

### `_input_update_analog_button_state`

The only difference is January `sub al,0x20` against VC7's `add al,0xe0`. January's inlined copy inside `_input_get_device_states` spells it `sub cl,0x20` as well, so the spelling is systematic.

Rejected source shapes:
- `FLOOR(value,32)-32`: phi form, fewer instructions.
- The FLOOR form plus `CEILING(value,191)+64`: 64 bytes.
- Ternary `value < 32 ? 0 : value-32`: identical to baseline.
- `MAX(value,32)-32`: phi form.
- `if (value >= 32)` polarity: still `add`.

Mechanism probes (not candidates), all still `add al,0xe0`:
- `-= 32U`
- `-= (byte)32`
- `(byte)(value-32)`
- `+ (-32)`

The saturating `MIN` probes emit `sub al,cl` on a phi and flip the private register ABI.

Finding: VC7 emits `sub r8,imm` only when the subtrahend applies to a join value, never to the direct byte-parameter arm, in every spelling tested.

Reopen criterion: a measured VC7 law selecting `sub r8,imm` for `param - const` in a conditional arm, or first-party source.

### `_input_get_device_states`

This work applied the new law, which is not on the 76bdaf763 do-not-repeat list: those shapes used `long` indexed loops.

| shape | spelling | result |
| --- | --- | --- |
| d1 | `short` index `for` loops, direct globals for handles and raw sticks, `gamepad_state` pointer, `short button_index` loops | 1216 / **28** |
| **d2** | d1 plus gamepad insertions/removals scoped in their own block | frame `sub esp,0x30` now exact |
| d3 | memory-unit pair also block-scoped | frame too small |
| d4, d5 | no `gamepad_state` pointer | IV base +356, worse |
| d6 | d2 with the binary loop running from `FIRST_GAMEPAD_BINARY_BUTTON` | 1232 but more differing instructions |

After d2, only two differences remain:
1. The inlined helper spelling `sub cl,0x20` against `add cl,0xe0`.
2. The binary-button loop uses EAX/ECX in swapped roles. `cmp eax,imm` is one byte shorter, which gives 1216 instead of 1232 padded bytes.

The function cannot become exact until the helper spelling is solved.

Reopen criterion: a helper `sub` lever; then re-test `fn/d2.c`. That file is also the recommended re-baseline body if the park measurement should be refreshed.

### `_input_update_keyboard_devices`

Shapes tried:

| shape | spelling | result |
| --- | --- | --- |
| k1 | the 76bdaf763 pointer walk, re-verified | 640 / 49, sha differs |
| k2 | `short key_code` `for` loop with direct `key_ticks[key_code]` / `key_latches[key_code]` | identical to k1, more natural spelling |
| k3 | k2 with ascii store `? keystroke.Ascii : NONE` | fixes the `movsx eax` / `store al` selection |
| k4 | k3 with the ternary polarity inverted | same as k3 |

What remains is VC7 pinning 0xff in EBX, spent three times. January rematerialises it and shrink-wraps EBX to the hotplug block.

Reopen criterion: a proven constant-pinning lever; then re-test `fn/k3.c`.

## Proposals for the orchestrator

1. **Park reopen** `_input_flush_rumble` and `_input_get_raw_data_string` from `scratch/workers/input_xbox/candidate.c`:
   - remove both `config/parked.json` entries;
   - apply the two bodies in one dedicated commit.
2. **Park refresh (optional, zero credit):**
   - `_input_get_device_states`: 1280/32 → 1216/28 with `fn/d2.c`;
   - `_input_update_keyboard_devices`: 640/51 → 640/49 with `fn/k3.c`.

   Both bodies apply the same TU law. Neither is exact, so both need an orchestrator decision.
