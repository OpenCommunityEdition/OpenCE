# `main.obj` opus5 100k wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/main/main.c`, target `build/split/source/main/main.obj` (January 2002 Xbox debug
  build, VC7 13.00.9254, repository `build.ninja` flags `/O2 /Oy- /DDEBUG /Dxbox` plus include paths).
- Evidence: January target COFF (aligned Capstone streams, relocations, literals), the January debug-map atlas
  records for `main.obj` (dumped read-only to `scratch/workersD/main/atlas_main.obj.txt`; no prepared
  atlas/Ghidra output existed for this object), HCEA `src/blam/main/*.c` (names, topology, local types only),
  Stian `src/halo/main/main.c` (topology only), and `CL /Zs /W3` warnings on the unit.
- Ten of the eleven residuals are PARKED. Every park experiment was made on scratch copies; the real source file
  is unchanged (`git diff --stat -- source/main/main.c` empty). Worker notes with per-function blocks:
  `scratch/workersD/main.md`; candidates and objects: `scratch/workersD/main/`.
- Duplicate prevention: all Main ledgers in this directory were read; all 53 historical blobs of
  `source/main/main.c` were compiled through `gate.py --source` (`scratch/workersD/main/sweep.py`): none has any
  of the 12 non-exact rows exact (19 blobs no longer compile).

## Validated baseline (lane HEAD)

`gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d`: exact 83, residual 11, unwritten 1
(95 functions). Residual measurements equal `config/parked.json` `measurements.base`; the non-parked residual
`_main_load_last_solo_map` is 192/192 padded with identical relocations.

## Results

### A. Park-reopen set (strict EXACT in scratch) - `scratch/workersD/main/park_reopen_combined.c`

Combined gate: **exact 88, residual 6, unwritten 1**; point guard passed; every baseline-EXACT row still EXACT;
only the five reopened parks change (each to its January normalized hash), the other five parks equal their base
measurements; owner census: no new defined symbol versus `build/base`; fake scan 0 leads.

| Function | Padded / relocs | Evidence that closed it |
| --- | ---: | --- |
| `_main_get_solo_level_from_name` | 368 / 22 | Only difference was `mov ax,N` vs January `mov eax,N` on ten returns. The exact caller `_main_won_map_private` proves a 16-bit visible return (`xor edi,edi; mov di,ax`; a `long` return plus casts folds to `mov edi,eax`). Single-exit law: a `short level` result assigned in an if/else-if chain and returned once; VC7 keeps per-branch epilogues. |
| `_main_setup_connection` | 192 / 17 | HCEA topology: `if (playback) want_menu = FALSE; if (want_menu) { main_menu_load(); return; } if (playback) {film} else {local}`. VC7 threads the known-false test into the film branch, whose menu load falls to the function end, so January's two `main_menu_load` tails are distinct and not cross-jumped. January's immediate error in the film branch is kept. |
| `_main_change_map_name` | 384 / 35 | The sunk not-loaded arm (`timer = 0`) jumps past the `frame_start < timer` compare: the compare is a separate statement after the if/else, jump-threaded for timer 0. Single-exit: teardown guarded by `if (frame_start >= timer)`, no early return. |
| `_compute_window_bounds` | 496 / 25 | HCEA declares the inset `int16_t gutter`; `short safe_frame_inset` changes the implicit conversion in `horizontal_index*safe_frame_inset`, which decides the commutative imul copy. Operand order and declaration order are inert. |
| `_set_window_camera_values` | 432 / 19 | (a) `CL /W3` C4013: `player_effect_get_camera_effect_matrix` was implicitly declared, so its short argument was promoted (`movsx`); January's `xor ecx,ecx; mov cx,[edi]` needs the owner prototype -> `#include "effects/player_effects.h"`. (b) January addresses `window->rasterizer_camera.<field>` directly ([edi+0x64]/[edi+0x70]/[edi+0x80]...), no camera pointer local. |

