# review source/ai/action_charge
approve=False

## checks
I re-measured everything independently in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_action_charge\ (REVIEW.md, LEDGER.md, cand_gate.txt, variants_gate.txt, v\*.c/.obj, mkv*.py).

Byte and whole-object claims (all confirmed):
- gate.py --source cand.c --all (cand.c is the worker's candidate.c, sha 442f4388): EXACT 3248 _action_charge_perform, == exact 22 residual 0 unwritten 0. A fresh production compile gives [size 3264!=3248, sha] and 21/1.
- object_audit: PASS, 37 January symbols, 0 differ. The surplus list is identical to the production compile's.
- coffsecs against build/base/source/ai/action_charge.obj: only .text perform and .debug$S differ.
- relocdiff: 84/84, 0 differing rows.
- provider_link: SELECTED-PROVIDER LINK PASS.
- surplus_identity: 2/2 IDENTICAL.
- pdb_storage: 0 disagreements.
- build/report.json: data 256/256.
- fake_match_scan: 0 leads.
- CL /Zs /W3: same 12 header warnings as production, none from action_charge.c.
- `patch -p1 --binary` on a copy of production reproduces cand.c byte-for-byte.
- Parks, rejections and config.json index 470 checked; no action_charge entries.
- HCEX.pdb dump: neutral (no scalar local names).

/Od evidence re-read from od_perform.txt, with constants read from the exe as data only:
- 0.4f at 0x93dd48 and 0.3f at 0x93dd44.
- 0xa31df4 is global_forward2d: global_forward3d is 12 pointers later at 0xa31e24, matching the order in real_math.c.
- C1, C2, C4, C5 and C6 are integer 8-byte copies in /Od (0x4310c8, 0x4311c7, 0x430f04, 0x430cec, 0x430d28). Scalar copies there are movss.
- target_point is dead in /Od as well: projectile_aim_ballistic at 0x430e42 is passed prop+0x9c, i.e. &prop->body_position.

Strip tests and diagnostics (lab only):
- no_a4 (all dead leap-arm code removed): residual.
- no_a4_vec (only the dead alignment_vector chain removed, including casts C5/C6): EXACT 22/22, audit PASS.
- no_a4_tp, tp_noz, tp_decl, tp_long, tp_real: residual.
- tp_used (pass &target_point to projectile_aim_ballistic): size 3296, 85 relocs.
- c1m and c2m (member-wise copies): residual.
- full_od and plus_a3b: residual.
- swap_a3a_a3b (name the leap dot result instead of the suicide one): EXACT.
- no_named: residual.
- x_top, x_same_site_z, and x_suicide (a made-up dead `scratch_vector = unit->object.translational_velocity; scratch_vector.k += 0.4f;` in the suicide arm): all EXACT.
- Initialised dummy `long m8_N = 0;` locals: sc0 is EXACT at D=6/7, which reproduces the worker's result. no_a4 is EXACT at D=3..7.
- Uninitialised dummy locals: sc0 stays residual at D=6/7.

## issues
Recommendation: HOLD_FOR_OWNER. Do not apply production.patch and do not flip config.json index 470. The byte and audit claims are all true, but the closure is not admissible under the brief or the owner rulings.

B1 (blocking). The lever that closes the function is zero-byte dead code with no January evidence behind it: `real_point3d target_point = prop->body_position; target_point.z += 0.4f;`, and target_point is never read.
- In the /Od build it is dead too: the raised aim point is never passed to projectile_aim_ballistic. It looks like a defect in that later build.
- The byte match cannot confirm it. A made-up dead aggregate copy-and-modify in the suicide arm (x_suicide) closes the function to EXACT just as well. So does the same code at another position (x_top) or with another source vector (x_same_site_z).
- With all the dead leap-arm code removed, 3 to 7 initialised dummy longs also give EXACT. Three `long m8_N = 0;` locals are a drop-in replacement for the landed dead code.
- This is the name-count / IL-oracle instrument, which the brief says must never land. Its only support is a later build that is known to differ in this same function (lead-fraction arithmetic, the state layout).
- Astra research8 and the Lane B reviewer both declined this route before.
- Reproducing an apparent authentic bug needs target proof, a BUG comment and owner approval. The patch carries no comment at all.

B2 (blocking). Rule-24 casts C1 and C2 are load-bearing: the member-wise forms c1m and c2m are residual. Owner ruling #6 (2026-09-21) requires a byte-inert cast. The only exception is the per-site admission of the action_obey.c aggregate copy (3ba2eb91). C1/C2 are the same class and their /Od integer-copy evidence is correct (0x4310c8, 0x4311c7), but they need an explicit per-site owner ruling and cannot be self-admitted.

B3. The set of named result locals is fitted to the count, not recovered.
- The /Od build names both 3D dot results (-0xf4 suicide, -0xf8 leap), yet the full /Od set is residual.
- Naming the leap dot instead of the suicide dot is also EXACT (swap_a3a_a3b).
- The landed closing_velocity + alignment pair is one of many exact choices, and it departs from the /Od build on purpose.

Inaccurate worker claims:
- "A4 is REQUIRED" is only true of the target_point half. no_a4_vec drops the whole dead alignment_vector normalize2d chain, including rule-24 casts C5 and C6, and is still EXACT 22/22 with object_audit PASS. The candidate therefore carries an unnecessary dead block and two unnecessary view casts, and C5's "load-bearing" label is misleading.
- "The key is a count of IL locals" is too broad. Uninitialised declarations do not count (sc0 plus 6/7 bare longs stays residual; so does tp_decl). What counts is locals that receive an assignment.

Minor, only relevant if the owner admits the items:
- The dead code breaks rule 21 unless it is commented as inferred or as an authentic defect.
- The new casts use `(real_vector2d *)`, while the admitted precedent uses `(real_vector2d const *)`.
- Removing unit_index and target_range, and `success = FALSE`, are /Od-attested and fine.
- No header, config or symbols changes are involved, so there is no risk to other objects.

Questions to put to the owner:
(a) Admit C1/C2 on the action_obey precedent?
(b) Admit the /Od-only dead target_point copy-and-modify, knowing the bytes cannot tell it apart from fillers?
(c) Which named-result subset?
If (b) is refused, no admissible exact form is known. If (b) is admitted, land the minimal no_a4_vec.c form (scratch/w/review_action_charge/v/no_a4_vec.c), with a BUG/inferred comment and a matching log that discloses every item.
