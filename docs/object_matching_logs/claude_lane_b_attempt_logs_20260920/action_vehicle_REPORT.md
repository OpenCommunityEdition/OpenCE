# Lane B worker report: source/ai/action_vehicle

## Baseline (real source, gate.py source/ai/action_vehicle --all)

    EXACT        80  _action_vehicle_begin
    EXACT       176  _action_vehicle_control
    EXACT       272  _action_vehicle_desirable
    EXACT       976  _action_vehicle_evaluate_seat
    residual   1232  _action_vehicle_find_destination  [size 1248!=1232, sha]
    EXACT       304  _action_vehicle_find_impromptu_seat
    EXACT        48  _action_vehicle_flush_structure_indices
    EXACT       768  _action_vehicle_perform
    residual    464  _action_vehicle_setup_impromptu  [sha]
    EXACT       336  _action_vehicle_setup_specific
    EXACT        16  _action_vehicle_update
    EXACT        48  _distance2d
    EXACT        64  _distance3d
    EXACT        48  _distance_squared2d
    EXACT        32  _negate_vector2d
    EXACT       144  _object_get_bounding_sphere
    EXACT        48  _point_from_line2d
    == exact 15  residual 2  unwritten 0  (of 17 listed)

candidate.c started as a byte-identical copy of source/ai/action_vehicle.c (cmp verified; file is uniformly CRLF).

## Evidence gathered before any compile

1. Prior ledgers read: action_vehicle_obj_opus5_150k_w1_20260914.md, action_vehicle_obj_opus5_250k_w3d_20260915.md,
   astra_90pct_vehicle_independent_review_20260920.md, research12/14 scout rows. No park entries for this unit.
   w3d fixed point fd_cand.c (opus5-150k worktree) reached 1232/34 [sha] with 11 instructions left in four x87 families.
2. NEW first-party evidence (not used by any prior ledger for this function): the 2020 unoptimized debug executable
   research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe contains find_destination at 0x004422A0
   (neighbour of the authenticated desirable at 0x004417E0). Disassembly banked in dbg_fd_2020.txt, helper identities
   in dbg_helpers.txt. Unoptimized code shows every SOURCE-LEVEL helper call and its argument order:
   - boolean flag = FALSE at [ebp-0x29] declared between the destination copy and the ignore-hint flag; single return flag.
   - distance3d(&vehicle_center, hint_point); radius = MAX(radius, distance + 0.3f).
   - target_point = *(ignore_hint ? entry_point : hint_point) (pointer select, then one aggregate copy).
   - three vector_from_points2d calls: (actor pos, vehicle_center)->A, (actor pos, target_point)->B, (target_point, vehicle_center)->C.
   - vector_from_points2d(entry_point, hint_point, &D); point_from_line2d(&B, &D, -dot_product2d(&B, &D), &E);
     magnitude_squared2d(&E) < K.
   - m = magnitude_squared2d(&B) stored in a local; if (m > 0); t = dot_product2d(&B, &A)/m; refined = FALSE AFTER t.
   - perpendicular2d(&B, &avoid); dot_product2d(&A, &avoid) > 0 -> negate_vector2d(&avoid, &avoid).
   - else-arm negate_vector2d(&C, &avoid).
   - point_from_line2d(&vehicle_center, &avoid, radius*1.1f, &modified_destination) (no z store).
   - vector_from_points3d(actor pos, &modified_destination, &V); magnitude_squared3d(&V); square_root(...).
   - perpendicular2d(&C, &tangential); dot_product2d(&tangential, &V) < 0 -> negate_vector2d(&tangential,&tangential);
     tangential.k = 0; normalize2d(&tangential) > 0 -> point_from_line3d(&dest, &tangential, 2.0f - distance, &dest).
   - tail: bsp = global_collision_bsp_get() FIRST, point_from_line3d(&dest, global_up3d, 1.0f, &origin),
     scale_vector3d(global_down3d, 4.0f, &vector), test_vector, then *surface_index_reference = result.surface_index
     BEFORE point_from_line3d(&origin, &vector, result.t, destination_point), flag = TRUE.
3. HCEX_Release.pdb locals for the function (names only): modified_destination, vehicle_center, target_point_to_vehicle,
   target_point, actor_to_target_point, avoid_vehicle_direction, tangential_offset_vector, result (collision), collision_origin,
   collision_vector. Banked in hcex_release.txt.
