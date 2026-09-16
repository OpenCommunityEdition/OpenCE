# `hud.obj`, next-150K lane Tier B wave (2026-09-16)

Scope: one function, research campaign target **B8** `_temporary_hud_draw_reticle` (196 meaningful / 208 padded).
It is parked, so only scratch copies were edited. `source/interface/hud.c` is unchanged: `git diff --stat` is empty.
Worker notes are in `scratch/workers/tB_hud.md`. All artifacts are in `scratch/workers/tB_hud/`.

## Outcome

| function | target | real file (baseline = final) | scratch candidate | status |
| --- | --- | --- | --- | --- |
| `_temporary_hud_draw_reticle` | 208 / 6 / `34a33fe789d28588` | 192 / 6 / `676b43cbdb66de19` | **208 / 6 / `34a33fe789d28588`, strict EXACT** | PARK-REOPEN-PROPOSED |
| `_hud_show_action_response` | 1744 / 120 | 1744 / 120 | 1744 / 120 (no drift) | out of scope |

- Candidate: `scratch/workers/tB_hud/reopen_candidate.c`. It is the current real file plus the reopened body and its TU-local count constant.
- Candidate whole-TU gate: exact 20 → **21**, residual 2 → 1. Only the target row changed.
- Real file at the end: gate identical to the baseline (20/2/0), guard passed, parkcheck 2 parks / 0 drift.

## Census (recorded before any shape)

- **Frame:** January `sub esp,0xc4`, ours the same. Slots on both sides: `radius` at `[ebp-4]`, `points[16]` at `[ebp-0xc4]`, the `angle` parameter reused as the loop accumulator at `[ebp+8]`, and `color` at `[ebp+0xc]`. frameslot.py says FRAME-CLEAN.
- **Relocations:** 6 against 6, with the same targets: `__real@3f000000`, `__real@3d800000`, `_render+168`, `_matrix4x3_transform_point`, `__real@3ec90fdb`, `_rasterizer_debug_line`. The literal rows differ only in the systemic symbol-vs-defined literal class.
- **Hunks at HEAD** (alndiff 77 vs 66 instructions; f4law STRUCT-COUNT):
  1. January `call matrix4x3_transform_point; fld [ebp+8]; fadd [pi/8]; add esp,0xc; …; fstp [ebp+8]`. Ours emits the `add esp,0xc` before the `fadd`.
  2. January `and eax,0x8000000f; jns; dec eax; or eax,-16; inc eax`, a signed `% 16`. Ours emits `and eax,0xf`.
- **The two campaign lenses still hold at this HEAD.**
- **The campaign's implied source change is refuted.** It said to declare the dividend as a signed type, but `next_point_index` is already `long`. The unsigned remainder comes from the divisor: `NUMBEROF` is `sizeof/sizeof`, a `size_t`, so `long % size_t` is unsigned whatever type the dividend has.

## Mechanism and evidence

1. **x87 schedule = laws_w3 A31(a), constructor routing controls the flush.**
   - Three x87 member stores (`points[i].x = …; .y = …; .z = …;`) flush the deferred cdecl pop early.
   - Routing the same values through `set_real_point3d(&points[point_index], cosine(angle) * radius, sine(angle) * radius, -0.0625f)` flushes it late, and the bytes before the call stay identical.
   - A31(a) had been measured on only one function (`_lightning_offset_marker_position`). This is a second function, and it comes with a necessity control.
   - Same-family precedent: render_debug.c:710-715 and 922-931 build `rasterizer_debug_line` points with `set_real_point3d`. laws_w2 A4 closed the render_debug circle/sphere/point functions with the same routing.
2. **Signed remainder = a signed-int named count.**
   - January's generic signed sequence proves the divisor was a signed `int` constant, not `NUMBEROF`.
   - The house idiom for a fixed count used as an array extent and a ring modulus is an anonymous enum constant in the TU constants section. There are 301 `NUMBER_OF_/MAXIMUM_ = N` enum constants in `.c` files, against 6 `#define`s.
   - Same-pattern precedents: `VECTOR_AVOIDANCE_NUMBER_OF_DIRECTIONS` (actor_moving.c:246, used as an array extent and in the `(i + 1) %` ring walk) and `NUMBER_OF_EDITOR_CAMERA_SPEED_STEPS` (editor_flying_camera.c:104-107, `%` ring).
   - It is not a cast, so the R7 rejection of `% (long)NUMBEROF(points)` does not apply. It also replaces the bare `16` array extent.
