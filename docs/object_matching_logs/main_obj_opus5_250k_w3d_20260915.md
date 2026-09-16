# `main.obj` opus5 250K house-clean lane, wave w3d - structural pass (2026-09-15)

Worker `w3d:main`. Unit `source/main/main.c`. **Real file unchanged** (`git diff --stat -- source/main/main.c`
empty). Notes with per-function report blocks: `scratch/workers/w3d_main.md`; candidates, micro-labs and the two
census helpers: `scratch/workers/w3d_main/`.

## Baseline and final

`gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 88  residual 6  unwritten 1`**; `_point_from_line3d` guard passed;
`scratch/parkcheck.py` 5 parks drift 0; `tools/fake_match_scan.py` 0 review leads.
Nothing landed: the three structural targets are PARKED and none reached strict EXACT, and `_main_crash` stays
owner-blocked. Baseline listing: `scratch/workers/w3d_main/base_main.txt`.

Duplicate prevention: read `main_obj_opus5_150k_w1_20260914`, `main_obj_opus5_100k_waveD_20260914`,
`main_obj_natural_state_and_render_packet_20260909`, `main_obj_lifecycle_and_fuzzy_reconstruction_20260909`,
`main_obj_jonas_crash_policy_closeout_20260826`, `small_unwritten_threshold_wave_main_hs_ai_20260913` and
`scratch/workers/tiny_stubs.md`. **waveD's scratch tree (`scratch/workersD/`) no longer exists**, so its
`best_update_time.c` / `frd3a.c` / `gr1.c` / `wc2.c` could not be reused; every finding below was re-derived from
January's bytes. w1's `branch_sweep` over 54 historical blobs (nothing better) was not repeated.

## Tooling added (reusable)

- `scratch/workers/w3d_main/realdiff.py <unit> <fn> <obj>` - alndiff with the relocation *offset* and the
  `defined-noncode:.rdata:` vs `symbol:` literal-owner spelling normalised away, so the printed count is genuine
  code difference. Raw alndiff row counts over-report by 3-4x on relocation-dense functions.
- `scratch/workers/w3d_main/slots.py <unit> <fn> <obj>` - per-`[ebp+-N]` reference census of both sides
  (the laws_w3 A24 detector; modelled on `W:w3_projectiles/slots.py`).

## Per-function outcomes

| Function | Padded | Baseline | Best this wave | Verdict |
| --- | ---: | --- | --- | --- |
| `_main_update_time` (parked) | 1440 | 1440 / **118** relocs, 65 blocks / 353 rows | **1440 / 117 relocs** (== January), 27 blocks / 86 rows, sha c6c6733d193740aa (`ut_v10.c`) | park re-baseline proposed |
| `_main_frame_rate_debug` (parked) | 544 | **528** / **51** relocs | **544** / 51 relocs, 3 blocks / 12 rows (`frd_v2.c`) | 8 B + 1 reloc short |
| `_main_game_render` (parked) | 448 | 464 / 29 relocs | 464 / 29 (one of the two defects closed, `gr_v4.c`) | not closer overall |
| `_main_crash` | 16 | UNWRITTEN | - | OWNER-BLOCKED (NULL-deref UB) |

### `_main_update_time` - size and relocation identity now both EXACT

Census (mandatory step): frame `sub esp,0x34` vs our `0x30` (one missing dword local); relocation count-by-target
differs in exactly one row, `_main_globals+950` 4 vs 5 (a second read of `vblank_interval_current` inside the
ignore-failure conjunction). Twelve byte-proven source defects were found and fixed (full list with the January
byte evidence in the notes file), the most load-bearing being:

1. `60 / (requested_rate ? requested_rate : 30)` - January's `jne` reuses the `test ax,ax` flags of the `>= 0`
   test and puts the default 30 in a 32-bit register, so the rate selection is an **int-typed conditional
   expression**, not an `if (rate == 0) rate = 30;` statement; the field read must stay inside the `&&` so it CSEs.
2. `short_target_index` and `best_interval = 5` are assigned **after** the first `_snprintf` (this alone restores
   the missing frame slot and took the frame from 0x30 to 0x34).