4. January stream (fd_january.txt) agrees with the debug topology at every site checked: the three 2D vector
   subtractions at +0x116..+0x167 in exactly that order, no z store after the 2D point_from_line, bsp call before the
   global_up3d read, surface_index store first, mov al,1 / xor al,al twin epilogues.

## Attempts

### fd attempt 1 (try_1.c / fd_1.txt) - debug-topology rewrite of action_vehicle_find_destination
- Hypothesis: the body should be the statement list the 2020 unoptimized build shows (helper CALLS: distance3d,
  MAX, pointer-select aggregate copy, three vector_from_points2d, point_from_line2d + dot_product2d + magnitude_squared2d,
  magnitude_squared2d local, negate_vector2d, vector_from_points3d + magnitude_squared3d, scale_vector3d, single-exit flag,
  collision_bsp local fetched first, surface_index store first). The two tail point_from_line3d sites stay hand-expanded
  because REAL_MATH_EXTERNAL_POINT_FROM_LINE3D (ownership guard) makes the helper an extern prototype in this TU.
- Evidence: dbg_fd_2020.txt (every call + argument order), January +0x116..+0x167 order, frame 0x464.
- Before: residual 1232 [size 1248!=1232, sha], frame 0x468, 412 vs 420 insns, 62 real regions.
- After:  residual   1232  _action_vehicle_find_destination  [sha]   (412 vs 412 insns, frame 0x464, all relocs equal)
  whole TU == exact 15 residual 2, no baseline-exact row lost.
- Remaining after attempt 1 (fd_1.aln.txt): TWO families only
  (a) +0x3dd: January loads the three modified_destination words BEFORE `mov byte [esi+0xf4],1`; ours stores the byte first
      and interleaves load/store (5 instruction rows move, same instruction multiset).
  (b) +0x483/+0x48c: X destination product January `fld vec.i; fmul t`, ours `fld t; fmul vec.i` (Y and Z already match).
- Verdict: KEPT as working base.
### fd attempt 2 (try_2.c / fd_2.txt) - declaration order from the debug-build frame
- Change: closest_point before entry_to_hint; tangential_offset_vector before distance; collision `result` before
  collision_origin/collision_vector (descending debug frame offsets -0xa4/-0xb4, -0xf8/-0x100, -0x520/-0x534/-0x548).
- Gate: residual 1232 [sha]; byte-identical to attempt 1 (declaration order INERT, as the brief says). Kept only because it is
  the evidence-backed order; zero byte effect.

### fd attempts 3a/3b (try_3a.c, try_3b.c) - hand-written `collision_vector.i = 4.0f*global_down3d->i` (and swapped) instead of scale_vector3d
- Motivation: newsyms reports `_scale_vector3d` as a newly defined header-inline COMDAT (systemic class; the baseline object
  already carries 10 such January-absent COMDATs: _distance_squared3d _dot_product2d _magnitude2d _magnitude_squared2d
  _magnitude_squared3d _normalize2d _scale_vector2d _square_root _vector_from_points2d _vector_from_points3d, 7 of them
  unreferenced by any relocation, i.e. VC7 emits every USED non-static __inline).
- Gate: both residual 1232 [sha] but WORSE: the Y and Z destination products also flip (fld t; fmul vec.j/.k), 18 differing rows vs 14.
- Verdict: REJECTED. scale_vector3d is both the debug-attested call and the closer spelling.

