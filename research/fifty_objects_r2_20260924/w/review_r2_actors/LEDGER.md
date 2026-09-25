# REVIEW r2 actors ledger (reviewer)

## R0 patch provenance + gate (reviewer, HEAD b7c1b7d9)
- git apply --check: each of the 3 patches OK alone, all 3 together OK.
- Applied the 3 patches (GNU patch -p1) to a private copy tree (tree/) built from `git show HEAD:`; patched actors.c == final.c (sha 508ab4ec) modulo CRLF.
- gate --source tree/source/ai/actors.c --all --out patched.obj: `== exact 76 residual 0 unwritten 0`; _actor_input_update EXACT 2384; _actors_spawn_from_unit EXACT 672.

## R1 admission re-run (reviewer, on patched.obj built from HEAD+patch)
- HEAD source gate: 74/76 (input_update [sha]; spawn [size 688!=672, sha]); head.obj section-identical to build/base actors.obj (181/181).
- object_audit patched.obj: PASS, 137 January symbols 0 differ; new surplus vs base = +_add_vectors3d 48, +_point_from_line3d 48.
- cand_checks.py (surplus_identity+pdb_storage logic on candidate path): 18 candidate-only COMDATs, 0 not identical;
  _add_vectors3d == actor_combat, _point_from_line3d == action_charge, _real_random_range == action_alert; pdb storage 137 / 0 disagreements.
- provider_link --baseline=build/base actors.obj: both new symbols PASS (both orders inside tool); full run 46 PASS / 0 FAIL.
- nodup_census: _add_vectors3d 17 definers, _point_from_line3d 24, _real_random_range 12, NODUP none.
- secdiff base vs patched: only _actor_input_update + _actors_spawn_from_unit differ (+2 new COMDATs); split vs patched: 137 same 0 diff (all data sections equal).

## R2 CONCURRENCY FINDING (reviewer, 08:49)
- final.c CHANGED during my review (08:40:25, sha256 508ab4ec -> 80655bd2, sha1 d9dc3764 = primary SD1 declaration reorder in spawn).
  Primary worker MANIFEST.md (08:43) now exists and is PRIMARY: it supersedes r2w1b_actors_c_v2.patch with actors_final.patch
  (sha1 691e8fbc) + parked_retire.patch + config_status.patch. r2w1b v2 == SP1 state (angle declared first, acceleration last).
- My R0/R1 results above are for r2w1b_actors_c_v2 (== final.c sha256 508ab4ec at the time). Re-verifying the primary set below.

## R3 primary patch set verification (reviewer)
- git apply --check at HEAD b7c1b7d9: actors_final.patch (sha1 691e8fbc), parked_retire.patch (7e937267), config_status.patch (0e7d18f0) each OK, all 3 together OK.
- Applied to private tree2 (copy of worktree CRLF files): actors.c == final.c (sha1 d9dc3764) byte-for-byte.
- gate --all on tree2 actors.c: 76/76 EXACT; primary.obj section-identical to the r2w1b patched.obj (183/183) -> SD1 byte-inert.
- object_audit PASS; surplus 18/0 not identical; pdb 137/0; provider_link --baseline PASS both new symbols; full provider_link PASS.
- parked.json result identical to r2w1b's (84 entries, no actors parks); config.json result identical (actors.c Matching).

## R4 /Od re-derivation (reviewer; od_input_update.txt / od_spawn.txt regenerated with odbuild.py, 0x4486c0 located via the
   "is flying but was placed in a non-3d" string)
- swarm: short i [-0x14]; component=swarm_component_get(component_indices[i]) [-0x18] BEFORE call 0x44bb30(unit_indices[i], component_indices[i]);
  call 0x44e8e0(&comp->pos(+4), &swarm+0xc, &swarm+0xc) = add_vectors3d body a+b->result; scale 0x42e220; csmemset 0xa0;
  `cmp [actor+0x18],-1; je` skip sample then `jmp 0x4494fa` (the ONLY jump to the epilogue) => single exit. CONFIRMED.
- parent: ?: temp [-0x80] <- object_get_and_verify_type(idx,-1) -> [-0x24]; vehicle alias [-0x3c] pure copy after `type==1`. CONFIRMED.
- gunner: `or [0x128],8` store, THEN call 0x4507a0 -> [-0x44] named result. CONFIRMED (decl cannot carry the initialiser).
- migrate [-0x45]=0; `!=` arm sets 1 first (0x448b7b) else squad test; encounter [-0x4c] then migrate=1; squads [-0x50]/[-0x54]. CONFIRMED.
- prevehicle encounter [-0x58] named, RMW `or [eax+0xc],4`. CONFIRMED. Non-vehicle arm: 4 stores then direct read of meta bit 1 (no alias). CONFIRMED.
- mismatch flag [-0x61]=0; store 1 at 0x448da3 (if) and 0x448dc6 (else-if re-tests flying). CONFIRMED two stores.
- threat [-0x6c]=object_get(push -1); biped arm; `type==5` -> projectile alias [-0x70] copy; stores at 0x448f6f (flag 0x80) and 0x448f9a
  (danger_type==2 && idx==object_index). CONFIRMED two stores.