3. the failure-count clamp is `x = counts[slot]; if (x >= 99) x = 99;` - cseries `MIN` is `((a)>(b)?(b):(a))` and
   emits the 99 store first, January stores the value first and overwrites (`jl`).
4. `if (!ignore_failure) {record} else {label = "ignor";}`, `if (target >= last + 15) {"ok"} else {"wt"/"dn"}`,
   `counts[slot] >= 4 ? "dn" : "wt"` - three arm orders each proven by which block January makes the fall-through.
5. `slot >= vblank_interval_requested`, `(short)(current_interval / 2) <= slot_bucket`,
   `(__int64)rasterizer_globals.frame_and_vertical_blank_index > target_index`, strict `>` for
   `throttle_start_index > initial_index + slot`, and `" des %d"` printing `best_interval`.

Whole-TU gate of `ut_v10.c`: 88/6/1 with every baseline-EXACT row unchanged, the other four parks drift 0, guard
passed, fake scan clean, and the owner census versus `build/base` shows no new owner. The remaining 27 blocks are
callee-saved assignment of the 64-bit `target_index` pair, one 16-bit-vs-32-bit load of a short struct field whose
upper half is padding, the conditional-expression format-string phi (per-arm push vs a register), and the
placement of the cold `"ignor"` block.

### `_main_frame_rate_debug` - size closed, one relocation short

Census: frame identical (`sub esp,0x214`); exactly two relocation rows differ - `_global_screenshot_count+902`
(`framerate_sample_index`) 4 vs 3, and the threshold literal `__real@3fa26e978d4fdf3c` vs our
`__real@3fa26e978d4fdf3b`. Three byte-proven defects were fixed:

- the threshold is the **folded value of an expression** (0.036000000000000004), not the literal `0.036`;
  `1.08/30.0`, `36*0.001`, `3.6/100.0` fold to it, `0.036`, `36.0/1000.0`, `0.03*1.2` do not (measured).
  The spelling itself is not byte-identifiable - `1.08 / 30.0` was used and is disclosed as a choice.
- the ring-buffer wrap is a **signed** `% 8` (`movsx`, `and eax,0x80000007`, `jns/dec/or/inc`, 15 B);
  `% NUMBEROF(...)` makes the expression `size_t` and collapses to `and dl,7` (3 B) - 12 of the 16 missing bytes.
  (R7 forbids `% (long)NUMBEROF(x)`, so the divisor has to be a genuine signed constant.)
- the index is incremented at **char width first** (`inc dl; movsx eax,dl`), i.e. `sample_index++;` as its own
  statement; `(sample_index + 1) % 8` reuses the already-extended index and also loses January's `mov eax,1`
  hoisted above the SET_FLAG branch.

After those, the only difference in the whole function is that January emits the `framerate_sample_index` load
TWICE with different successors (0x56 on the `!framerate_reset` edge, before the second `debug_frame_rate` test,
and 0x6a on the `framerate_reset && debug_frame_rate` edge, which is jump-threaded straight into the body) while
our build cross-jumps the two identical one-instruction loads into one block - 8 bytes and one relocation.
All four admissible CFG spellings of the predicate (nested both ways, `&&`, `||`) are byte-identical; every
placement that moves the load out of the pre-test position sinks it to a single copy (frd_v3/v7/v8/v10, 528 B).

### `_main_game_render` - the width defect is solved, the tail duplication is not

Census: frame identical (`sub esp,0x14`, five slots, same roles); exactly one relocation row differs,
`_observer_get_camera` 1 vs 2; instructions 0..66 are byte-identical to January.

- **`last_local_player_index` is a `short`, not a `long`.** January stores the raw return of the short-returning
  `local_player_get_next` (`mov dword ptr [ebp-0xc], eax`, no `movsx`) yet compares and stores 16-bit. Micro-labs
  (`lab/l1.c`, `lab/l2.c`) show VC7 always emits `movsx` for `long x = short_fn()`, and that a **`short` local
  whose upper half is never read gets a full dword slot with dword stores** - exactly January's pattern. Stian
  (`main.c:4224 __int16 next_player`) and HCEA (`int16_t last_assigned_player`) agree. `gr_v4.c` closes that hunk.
