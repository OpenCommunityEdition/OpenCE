# `main.obj` - opus5 next-150K lane, wave n5 (LAW Z pass, 2026-09-16)

Worker `n5:main`. Unit `source/main/main.c`. **Real file unchanged** (`git diff --stat -- source/main/main.c`
empty). Per-function report blocks: `scratch/workers/n5_main.md`. Candidates, probes and the probe harness:
`scratch/workers/n5_main/` (`v1.c` = a verbatim copy of w3d's `frd_v2.c`, `p0..p3.c`, `d1.c`, `d2.c`,
`probe.py`, `run1.py`, `run2.py`, `jan.txt`, `base_all.txt`, `final_all.txt`).

## Baseline and final

`gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 88  residual 6  unwritten 1`** (the two listings diff clean, row for row);
`_point_from_line3d` guard passed; `scratch/parkcheck.py` 5 parks / drift 0
(`_main_game_render` 464/29, `_main_get_window_count` 64/5, `_compute_subframe_counts` 112/4,
`_main_frame_rate_debug` 528/51, `_main_update_time` 1440/118); `tools/fake_match_scan.py source/main/main.c`
0 review leads. Nothing landed; no exact byte is claimed.

Duplicate prevention: read `main_obj_opus5_next150_n4_20260915`, `main_obj_opus5_next150_n3a_20260915`,
`main_obj_opus5_250k_w3d_20260915`, `main_obj_opus5_150k_w1_20260914`, `main_obj_opus5_100k_waveD_20260914`,
`main_obj_jonas_crash_policy_closeout_20260826`, `main_obj_jonas_window_count_rejection_20260828`, plus
`scratch/workers/n3_main.md` and `scratch/workers/w3d_main.md`; every prior candidate object in
`scratch/workers/w3d_main/` and `scratch/workers/n3_main/` was re-measured rather than re-derived.

## LAW Z applied as a detector across the whole unit

`python -B scratch/hs_pin/webprofile.py source/main/main <fn>` was run on every non-exact row:

| Function | January ebx | Ours ebx | Zero pin present? |
| --- | --- | --- | --- |
| `_main_frame_rate_debug` | 1 def @0x0f, **16** uses | 1 def @0x0f, **17** uses | YES - the only pin row in the unit |
| `_main_load_last_solo_map` | 0 defs / 0 uses | 0 defs / 0 uses | no |
| `_main_game_render` | 3 defs / 5 uses | 3 defs / 5 uses | no (general web, identical) |
| `_main_update_time` | 10 defs / 14 uses | 8 defs / 9 uses | no (general web) |
| `_compute_subframe_counts` | 0 defs / 3 uses | 0 defs / 3 uses | no |
| `_main_get_window_count` | 0 defs / 0 uses | 0 defs / 0 uses | no |

So LAW Z reclassifies exactly one row in this unit, `_main_frame_rate_debug`; the register-tie verdicts on
the other five are untouched by it and their recorded reopen criteria stand unmet.

## `_main_frame_rate_debug` - the pin row is a single constant-folded store, and the residual is a CFG fact

**The orchestrator's leverage statement ("ours is 16 bytes smaller, 150 vs 156 instructions, statements are
MISSING") describes the REAL-FILE parked body, and w3d already closed that gap in scratch.** w3d's `frd_v2.c`
(copied here as `v1.c`) fixes all three byte-proven defects - the folded threshold
(`1.08/30.0` -> `__real@3fa26e978d4fdf3c`), the signed `% 8` instead of `% NUMBEROF(...)`, and the char-width
`sample_index++` as its own statement - and measures **544 padded / 536 raw / 51 relocations**, i.e. size
identity with January.

Against `v1.c`, `scratch/workers/w3d_main/realdiff.py` reports 36 blocks with **3 genuinely differing**
(12 rows), and only two of them are real:

