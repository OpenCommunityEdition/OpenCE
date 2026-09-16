# `main.obj` opus5 next-150K lane, wave n3a - structural park pass (2026-09-15)

Worker `n3a:main`. Unit `source/main/main.c`. **Real file unchanged** (`git diff --stat -- source/main/main.c`
empty). Per-function report blocks: `scratch/workers/n3_main.md`. Candidates and micro-labs:
`scratch/workers/n3_main/` (`f1.c`, `f2.c`, `f4.c`, `s1.c`, `s2.c`, `w1.c`, `lab/sf.c`, `lab/sf2.c`, `lab/wc.c`).
The previous wave's candidates (`scratch/workers/w3d_main/`) survive and were reused, not re-derived.

## Baseline and final

`gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 88  residual 6  unwritten 1`**; `_point_from_line3d` guard passed;
`scratch/parkcheck.py` reports 5 parks / drift 0; `tools/fake_match_scan.py` 0 review leads.
Nothing landed. Every target is PARKED (or owner-blocked) and none reached strict EXACT.

Duplicate prevention: read `main_obj_opus5_250k_w3d_20260915` (the immediately preceding wave on this unit),
`main_obj_opus5_150k_w1_20260914`, `main_obj_opus5_100k_waveD_20260914`,
`main_obj_natural_state_and_render_packet_20260909`, `main_obj_lifecycle_and_fuzzy_reconstruction_20260909`,
`main_obj_jonas_window_count_rejection_20260828`, `main_obj_jonas_crash_policy_closeout_20260826`,
`main_obj_tiny_geometry_input_closeout_20260901`, plus `scratch/workers/w3d_main.md`.

## Per-function outcomes

| Function | Padded | Baseline (parked) | Best measured | Verdict |
| --- | ---: | --- | --- | --- |
| `_main_frame_rate_debug` | 544 | 528 / 51 | **544 / 52** (`f1.c`: size AND relocation count both == January) | source form refuted by a new law |
| `_main_get_window_count` | 64 | 64 / 5, objdiff 79.2 % | micro-lab `lab_long`: 61 meaningful / 19 insns / 5 relocs == January | park premise REFUTED; owner-blocked |
| `_compute_subframe_counts` | 112 | 112 / 4, objdiff 94.3 % | unchanged | park premise CONFIRMED (19 spellings) |
| `_main_game_render` | 448 | 464 / 29 | w3d `gr_v4.c` 464 / 29 | census re-verified; no new lever |
| `_main_update_time` | 1440 | 1440 / 118 | w3d `ut_v10.c` 1440 / **117** | re-baseline still pending from w3d |
| `_main_crash` | 16 | UNWRITTEN | - | OWNER-BLOCKED (NULL-deref UB) |

## `_main_frame_rate_debug` - the missing relocation is closed, and the residual is now proved unreachable

Census (re-verified): frame `sub esp,0x214` identical; from 0x70 to the end (86 instructions) w3d's `frd_v2.c` is
byte-identical to January; the only relocation row missing was a fourth `_global_screenshot_count+902`
(`framerate_sample_index`). January's decoded control flow holds **two distinct copies** of
`sample_index = global_screenshot_count.framerate_sample_index;` - one at 0x56 reached by the `!framerate_reset`
edge (falling into the second `debug_frame_rate` test) and one at 0x6a reached by the `framerate_reset &&
debug_frame_rate` edge (falling straight into the sample body, the redundant test threaded away) - laid out
**after** the clear block.

**New measured law (n3a-1, identical-arm merge).** With everything else in the function held fixed, VC7 13.00.9254
merges two textually identical assignment arms of the same if/else tree into a single block in every spelling
except the one where both copies are `then` arms:

| shape | source | size / relocs | copies kept |
| --- | --- | --- | ---: |
| `f1.c` | `if (!reset) S else if (debug) S else CLEAR` | **544 / 52** | 2 |
| `f2.c` | `if (reset && !debug) CLEAR else if (!reset) S else S` | 544 / 51 | 1 |
| `f4.c` | `if (!reset) S else if (!debug) CLEAR else S` | 544 / 51 | 1 |
| w3d `frd_v2/v4/v5/v6` | nested both ways, `&&`, `\|\|` | 544 / 51 | 1 |

Block layout follows source arm order in all of them (confirms laws_w3 A44). That settles the row: January's
layout puts the clear block first, which forces `S` into the outer `else` position, and an `else`-arm `S` always
merges. Keeping two copies (f1) forces them **before** the clear block. The two requirements are mutually
exclusive in C, so no source shape in this family reaches January's bytes. `f1.c` is nonetheless the first body
whose size AND relocation count both equal January's; it is **not** proposed for landing because its duplicated
`else if` arm is not a spelling a programmer would write.

## `_main_get_window_count` - the recorded park premise is refuted

Recorded criterion (`config/parked.json`): *"Ordinary source produces a 79.210526 percent residual because VC7
reorders the repeated local_player_count calls and branch tails ..."*. That is not what the bytes say.

January returns a **32-bit** value on all three paths: `mov eax,1` (0x12), `mov eax,4` (0x2e) and
`call _local_player_count; movsx eax,ax; ret` (0x34..0x3c). Our `short main_get_window_count(void)` emits
`mov ax,1`, `mov ax,4` and a tail `jmp _local_player_count` - reproduced exactly by micro-lab `lab_short`
(`scratch/workers/n3_main/lab/wc.c`). Therefore **January's definition returns `long`** (laws_w2 A3c return-width
detector applied to constant returns). `lab_long` is 61 meaningful bytes / 19 instructions / 5 relocations,
identical to January on size and on relocation identity; the single remaining difference is that January keeps the
merged `return 1;` block at the first return's source position and branches backward into it, while our build sinks
it to the function end (laws_w3 A43 cross-jump-survivor family, a recorded negative; `lab_long_single`, the
single-exit result-variable form, is byte-identical to `lab_long`).