- The remaining defect is that VC7 tail-duplicates the 5-instruction shared observer tail into the film-playback
  arm. Micro-labs `lab/l3.c`-`l5.c` reproduce both behaviours exactly: a 5-instruction / 17-byte join block is
  duplicated into a fall-through predecessor, a 6-instruction / 20-byte one is not, and January's sixth
  instruction is `mov dword ptr [ebp-0x14], eax` - i.e. January does not register-forward `observer` across the
  join. Refuted causes: observer live-out of the loop (January pushes literal 0 for the console window and for
  `render_frame`), loop-scope declaration, arm polarity, NULL assigned in the arms, `console_window` before
  `set_window_camera_values`, and the HCEA flat else-if chain (480/30, worse).
- `gr_v4` is not strictly closer overall (the short type shifts the console-window register rotation), so nothing
  is proposed for this park; the finding and the reopen criterion replace waveD's.

### `_main_crash`

Unchanged: the 16 bytes are a store of a literal's address through absolute address 0. Brief section 5 and the
crash-policy closeout forbid reproducing NULL-dereference UB and no new provenance appeared this wave.

## Not spent this wave

`_main_get_window_count` (tail-merged conditional-arm constant block placement) and `_compute_subframe_counts`
(`setl/test/je` for a loop-body if) were left untouched - they are not [size]/[relocs] rows and neither waveD's
reopen criterion has been met by a new law. `_main_load_last_solo_map` (the only non-parked residual) is the
recorded ESI/EDI callee-saved permutation, exhausted in waveD and w1.

## Proposals

1. **Park re-baseline `_main_update_time`** to `scratch/workers/w3d_main/ut_v10.c` (or the combined file
   `scratch/workers/w3d_main/combined_ut_frd.c`): 1440 / **117** relocs / sha `c6c6733d193740aa` versus the
   recorded base 1440 / 118 / `5f8cfd1a...`. Strictly closer on every axis (relocation identity now equals
   January; 353 -> 86 differing instruction rows; the recorded objdiff base was 62.5%). Zero strict credit.
2. **Park re-baseline `_main_frame_rate_debug`** to `scratch/workers/w3d_main/frd_v2.c`: **544** / 51 /
   sha `3c132f904203944a` versus the recorded base 528 / 51 / `5608160797d3d13d`. Size now equals January and the
   owner census picks up January's own `__real@3fa26e978d4fdf3c` literal COMDAT in place of our `...3b`.
   Zero strict credit. Discloses the un-identifiable threshold spelling (`1.08 / 30.0`).
3. No header or config change is needed by anything in this wave.

## Reopen criteria (replacing waveD's)

- `_main_update_time`: start from `ut_v10.c`; what remains is the callee-saved assignment of the 64-bit
  `target_index` pair (January keeps the low half in EBX and reloads the high half from its home after the
  throttle-flag store) and the format-string conditional-expression phi (per-arm `push` versus a materialised
  register).
- `_main_frame_rate_debug`: a natural source form in which the `framerate_reset && debug_frame_rate` path reaches
  the sample body without re-testing `debug_frame_rate`, so the two `framerate_sample_index` loads keep different
  successors and are not cross-jumped. Everything else already matches byte for byte in `frd_v2.c`.
- `_main_game_render`: a source form that makes VC7 keep the `observer` store at its definition (memory home, not
  EAX forwarding) with the loop's register pressure unchanged; with `short last_local_player_index` the rest of
  the function is byte-identical to January.
- `_main_crash`: an explicit owner ruling on intentional crash routines (unchanged).

## Candidate index (`scratch/workers/w3d_main/`)

`ut_v1..ut_v10.c` (update_time), `frd_v1..frd_v10.c` (frame_rate_debug), `gr_v1..gr_v8.c` (game_render),
`combined_ut_frd.c` (both park re-baselines in one file), `lab/l1.c..l5.c` + `lab/run.py` (micro-labs),
`realdiff.py`, `slots.py`, `base_main.txt`, `ut_v10_all.txt`, `frd_v2_all.txt`, `combined_all.txt`.
