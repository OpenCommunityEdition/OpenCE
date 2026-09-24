# action_charge worker ledger (claude/fifty-objects-20260925)

Task: close `_action_charge_perform` (3248 padded, unparked). Do not touch helper copies.

## 0. Baseline (2026-09-25 start)
- gate.py --all (production): `== exact 21 residual 1 unwritten 0`; perform `[size 3264!=3248, sha]`.
- Production state = Lane B landing_partial (melee-first, !ranged||underwater, two-store unreachable)
  + helpers taken from real_math.h (5e81dabf removed the local helper package) => the SELECT_ANY
  linkage family (== Lane B try_6 in bytes, presumably).
- Prior evidence read: all 15 action_charge ledgers, Lane B worker REPORT.md + REVIEW.md
  (claude-lane-b-ai-physics-units-20260920/scratch/lane_b/w/action_charge/).
  Lane B chain: diag_5/10/12/14 showed the residual = scalar component reads of `direction`
  (4 sites) where January/Od do 8-byte aggregate copies; `*(real_vector2d *)&v` copies close
  all but dot #2 term order; diag_14 + 6..7 dead locals => EXACT (instrument only).
- Recorded negatives NOT to repeat: decl order/scope of check_range/berserk/abort_range/time/
  target_range/unreachable; dot_product3d arg order; danger+attack order; fadd operand swaps;
  moving perform before helpers; three named dot-result reals (diag_11); M8 dead-local sweeps
  as a landing.

## 1. Reproduction on this tree
- base.c = production copy: `[size 3264!=3248, sha]`, 32 REAL regions (dots #1/#2/#3 leaf order,
  fadd +0x944/+0x956, +0x9ec copy, tail regs).
- sc0.c = production perform replaced by Lane B owner_ruling_struct_copy.c body (4 `*(real_vector2d *)&v`
  copies, no unit_index / target_range locals): `[sha]`, 982/982, 2 REAL regions = dot #2 only
  (+0x8e1..+0x8f8: January i-term (0x2c/0x30? see sbs) order differs; 0-byte). Confirms Lane B result
  holds with the real_math.h (SELECT_ANY) helper linkage now in production.

## 2. M8 instrument: position is INERT (diagnostic only, never a landing)
Hypothesis: the dot #2 term key is a temp ordinal allocated after all user locals, so a user local
declared ANYWHERE shifts it equally. Probe v1.py: D dead `long` at 4 positions (function top, after
`melee`, in the suicide arm after relative_velocity, in the tail after `time`), D=1,2,6,7 on sc0.
Result: position completely inert. D=1: dot#2 only (same as sc0); D=2: dots #1/#2/#3 wrong;
D=6 and D=7: `EXACT 3248 _action_charge_perform`, object 22/22, at EVERY position.
=> needed: net user-local count change of +6 or +7 (== -2/-1 mod 8) relative to sc0, anywhere.
/Od (fn 0x430170) slot census vs sc0's 38 locals: /Od has exactly 7 more named slots:
-0x3e dead boolean (=0 after melee, never read), -0x88 alignment_vector (RTC, 2D, leap arm, dead),
-0x8d boolean (projectile_aim_ballistic result), -0xac target_point (RTC, dead adjusted copy),
-0xf4 real (suicide dot result), -0xf8 real (leaping dot result), -0x110 real (dot2d result).
The full /Od set is +7 == the needed count. The two RTC aggregates are DEAD in the /Od build too
(-0x88 written 3x + passed to normalize2d, never read; -0xac written, z adjusted, never read).

## 3. /Od-attested local sets on sc0 (v2.py; p/<name>.c)
A1 = `boolean aim_valid = FALSE; aim_valid = projectile_aim_ballistic(...); if (aim_valid)` (/Od -0x8d)
A2 = dead `boolean leap = FALSE;` after `melee` (/Od -0x3e: stored 0 at 0x430982, never read)
A3a/b/c = named reals for suicide dot (-0xf4), leaping dot (-0xf8), alignment dot2d (-0x110)
A4 = /Od dead leap-arm code: `real_vector2d alignment_vector = *(real_vector2d *)&direction;`
     normalize2d fallback chain (facing_vector cast, then *global_forward2d = /Od 0xa31df4, which is
     global_forward2d by pointer-table arithmetic from forward3d 0xa31e24 / up3d 0xa31e2c) and
     `target_point = prop->body_position; target_point.z += 0.2f;` (constant not yet verified).
