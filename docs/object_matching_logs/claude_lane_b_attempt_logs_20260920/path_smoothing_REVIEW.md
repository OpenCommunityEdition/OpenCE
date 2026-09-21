# Independent review: source/ai/path_smoothing (Lane B)

Verdict: **DO_NOT_LAND - HOLD_FOR_OWNER** (`_find_tangent_point`). The byte match is real and the
source is credible, but the candidate NEWLY DEFINES `_set_real_point2d`, which the production object
does not define and January's split object lacks. Under the lane's ownership rule that is a violation,
and no spelling that avoids the symbol matches. Only the owner can rule on it.

## 1. Re-gate (from scratch)
- unitcheck: BASELINE exact 5 / residual 1 -> CANDIDATE exact 6 / residual 0; gains=1 (`_find_tangent_point` residual 336 -> EXACT 336), regressions=0, other=0.
- gate --fn: `EXACT 336 _find_tangent_point`.
- Own check with coff_compare.section_info: size 336/336, normalized sha256 1b6ac8d6...eaa26 both sides,
  5/5 relocations identical in offset/type/target (3x `__real@00000000`, `_normalize2d` REL32, `_global_left2d`);
  `section_infos_equal` = True (baseline object = False, sha 9275ddd1...).
- alndiff: 129/129 instructions; the only listed rows are symbol-vs-defined-noncode spelling of `__real@00000000` (same target).

## 2. Guard
`--forbid-emitted-symbol _point_from_line3d`: passed.

## 3. Ownership (newsyms) - FAIL
`+ _set_real_point2d  .text  VIOLATION: January does not define it`. Nothing removed.
Facts for the owner (all measured here):
- Production path_smoothing.obj does NOT define it; January split path_smoothing.obj does NOT define it.
- January owns it in path_obstacles.obj (symbols.json file_offset 328720), which PRECEDES path_smoothing
  (`_find_tangent_point` 332304) in link order -> a folded COMDAT copy in January's path_smoothing.obj is consistent, not proven.
- Our emitted 32-byte copy is `section_infos_equal` to January's path_obstacles copy.
- Production already emits 9 sibling real_math.h inline COMDATs January's split lacks (_cross_product2d, _magnitude2d,
  _magnitude_squared2d, _normalize2d, _point_from_line2d, _scale_vector2d, _set_real_vector2d, _square_root, _vector_from_points2d);
  9 other production objects already emit `_set_real_point2d` where January's split does not.
- It cannot be suppressed from the .c (header is frozen; header-inline-linkage law).

## 4. fake_match_scan
candidate 0 leads, real source 0 leads. No new findings.

## 5. Diff read (one hunk, find_tangent_point body only)
Prefix 3061 B and suffix 12845 B byte-identical to the real source; all CRLF (625 -> 614 lines), no BOM, no lone LF.
No casts, volatile/register/pragma, offsets, punning, dummy locals, self assignments or impossible conditions.
Every call is a real real_math.h helper. /Od fn 0x4cd3c0 (reproduced with odbuild.py; dump identical to the worker's;
called twice from 0x4cdf20 = path_smooth, as in our source) attests: vector_from_points2d + magnitude_squared2d,
square_root, 2x set_real_point2d with exactly the candidate's x/y expressions and operand order, 2x vector_from_points2d,
cross_product2d(&tv[0], &tv[1]), unnamed compare/index temps (-0x64/-0x68/-0x6c sit past the last RTC local with the
normalize2d return temp -0x70), and RTC names center_to_point / tangent_points / tangent_vectors / radius_vector.
Slot order -0x18/-0x1c/-0x20 = distance_squared, inverse_distance_squared, tangent_length = candidate's order.

## 6. Strip tests (review/mkvariants.py, binary-safe)
| variant | result |
|---|---|
| r1_plain_stores (no set_real_point2d, x then y) | residual [sha], 131 insns - magnitude block reshapes too; worse than baseline |
| r2_diag_yfirst (y then x, diagnostic) | residual [sha] - not an evaluation-order effect |
| r3_function_scope (tangent_points/vectors at function scope) | EXACT - scoping byte-inert |
| r4_baseline_decl_order | EXACT - declaration order byte-inert |
| worker try_3 (reuse center_to_point in else arm) | EXACT - radius_vector byte-inert |
| worker try_6 (top helpers hand-spelled) | residual [sha] |
| worker try_7 (tail helpers hand-spelled) | EXACT - kept as calls (manual expansion inadmissible) |
set_real_point2d and the top helper pair are load-bearing; they are real /Od-attested helper calls, not decorations.
radius_vector: not a zero-init/pointer/accumulator local; /Od RTC-named; January frame 0x24 and all bytes agree with it.
It replaces the old body's redundant recompute into center_to_point, so it is the more credible spelling. No plain
spelling without `_set_real_point2d` matches, so there is nothing landable to build.

## 7. Semantics
Byte-identical to January on every path; source reads correctly (else arm = center + normalized(point-center)*radius, left2d fallback).

## 8. Parked / fuzzy
No park entries for this unit; no other function's bytes change. No zero-credit partial: r1/try_5 are further from January than the baseline.

If the owner admits the systemic-COMDAT emission, land `scratch/lane_b/w/path_smoothing/candidate.c` unchanged.