3. HCEA, Stian, punpckhdq and the hs7 exports have no copy of this function, so there is no cross-build evidence either way.

Candidate change (all in hud.c):

```c
enum
{
	NUMBER_OF_TEMPORARY_HUD_RETICLE_POINTS = 16
};
...
	real_point3d points[NUMBER_OF_TEMPORARY_HUD_RETICLE_POINTS];
...
	for (point_index = 0; point_index < NUMBER_OF_TEMPORARY_HUD_RETICLE_POINTS; point_index++)
	{
		set_real_point3d(
			&points[point_index],
			cosine(angle) * radius,
			sine(angle) * radius,
			-0.0625f);
		matrix4x3_transform_point(...);
		angle += _pi / 8.0f;
	}
...
	line_count = NUMBER_OF_TEMPORARY_HUD_RETICLE_POINTS;
...
			&points[next_point_index % NUMBER_OF_TEMPORARY_HUD_RETICLE_POINTS],
```

## Shapes (4 of the 5 allowed)

| shape | change | result |
| --- | --- | --- |
| S1 | A31 `set_real_point3d` routing | 192 [size, sha]. Hunk 1 is gone; only the remainder is left. |
| S2 | S1 + the enum count constant | **208/6 strict EXACT**, 21/1 (this is the candidate) |
| C1 (control) | enum only, no A31 | 208 [sha]: the fadd/`add esp` hunk returns, so A31 is necessary |
| C2 (control) | A31 + `#define … 16` instead of the enum | EXACT: the closure is not a declaration-count (H3) artifact of the added enum |

## Admission checks on the candidate

1. Strict EXACT in the whole-TU gate, with the same sha as January.
2. Row diff against the baseline `--all` listing: only `_temporary_hud_draw_reticle` changes, residual → EXACT.
3. The `_point_from_line3d` guard passed.
4. parkcheck: `_hud_show_action_response` OK (1744/120). The target drifts to exact, which is the reopen itself.
5. Owner census against `build/base`: the only new owner is `_set_real_point3d`, a select-any COMDAT (selection 2).
   - This is the accepted systemic real_math.h class: laws_w2 A4 names `_set_real_point3d`, laws_w3 A30 covers it, and 16 base objects already emit it.
   - Nothing is missing relative to base.
6. fake_match_scan: 0 leads, the same as HEAD.
7. Authenticity: the programmer builds a 16-point ring in view space with the ordinary point constructor, transforms it to world space, and closes it with a wrap-around index modulo the named point count.
8. There are no new private statics.

## Proposal (orchestrator)

**PARK-REOPEN `_temporary_hud_draw_reticle`.** Apply `scratch/workers/tB_hud/reopen_candidate.c` and delete the function's `config/parked.json` entry in the same commit. No other park needs re-measuring. There is no header change; the consumers are hud.c only.

The park's criterion is "authoritative January source/local-lifetime evidence or a natural same-compiler donor explaining both the x87 and remainder schedules". This candidate meets it on both halves:
- **x87 schedule:** met by A31(a), a measured same-compiler law, with a necessity control.
- **Remainder:** the park's premise ("VC7 proves the index nonnegative") is refuted by the `size_t` divisor. A signed named count, not a cast, reproduces January's sequence.

Verifier decision point carried over from w3: that worker asked for a ruling on "a `% 16` literal vs a TU-local count define". This candidate uses the house enum-constant idiom, not a literal or a cast.

## Reopen criteria / do-not-repeat

- If the verifier rejects the named count constant, the only remaining difference is the signed remainder. A31 alone (S1) closes the x87 schedule. Do not re-spend on loop, increment, pointer or accumulator-local variants (w2 x1-x5, w3 e2/e3/t1/t2).
- The campaign's "declare the dividend signed" hypothesis is refuted and must not be retried.
