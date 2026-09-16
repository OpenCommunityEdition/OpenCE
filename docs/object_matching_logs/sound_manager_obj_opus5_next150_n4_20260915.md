# `sound_manager.obj`: opus5 next-150K lane, wave n4 (relocation-census pass, 2026-09-15)

## 1. Scope and provenance

- Translation unit `source/sound/sound_manager.c`; target `build/split/source/sound/sound_manager.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, XDK 3911, repository flags; no flag changed or passed).
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
  `opus/next-150k-house-clean-20260915`.
- Assigned target: `_sound_dispose_from_old_map` (active residual, structural size + relocation gap), with
  the orchestrator's relocation-by-target leverage "ours owns `_system_milliseconds` ×1 that January does not".
- **Nothing was written to `sound_manager.c`, to any header, or to `config/*.json`.** No ninja, configure,
  pytest or git write command was run. `git diff --stat -- source/sound/sound_manager.c` is empty.
- Worker notes `scratch/workers/n4_game_sound_sound_manager.md`; scratch
  `scratch/workers/n4_game_sound_sound_manager/`.
- Ledgers read: all thirteen `sound_manager_obj_*`, in particular `opus5_next150_n3a_20260915`,
  `opus5_250k_w3c_20260915`, `opus5_150k_w1_20260914`, `opus5_100k_waveC_20260914`,
  `codex_large_wave_20260902`, plus `scratch/workers/w3c_sound_manager.md`, `scratch/workers/n3_sound_manager.md`,
  `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, `config/parked.json`.
- Evidence: January split-object bytes, relocations and section order (authoritative); `build/base` objects
  for all 833 split units (for the new image-wide loop census); HCEA
  `cache/sound_dispose_from_old_map.c` (later build, semantics only).

## 2. Baseline and final state (identical)

`python -B tools/campaign/gate.py source/sound/sound_manager --all --forbid-emitted-symbol _point_from_line3d`
reports `== exact 58  residual 7  unwritten 0  (of 65 listed)` at both ends; the guard passed;
`scratch/parkcheck.py` on the real object reports `parks 5 drift 0`; `tools/fake_match_scan.py` reports
0 leads. `branch_sweep.py` sees 21 unique blobs; the best historical one (`e66b63e9`) is also 58/7/0 but with
a worse size delta (96 vs 48), so the current tree stands.

HEAD already carries wave n3a's two accepted park re-baselines: `_sound_refresh_looping` is now
1120/43 (tag `size 1120!=1136`) and `_update_channel_for_looping_sound` is now 1360/61 (tag `[sha]` only).

## 3. Result

Nothing landed. The wave resolved the assigned relocation leverage to a **duplicated loop header**, i.e.
an extra *emission* of an existing call rather than an extra source statement, and produced one new
image-wide measurement (a loop-rotation census over all 833 January objects) plus one new measured-inert
loop spelling. It also confirmed, by running `relocdiff --count-by-target` over every non-exact row, that
**no other row in this unit has a missing or extra call, a wrong ABI/type or a frame-slot defect** — so the
owner directive for this wave excludes them all.

## 4. `_sound_dispose_from_old_map` — leverage resolved, no source lever

Target **240 padded / 236 meaningful / 21 relocs / `80892c74c0d90b7a`**; ours **272 / 23 /
`c54fba50726ce6f7…`**; 77 vs 89 instructions.

### 4.1 Relocation census

| target | JAN | OURS |
| --- | ---: | ---: |
| `_system_milliseconds` | 3 | **4** |
| `__real@4f800000` | 1 | **2** |
| `_sound_manager_globals` (+0/+1/+2×3/+8/+12) | 7 | 7 |
| `_sound_data` / `_data_next_index` | 2 / 2 | 2 / 2 |
| `_sound_start_fade`, `_sound_idle`, `_sound_stop_all`, `_looping_sound_data`, `_data_delete_all`, `__real@43960000` | 1 each | 1 each |
| TOTAL | 21 | 23 |

Both extras lie inside one contiguous 11-instruction block at our `0xa6..0xc3`.

### 4.2 That block is a second copy of the loop header, not a second statement

```
JAN   76 fild [ebp-4] ; 79 fadd __real@43960000 ; 7f fstp [ebp-4]      <- stop_time, same cell as start_time
      82 call _system_milliseconds ; 87 test eax,eax ; 89 mov [ebp-8],eax ; 8c fild [ebp-8]
      8f jge 0x97 ; 91 fadd __real@4f800000                            <- unsigned -> float fixup
      97 fcomp [ebp-4] ; 9a fnstsw ax ; 9c test ah,5 ; 9f jp 0xa8      <- single loop exit
      a1 call _sound_idle
      a6 jmp 0x82                                                      <- SINGLE header, unconditional back-edge
OURS  0x00..0xa5 byte-identical to January (guard copy at 0x82..0x9f, body at 0xa1)
      a6..c3  verbatim second copy of 0x82..0x9f ending `jnp 0xa1`     <- rotated: guard + do-while latch
      then byte-identical again to the end, shifted +0x17
```

Frame census: both `sub esp,8`; `[ebp-4]` holds `start_time` and then `stop_time` (the same cell on both
sides, `fstp [ebp-4]` @0x7f); `[ebp-8]` is the unsigned-conversion temp. No slot is missing or added.

So the +32 padded bytes, +12 instructions and +2 relocations are one post-IL **loop-rotation** decision.
The orchestrator's leverage is therefore not a missing statement, not a wrong ABI and not a frame defect —
stated plainly as the wave method requires.

### 4.3 New image-wide measurement: the loop-rotation census

`scratch/workers/n4_game_sound_sound_manager/rotscan.py` disassembles every code symbol in all 833 January
split objects and reports natural loops that keep a SINGLE header copy and close with an unconditional
back-edge. Re-join jumps are rejected by requiring that no branch originating outside `[header, jmp]` lands
inside it; the header walk skips internal conditional jumps (such as the unsigned fixup) so multi-block
headers are found. Each hit records header length, exit count and whether our `build/base` object matches
the function byte-for-byte. Output: `rotscan_exits.txt` — **47 unrotated loops whose header contains a call,
21 of them inside functions we already match EXACTLY.**

| unrotated loop (EXACT in our build) | hdr insns | exits | back-edges |
| --- | ---: | ---: | ---: |
| `_ai_conversation_update` | 6 | 2 | 1 |
| `_vehicle_moving_near_any_player` | 7 | 2 | 2 |
| `_deflate_slow` | 12 | 2 | 3 |
| `_qsort_2byte`, `_qsort_4byte` | 17 | 1 | 3 |
| `_scenario_ensure_point_within_world` | 19 | 2 | 1 |
| `_cache_copy_run_decompression` | 37 | 3 | 4 |
| `_rasterizer_draw_character_with_dropshadow` | 66 | 1 | 1 |
| `_lrar_flush` | 78 | 2 | 2 |
| `_heap_down` | 171 | 1 | 1 |
| **`_sound_dispose_from_old_map` (January)** | **10** | **1** | **1** |

Every EXACT unrotated donor has either **≥2 exits or ≥2 back-edges**, or a header far too large to duplicate
(66 and 171 instructions). January's wait loop is the only loop in the whole census that is unrotated with
one exit, one back-edge and a small header. Since our compiler reproduces all 21 donors byte-for-byte, this
is neither a flag nor a global policy difference. Image-wide there are exactly **two** functions where our
build rotates a loop January left unrotated: this one and `_load_symbol_table` (2 exits / 2 back-edges,
another worker's file, already a laws_w3 section D row).

### 4.4 Shapes

| id | shape | result |
| --- | --- | --- |
| d3 | `for (stop_time = (real)start_time + 300.f; (real)system_milliseconds() < stop_time; )` — the for-INIT spelling, never previously tried (w3c tried only `for(;c;)` and `for(;c;inc)`) | **byte-identical to baseline** (272 / 23 / `c54fba50…`) |

That makes **twelve** measured-inert loop spellings across seven lanes: `while`, `while{…continue;}`,
`while(TRUE)+break`, `for(;;)+break`, `for(;c;)`, `for(;c;sound_idle())`, `for(init;c;)`, inline deadline
expression, HCEA `any_sounds` flag, captured `paused`, deadline-local placement, whole-TU definition reorder.
No further shape was spent.

### 4.5 Reopen criterion (supersedes n3a's)

A source fact that gives the wait loop a **second exit or a second back-edge** without inventing a statement,
OR a measured VC7 rule for declining rotation on a one-exit / one-back-edge / ≤10-instruction header.
`rotscan.py` plus `rotscan_exits.txt` is the detector and the donor list. Do not re-spend loop spellings.

## 5. The other six rows — census clean, skipped as exhausted

`relocdiff --count-by-target` was run at HEAD on every non-exact row. **All six have an equal per-target
multiset** (the flagged rows are only the `defined-noncode:.bss` versus `symbol` spelling of
`_sound_manager_globals` and the `__real@…` constants, which relocdiff itself resolves equal; four rows
report "0 differing row(s)"). There is no missing/extra call, no wrong ABI and no frame-slot defect in any
of them, so this wave's owner directive excludes them.

| row | target vs ours | recorded verdict |
| --- | --- | --- |
| `_refresh_listener` | 496/25 vs 496/25, 178/178 insns | measured tie (w3c 5, n3a) |
| `_sound_find_like_channel` (P) | 272/16 vs 272/16 | measured tie (w3c 5, n3a) |
| `_sound_refresh_looping` (P) | 1136/43 vs 1120/43 | one tail-merge decision, laws_w3 A43 class (n3a 5.3) |
| `_sound_set_definition_end` (P) | 208/9 vs 208/9, 72 vs 73 insns | owner-blocked: the 16-bit third parameter of `sound_definition_find_pitch_range_by_pitch` was already ruled an ABI lie; laws_w3 R13 binds (n3a 6) |
| `_update_channel_for_impulse_sound` (P) | 528/29 vs 528/29 | measured tie (w3c 5) |
| `_update_channel_for_looping_sound` (P) | 1360/61 vs 1360/61 | two zero-extension instructions, same owner ruling (n3a 4.3) |

## 6. Proposals

None for this object. No header, config, symbols.json or park change is requested by this wave for
`sound_manager`.

## 7. Disposition

`sound_manager.obj` stays at 58/65 strict exact; this wave claims no bytes and changes no file. It converts
the assigned structural leverage from "extra `_system_milliseconds` call" to a proven duplicated-header
flow decision, adds a reusable image-wide detector for that decision, and closes the loop-spelling search
with a twelfth negative.
