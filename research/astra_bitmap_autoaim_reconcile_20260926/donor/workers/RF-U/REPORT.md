# RF-U report (wave 5): upstream collision_bsp (#59) and rasterizer_xbox_hardware_bitmaps (#60) transplants (saved by the lead)

**Outcome: one EXACT candidate, `_rasterizer_bitmap_new` (proposable, one owner flag). The three collision_bsp
residuals stay negative for landable source** (upstream reaches them only through the blocked shared-header strip,
held decorative parentheses, or held `__asm`). Patch-only; no lane edits; no ninja; no commits. Upstream read-only
(`git show` / `git archive` of origin/main into scratch/rf/workers/RF-U/uptree); nothing fetched or pushed. Records:
NOTES.md, cards/, patches/, evidence/.

Upstream "Matching": in upstream's tools/project_x86.py the status only sets objdiff `metadata.complete = True` (a
hand-set flag, not a gate). Measured upstream's own TUs in their tree with our CL and flags (measurement only):
collision_bsp 28 exact / 2 residual (`_collision_bsp_test_sphere`, `_collision_bsp_test_vector`: relocation identity
only; their usage-time statics are `sphere_start_time`/`vector_start_time`, January has one `_collision_bsp_usage_times`);
all three of our residuals exact there; 27 surplus COMDATs emitted. hardware_bitmaps 19/19. HEAD baselines:
collision_bsp 27/3, hardware_bitmaps 18/1, keyed 0/0/0.