Results: s4 (A4 alone) => NO extra bytes at all (VC7 inlines the two dead normalize2d and
eliminates everything) but dots wrong (count only). s1 (A1): same as sc0. s2 (A1+A2), s3, s5
wrong. s5b/s6b/s7/s6_noc (any set with BOTH A3a and A3b) => only fadd +0x944/+0x956 wrong.
EXACT (perform 3248, object 22/22): s4b = A1+A2+A4, s5_ac = +A3a, s6_nob = +A3a+A3c,
s6_noa = +A3b+A3c. Pure dead-long D=3,4,5 on sc0: NOT exact (dots wrong) - so A4's inlined
dead normalize2d expansions contribute more than 2 IL symbols.
Caveat: sc0 still carries the 4 load-bearing struct-copy casts (rule 24: not byte-inert).
- Constants read from the /Od exe data: target_point.z += 0.4f (0x93dd48), minimum_velocity =
  0.3f * leap_velocity (0x93dd44), suicide 0.023333 (0x93def0), 0.866 (0x93dd58). v2.py fixed to 0.4f.
- Cast-free control (v4.py on base.c = production scalar copies): base + A1 + A2 + A4(member-wise)
  +/- unit_index/target_range removal, and D=1..7 dead-long instrument on top: ALL `[size 3264!=3248]`
  32 REAL regions, completely count-insensitive => the aggregate-copy spelling is load-bearing
  (confirms Lane B); no cast-free route to exact exists in this family.
- Subsets on sc0 (0.4f): A1+A4 fail, A2+A4 fail, A4+A3c fail; EXACT: A1+A2+A4, A1+A4+A3c,
  A1+A4+A3a, A1+A4+A3a+A3c, A1+A4+A3b+A3c, A2+A4+A3c. Full /Od set (s7) fails only on fadd.
- Precedent found: action_obey.c:1259 `alignment_vector = *(real_vector2d const *)&actor->input.facing_vector;`
  was ADMITTED by the owner under the per-site rule-24 admission (commit 3ba2eb91), with the caveat that an
  aggregate-copy cast cannot be strip-tested; shown by /Od L,L,S,S single-base-load copy + all undecorated
  alternatives measured residual. Same idiom, same name, same facing_vector source as here.
- Combo map (all on sc0 + 4 casts): A4 is REQUIRED (no set without it is exact, u_no4/u_no4b).
  With A4: EXACT iff >=2 of {A1,A2,A3a,A3b,A3c} AND NOT (A3a and A3b together) [A3a+A3b => fadd
  +0x944/+0x956 flips, every scope of A3b tried: s7_else, s7_blk]. EXACT sets measured: A1+A2, A1+A3a,
  A1+A3b, A1+A3c, A2+A3a, A2+A3b, A2+A3c, A1+A2+A3b, A1+A2+A3c, A1+A3a+A3c, A1+A3b+A3c, A2+A3a+A3c,
  A1+A2+A3a+A3c, A1+A2+A3b+A3c. Full /Od set (a+b) fails => January lacked at least one of the two
  3D-dot named reals (the later build differs anyway: charge_state_data layout moved ticks 0x32->0x24,
  lead-fraction arithmetic differs). The exact subset is NOT byte-decidable.
- object_audit on p/t_a124.obj: OBJECT AUDIT PASS; surplus identical to production (no __real for 0.4f,
  no new undefined external such as _global_forward2d - VC7 drops the dead references entirely).
Decision: land A4 (RTC-named, required) + A1 (success result) + A3a + A3c (named dot results), i.e.
the /Od set minus the unnamed dead boolean (A2, filler-like) and minus the leaping real (A3b); RTC names
adopted for the aggregates. Alternative exact sets listed above for the reviewer.

## 4. cand1.c (sc0 + melee_block.txt) - EXACT
Changes vs production perform: (1) unit_index local removed (/Od reads actor->meta.unit_index twice);
(2) target_range local removed (/Od `state_data->acceptable_target_range = action_charge_find_target_range(...)`
and MAX(minimum_move_range, state_data->acceptable_target_range)); (3) four 2D aggregate copies
(`*(real_vector2d *)&...`) at /Od 0x430f04, 0x4310c8, 0x4311c7, 0x431203; (4) /Od dead leap-arm block:
alignment_vector (RTC -0x88) normalize2d fallback chain + nested block { success = FALSE (/Od -0x8d),
minimum_velocity (RTC), target_point (RTC -0xac) = prop->body_position, z += 0.4f, aim_vector,
vertical/horizontal (RTC); success = projectile_aim_ballistic(...); if (success) }; (5) named reals
closing_velocity (/Od -0xf4) and alignment (/Od -0x110); (6) RTC names: vector_to_target,
estimated_target_position, target_relative_velocity, target_alignment_vector, alignment_vector,
minimum_velocity; /Od declaration order (vector_to_target before debug_info; distance in the else arm).
gate: `== exact 22 residual 0 unwritten 0 (of 22 listed)`.
- Section-4 audit on cand1.obj: object_audit PASS (37 January symbols, 0 differ; surplus identical to
  production: .drectve, 2 assert strings, 3 D3D tables, __real@3f000000/__real@41f00000,
  _magnitude_squared3d, _vector_from_points3d); relocdiff 84/84, 0 differing rows; pdb_storage 0
  disagreements; surplus_identity 2 COMDATs IDENTICAL; provider_link SELECTED-PROVIDER LINK PASS;
  build/report.json data 256/256 (100%); coffsecs vs build/base: only `.text _action_charge_perform`
  (3264->3248) and .debug$S differ - every other section's bytes, flags and COMDAT selection identical.
  No parks, no object_admission_rejections entry for the unit (objects.obj's rejection names
  action_charge's NODUP `_real_random` as a PROVIDER conflict - that is objects.obj's blocker).
