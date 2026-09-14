# profile.obj — Opus5 150K house-clean lane, wave w3 (2026-09-14)

## Outcome

`source/cseries/profile.c` is **unchanged** (`git diff --stat` empty); no header or config file was touched.
Both remaining non-exact functions are parked, so all work was scratch-only.

| gate | exact | residual | unwritten | guard | park drift |
| --- | ---: | ---: | ---: | --- | --- |
| baseline real file | 42 | 2 | 0 | pass | 0 |
| final real file (unchanged) | 42 | 2 | 0 | pass | 0 |
| scratch `scratch/workers/w3_stack_walk_windows_profile_etc/p1.c` | **43** | 1 | 0 | pass | 1, to the target hash |

| function | January padded / relocs | disposition |
| --- | --- | --- |
| `_compare_profile_sections` | 288 / 8 | **park-reopen proposed** (strict EXACT in scratch; owner-ruling class: post-fatal-assert unassigned result) |
| `_profile_frame_get_value` | 928 / 50 | not landed; no new evidence after the w2 R2 rejection |

## Duplicate prevention

Read: `profile_obj_dump_comparator_natural_boundary_20260905.md` (park source), `profile_obj_frame_value_getters_natural_boundary_20260905.md`,
`profile_obj_opus5_150k_w2_20260914.md`, `scratch/workers/profile.md`, the w2 verifier verdicts in `scratch/w2/w2-results.json`,
`scratch/w2/laws_w2.md`. Neither function is in the 100K admission manifest; git history holds no exact body.

## `_compare_profile_sections`

**January bytes.** Every mode path returns directly. The invalid-mode path runs `display_assert` and `system_exit(-1)` and then
`mov eax,[ebp+8]`: the return of the unassigned result variable, whose home VC7 coalesced with the first parameter slot (the
same slot the average-time path uses for its `fidiv` sample-count temporary).

**Why this is reopened now (new since the park and since w2).**
1. The w2 verifier **accepted** the identical construct for `bitmaps.obj::_bitmap_format_to_a8r8g8b8` (landed; see also
   `bitmap_mipmap_address`): an uninitialized `result` assigned in every switch arm, a fatal assertion in `default:`, and a terminal
   `return result;`, reasoning that the read can only happen if `system_exit` returns. That is the R5 contrast clause in `laws_w2.md`.
2. Marathon, the house style exemplar, uses the same idiom: `marathon2/shapes.c` `get_shading_table_size` (`long size;` assigned per
   case, `default: halt();`, `return size;`) and `marathon2/render.c` `update_render_effect`.
3. No defined form is equally exact: the w2 single-exit body with `result = 0` after the assertion (`scratch/workers/profile/c1.c`)
   tail-merges that zero with the equal-values return and stays non-exact (R5 contrast holds).

The same-TU-precedent clause of the bitmaps admission is **not** met here (profile.c has no other instance), so this needs an explicit
orchestrator/owner ruling.

**Candidate p1** (the current real file plus this body only; CRLF preserved):
- `int result;` without the dead `= 0` initializer;
- explicit `else result = 0;` arms in the average-time and total-time modes, so every reachable path assigns;
- default arm unchanged (`match_assert(..., 844, !"unreachable"); break;`), terminal `return result;` kept;
- comment: "January relies on the fatal unreachable assertion for an invalid sort mode; there is no fallback result if system_exit
  unexpectedly returns."

**Measurements.** Strict EXACT 288/8, sha `d94ba79bf813177b`. Whole-TU gate 43/1; row diff vs baseline: only this row changes. Guard
passes. `scratch/parkcheck.py`: only this park drifts, to the target; `_profile_frame_get_value` unchanged (928/50). relocdiff 8/8 with 0
differing rows. Owner census 146 = base 146, 0 candidate-only, 0 lost, 0 COMMON (13 literal COMDATs vs target predate this work).
Non-code sections identical to the baseline object except `.debug$S`. Fake scan 7 leads = HEAD (QUERY_TIMEBASE asm).

**Reopen criterion if rejected:** an owner ruling that extends the bitmaps post-fatal-assert admission beyond a same-TU precedent.

## `_profile_frame_get_value`

The w2 both-arm body (EXACT) was rejected under R2. The verifier asked for a strict-exact single-arm spelling or January-side
corroboration. The w2 laws were run as detectors (A11, A13, A16, x87 PHI); none predicts January's `mov bl,[ids]; test bl,bl; lea; fadd; je`
from a single-arm statement order, and the six single-arm labs from w2 already cover the natural spellings. No new shapes were
spent. Reopen criterion unchanged.

## Proposals

- park-reopen `_compare_profile_sections` (candidate `scratch/workers/w3_stack_walk_windows_profile_etc/p1.c`), ruling required as above.
- No header or config proposals.
