# Independent review - source/ai/actor_combat (Lane B) - 2026-09-21

Reviewer re-gated everything from scratch; no worker number was reused.

## 1. Re-gate
- `unitcheck.py source/ai/actor_combat candidate.c`: BASELINE exact 31 / residual 3 -> CANDIDATE exact 32 / residual 2.
  GAINS: `_actor_aim_projectile` residual 720 -> EXACT 720. REGRESSIONS: none. other: none.
- `gate.py --fn _actor_aim_projectile`: `EXACT 720`. alndiff vs January: 237/237 instructions, only
  relocation-SPELLING rows (`symbol:__real@..` vs `defined-noncode:.rdata:__real@..`) remain.
- Whole-object section compare `review/objcmp.py base.obj cand.obj`: the ONLY .text section whose bytes/relocs
  change is `_actor_aim_projectile` (plus `$L` label renumbering in `_actor_combat_update`, same sha, and .debug$S).
  `_actor_combat_plan_grenade_trajectory` (PARKED) and `_actor_aim_grenade` are byte-identical to baseline.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed.
- `newsyms.py`: newly DEFINES 0, no longer defines 0 -> OWNERSHIP PASS. (The 10 surplus COMDATs the object emits
  are all already emitted by production - systemic, not a violation.)
- `fake_match_scan.py`: 0 leads on candidate, 0 on real source -> no new findings.
- Line endings: real 2458 CRLF / 0 LF; candidate 2459 CRLF / 0 LF. No tracked source/config/tool edits.

## 2. Diff (2 hunks, both inside `actor_aim_projectile`, a baseline NON-exact parked function)
1. `real_vector3d weapon_vector` moved from the bursting block head into a bare declaration block opened after
   `actor_debug_info->field_70 = *vector;`.
2. `real alignment` local dropped; `if (dot_product3d(&weapon_vector, vector) < GRENADE_AIMING_ANGLE_COSINE)`.
3. `field_88 = TRUE` now precedes `field_98 = weapon_vector`.
No casts, flags, pragmas, offsets, dummy locals, new symbols, or edits to any baseline-exact function.

## 3. Strip tests (all built under review/, gated with gate.py --all)
| variant | what | `_actor_aim_projectile` |
|---|---|---|
| v1_noblock_burstscope | no block, no alignment, new order (decl at bursting-block head) | residual [sha] |
| v2_noblock_fnscope | same, decl at function scope | residual [sha] |
| v3_base_orderswap | baseline + order swap only | residual [sha] |
| v7_base_noalign | baseline + alignment dropped only | residual [sha] |
| v6_block_oldorder | candidate with old 98-then-88 order | residual [sha] |
| v4_block_alignment | candidate + named `alignment` local kept | EXACT |
| v5_dot_swapped | candidate with `dot_product3d(vector, &weapon_vector)` | EXACT |
Load-bearing: the block AND the 88-before-98 order. Not load-bearing: the `alignment` local (dropping it is the
plain spelling and is what /Od shows), and the dot operand order (LAW M8 inert).

## 4. Attestation (my own /Od dump: review/od_aimproj.txt, fn 0x44f010)
- Statement order: /Od 0x44f328 `mov byte [eax+0x88],1` THEN the field_98 copy THEN field_8C. January 0x274-0x2a5
  interleaves the byte store inside the field_98 copy and only the 88-first source reproduces it. Double attested.
- No named `alignment`: /Od stores the dot result to [ebp-0x44], in the compiler-temp zone with the two
  normalize3d result temps (-0x48/-0x4c), below aiming_success (-0x3d). Declared-local positions are all taken
  by RTC guards. Plain spelling, attested.
- Bare block: /Od slots run in declaration order: actor -8, target_unit_index -0xc, actor_debug_info -0x10,
  prop -0x14, weapon_vector -0x24, rotation_axis -0x38, aiming_success -0x3d. weapon_vector sits AFTER `prop`
  (declared in a nested if-block), which in C89 is only possible if weapon_vector is declared in a block opened
  later than the prop block - i.e. the candidate's shape, not the baseline's. (The worker did not cite this; it is
  stronger than "scope cannot be attested".) January: only the block form matches (v1/v2 fail). First-party style
  precedent verified: haloleak network_server_manager.c lines 618 and 633 are bare declaration blocks; five such
  blocks already exist in this file's exact functions. Verdict: genuine C89 late-declaration idiom, admissible.

## 5. Flag for the orchestrator/owner (PRE-EXISTING, not in the diff)
The function's exactness also rests on baseline text the worker did not touch (eb5bb428f, 2026-09-02):
`real_point3d const *burst_target = &actor->control.burst_target;` + a hand-written three-line subtraction.
/Od 0x44f164-0x44f195 shows NO such pointer slot and a REAL call `vector_from_points3d(origin,
&actor->control.burst_target, vector)`. January has the body inline (`lea eax,[ebx+0x67c]; mov ecx,eax; ...
fld/fsub x3`). Measured here: helper call without pointer (v8a) and with pointer (v8b) = size 704!=720, relocs
28!=27 (our build keeps the helper out of line at this site - the systemic real_math inline boundary);
manual without pointer (v8c) = size 736. So no honest helper-call spelling is exact today. This is baseline
content and the same class as many already-credited functions; it does not make the DIFF inadmissible, but the
owner should know the credit for this function leans on it.

Optional fidelity follow-up (not required): review/v5_dot_swapped.c is also EXACT and uses the /Od-attested
operand order `dot_product3d(vector, &weapon_vector)`; January's bytes cannot discriminate.

## 6. Withheld variants (not part of candidate.c; correctly withheld)
- variant_plan_alias.c: `real_point3d const *target = grenade_target;` entry copy + bare block. The alias is a
  parameter-alias carrier with no /Od slot (worker's own finding) -> inadmissible under brief 6a without an owner
  ruling. Must not land. Candidate leaves the parked function's bytes untouched (verified).
- variant_aim_grenade_helpers.c: newly defines 5 real_math COMDATs January's object lacks -> ownership failure.
  Must not land.

## 7. Verdict
LAND candidate.c as is. `_actor_aim_projectile` ACCEPT_EXACT (unpark it). Other two NO_CHANGE.
Parked functions whose bytes change: `_actor_aim_projectile` only (it becomes exact -> remove the park entry).