## 1. `_rasterizer_bitmap_new`: EXACT candidate (B5)
Upstream vs ours: (a) arms: upstream's invented `D3DCALL` macro `{ HRESULT result = (call); success = success &&
SUCCEEDED(result); if (!success) rasterizer_error(result, #call); }` vs ours `result = call; if (result >= 0) success =
TRUE; else { success = FALSE; rasterizer_error(result, "<literal>"); }`; (b) tail: upstream's device arm ends
`if (!hw) success = FALSE; if (!success) hw = NULL;`, then `else { hw = NULL; success = TRUE; }`, then a shared
`if (!success) error(...)` and a single `return success`; ours had `if (!success) { hw = NULL; error(); } return
success;` inside the device arm followed by `hw = NULL; return TRUE;`; (c) minor: upstream drops the `(short)` casts and
the default `break`, uses a plain-array format table.

| Card | What | Result |
|---|---|---|
| B2 | full upstream semantics, macro hand-expanded | EXACT, keyed 1/0/0 |
| B3 | tail only | EXACT |
| B4 | arms only | = HEAD (sha 71946946) |
| B5 | B3 without the redundant else-arm `success = TRUE` | EXACT, sha 041cef84 = January |
| B8 | B5 re-indented | byte-identical to B5 |

Candidate: the tail only; arms as HEAD. Patches `patches/rasterizer_bitmap_new_single_exit_B5.patch` (sha256 f64f27d4...)
and the indented `..._B5_indented.patch` (sha256 35161557..., keyed-identical). Checks
(evidence/bitmap_new_candidate_checks.txt): gate --all 19/19; keyed diff 1 changed (EXACT vs January; 400 B, 26
relocations), 0 added, 0 removed; /W3 14 = 14 (only line shifts of two existing C4102 warnings); fake_match_scan 0/0.
Strip test: B6 (error() back in the hw=NULL block, single exit kept): sha 9f2d9d45, not exact (EBX constant pin lost,
147 insns, jne still binds to the last epilogue copy; = the recorded fifty-objects a4 / Lane C A4 shape); B7 (error() in
its own `if (!success)` but HEAD's early return kept): = HEAD. The single exit and the error-after-join placement are
exact only together.
Evidence: /Od dx9 0x7ea7d0 (first-party, later revision) attests the single exit `return success`, the device-arm
`if (!hw) success = FALSE; if (!success) hw = NULL;`, and `else { hw = NULL; }` with no `success = TRUE`. Retail
(R1/R2; per-arm HRESULT checks modelled as debug-only, as the retail body shows): B5-retail and B6-retail MATCH
Sept-25-2001 cache.exe 0x504890 and Oct-12-2001 2276P (0x115de0); HEAD-retail and B7-retail do not (224 B vs 208 B;
`success` stays a stack byte): corroborates the single exit, does not decide where error() goes.
Owner flag: the placement of error() after the join is decided only by January's debug bytes (the /Od revision dropped
the call; retail cannot tell B5 from B6). It is an authentic January statement (its string is January's) placed
naturally: no decoration, dead store or cast. The function is parked; the lead retires the park after fresh
verification (rule 63).
Other upstream differences: else-arm `success = TRUE` redundant (rule 20), not needed; D3DCALL arms byte-inert and the
macro name invented (rule 13; January's rasterizer_error strings are exact stringifications of the call, suggesting a
stringifying macro whose name/semantics are unknown); casts, `break`, table spelling neutral, not transplanted.
Mechanism (T1, stock-equal, RF-R's tools): HEAD at MARK has two return blocks (`L_ret: s = success` and the no-device
arm's `s = #1` falling into EXIT); the cross-jump makes the no-device arm the host, so the jne follows into the second
copy. B5 at MARK has one return block: the no-device arm is `hw = NULL; JMP L_ret` (the error test is threaded away
when `success` is known TRUE), and the error() call falls into L_ret. In the backend the exit split (RF-R's R3) places
the conditional-edge copy (N55) right after the error() call; at step #2 both the no-device JMP and the error-block
JMP are cross-jumped into N55 (trace t1_gt): the jne binds to the first copy (+0x171) and the no-device arm gets an
unlabelled tail copy at the end (+0x177).

## 2. `_bsp3d_test_sphere_recursive`: NEGATIVE (upstream exactness comes from a blocked header strip)
Differences: in the TU, tail-recursive if/else instead of our `while`; `leaf_indices[leaf_count++]`; the /Od helper leaf
(`point_from_line3d(-distance)`, `projection_from_vector3d`, `!= ((d & LONG_MIN) != 0)`); non-const pointers; names. In
the header, `plane3d_distance_to_point` returns `dot_product3d(&plane->n, (real_vector3d const *)point) - plane->d`
(outer parentheses stripped, dot operands swapped) vs ours `(dot_product3d((real_vector3d *)point, &plane->n) -
plane->d)`.
| Card | What | Result |
|---|---|---|
| S1 | upstream body, stock header | sphere = HEAD (947b5817); +2 surplus COMDATs (`_point_from_line3d`, `_projection_from_vector3d`) |
| H1 | HEAD TU + shadow header with upstream's plane3d_distance_to_point | sphere EXACT; pill changed, residual |
| H2 | operand swap only, parentheses kept | whole object byte-identical to HEAD |
Every TU difference is inert; the byte-bearing part is RF-O's parentheses strip. /Od 0x56d580 plane3d_distance_to_point
pushes plane then point, i.e. `dot_product3d(point, &plane->n)` = our order (upstream's swap contradicted; parentheses
invisible in /Od). Consumer sweep S1H (upstream's header form, 557 units, 255 saw the shadow): gains this function (800)
and `_compute_ground_plane` (336); LOSSES 4: `_rasterizer_frame_statistics_draw` 4,176, `_item_accelerate` 944,
`_ai_communication_update_speech_timers` 672, and new `_render_frustum_build_point_flags` 256 (caused by the swap).
Blocked (rule 62); strip-only already blocked by RF-O (3 losses). Mechanism (V1, vndag, stock-equal): the parentheses
create one 0x267 real VN temp per inlined expansion; H1 deletes exactly two (HEAD ordinals 35 node, 135 leaf); the
k-centre base goes 111 -> 110 (even) and the j-centre 118 -> 117 (odd), exactly RF-Q's January condition.

## 3. `_bsp3d_test_pill_recursive`: NEGATIVE (held and blocked forms)
Differences: /Od node locals (`distance, direction, distance0 = distance, distance1 = distance + direction`); /Od helper
leaf (`projection_from_vector3d`, sign form, `point_from_line3d` x4 with view casts `(real_point3d const
*)data->vector`, `(real_point3d *)&v3d`, `(real_point2d *)&data->vector2d`, `plane3d_negate`); `t = -distance * inverse
- fabs(inverse) * radius` (/Od 0x7aa96c attests; ours `-(inverse*distance0)`); decorative argument parentheses
`(-distance)`, `(-direction)`; `leaf_count++`, literal 0.000244140625f; the same header change as the sphere.
| Card | What | Result |
|---|---|---|
| P1 | upstream body, stock header | residual; +2 COMDATs |
| P2 | upstream body + upstream header | pill AND sphere EXACT (29/1) |
| P3 | upstream body + strip only (our /Od-attested operand order) | EXACT both (swap not needed) |
| P4 | P2 without the decorative argument parentheses | pill residual |
| P5 / P6 | each parenthesis pair removed separately | residual at exactly the fifty-objects class-(4) sites (+0x4da/+0x4ea, +0x520/+0x52c) |
Mechanism (V2): each parenthesis pair adds one 0x267 VN temp taking the integer issue slot; the strip supplies the
shared parity bit; the /Od leaf plus node locals fix the i-terms (fifty-objects pv2). Verdict: exact only with the
BLOCKED header strip plus two HELD, unattested decorative parentheses (rule 22; /Od shows plain `xorps` negations); the
/Od-attested parts need the rule-42 COMDAT checks and an exact caller. Not proposable.

## 4. `_collision_surface_test_sphere`: HELD
Upstream is exact only through `fast_distance_squared3d`, an SSE `__asm` helper in real_math.h (held: rule 26, owner
2026-09-15); never compiled in our tree. C2 (upstream's structure with HEAD's scalar deltas instead of the asm): 305
insns = HEAD; only the `found = FALSE` store moves after the breakable test (January's position, +0x5e): fuzzy
improvement, zero credit; the remaining gap is the SSE code (+32 B, frame 0x2c vs 0x24).

## Header helper differences (upstream vs ours)
| Helper | Difference | Bearing |
|---|---|---|
| plane3d_distance_to_point | parentheses stripped, operands swapped, const cast | see above |
| triple_product3d | ours already uses dot(cross); only the local's name differs (`cross` vs `c`) | none |
| cross_product3d | upstream stores `result->i` directly; ours via a local `i` | none of the four targets use it; `_collision_surface_area` exact in both |
| projection_from_vector3d | upstream trailing `return _x;`; ours `else { return _x; }` | not needed (P3 exact with ours) |
| fast_distance_squared3d | upstream only (`__asm`) | held |
Everything else these functions use (dot_product3d, point_from_line3d, projection_sign_from_vector3d, project_point3d) is
identical.

## Failed predictions
B6 (predicted = HEAD; lost the EBX pin); R1 (predicted retail could not discriminate; it discriminates the single exit);
T1 route (the host is the conditional-edge split copy N55, not the MARK L_ret label); C2 (predicted = HEAD; one store
moved); S1 (the p 0.35 EXACT branch did not occur).

## Budget and disclosures
bitmap_new ~12 gate compiles, 4 retail, 7 traces; collision_bsp 10 gate compiles, 1 sweep (557 units), 4 VN traces,
2 upstream-tree measurement compiles. Disclosures: B2 had 2 runs on a copy whose assert literals lost backslashes in a
Bash heredoc; the HEAD-retail compile was re-run twice for display only; the S1H outcome was edited right after
appending (to add the 256-byte size); the first T1 trace used a relative work directory (0 hits) and was re-run.

## Lead verification (2026-09-26)
Indented B5 applied in the lane (the patch also fixes HEAD's un-indented `switch`: whitespace only). Semantics:
`success` is initialised TRUE, so the no-device path still returns TRUE with hardware_format cleared and the device
path's error() fires under the same condition. /Od 0x7ea7d0 tail (scratch/rf/lead/od_7ea7d0.txt, +0x2e1..+0x33b):
`cmp [hw],0; jne; mov [success],0; movzx success; test; jne; mov [hw],0`, else arms `mov [hw],0`, single exit
`mov al,[ebp-1]`: matches. Gate 19/19, `EXACT 400 _rasterizer_bitmap_new`; keyed diff vs build/base 37 -> 37, 1 changed
(EXACT vs January), 0 added, 0 removed; diff --check clean. Park entry retired (narrow 19-line removal; 73 remain).
Full clean-build checkpoint R4 pending (waits for workers using the lane build).