- cross_product3d(global_up3d,&looking,&left) real call, then normalize3d(push recomputed actor+0x164), then second cross. CONFIRMED.
- spawn: short count [-8]; ternary temp [-0xe8] -> source_actor_index [-0x10]; actor_get(unit+0x1b8) (uses actor_index, NOT the local; same
  semantics as production OR-chain); random 0x42f360 = real_seed_random_range(get_global_random_seed_address(),lo,hi) = real_random_range;
  point_from_line3d 0x42e0d0 body v*t+p called (&pos,&fwd,0.3f,&pos). Loop-body slots placement_data -0xb4 (RTC), spawned_unit_index -0xbc,
  angle -0xc0; if-block spawned_unit -0xc4, actor_index -0xc8; throw block acceleration -0xd8, speed -0xe0, vertical -0xe4.
  input_update's frame (center_of_mass aggregate at -0x34 AFTER -0x24 parent) shows no /GS aggregate hoisting in this build, so
  slot order = declaration order -> SD1 order is attested. CONFIRMED.
- HCEX actor_input_update: only actor_index (param) + center_of_mass; no other names -> descriptive names acceptable.

## R5 strip tests / warnings / scan (reviewer, on primary final)
- strip_addcasts (drop the 3 (real_vector3d *) casts): 76/76, object 183/183 identical to primary; /W3 adds 3 C4133 -> casts are
  type-required, byte-inert (per-site /Od attestation: /Od passes &component->position and &swarm->swarm_center to 0x44e8e0).
- strip_viewcasts (drop (struct unit_datum *) and (struct projectile_datum *)): 76/76, identical object -> byte-inert views.
- merge_threat_arms (|| single store): 75/76 (input_update sha) -> load-bearing; /Od-attested two stores.
- merge_mismatch_arms (|| single condition): 75/76 -> load-bearing; /Od-attested two stores.
- /W3 /Zs: primary 18 lines vs head 19 (removes the spawn `return long->short` C4244); no new warnings; no C4133/C4013.
- fake_match_scan primary: 0 leads (head 0).

## R6 amended patch + verdict (reviewer)
- Frozen copies of the primary set in amended/ (actors_c 691e8fbc, parked_json 7e937267, config_json 0e7d18f0) and
  combined amended/actors_admission_combined.patch (sha1 096b9dff): git apply --check OK at HEAD; applied to tree3 -> actors.c == final.c
  (d9dc3764) modulo CRLF, parked/config equal to tree2 results.
- objdiff 3.3.1 mini report (odproj/): base 15638/19130, 73/76; candidate 18686/19130 (+3048), 75/76; only _actor_handle_unit_effect
  96.8% (strict EXACT, already in semantic_report hidden_exact/accepted_ledger) -> strict 76/76, Matching label not contradicted.
- VERDICT: APPROVE object (OBJECT_COMPLETE_CANDIDATE) with amended patch = primary set (SD1 /Od declaration order); r2w1b v2 patch alone
  superseded (contradicts /Od declaration order in spawn loop, byte-inert). REVIEW.md written. git status clean.

## R2 independent re-verification (reviewer, second pass, 08:44-09:10, HEAD b7c1b7d9, tree clean)
- Patches: git apply --check OK singly and all 3 together (working tree, CRLF). `git apply --check --cached` FAILS
  (patch content lines are CRLF, index blobs are LF) -> integrator must apply to the working tree (plain git apply), not --index/--cached.
- tree2/ = worktree copies of actors.c/parked.json/config.json + GNU patch -p1 of all 3: patched actors.c sha1 d9dc3764 == final.c (cmp IDENTICAL).
- gate --source tree2/source/ai/actors.c --all --out rv.obj: `== exact 76 residual 0 unwritten 0`.
  gate of HEAD source (rv_head.obj) is section-identical to build/base actors.obj (181/181) -> gate compile == ninja compile for this unit.
- rv.obj vs worker final.obj: 183/183 sections identical. rv.obj vs build/base: only _actor_input_update, _actors_spawn_from_unit differ; +_add_vectors3d, +_point_from_line3d.
- object_audit rv.obj: PASS, 137 January symbols 0 differ; surplus delta vs production = exactly +_add_vectors3d 48, +_point_from_line3d 48.
- rv_surplus.py (surplus_identity logic on rv.obj): 18 candidate-only code COMDATs, 0 not identical; _point_from_line3d == action_charge,
  _add_vectors3d == actor_combat; January actors.obj references neither (all-inlined class); both COMDAT selection 2 (ANY).
