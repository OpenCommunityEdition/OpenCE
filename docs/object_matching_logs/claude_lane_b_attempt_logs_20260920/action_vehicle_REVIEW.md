# Independent review: source/ai/action_vehicle (Lane B)

Verdict: LAND_PARTIAL -> `scratch/lane_b/w/action_vehicle/review/landing.c`
(candidate.c with one inert decoration stripped and three first-party local names restored; byte-identical object rows).

## Re-gate from scratch (reviewer-run)
- unitcheck candidate.c: baseline 15/2 -> 16/1, gains=1 (`_action_vehicle_setup_impromptu` 464), regressions=0,
  other=1 (`_action_vehicle_find_destination` [size 1248!=1232, sha] -> [sha]).
- gate --fn `_action_vehicle_setup_impromptu`: EXACT 464; relocdiff 18/18, 0 differing rows; alndiff 176/176, only reloc-spelling rows.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: newly DEFINES 0, OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate, 0 on real source -> no new findings.
- CL /Zs /W3 (review/warn.py): warning set identical to the real source (4 distinct, no C4013/C4133).
- Line endings: 1105 CRLF / 0 bare LF in real source, candidate and landing.c. Diff touches only find_destination body and
  the two argument lines in setup_impromptu. No baseline-exact function edited. No parks exist for this unit (parked.json: 0 hits).

## setup_impromptu (ACCEPT_EXACT)
`state_data->attempt_distance, state_data->continue_distance` instead of the parameters.
- /Od (odbuild fn 0x4438e0, review/od_si.txt): stores `[state_data+0x1c]=[ebp+0x10]`, `[state_data+0x20]=[ebp+0x14]` at 0x443951..66,
  then the desirable call pushes `[state_data+0x20]` and `[state_data+0x1c]` (0x4439a3..ba). First-party source shape.
- January: whole function EXACT with this spelling; the production (parameter) spelling leaves the 3-insn x87 rotation at +0xe2.
- No cast/temp/paren involved; nothing to strip. si_only.c verified = real source + these 2 lines.

## find_destination (ACCEPT_FUZZY_ZERO_CREDIT)
/Od function 0x4422a0 (review/od_fd.txt) read in full; every helper call, argument order, local order, the MAX temp,
the `[ebp-0x29]=0` flag / single `mov al,[ebp-0x29]` return, `collision_bsp` local fetched first and surface_index store first
are as the worker says. HCEX_Release locals confirm vehicle_center/modified_destination are real_point3d and
tangential_offset_vector real_vector3d, so the 3D->2D address casts are implied by the original (72 such casts already in
21 production files; stripping all 18 is byte-identical and only adds C4133).
Candidate vs January: 412/412, frame 0x464, relocs equal; residual = (a) 3 stack loads vs one byte store reordered,
(b) fld/fmul operand order x3. Both are value-identical, so semantics equal January on all paths.

Strip tests (variant object aligned against the candidate object; January == candidate at each site):
| variant | result |
|---|---|
| A no `success` accumulator (early return FALSE / return TRUE) | 413 insns, early epilogue inline: DIFFERS -> accumulator attested by January AND /Od |
| B no `collision_bsp` local | 414 insns, register/layout shifts: DIFFERS -> attested by both |
| C production `ignore_hint ? *entry_point : *hint_point` | byte-identical -> PLAIN spelling lands (done in landing.c) |
| D `else if (r < d + 0.3f)` instead of MAX | `test ah,5/jp` vs January `test ah,0x41/je`: DIFFERS -> MAX attested by both |
| E no named distance-squared local | byte-identical, but /Od shows one call + slot [ebp-0x94]; natural, not a dummy: kept |
| F no casts (worker strip_casts.c) | byte-identical, C4133: casts are required type hygiene |
| G surface_index store last | DIFFERS -> January order = candidate |

RTC descriptor names the worker did not use (byte-inert, restored in landing.c): `hint_direction` (was entry_to_hint),
`perpendicular_vector_to_hint_line` (was closest_point, now real_vector2d), `vector_to_destination` (was actor_to_destination).

landing.c battery: unitcheck gains=1 regressions=0 other=1; guard passed; OWNERSHIP PASS; fake scan 0; warnings identical;
find_destination object stream identical to the candidate's ("no normalized instruction differences").

## Owner item (not landed)
The tail still hand-expands point_from_line3d (x2) and scale_vector3d; this is PRE-EXISTING production text, kept because the real
calls emit `_point_from_line3d` (guard) and `_scale_vector3d` (new COMDAT). owner_ruling_exact.c (17/17) stays HOLD_FOR_OWNER.
