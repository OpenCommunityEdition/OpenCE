# `light_volumes.obj`: next-150K lane, Tier B wave (B4 `_light_volume_render`, 2026-09-16)

## Result

Nothing landed. `source/objects/widgets/light_volumes.c` is **unchanged** (`git diff --stat` is empty).

| gate | result |
| --- | --- |
| baseline, with the `_point_from_line3d` guard | exact 9 / residual 1 / unwritten 0 |
| final, with the guard | the same, row for row |
| park drift | 0 |
| fake scan | 0 leads |

**Scope.** One function, `_light_volume_render`. It is parked as `tu-context-optimization`, so any work on it happens only in scratch.

**Verdict: SKIPPED-EXHAUSTED at census time.**
- Zero new source shapes were spent.
- After the mandatory census, only x87 register allocation remains: which real value lives on the x87 stack, and whether a phi is kept in memory or on the x87 stack.
- Owner requirement 3 says to stop at that point.

| | padded / relocs / frame / sha16 | instructions |
| --- | --- | ---: |
| January | 912 / 47 / 0xa4 / `2e43dbc111ad1255` | 291 |
| real file (park base) | 928 / 47 / 0xa8 / `a4e273c7939882fc` | 296 |
| w2 `lvJ`, re-measured at HEAD | 944 / 48 / 0xac | 302 |

Worker notes: `scratch/workers/tB_light_volumes.md`. Scratch artifacts: `scratch/workers/tB_light_volumes/`:
- `v0.c` and `v0.obj` (the real file);
- `lvJ_re.c` and `lvJ_re.obj` (a byte copy of the w2 `lvJ.c`);
- `jan.dis`, `ours0.dis` and `lvJ.dis`;
- `slots.py`.

## Duplicate checks

**Ledgers read:**
- `light_volumes_obj_jonas_partial`;
- `_jonas_interpolator_recovery_20260826`;
- `_jonas_distance_fade_schedule_fixed_point_20260826`;
- `_jonas_exhaustive_census_20260830`;
- `_credible_fuzzy_closeout_20260831`;
- `_opus5_150k_w2_20260914`;
- `_opus5_250k_w3d_20260915`;
- the research campaign `opus5_next150_research_campaign_20260916` (row B4; xstudy F3=DECLARATION, F6=TIE);
- `laws_w2` and `laws_w3`;
- both worker briefs.

**Other checks:**
- `git log --all`: last touched in `3cbc29777`.
- `branch_sweep`: 15 blobs. None does better than exact 9 / residual 1 (8 fail to compile, 2 are early states).
- The park criterion in `config/parked.json` was re-read.

## Census, recorded before any shape

**Relocations: 47 == 47, identical multiset.**
- Every call count is equal: `_datum_get`, `_tag_get`, `_light_volume_interpolate_frames`, `_object_get_marker_by_name`, `_object_get_function_value`, the four `_rasterizer_widget_*`, `__CIpow` x4, `_rgb_colors_interpolate` and `_real_argb_color_to_pixel32`.
- Every global count is equal: `_render+16..36`, `__real@00000000` x11, `__real@3f800000` x11 and `_light_volume_globals+176`.
- The gate tag is `[size, sha]`. No statement is missing.

**Frame: January `sub esp,0xa4` (164) vs ours `0xa8` (168). Ours has one extra dword.**

| cell | January | ours (real file) |
| --- | --- | --- |
| marker / color | -0xa4 / -0x38 | -0xa8 / -0x3c |
| -0x28, -0x24 | radius; color_fraction (x87 phi, stored at the join) | loop `position` y, z |
| -0x20 | `divisor` (fild, fstp, fdiv) | radius |
| -0x1c | pow1#1 exponent (integer copy) | pow1#1 exponent (integer copy) |
| -0x18..-0x10 | `delta`, **sharing the loop `position` cells** | pow1#2 exponent; pow1#4 memory phi; sprite_index |
| -0xc | sprite_index (32-bit shadow) | pow1#3/#4 exponent (fst) |
| -0x8 | distance_fade (`mov 1.0f`), then intensity | intensity |
| -0x4 | pow1#2/#3/#4 exponent (fst x3) + brightness_fraction | distance_fade + color_fraction memory phi |
| [ebp+8] | function_value (`mov 1.0f` at +0xd3), count-1, offset_fraction | parallel_factor, function_value, count-1, offset_fraction |

`frameslot.py` at HEAD returns MIXED-EXTRA / DECLARATION, identical to the research row.

## Re-verification of the two lenses