### B. Parks investigated and left locked

| Function | Best scratch | Result | Remaining residual |
| --- | --- | --- | --- |
| `_main_get_window_count` | `wc2.c` | 64/5, 22/22 insns identical | January's `mov eax,1`/`mov eax,4`/`movsx eax,ax` prove `return guard ? 1 : PIN(local_player_count(), 1, MAXIMUM_WINDOWS)` (int ?: arms, three calls). Only the placement of the tail-merged `mov eax,1; ret` block differs (after the cinematic test vs at the end). |
| `_main_game_render` | `gr1.c` | 464/29 | January's `last_local_player_index` is `short` and the player arm is one shared store+observer tail; VC7 still duplicates the tail into the film path and carries `observer` in EAX across the join (January homes it in its slot). |
| `_main_frame_rate_debug` | `frd3a.c` | 544/51 | Proven: signed `% 8` (not size_t NUMBEROF), char-width `sample_index++` before the modulo, threshold literal is folded `0.036000000000000004` (`1.08/30.0`, `36*0.001`, ... all fold to it; spelling not identifiable). Remaining: January keeps the reset path's zero as a DL phi and a separate load for the reset&&debug path (52nd reloc). |
| `_compute_subframe_counts` | park body | 112/4 | Correction to the park text: the `setl/test/je` is the inner `horizontal<vertical` test, not the loop predicate. A boolean temporary folds; `horizontal-vertical<0` is worse. |
| `_main_update_time` | `best_update_time.c` | 1440/117 (was 118), 21 differing insn lines (was ~78 hunks) | Decoded two target-index variables, int `?:` rate, MIN(99, x) keep-when-less, strict `start > initial + slot`, "ok" then-arm, `slot >= requested`, `best > current ? FAILDOWN : ...`, " des %d" prints best_interval. Remaining: flag store position, sunk "ignor" arm, FAILDOWN ?: argument evaluation order, throttle-flag store position, tail register pairing. |

### C. Non-parked rows

- `_main_load_last_solo_map` (192/14): callee-saved register swap only (January file->ESI, count->EDI). Function-top
  declarations in both orders and a `MIN` clamp are byte-identical to the current body. Not landed.
- `_main_crash` (16): intentional null write; undefined behaviour per the crash-policy ledger. Not written.

## Do-not-repeat list

- `long` return for `main_get_solo_level_from_name` (with or without `(short)` caller casts): breaks
  `_main_won_map_private`.
- Window count: separate early returns, default-1 local with negated guard, if/else returning 1/PIN (80 bytes).
- Game render: goto form, loop-scope observer, swapped arm polarity (all 464/29).
- Frame-rate debug: `if (reset && !debug)` restructure, declaration-initialised sample index (528).
- Subframe counts: boolean temporary for the inner test, subtraction-sign test.
- Window bounds: operand swaps, HCEA declaration order (inert).
- Update time: moving `ignore_failure = FALSE` ahead of target_age (breaks the vblank_interval_current CSE);
  statement swaps at the top/tail (1456 bytes, loop alignment); ignor then-arm polarity.
- Adding owner includes for every C4013 in main.c keeps all rows but flips the `_main_update_time` tie hash.

## Reopen criteria

- Window count: a VC7 law for where a tail-merged `?:`-arm constant block is placed.
- Game render: a law for when a loop-body pointer local is homed at a join instead of register-carried.
- Frame-rate debug: a law for keeping a zero assigned to a char local as a phi; authentic threshold spelling.
- Subframe counts: a natural source form that keeps `setl/test/je` for a loop-body if.
- Update time: start from `best_update_time.c`; remaining items are placement/evaluation-order ties.

## Disposition

No strict-exact function landed in the real file this wave (final gate identical to baseline: 83/11/1, no park
drift). Five parks are proposed for reopening with scratch strict-exact bodies (section A); the orchestrator
unparks and lands them. No fuzzy credit is claimed.