- provider_link --baseline=build/base actors.obj: both PASS (both orders); full run 46 PASS, 0 FAIL.
- nodup_census: _add_vectors3d 17 definers, _point_from_line3d 24, NODUP none.
- Precedent: 14 base objects already emit each helper in the all-inlined class; Matching ones include actor_firing_position,
  actor_type_infection, path, render_sky, leaf_map, biped_limp_noodle (pfl3d) and device_machines (add_vectors3d).
- pdb storage (pdb_storage logic on rv.obj): 137 split symbols, 0 disagreements. cachebeta: _add_vectors3d/_point_from_line3d public,
  _actors_spawn_from_unit public, _actor_input_update/_actor_swarm_component_refresh absent (static) -- consistent.
- /W3 /Zs: candidate 6 file-level C4244 (all pre-existing lines), production 7 (spawn `return long->short` gone); no C4013/C4133.
- STRIP TEST (strip_casts.c): all three add_vectors3d point->vector casts + both typed-view casts removed -> _actor_input_update EXACT 2384 (byte-inert).
- fake_match_scan on patched actors.c: 0 review leads.
- objdiff 3.3.1 mini-project (odproj/): candidate 75/76 raw, code 18686/19130 (+3048 vs production 15638, = 668+2380), data 2288/2288;
  only <100 row is pre-existing _actor_handle_unit_effect 96.8, which build/semantic_report.json accepted_ledger credits
  (proof semantic-coff) before revoke_incomplete_units -> Matching label will not be revoked; no semantic_matches entry needed.
- parked.json: patched file = original minus the only two actors entries (86 -> 84), version unchanged; config.json one-line status flip; both parse.
- /Od re-read of every changed construct (od_actor_input_update / od_spawn_from_unit identical to my regenerated copies):
  swarm component get -> direct static call 0x44bb30(unit_indices[i], component_indices[i]) -> add_vectors3d thunk (&comp->pos,&center,&center);
  single `je` over the sample call then jmp end (if/else); parent ?: temp -0x80 -> -0x24 with push -1; view -0x3c pure copy;
  gunner store precedes call -> -0x44; `!=` arm first sets -0x45=1; -0x58 prevehicle encounter; stored flag read AFTER the 4 stores (no alias);
  -0x61=0, store at 0x448da3 then jmp 0x448dca over the else-if re-test, store 0x448dc6; threat object_get(-1) -0x6c, view -0x70,
  store 0x448f6f + jmp, store 0x448f9a; normalize3d(push actor+0x164) separate. Full /Od named-local inventory = final.c declaration order.
  Spawn: -8 short count ... -0x10 ternary local, -0xb4 placement, -0xbc spawned_unit_index, -0xc0 angle (random call before
  placement_data_new), -0xd8 accel, -0xe0 speed, -0xe4 vertical; random args (0,2pi),(0.5,1),(0.8,1.5); 0x42e0d0(&pos,&fwd,0.3,&pos).
- January spawn bytes 0x35-0x5c: ternary on [unit+0x1a8]/[unit+0x1a4], then RELOAD [unit+0x1a4] for datum_get -> January fetches
  unit.actor_index, not the ternary result (latent: swarm member => actor_get(NONE), asserts 1301/1302/870/871 make the fields exclusive).
  Semantics identical to production's OR-chain -> not a newly reproduced bug; note only.
- Assert-anchor drift (odbuild file "ai\actors.c"): Jan 868-2038 -> /Od +10, Jan 2733 (actor_general_update) -> +13,
  Jan 3313-3319 (input_update) -> +21, Jan 3436 -> +21. HCEX line table (same later numbering: input_update 3010-3345,
  input_sample_position 2988-3005, get_timeslice 2862-2897, general_update 2716-2857) cannot localise the 8 extra later-source lines
  between Jan 2733 and 3313 -> some may lie inside input_update. Residual authenticity risk for tie-only declarations; disclosed, not blocking.
- VERDICT: approve (no amendment needed).
- INCIDENT (pass 2): my odproj/ run (09:02) reused the directory name pass 1's mkodproj.py writes; its odproj files were overwritten.
  Same numbers either way (75/76, 18686/19130). No tracked file touched (git status clean). My review: REVIEW_pass2.md (+ addendum in REVIEW.md).
- Pass-1 amended/actors_admission_combined.patch (096b9dff) == cat of the three primary patches (cmp); git apply --check OK.