### fd DIAGNOSTICS (not admissible as-is: emitted-symbol guard) - the whole residual is the point_from_line3d plumbing
All four delete line 77 `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` so real_math.h supplies the genuine __inline helper.
| file | tail site 1 (dest+up*1.0f) | tail site 2 (origin+vector*t) | gate row |
|---|---|---|---|
| diag_nodefine.c | hand-expanded | hand-expanded | residual [sha]; family (a) CLOSED, only family (b) (2 rows) left |
| diag_s1.c | point_from_line3d(...) | hand-expanded | residual [sha]; only family (b) left |
| diag_s2.c | hand-expanded | point_from_line3d(...) | **EXACT 1232** |
| diag_inline.c | point_from_line3d(..., 1.0f, ...) | point_from_line3d(...) | **EXACT 1232**, whole TU == exact 16 residual 1, no exact row lost |
| diag_noscale.c | call | call, but hand-written 4.0f multiplies | residual [sha] (X,Y,Z products flip): scale_vector3d is REQUIRED |
- Guard on diag_inline.c: `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48`; newsyms: + _point_from_line3d, + _scale_vector3d, OWNERSHIP FAIL.
- Reading: family (a) (the three modified_destination loads hoisted above `mov byte [esi+0xf4],1`) needs nothing but the
  inline DEFINITION being visible for the out-of-line deep-site call (with an extern prototype &modified_destination escapes to
  an opaque callee and the loads cannot cross the byte store). Family (b) (X product leaf order) is LAW M8: it needs the helper's
  parameter temps at tail site 2. Both are exactly what January's TU had: real_math.h's __inline point_from_line3d, inlined twice,
  called once, COMDAT emitted in the original object and folded by the linker into the first owner (action_charge.obj).
- diag_alias.c (diagnostic): header inline aliased to action_vehicle_point_from_line3d_inline (actor_combat.c precedent), used at tail
  site 2, deep site stays an EXTERN call -> family (b) closes, family (a) STAYS OPEN. So family (a) requires the deep-site callee
  `point_from_line3d` itself to be a visible inline definition => `_point_from_line3d` must be emitted => cannot be exact while the
  emitted-symbol guard holds. NEEDS OWNER RULING (see final summary).

### Ownership decision for the banked candidate
- Lane A handoff section 3/9: "One Lane A variant died because it newly emitted `_negate_vector3d`" - a newly emitted systemic
  header-inline COMDAT is fatal. try_2.c newly defines `_scale_vector3d` (newsyms: OWNERSHIP FAIL). try_3b.c (hand-written
  `global_down3d->i*4.0f`, the spelling the production source already uses) newly defines nothing: OWNERSHIP PASS.
- candidate.c := try_3b.c. unitcheck: gains=0 regressions=0 other=1
  (`_action_vehicle_find_destination residual 1232 [size 1248!=1232, sha] -> residual 1232 [sha]`).
- STRIP TEST (casts): strip_casts.c removes all 18 `(real_point2d [const] *)`/`(real_vector2d [const] *)` casts; the function
  section is byte-identical (1232 bytes) - the casts are type hygiene only (without them CL emits C4133), not a lever.

## _action_vehicle_setup_impromptu

Byte ledger before any shape (si_base.aln.txt): 176 vs 176 instructions, frame 0x2c both, 1 ret both, relocation multiset equal.
ONE real family: the inlined action_vehicle_desirable distance sum at +0xde..+0xec. January x*x, z*z (+), y*y (+); ours
x*x, y*y (+), z*z (+) - three instructions rotated (6 rows), the classic LAW M8 stack-leaf ordinal signature. Standalone
_action_vehicle_desirable (EXACT) uses z,x,y, so the inlined order is keyed by the CALLER's IL temps, i.e. by how the call
arguments are spelled at the call site.

### si attempt 1 (try_si1.c) - pass the stored state_data fields, not the parameters
- Evidence: 2020 unoptimized build, setup_impromptu at 0x004438E0 (same assert line 0x38 = 56 as January; dbg_si_2020.txt).
  After `state_data->attempt_distance = attempt_distance; state_data->continue_distance = continue_distance;`
  (0x443951..0x443966) the desirable call pushes `[state_data+0x20]` then `[state_data+0x1c]` (0x4439a3..0x4439ba), i.e.
  `action_vehicle_desirable(actor_index, vehicle_index, FALSE, state_data->attempt_distance, state_data->continue_distance, FALSE, FALSE)`.
  January still reads `[ebp+0x14]` for the square because no call/store sits between the two field stores and the argument
  loads, so VC7 forwards the stored parameter - the spelling only changes the IL temps (M8), which is exactly the residual.
- Change: candidate.c lines 1061-1062 `attempt_distance, continue_distance,` -> `state_data->attempt_distance, state_data->continue_distance,`.
- Before: residual    464  _action_vehicle_setup_impromptu  [sha]
- After:  EXACT       464  _action_vehicle_setup_impromptu
- unitcheck: gains=1 regressions=0. Shapes spent: 1.
- Admissibility: plain member reads of fields assigned two statements earlier; no cast, temp, barrier or decoration involved, so
  there is nothing to strip. The alternative (parameters) is the production spelling and is measured non-matching.