- Robustness (informational, provider question NOT decided here): cand1 with the `real_random` rename
  package deleted (helper from real_math.h) stays `== exact 22`; only _real_random comdat-sel 1->2.

## 5. Strip tests on cand1 (v5.py)
- C1 target_alignment_vector = *(real_vector2d *)&vector_to_target -> member-wise: residual (dots, count-keyed)
- C2 melee alignment_vector = *(real_vector2d *)&vector_to_target -> member-wise: residual [size 3264] 29 REAL
- C3 melee fallback = *(real_vector2d *)&actor->input.facing_vector -> member-wise: EXACT (byte-inert)
- C4 leap_alignment_vector = *(real_vector2d *)&aim_vector -> member-wise: EXACT (byte-inert)
- C5 dead-block alignment_vector init -> member-wise: residual (count)
- C6 dead-block fallback from facing_vector -> member-wise: EXACT (byte-inert)
- all four live sites member-wise: residual [size 3264]; both dead-block sites member-wise: residual.
- `boolean success;` without `= FALSE`: EXACT (init byte-inert; kept because /Od stores 0 at 0x430d83).
- drop closing_velocity alone: EXACT; drop alignment alone: EXACT; drop both: residual (count).
So: C3/C4/C6 casts satisfy the owner's rule-24 byte-inert test outright (plus /Od single-base-load
L,L,S,S struct-copy pattern at 0x431203 / 0x430f04 / 0x430d28). C1/C2/C5 are load-bearing
aggregate copies = the action_obey (3ba2eb91) precedent class (cannot be strip-tested; every
undecorated alternative measured residual).

## 6. FINAL (candidate.c == cand1.c; production.patch verified to reproduce it byte-for-byte)
- gate: `EXACT 3248 _action_charge_perform`, `== exact 22 residual 0 unwritten 0` (candidate_gate.txt).
- OBJECT AUDIT: PASS (candidate_audit.txt); SELECTED-PROVIDER LINK: PASS (candidate_provider_link.txt);
  pdb_storage 0 disagreements; surplus_identity 0 not identical; data 256/256; fake_match_scan 0 leads;
  /W3 compile clean. Only the perform .text section changes vs build/base (coffsecs).
- Patch touches ONLY the body of action_charge_perform (prefix/suffix bytes identical, CRLF kept).
- NODUP note: nodup_census `_real_random` -> action_charge.obj is the only NODUP definer (6 definers);
  that is the provider worker's question; measured: removing the real_random rename package keeps 22/22.
Owner-review items (must be disclosed on landing):
 (a) rule-24 aggregate-copy casts: 6 sites. C3/C4/C6 byte-inert (member-wise equally exact) with /Od
     single-base-load L,L,S,S struct-copy pattern (0x431203, 0x430f04, 0x430d28) - satisfies every
     condition of the 2026-09-21 per-site admission. C1/C2/C5 load-bearing (local->local from
     vector_to_target, /Od 0x4310c8/0x4311c7/0x430cec into RTC 8-byte 2D locals) - same class as the
     admitted action_obey.c:1259 copy (3ba2eb91).
 (b) /Od dead leap-arm code (alignment_vector normalize chain + adjusted target_point, RTC-named, dead in
     the /Od build too) - compiles to ZERO bytes but is required by the M8 count key; no exact set of
     /Od-attested locals exists without it. Previously dismissed (Astra research8) on the premise
     "absent from January's bytes"; measured here that VC7 eliminates it without trace.
 (c) named result locals success / closing_velocity / alignment (/Od -0x8d/-0xf4/-0x110): each alone
     byte-inert, >=2 of the small /Od set required. Exact subset not byte-decidable (see section 3 list).