1. `@0x3c` January `mov byte ptr [_global_screenshot_count+902], dl`; ours `..., bl`.
   **This is the entire LAW Z pin-use surplus (17 vs 16).** It is *not* a surplus zero-valued statement:
   January emits exactly the same store. The difference is that in our build VC7 constant-folds
   `global_screenshot_count.framerate_sample_index = sample_index` (the local is provably 0 on that path) and
   then allocates the zero pin EBX for the byte store, while January keeps the local's own register DL.
   Both builds emit `xor dl, dl` at 0x33 for the join value, so the local and its live range are identical.
2. `@0x68..0x6a` January has `jmp 0x70` plus a **second** `mov dl, byte ptr [...+902]`; ours has neither.
   8 bytes and the 52nd relocation.

There is therefore no surplus source statement to delete, and per the brief the pin row must not be bought
with a redundant store, carrier, `volatile`, `register`, asm, fake dependency or operand permutation - the
two shapes that do change the store's operand (p1, p3) both destroy `xor dl,dl` and are strictly worse.

### Shapes measured this wave (6, scratch only)

| id | shape | size / relocs | realdiff |
| --- | --- | ---: | --- |
| p0 | `v1.c` control | 544 / 51 | 3 blocks, 12 rows |
| p1 | `...framerate_sample_index = sample_index = 0;` (chained assignment) | 544 / 51 | 4 blocks, **14** rows; loses `xor dl,dl`, store still `bl` |
| p2 | field write-back moved after the if/else join | 544 / 51 | 4 blocks, **18** rows |
| p3 | `framerate_sample_index = 0;` then `sample_index = framerate_sample_index;` | 544 / 51 | 4 blocks, **15** rows; loses `xor dl,dl` |
| d1 | `if (reset && !debug) CLEAR else if (!reset) S else if (debug) S` | 544 / 51 | 3 blocks, 12 rows - **arms merged** |
| d2 | `if (reset && !debug) CLEAR else { if (!reset) S else S }` | 544 / 51 | 3 blocks, 12 rows - **arms merged** |

### LAW n5-1 (strengthens n3a-1 and closes the family)

n3a framed the obstruction as a *layout* conflict ("two copies of the `framerate_sample_index` load" versus
"laid out after the clear block"). Dumping `f1.c` - the only shape that keeps two copies - shows the real
obstruction is a **control-flow** fact:

```
f1.c    : S1 @0x15 -> jmp test ; S2 @0x25 -> jmp test ; CLEAR @0x2d ; test @0x64 ; body @0x70
January : CLEAR @0x1d ; S1 @0x56 -> falls into test ; test @0x5c ; jmp 0x70 ; S2 @0x6a -> falls into body
```

In `f1.c` **both** copies' successor is the second `debug_frame_rate` test. In January, `S2`'s successor is
the body, and the test's own fall-through has to `jmp 0x70` *over* `S2`. January's object therefore contains a
conditional-branch-threaded edge: on `framerate_reset != 0 && debug_frame_rate != 0` the sample body is
reached without re-testing `debug_frame_rate`. No permutation of C arms produces that, because a C `if`
always routes its arms into the following statement. d1 and d2 supply the missing datapoint that n3a lacked:
with the CLEAR block in the FIRST arm, the identical `S` arms are **always** merged (12 rows, byte-identical
to `v1.c`), so "CLEAR first" and "two copies" are unreachable together *and* even f1's two copies have the
wrong successor. Together with n3a's four spellings, w3d's ten and waveD's two, the family is closed.

Ghidra (`scratch/ghidra/out/main.obj.decomp.c:1339`) renders the threaded edge as
`if (_debug_frame_rate != '\0') goto LAB_000045c0;` inside the reset block - the same finding expressed as an
irreducible goto. It is a decompiler artifact of the threading, not evidence for a `goto` spelling: a source
`goto` would still need the CLEAR arm to carry its own copy of the second test, which January does not have.

**No park reopen is proposed** - nothing reached strict EXACT.