### fd attempts 4a/4b (try_4a.c, try_4b.c) - operand/sum order of the hand-expanded destination products
- `result.t*collision_vector.i + collision_origin.x` and `collision_origin.x + collision_vector.i*result.t`: both residual [sha],
  18 differing rows, byte-identical to candidate. INERT (confirms LAW M8 "source operand order is inert"). REJECTED.

## FINAL STATE (candidate.c)

    == emitted-symbol guard passed (1 forbidden names checked)
    EXACT        80  _action_vehicle_begin
    EXACT       176  _action_vehicle_control
    EXACT       272  _action_vehicle_desirable
    EXACT       976  _action_vehicle_evaluate_seat
    residual   1232  _action_vehicle_find_destination  [sha]
    EXACT       304  _action_vehicle_find_impromptu_seat
    EXACT        48  _action_vehicle_flush_structure_indices
    EXACT       768  _action_vehicle_perform
    EXACT       464  _action_vehicle_setup_impromptu
    EXACT       336  _action_vehicle_setup_specific
    EXACT        16  _action_vehicle_update
    EXACT        48  _distance2d
    EXACT        64  _distance3d
    EXACT        48  _distance_squared2d
    EXACT        32  _negate_vector2d
    EXACT       144  _object_get_bounding_sphere
    EXACT        48  _point_from_line2d
    == exact 16  residual 1  unwritten 0  (of 17 listed)

- unitcheck: gains=1 (_action_vehicle_setup_impromptu 464) regressions=0 other=1 (find_destination size tag removed).
- newsyms: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`.
- fake_match_scan: 0 review leads. CL /Zs /W3: warning set identical to the baseline (no C4013, no C4133).
- candidate.c differs from the real source ONLY in find_destination (lines 530-735) and lines 1061-1062; uniformly CRLF.
- si_only.c = real source + the two-line setup_impromptu change alone: gains=1 regressions=0 (the closure does not depend on
  the find_destination rewrite).

### find_destination remaining byte ledger (candidate.c, fd_cand.aln.txt): 412/412 instructions, frame 0x464, 34/34 relocs equal
- (a) +0x3dd..+0x3f8, 5 rows reordered, same instruction multiset: January loads the 3 words of modified_destination
  (eax/ecx/edx) BEFORE `mov byte [esi+0xf4],1`; ours stores the byte first and interleaves load/store.
  Decision: is the deep-site callee `point_from_line3d` a visible inline definition (January) or an extern prototype (ours)?
  Measured: diag_nodefine.c closes (a) with no other change; diag_alias.c (extern deep site) cannot close it.
- (b) +0x483/+0x48c, +0x49f/+0x4a2, +0x4ae/+0x4b1, 6 rows: `fld vec; fmul t` (January) vs `fld t; fmul vec` (ours) for X, Y, Z.
  Decision: are collision_vector/collision_origin/t the parameter temps of genuinely inlined scale_vector3d + point_from_line3d
  (January) or hand-expanded member arithmetic (ours)? Measured: scale_vector3d call closes Y,Z (try_2); inlined point_from_line3d
  at tail site 2 closes X (diag_s2.c -> EXACT).

### NEEDS OWNER RULING - owner_ruling_exact.c (whole object 17/17)
owner_ruling_exact.c = candidate.c with (1) lines 77 and 79 deleted (`#define/#undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`),
(2) tail `point_from_line3d(&modified_destination, global_up3d, 1.0f, &collision_origin);`
`scale_vector3d(global_down3d, 4.0f, &collision_vector);` and
`point_from_line3d(&collision_origin, &collision_vector, result.t, destination_point);` - the three calls the 2020 unoptimized
build shows verbatim. gate: `== exact 17  residual 0  unwritten 0  (of 17 listed)`, unitcheck gains=2 regressions=0.
BUT: `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48` and newsyms `+ _point_from_line3d`, `+ _scale_vector3d`,
`OWNERSHIP FAIL`. Not admissible under the current "January ownership matters" ruling; preserved for the owner.