1. **The frame byte fact holds**, with one correction. The research row reads "frame 164 vs January 168", but January is the **smaller** frame.
2. **The F3 DECLARATION slot class holds.**
3. **The implied change "one more x87-homed real local in January" is already measured.**
   - The only real value January homes that ours lacks is `divisor` (w3d defect (a)).
   - w3d measured it with v7 at 944/47/0xac, and `lvJ` carries the equivalent loop header.
   - Because January's frame is smaller, no added local can explain the gap.
4. **"`lvJ` lost a statement" is refuted.**
   - `lvJ`'s 48th relocation is one extra `__real@3f800000`.
   - That is `fld 1.0f`, which keeps distance_fade on the x87 stack instead of January's immediate `mov [ebp-8],0x3f800000`.
   - All call and global counts are equal.

## New evidence and corrections

**(1) `lvJ` already reproduces January's `delta` topology.**
- `delta` is homed in memory (`fstp [ebp-0x1c/-0x18/-0x14]`) and shares the loop `position` cells (`lea edx,[ebp-0x1c]` at +0x382).
- `lvJ` uses `vector_from_points3d`, `parallel_factor = ABS(dot_product3d(...))` as one statement, and `dot_product3d(&render.camera.forward, &delta)`.
- This **corrects w3d finding (f)**, which says `vector_from_points3d` never homes `delta`.
  - Re-reading `w3d_glow_light_volumes/v3.obj` shows `delta` stays on the x87 stack only when the ABS is written as two statements and distance_fade is homed.
- `lvJ`'s loop header (+0x235..+0x25c) is instruction-identical to January's.

**(2) Every remaining `lvJ` hunk (66 in total) comes from three x87 residency outcomes and their consequences.** The consequences are slot colouring, operand load order, deferred pops and the loop-head alignment nop.

1. **Pre-loop contest** among the three `delta` webs, `parallel_factor` and `distance_fade`:
   - January: `delta` and `distance_fade` in memory, `parallel_factor` on x87.
   - `lvJ`: `delta` and `parallel_factor` in memory, `distance_fade` on x87.
   - Retained body and w3d v3: `delta` on x87, the other two in memory.
2. **Inlined pow1 #1, #3 and #4:**
   - January uses an x87 phi: an eager `fld value` before the `jnp`, then a store at the join.
   - Ours uses a memory phi.
3. **pow1 #2 exponent:**
   - January uses `fld; fst [ebp-4]` into a shared home.
   - Ours uses an integer copy into its own home.

The two extra `lvJ` frame cells come from outcomes 2 and 3. By F3 law 0, frame layout runs after register allocation. By LAW D, no declared local is missing or extra.

**(3) `mod8.py` detector run on `lvJ_re.c` (an instrument, never used as source).**
- D=0..8 all give 944 B and 142-143 differing instructions; the verdict is INSTRUMENT-PERTURBS.
- No IL-local count moves the residency pattern or the size.
- So the contest is not M8- or count-keyed, and no genuine local-inventory change can close it.

**(4) pow1 spelling cannot be the lever (LAW I).**
- Re-reading w2 `lvK` (ternary), `lvQ1` (`result = value`) and `lvQ2` (if/else result) shows the same two effects in each:
  - the exact out-of-line `_pow1` changes (26/25/26 instructions vs January's 28);
  - render calls `_pow1` out of line (4 `_pow1` relocations, 44 in total).
- Either effect would trade an exact row, which is prohibited. So A10 cannot be applied to pow1.

## Levers checked (no shape spent)

- **A10 conditional initializer.** January's memory-homed distance_fade already matches ours' default-then-if form. A10 was measured on an integer boolean, not on x87 residency.
- **Initializer position and intensity accumulator.**
  - w2's `lvG` 12-cell grid placed distance_fade and function_value at T/D/P/C; every cell was 944/48.
  - The `lvB`/`lvC` intensity accumulator hashes the same as `lvA` (web renaming).
- **Other research laws.**
  - H3's only levers are filler declarations.
  - LAW C, LAW E and CJ4 have no matching construct in this function.
  - M8 is refuted for residency (3).

## Reopen criterion

Reopen only on a measured VC7 law, proven in a micro-TU, that does one of these:
- moves x87 residency between overlapping real webs (which of `parallel_factor`, `distance_fade` and `delta` stays on the x87 stack);
- turns an inlined `if (c) v = f(v); return v;` helper's memory phi into an x87 phi, **without** changing the helper's out-of-line bytes or its inline decision.

Then start from `lvJ`, whose `delta` topology and loop header are already January's.

**Do not repeat:**
- pow1 respellings;
- the init-position grid;
- the intensity accumulator;
- dot-product argument order;
- declaration or scope permutations;
- local-count instruments;
- any probe on the w2 or w3d do-not-repeat lists.
