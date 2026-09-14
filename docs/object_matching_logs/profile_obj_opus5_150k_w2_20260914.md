# profile.obj — Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and outcome

Unit `source/cseries/profile` (January `profile.obj`, 44 code owners). Only `source/cseries/profile.c` was in scope.
All three non-exact functions are parked, so every experiment was scratch-only. **The real file is unchanged**
(`git diff --stat -- source/cseries/profile.c` is empty) and no header or config file was touched.

| gate | exact | residual | unwritten | point guard | park drift |
| --- | ---: | ---: | ---: | --- | --- |
| baseline real file | 41 | 3 | 0 | pass | 0 |
| final real file (unchanged) | 41 | 3 | 0 | pass | 0 |
| scratch `scratch/workers/profile/f2.c` (both reopens) | **43** | 1 | 0 | pass | 2, both to the target hash |

| function | January padded / relocs | disposition |
| --- | --- | --- |
| `_profile_dump` | 864 / 44 | **park-reopen proposed**: strict EXACT in scratch |
| `_profile_frame_get_value` | 928 / 50 | **park-reopen proposed**: strict EXACT in scratch; there is a credibility caveat for the verifier |
| `_compare_profile_sections` | 288 / 8 | owner-blocked: the residual is January's uninitialized return after a fatal assertion |

There are three scratch candidates. Each is the current real file plus only the reopened bodies (CRLF preserved):

| candidate | contents | gate |
| --- | --- | --- |
| `scratch/workers/profile/d4.c` | dump reopen only | 42 / 2 |
| `scratch/workers/profile/g1.c` | getter reopen only | 42 / 2 |
| `scratch/workers/profile/f2.c` | both reopens | 43 / 1 |

The two reopens are independent of each other.

## Duplicate prevention

Ledgers read:
- `profile_obj_dump_comparator_natural_boundary_20260905.md`, the park source for the dump and the comparator;
- `profile_obj_frame_value_getters_natural_boundary_20260905.md`, the park source for the getter;
- `profile_obj_getters_header_consumer_and_data_audit_20260905.md`;
- `profile_obj_jonas_dump_original_close_20260827.md`;
- `sort_obj.md`, for the comparator ABI;
- `opus5_100k_consolidated_HANDOFF_20260914.md`.

None of the three functions is in the 100K admission manifest. `git log --all -- source/cseries/profile.c` returns 40 commits, and none of them contains an exact body for these functions. `branch_sweep.py` found no historical blob above the current 41 exact.

## `_profile_dump`: explicit-cast CSE-kill law

**Residual at the park.** The residual is a single x87 site at target `+0x252`. `(double)profile_globals.timebase_frequency` is CSE'd onto the x87 stack: it is loaded once and consumed through `st(1)` by five divisions.
- January keeps the value through the last accumulation: `fdiv st(1); fadd [frame_msec]; fstp [frame_msec]; fstp st(0)`.
- The park body pops it at the last division: `fdivrp st(1)`.

**New measured law (VC7 13.00.9254, profile.c cflags; labs `scratch/workers/profile/lab3.c` and `lab7.c`).** In a compound assignment `x += expr` whose `expr` is the last consumer of an x87 CSE:
- an explicit `(real)(expr)` cast lets VC7 kill the CSE inside the expression, giving a popping `fdivrp`;
- the implicit double-to-real conversion (`x += expr;`) keeps the CSE until after the store, followed by `fstp st(0)`.

Lab results:

| lab function | spelling | result |
| --- | --- | --- |
| f10 | no casts | delayed pop |
| f2 | all casts | `fdivrp` |
| f12 | casts on the first two statements only | delayed pop |
| f13 | cast on the last statement only | `fdivrp` |
| f14, f15 | plain `=` assignments | delayed pop with or without casts |

So January's `+0x25a fstp st(0)` proves that the final `frame_msec +=` statement had no explicit narrowing cast.

**Shapes** (five, all whole-TU gates):

| id | shape | result |
| --- | --- | --- |
| d1 | named `double timebase_frequency` local in the do body | 880/45; VC7 builds a `1/x` reciprocal; reverted |
| d2 | `++section_index` in the while condition | inert |
| d3 | `frame_calls` accumulated after `frame_msec` | worse (moves the integer schedule; confirms the calls-then-time statement order) |
| **d4** | remove the four `(real)(...)` wrappers on `total_time`, `average_msec`, `peak_msec`, `frame_msec` | **EXACT 864/44, sha 1b5a353ac0e64397** |
| d5 | remove only the `frame_msec` cast | EXACT; the other three casts are byte-inert, so d4 is the uniform spelling |

The change in d4 (the `average_msec` term keeps its precedence parentheses):

```c
total_time += sections[section_index]->total_elapsed_timebase/
	(double)profile_globals.timebase_frequency;
...
frame_msec += sections[section_index]->frame_elapsed_timebase*1000.0/
	profile_globals.timebase_frequency;
```

**Why the park criterion is met.** The park asked for "authentic source/type/compiler-context evidence, not FP schedule searches". This change is not a schedule search. It removes reconstruction-added casts under an isolated, reusable compiler law, and one targeted spelling closed the function. The semantics are identical: the conversion to `real` happens at the store either way.

