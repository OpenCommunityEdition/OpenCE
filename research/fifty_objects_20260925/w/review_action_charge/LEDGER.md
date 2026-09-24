# review_action_charge ledger (adversarial review of scratch/w/action_charge candidate)

## R0 independent re-measure (2026-09-23)
- cand.c = copy of scratch/w/action_charge/candidate.c (sha256 442f4388...). Tree HEAD e9e62b78; production action_charge.c sha cdfd8577 (unmodified).
- gate --all: EXACT 3248 _action_charge_perform; == exact 22 residual 0 unwritten 0 (cand_gate.txt). Production compile: [size 3264!=3248, sha], 21/1.
- object_audit cand.obj: PASS, 37 January symbols 0 differ; surplus list == production compile surplus (prod_audit.txt).
- coffsecs vs build/base/source/ai/action_charge.obj: only .text _action_charge_perform (3264->3248) and .debug$S differ.
- relocdiff perform: 84/84, 0 differing rows. provider_link: SELECTED-PROVIDER LINK PASS. surplus_identity: 2 COMDATs IDENTICAL.
- pdb_storage: 0 disagreements. build/report.json data 256/256 (100%).
- HCEX.pdb (hcex_perform.txt): stack locals vector_to_target, horizontal/vertical_velocity, aim_vector, minimum_velocity, target_alignment_vector, ONE alignment_vector. No success/closing_velocity/alignment/target_point names (optimized build) -> neutral.
- /Od re-read (od_perform.txt): 0x430cec..0x430d7d dead alignment_vector (-0x88) chain; 0x430d83 success=0 (-0x8d); 0x430d8a..0x430dbc target_point (-0xac) = prop->body_position, z += [0x93dd48]; projectile_aim_ballistic at 0x430e42 is passed prop+0x9c (&prop->body_position), NOT &target_point -> target_point dead in /Od too (looks like a later-build bug: raised aim point never used).
  /Od names BOTH dot results -0xf4 (suicide, 0x430fcf) and -0xf8 (leap, 0x431021); candidate names only -0xf4 (count-selected).

## R1 strip/variant gates (mkv.py, v/*.c; variants_gate.txt) - hypothesis: each claimed-required piece should be individually load-bearing, and the dead-long count model should predict any removal
- no_a4 (drop dead alignment_vector chain AND target_point): residual [sha]
- no_a4_vec (drop ONLY the dead alignment_vector normalize chain incl. casts C5/C6; keep target_point): **EXACT 3248, 22/22** -> the dead alignment_vector block is NOT required; worker's "A4 REQUIRED" is true only of the target_point half.
- no_a4_tp (drop ONLY target_point + z += 0.4f): residual -> the load-bearing zero-byte item is the dead, never-read target_point.
- c1m / c2m (member-wise C1 / C2): residual [sha] / [size 3264] -> casts load-bearing (confirms worker).
- full_od (+ named leap-dot real + dead boolean after melee): residual; plus_a3b: residual (confirms worker).
- swap_a3a_a3b (name the LEAP dot, unname the SUICIDE dot): **EXACT** -> another exact alternative; named-result choice not byte-decidable.
- no_named (drop closing_velocity and alignment): residual.
- no_a4 + D dead longs after `melee` D=0..7: ALL residual -> the plain count model (worker section 2, "D=6/7 exact anywhere") does NOT predict this family; the key is not a pure local count.
- no_a4_vec object_audit: PASS (same surplus as production).
- What about target_point is load-bearing (base = no_a4_vec, EXACT): tp_noz (keep `= prop->body_position`, drop `z += 0.4f`) residual; tp_decl (bare `real_point3d target_point;`) residual; tp_long / tp_real (bare scalar) residual; tp_used (pass &target_point to projectile_aim_ballistic) residual [size 3296, relocs 85] -> January passes &prop->body_position. The lever is the DEAD aggregate copy + dead RMW, which emit zero bytes.
- /W3: candidate and production give the identical 12 header warnings, none from action_charge.c.
- production.patch applied (patch -p1 --binary) to a copy of production reproduces cand.c sha 442f4388 byte-for-byte.
- config/object_admission_rejections.json: action_charge only appears as the NODUP provider in other objects' rejections; no park; config.json index 470 NonMatching.

## R2 is the zero-byte lever specific to the /Od content? (mkv3.py; base = no_a4, residual) - hypothesis: if the byte match really confirms January's dead target_point, unrelated dead code should NOT close it
- x_top (the /Od target_point copy + z += 0.4f moved to the top of the melee block): EXACT -> position inert.
- x_same_site_z (same site, copy from actor->input.position.body_position, z += 0.4f): EXACT -> source object inert.
- x_same_site_other (same site, actor body_position, x += 1.f): residual.
- **x_suicide (UNATTESTED `{ real_vector3d scratch_vector = unit->object.translational_velocity; scratch_vector.k += 0.4f; }` in the suicide arm, no target_point at all): EXACT 3248, 22/22.**
=> the closing lever is "any dead 12-byte aggregate copy + dead RMW of its third component, anywhere in the melee block". It emits zero bytes and a fabricated statement satisfies it equally, so the byte match gives NO confirmation that January contained the /Od target_point code. This is the name-count/IL-oracle class (brief: never a landing) unless the owner rules otherwise.

## R3 dead-long instrument re-run (diagnostic only)
- UNinitialised `long x;` after `melee` on sc0, D=6/7: residual (sc0_d6/d7) - bare declarations do not count.
- Worker's form `long m8_N = 0;` after `melee`: sc0 D=6,7 EXACT (reproduces worker); D=0..5 residual.
- Same initialised instrument on no_a4 (candidate minus ALL dead leap-arm code): D=0..2 residual, **D=3..7 EXACT**.
=> three initialised dummy longs are a drop-in replacement for the candidate's dead target_point/alignment_vector code. The landed dead code is doing exactly what the banned filler instrument does; its only distinguishing support is the later, divergent /Od build.