## `_main_load_last_solo_map` - the unit's only non-parked residual, confirmed exhausted

192 padded / 12 relocations on both sides, 54 instructions on both sides, relocation identity equal. LAW Z
does not reach it: **ebx has 0 defs and 0 uses on both sides**, so there is no pin and the orchestrator's
reclassification does not apply. The residual is an ESI/EDI permutation of two genuine call-crossing webs
with identical reference counts:

- January: `file` = 1 def (`mov esi,eax` @0x33) + 3 uses (`test`, `push` @0x3d, `push` @0x51) -> **ESI**;
  `character_count` = 2 defs (`mov edi,eax` @0x52, `mov edi,0xff` @0x64) + 2 uses -> **EDI**.
- Ours: exactly reversed.

Both builds shrink-wrap the saves identically (the outer web's `push` at 0x23, the inner web's at 0x3c, pops
in reverse order at 0x89 / 0xb1). The `push edi` / `push esi` pair at 0x3c-0x3d that `alndiff` reports as an
insert plus a delete is the callee-save push meeting the `fread` FILE* argument, **not** an argument-order
difference - worth recording, because it reads like a source signal and is not one. waveD (declaration order
both ways, a `MIN` clamp) and w1 (declare+initialise `file` / `character_count`, either at function top, the
validity flag scoped inside the `if`) already spent the shape budget; no law in `laws_w2` / `laws_w3` is a
detector for a two-web callee-saved tie with equal reference counts.

## Not spent (recorded exhausted, no new evidence from LAW Z)

`_main_game_render` (448, census re-verified by w3d, n3a and n4; the residual is the 6-instruction join
threshold on the `observer` home store, and every construct that keeps `observer` live across the join is a
synthetic lifetime anchor), `_main_update_time` (1440, `ut_v10.c` already 1440/**117**),
`_compute_subframe_counts` (112, 19 spellings in n3a, all natural forms fold `setl/test/je` to `jge`),
`_main_get_window_count` (64, park premise refuted by n3a but the `long` return trades 3,312 exact bytes in
`interface.obj`, `rasterizer_xbox_draw_primitives.obj` and `rasterizer_xbox_screen_effect.obj`, which brief
section 7 forbids), `_main_crash` (16, intentional NULL-dereference UB, owner-blocked since 2026-08-26).

## Proposals

1. **No park reopen**, no header change, no config change from this unit.
2. Still outstanding from w3d and repeated by n3a and n4 (not re-litigated here, re-verified this wave):
   the zero-credit park re-baselines of `_main_update_time` to `scratch/workers/w3d_main/ut_v10.c`
   (1440 / 117 relocs, versus the recorded base 1440 / 118) and of `_main_frame_rate_debug` to
   `scratch/workers/w3d_main/frd_v2.c` (**544** / 51, versus the recorded base 528 / 51). Both are strictly
   closer on size and/or relocation identity; `frd_v2.c` additionally picks up January's own
   `__real@3fa26e978d4fdf3c` literal COMDAT in place of our `...3b`.

## Reopen criteria

- `_main_frame_rate_debug`: **superseded** - reopen only on new evidence of a VC7 13.00.9254
  conditional-branch-threading trigger that lets the `framerate_reset && debug_frame_rate` path reach the
  sample body without the second `debug_frame_rate` test in its successor chain. Another CFG spelling of the
  same two tests is not new evidence (n5-1). Do not re-spend on the arm-order family, on the `xor dl,dl` /
  `mov [902],dl` operand, or on the three defects w3d already fixed.
- `_main_load_last_solo_map`: a measured mechanism that ranks two equal-reference callee-saved webs, or
  authoritative January source. Do not re-try declaration order, function-top declarations, `MIN`, or
  scoping the validity flag (waveD, w1).
- `_main_game_render`, `_main_update_time`, `_compute_subframe_counts`, `_main_get_window_count`,
  `_main_crash`: unchanged from n3a / n4.