## `_profile_frame_get_value`: common-arm hoisting law

**Residual at the park.** The only divergence is in the `render0_3np` loop.

January's loop body:

```
mov bl,[ids]; test bl,bl; lea eax,[eax+eax*2]; fadd [esi+eax*8+0xe50]; je skip; inc edx; skip: inc ecx; cmp cx,di; jl
```

- The designator test comes before the add, but its branch comes after the add.
- The registers are di = `window_count` and edx = player count, the same as the four exact `render0..render0_3` loops.
- The park body adds first (`lea ebx; fadd; mov bl; test; je`) and swaps dx and edi.

**New measured law (labs `lab4.c`, `lab5.c`, `lab6.c`).** When both arms of an if/else perform the same x87 statement, VC7 hoists that statement above the branch and after the condition's test.

Every single-arm spelling fails to reproduce January's order:

| lab | spelling | result |
| --- | --- | --- |
| A | add, then `if` | add before the test |
| D | `if`, then add | add deferred past the loop increment and compare |
| M | `continue` | same as A |
| N | named boolean local | same as A |
| I | `count += ids ? 1 : 0` | `setne` / `add` |
| J | window pointer local | different code |

Only the both-arm forms (E, and K with inverted polarity) produce `test; lea; fadd; je`.

**Shapes:**

| id | shape | result |
| --- | --- | --- |
| f1 | `if` then add | 928/50 residual (add deferred to the end of the block) |
| **f2** | body below | **EXACT 928/50, sha 34007f52073a5931** |
| g1 | the f2 edit alone on the real file | EXACT |

```c
if (frame->window_ids[window_index])
{
	value += frame->windows[window_index].total;
	player_window_count++;
}
else
{
	value += frame->windows[window_index].total;
}
```

**Credibility caveat for the verifier.** The add is written in both arms.
- Each path executes exactly one add, and the arms are not identical, so there is no redundant store.
- The body is January's exact `render0_3` player-window body (`if (player) { add; count++; }`) extended with an `else` arm that also accumulates non-player windows, which is what `np` names.
- The both-arm control flow is supported by the January-side hoisting signature, with six single-arm spellings refuted.
- A reviewer may still classify arm duplication as steering. This needs an explicit ruling; it is not silently claimed as clean.

## `_compare_profile_sections`: owner-blocked

**What January does.** Every path returns directly, with the epilogue duplicated per path and only ebx/esi saved.

**The invalid-mode path.** After `display_assert` and `system_exit(-1)`, January executes `mov eax,[ebp+8]`. That loads the first parameter's home, which the average-time path reuses as its `sample_count` `fidiv` temporary. This is an uninitialized `result` read.
- No defined expression produces that load.
- The older donor's exact body reproduces it through an uninitialized local, which brief section 5 forbids.

**Closest defined shape.** One shape was measured: c1, a single-exit `int result;` assigned on every path, with explicit `else result = 0` arms and `result = 0;` after the unreachable assertion.
- It reaches **288 / 8, the same size as January** (the current park is 304 / 8).
- Its hash differs because the defined zero default tail-merges with the equal-zero return, where January has a separate `mov eax,[ebp+8]` block.
- c1 is not exact, so it is not landable into a parked body. It is recorded only as the closest defined body: `scratch/workers/profile/c1.c`.

**Reopen criterion.** An owner ruling that allows January's post-assertion undefined return, or recovered original source.

## Admission checks (both reopen candidates)

- **Strict EXACT:** both functions pass in whole-TU gates of `d4.c`, `g1.c` and `f2.c`, with no alias.
- **Row-by-row check:** `scratch/workers/profile/rows_base.txt` against `rows_f2.txt` shows only the two reopened rows changing, from residual to EXACT. All 41 baseline-exact rows stay exact.
- **`_point_from_line3d` guard:** passes.
- **`scratch/parkcheck.py`:** each candidate drifts only its reopened function(s), and to the January target hash. `_compare_profile_sections` is unchanged.
- **Owner census against `build/base`:** 146 owners in both, with no candidate-only owner, no lost owner and no COMMON. The 13 literal COMDATs present in the candidate but not the target predate this work.
- **Non-code sections:** byte-identical to the baseline object.
- **`tools/fake_match_scan.py`:** 7 leads in each candidate, the same 7 `QUERY_TIMEBASE` inline-assembly lines as HEAD. No new lead.
- **Headers and config:** none needed. There is no consumer blast because only `profile.c` changes.

## Reusable laws recorded

1. **Explicit-cast CSE-kill law.** In `x += (real)(expr)`, the last consumer of an x87 CSE pops it with `fdivrp`. `x += expr` keeps it until after the store and then emits `fstp st(0)`. Plain assignments are unaffected. Use this before classifying a delayed `fstp st(0)` as a scheduler tie.
2. **Common-arm hoisting law.** An x87 statement present in both arms of an if/else is emitted between the condition's `test` and its `jcc`. A `test; <x87 op>; jcc` pattern in January therefore proves both-arm source, and no single-arm reordering reproduces it.
