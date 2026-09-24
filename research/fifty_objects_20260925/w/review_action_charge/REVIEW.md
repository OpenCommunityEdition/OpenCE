# Adversarial review: source/ai/action_charge (OBJECT_COMPLETE_CANDIDATE)

Verdict: **REJECT for landing as submitted (approve=false). HOLD_FOR_OWNER.**
The byte claims all reproduce. The candidate is not admissible under the brief and the owner
rulings as they stand. What closes the function is a zero-byte dead-code count lever. Three
initialised dummy locals, or a made-up dead statement in another arm, reproduce it equally.

## 1. Byte and whole-object claims: all reproduced independently
The files are in this directory. Tree e9e62b78; production action_charge.c is unmodified (sha cdfd8577).
- gate --all on cand.c (== worker candidate.c, sha 442f4388): EXACT 3248 _action_charge_perform, 22/22.
  A fresh production compile gives [size 3264!=3248, sha], 21/1.
- object_audit: PASS, 37 January symbols, 0 differ. The surplus list is identical to the production compile's.
- coffsecs vs build/base/source/ai/action_charge.obj: only .text perform (3264->3248) and .debug$S differ.
- relocdiff 84/84, 0 differing. provider_link: SELECTED-PROVIDER LINK PASS. surplus_identity: 2/2 IDENTICAL.
- pdb_storage: 0 disagreements. build/report.json data 256/256. fake_match_scan: 0 leads.
  CL /Zs /W3: the same 12 header warnings as production, and none from action_charge.c.
- production.patch applied to a copy of production reproduces cand.c byte-for-byte.
- /Od re-read: the dead alignment_vector chain is at 0x430cec..0x430d7d. success=0 is at 0x430d83, and the
  dead target_point is at 0x430d8a..0x430dbc. The constants are 0.4f/0.3f
  (read as data: 0x93dd48/0x93dd44). 0xa31df4 = global_forward2d (forward3d is 12 pointers later,
  0xa31e24, matching real_math.c order). C1/C2/C4/C5/C6 are integer 8-byte copies in /Od, and scalar real copies there
  are movss. The aggregate-copy attestation therefore holds at each site.

## 2. Blocking findings
B1. **The closing lever is a zero-byte dead computation that has no January attestation.** It is
    `real_point3d target_point = prop->body_position; target_point.z += 0.4f;`, and target_point is never read.
    In /Od, projectile_aim_ballistic at 0x430e42 is passed prop+0x9c (&prop->body_position), so the raised
    point is dead there too. It looks like a later-build defect: an aim point that was adjusted and never used.
    - Dropping target_point makes the function residual (no_a4_tp). A bare `real_point3d target_point;` (tp_decl) is residual,
      and so is the copy without the RMW (tp_noz). What is load-bearing is the dead copy plus the dead RMW, and both emit zero bytes.
    - **An unattested `{ real_vector3d scratch_vector = unit->object.translational_velocity;
      scratch_vector.k += 0.4f; }` in the SUICIDE arm is EXACT (x_suicide).** The same dead code moved to the top of the melee block
      is EXACT (x_top), and a copy from a different source at the same site is also EXACT (x_same_site_z).
    - With all the dead leap-arm code removed (no_a4), 3..7 initialised dummy `long m8_N = 0;` also give EXACT.
    The byte match therefore cannot tell the /Od code apart from fillers. It is the name-count/IL-oracle
    instrument, which the brief says is "never a landing". The only evidence that January had this code is a later build,
    and that build is known to differ in this same function (lead-fraction arithmetic, the charge_state_data layout).
    The Astra research8 review and the Lane B review both declined this "spare RTC target_point" route. Reproducing an
    apparent authentic bug also needs target proof, a BUG comment and owner approval. The patch has no comment at all.
B2. **The rule-24 casts C1/C2 are load-bearing** (c1m and c2m are both residual). Owner ruling 2026-09-21 #6 requires a
    byte-inert cast. The only exception is the action_obey.c aggregate-copy precedent (3ba2eb91), which the owner
    admitted per site with an explicit caveat. C1/C2 are the same class and have correct /Od integer-copy attestation
    (0x4310c8, 0x4311c7). They still need an explicit per-site owner ruling and cannot be self-admitted.
B3. **The named-result set is chosen to fit the count, not recovered.** The /Od build names both 3D dot results
    (-0xf4 suicide, -0xf8 leap) and alignment (-0x110). The full /Od set is residual (full_od, plus_a3b). Naming the
    LEAP dot instead of the suicide dot is also EXACT (swap_a3a_a3b), and dropping both named reals is residual. The landed subset
    (closing_velocity + alignment) is one of many count-satisfying choices and deliberately departs from /Od.

## 3. Worker claims that are inaccurate
- "A4 is REQUIRED": this holds only for the target_point half. With the whole dead alignment_vector
  normalize2d chain removed, including the rule-24 casts C5 and C6 (no_a4_vec), the function is still **EXACT 22/22, and object_audit PASS**.
  The candidate carries an unnecessary dead block and two unnecessary view casts, so C5's "load-bearing" label is misleading.
- "the key is a count of IL locals": uninitialised declarations do not count (sc0 plus 6/7 bare `long x;` is residual; tp_decl is residual).
  The currency is locals that receive IL assignments.

## 4. House-rule notes on the diff (minor, only relevant if the owner admits the item)
- The dead code violates rule 21 (nonsensical logic) unless it is disclosed as inferred or an authentic defect with a comment.
- The new view casts use `(real_vector2d *)`. The admitted precedent uses `(real_vector2d const *)`.
- Removing unit_index and target_range is /Od-attested and consistent with Lane B diag_7/diag_8. It is acceptable.
- `success = FALSE` is byte-inert and /Od-attested (0x430d83). It is acceptable.
- There are no header, config or symbols changes, so there is no cross-object blast radius. Helper copies are untouched.

## 5. Recommendation
Do not flip config.json index 470, and do not land the patch. Preserve cand.c at zero credit as a HELD candidate
and ask the owner three separate questions, stating every fact:
 (a) Admit the load-bearing aggregate-copy casts C1/C2 on the action_obey precedent?
 (b) Admit the /Od-only dead `target_point` copy+RMW, knowing that a fabricated dead RMW or 3 dummy
     initialised longs close the function equally, so the bytes do not attest it?
 (c) Which named-result subset (several are exact, and the full /Od set is not)?
If (b) is refused there is no admissible exact form. If (b) is admitted, the minimal form is no_a4_vec.c, which has no dead
alignment_vector chain and no C5/C6 casts, carries a BUG/inferred comment on target_point and discloses it in a matching log.
