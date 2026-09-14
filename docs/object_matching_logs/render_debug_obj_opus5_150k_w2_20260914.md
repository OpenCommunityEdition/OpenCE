# `render_debug.obj` — Opus5 150K house-clean lane, wave w2 (2026-09-14)

Worker scope: `source/render/render_debug.c` only. No headers, config or build files were edited.
Worker notes: `scratch/workers/render_debug.md`. Probes and candidates: `scratch/workers/render_debug/`.

## Baseline and final (focused real-file gate with the `_point_from_line3d` guard)

| state | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 29 | 7 | 0 |
| final real file | **33** | 3 | 0 |

Four rows were non-exact at baseline and are now strictly EXACT, with no alias:

| function | padded | meaningful | relocs | normalized sha256 |
| --- | ---: | ---: | ---: | --- |
| `_build_circle_points` | 112 | 102 | 2 | `8ff853ea832e6034…` |
| `_build_pill_points` | 448 | 437 | 8 | `64cab2d3fdbb39ce…` |
| `_render_debug_circle` | 304 | 293 | 15 | `3b88079749360d3c…` |
| `_render_debug_sphere` | 400 | 389 | 15 | `d2729ec1a7534b06…` |

Worker-side total: 1,264 padded / 1,221 meaningful bytes. Only the orchestrator's full build counts.

The remaining three residuals are all parks: `_render_debug_add_cache_entry`, `_render_debug_point` and `_render_debug_player`. One park reopen is proposed below. The final real file leaves all three parks byte-identical to their `measurements.base` (`scratch/parkcheck.py`: drift 0).

## Checks (final real file)

- **Row diff:** the baseline `--all` listing differs only in the four rows above. No baseline-EXACT row changed.
- **Guard:** the `_point_from_line3d` emitted-symbol guard passes.
- **Fake-match scan:** `tools/fake_match_scan.py` reports 0 leads, the same as HEAD. `git diff --check` is clean.
- **Owner census vs `build/base`:** there is one new candidate-only code COMDAT, `_set_real_point3d` (32 B, 0 relocations).
  - It is the systemic header-inline class: a non-static `__inline` in `real_math.h`, emitted because the new sphere body genuinely inlines it.
  - It is byte-identical (size and normalized sha `8bcddd57839f98d1`) to January's own selected owner in `build/split/source/ai/ai_debug.obj`.
  - 15 current base objects already emit the same COMDAT.
  - It is disclosed here for the orchestrator's census decision. There are no other new or lost code, data, BSS or COMMON owners.

## Evidence and measured VC7 13.00.9254 laws (new this wave)

### 1. Named common-subexpression local fixes x87 product order (`_build_circle_points`)

January's rotation y-sum loads `cos*y` before `sin*x`. A lab TU compiled with render_debug.c's exact flags (`lab/b1-b3.c`) showed that none of these change the order: textual order, local names, or the declaration order of sine/cosine. It is fixed by computing the step once:

```c
real angle = 2*_pi/NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS;
real sine_of_angle = sine(angle);
real cosine_of_angle = cosine(angle);
```

### 2. Strength-reduction anchor law

For direct array-element accesses inside a loop, VC7 anchors the derived induction pointer at the address of the **second** array reference in IL order. This was confirmed by 8 lab variants (`lab/c1-c4.c`, `lab/r1.c`). The following are inert:

- declaration order and scope;
- operand textual order;
- `word`/`short` index;
- `do`-`while`.

Element-pointer locals re-root the anchor:

- one pointer anchors at the pointer itself (the January `build_pill_points` ring loop);
- two pointers anchor at the second pointer's last field (the January cap loop, circle and sphere).

### 3. Inline-helper argument law

Routing endpoint components through the `real_math.h` inline constructors changes VC7's x87/integer schedule and commutative load order compared with separate field stores. The constructors are `set_real_point2d(p, x, y)` and `set_real_point3d(p, x, y, z)`.

- With pointer locals, this closed `_render_debug_circle` and `_render_debug_sphere` (both textual operand orders are exact).
- With direct array accesses the sphere stays residual.
- It also closes the parked `_render_debug_point` without any other change.

The landed bodies read naturally:

- The ring loop names `circle_point`.
- The cap loop pairs `top_circle_point = &points[index]` with `bottom_circle_point = &points[index+N/2]`, which is the upper/lower semicircle split.
- Circle and sphere take `circle_point0/1 = &points[index]/[index+1]` and build each endpoint with Bungie's point constructor.
- There are no casts, dead locals, compiler controls or reordering-only edits.
- The circle's deferred cache call is unchanged, so the documented January omitted-offset boundary is untouched.

## Park reopen proposal

**`_render_debug_point` (288 B, 11 relocs).**

- **Candidate:** `scratch/workers/render_debug/park/point_p1.c`. It is the current real file with only this body changed: the 18 field stores become six `set_real_point3d(&points[k], ...)` calls.
- **Result:** strict EXACT, sha `d266ea4ebca71917…` equal to the target.
- **Whole-TU gate:** 33E -> 34E, and only this row changes.
- **Other checks:** add_cache_entry and player parks unchanged; census identical to the final real file; fakescan 0.
- **Park criterion:** "authentic source/compiler-context evidence, not equivalent-spelling or scheduling searches". The evidence is the measured inline-helper law above. That law independently closed the two sibling residuals, circle and sphere, in the same TU.
- **Variant not proposed:** p2 adds `real half_size` and is also exact. It is not proposed because it introduces a local.

## Parks investigated and not reopened

- **`_render_debug_player`** (frame `sub esp,0x404` vs `0x400`).
  - January never references `[ebp-4]`.
  - Five frame-gap shapes changed nothing: buffer at function scope, buffer at biped-block scope, a named `stuck_string` local, a named `player_index` local, and vehicle declared then assigned.
  - Reopen only with authentic evidence of the original local set; never with a padded buffer.
- **`_render_debug_add_cache_entry`** (656 vs 672).
  - Applying the single-exit law (`if (entry_count < MAXIMUM) {...} else if (!entry_overflow_reported) {...} return;`, the same shape as the exact `render_debug_add_cache_string`) reproduces January's prologue, the jump-threaded reset path, the tail overflow block and the shared default epilogue.
  - Also making `string_at_point` success-first (`park/ace_a2_raw.c`) reduces the alignment-diff blocks from 30 to 19.
  - Case-scoped `cached_string` locals are inert.
  - What remains is cross-jump tail merging. January merges line and cylinder into string_at_point's point/color copy tail, and box into pill's. Ours differs because string_at_point loads the point va_arg before storing the string.
  - Reopen with a measured cross-jump/register-choice law, starting from `park/ace_a2_raw.c`.

## Rejected / inert shapes (do not repeat)

- **`build_circle_points`:** name swaps, textual flips, and cosine-first declaration.
- **Circle:** 1-based vs 0-based array loops, textual operand order, pointer scope, `circle_point1 = circle_point0 + 1`, and pointer iteration.
- **Sphere:**
  - `point = *center; point.x += ...` forms (480 B);
  - field assigns with pointer locals (22 blocks);
  - `set_real_point3d` with direct array accesses.
- **Player:** the five frame shapes above.
- **add_cache_entry:** case-scoped locals.