The fix is blocked, and the block was measured rather than assumed:
- `long` on the definition alone does not compile - `scratch/workers/n3_main/w1.c` gives
  `error C2371: 'main_get_window_count' : redefinition; different basic types`, `main_runtime.h(17)`.
- January's **callers declare it `short`**: in `build/split`, `_interface_draw_screen` calls it twice and both
  times compares `cmp ax, 1`; `__rasterizer_window_end` does the same at +0x34. A `long` prototype makes those
  32-bit compares, and `_interface_draw_screen` (928), `_rasterizer_dynamic_geometry_begin` (256) and
  `_rasterizer_screen_effect_set_texture_transforms` (2128) are EXACT today.

So January's own source carried a cross-TU declaration mismatch. A header-only change trades 3,312 exact bytes for
at most 64, which brief section 7 forbids outright. Filed as type-fidelity evidence and an owner question; the
exact proposed text and the complete consumer list are in `scratch/workers/n3_main.md`.

## `_compute_subframe_counts` - park premise confirmed and narrowed

Census: no frame; 4 relocations identical; exactly one block differs - January materialises the loop-body
predicate (`cmp esi,edi; setl al; test al,al; je`) where ours folds it into `jge`. 19 spellings were measured
(two in-tree candidates plus two micro-lab batches, `lab/sf.c` and `lab/sf2.c`). Nothing natural materialises the
predicate: `long`/`boolean` temporaries in and out of the loop, `!= FALSE`, `== TRUE`, `| 0`, `?: TRUE/FALSE`,
a `(boolean)` cast, the negated reversed comparison, `short` counters and an inlined `static __inline` predicate
all fold to `jge` (the inline predicate additionally emits a candidate-only COMDAT, R14). The three spellings that
do materialise are all inadmissible: `(a<b) & (c>0)` (two `setcc` rows), `(boolean)(a<b) & (boolean)TRUE`
(`setl dl; test dl,1` - an R7 representation cast and the wrong test form), and inventing a second use of the
boolean. `horizontal - vertical < 0` emits `sub esi,ecx` and is refuted.

Byte-inert hygiene fact for whoever reopens this park: the plain `while (vertical*horizontal<num_players) {...}`
spelling (`s2.c`) is byte-identical to the parked body's hand-rotated `if (num_players>1) do {} while (...)`,
because VC7 rotates the loop itself and constant-folds the preheader test to `cmp ebx,1; jle`.

## `_main_game_render` and `_main_update_time`

Both censuses were re-run and reproduce w3d's exactly, so nothing new was spent on them.

`_main_game_render`: frame identical, one relocation row differs (`_observer_get_camera` 1 vs 2), instructions
0..66 byte-identical; the two remaining differences are one defect - January stores the `observer` call result to
its home (`mov [ebp-0x14],eax` at 0xfc) which makes the shared tail six instructions, so VC7 does not tail-duplicate
it into the film-playback arm; ours forwards EAX (five instructions) and duplicates. HCEA
(`blam/main/main_game_render.c`, a flat else-if chain with a call in every arm - refuted by w3d's gr_v2 at 480/30)
and Stian (`main/main.c:4224`, one shared call and `camera = NULL;` at the loop top - what `gr_v4.c` already has)
were re-read this wave; neither reads the camera after the loop, so w3d's `lab_d` mechanism has no source support
and would be a synthetic lifetime anchor.

`_main_update_time`: w3d's `ut_v10.c` remains 1440 / **117** (size and relocation identity both equal to January,
27 register-allocation blocks left) against the parked 1440 / 118. The zero-credit re-baseline proposal from
w3d is still outstanding.

## Proposals

1. **No park reopen** - nothing reached strict EXACT.
2. Still outstanding from w3d (not re-litigated here): the zero-credit park re-baselines of `_main_update_time`
   to `scratch/workers/w3d_main/ut_v10.c` (1440/117) and `_main_frame_rate_debug` to
   `scratch/workers/w3d_main/frd_v2.c` (544/51). `scratch/workers/n3_main/f1.c` is closer still on relocations
   (544/52) but is withheld on credibility grounds.
3. Owner question only (does not pay for itself): the `long main_get_window_count(void)` return-type fidelity
   packet described above.
4. No header or config change is requested by this wave.

## Reopen criteria

- `_main_frame_rate_debug`: a source form that keeps two distinct `framerate_sample_index` load blocks AND lays
  them after the clear block. Law n3a-1 says that combination is unreachable from C; reopen only on a new
  compiler mechanism, not another CFG spelling.
- `_main_get_window_count`: an owner ruling permitting the `long` definition with a `short` view preserved in
  interface.c, rasterizer_xbox_draw_primitives.c and rasterizer_xbox_screen_effect.c; then only the merged
  `return 1` block placement (A43) remains.
- `_compute_subframe_counts`: a natural spelling - no representation cast, no invented second use, no bitwise
  `&` of two comparisons - that makes VC7 emit `setl al; test al,al; jcc` for a simple `<` in a bottom-tested
  loop, or authoritative January source.
- `_main_game_render`, `_main_update_time`, `_main_crash`: unchanged from w3d.
