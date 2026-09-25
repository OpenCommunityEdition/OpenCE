# REVIEW source/ai/actors

approve: True

amended_patch: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2_actors/amended/actors_admission_combined.patch (sha1 096b9dff; the pass-1 reviewer wrote it, and I verified it with cmp and git apply --check). It is identical to applying the primary set scratch/w/actors/actors_final.patch (691e8fbc) + parked_retire.patch (7e937267) + config_status.patch (0e7d18f0) with no changes, so no amendment is needed.

## per_object
[
 {
  "unit": "source/ai/actors",
  "approve": true,
  "issues": "Nothing blocks approval. gate gives 76/76 EXACT and object_audit passes (137/0). The only new surplus is _add_vectors3d and _point_from_line3d, each byte-identical to January's selected copy (ANY, no NODUP), and provider_link passes in both orders. PDB storage shows 0 disagreements, data is identical to production and 100%, casts are strip-inert, and there are 0 fake-match leads. Notes: apply the CRLF patches to the working tree, not with --cached. input_update exactness rests on the complete /Od declared-local set (8 unlocalised later-source lines of assert drift remain as disclosed residual risk). January's spawn has a latent actor_get(unit.actor_index) after the ternary, which production already reproduces. Re-run provider_link if the reserved actor_combat packet changes its _add_vectors3d emission."
 }
]

## checks
All checks were re-run independently at HEAD b7c1b7d9 (canonical f6d00a8c plus one docs file; source/, config/ and tools/ are unchanged). The tree stayed clean. Evidence is in C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2_actors/ (LEDGER.md second-pass section, REVIEW_pass2.md, and an addendum appended to REVIEW.md).
(1) git apply --check passes for actors_final.patch, parked_retire.patch and config_status.patch, alone and together, on the working tree. The three patches, applied with patch -p1 to worktree copies in tree2/, give an actors.c byte-identical to final.c (sha1 d9dc3764).
(2) gate.py --source tree2/.../actors.c --all --out rv.obj gives `== exact 76 residual 0 unwritten 0`: _actors_spawn_from_unit is EXACT at 672 and _actor_input_update is EXACT at 2384. A gate compile of the HEAD source is section-identical to build/base (181/181), so gate matches ninja for this unit. rv.obj and the worker's final.obj are section-identical (183/183). rv.obj against production differs only in those 2 functions plus 2 new COMDATs; every data section and .drectve is identical.
(3) object_audit rv.obj: PASS, 137 January symbols, 0 differ. The surplus delta against production is exactly +_add_vectors3d (48 B) and +_point_from_line3d (48 B).
(4) The surplus_identity logic run on rv.obj: 18 candidate-only code COMDATs, 0 differ. _point_from_line3d matches the action_charge copy and _add_vectors3d matches the actor_combat copy. Both use selection ANY, and January's actors.obj references neither, which is the all-inlined class. nodup_census: 24 and 17 definers, no NODUP. The class is already landed in Matching objects: actor_firing_position, actor_type_infection, path, render_sky, leaf_map and biped_limp_noodle for pfl3d, and device_machines for add_vectors3d.
(5) provider_link --baseline=build/base actors.obj: both new symbols PASS in both orders. The full run is 46 PASS, 0 FAIL.
(6) The pdb_storage logic on rv.obj against cachebeta_publics: 137 symbols, 0 disagreements.
(7) /W3 /Zs: 6 C4244 warnings, all on pre-existing lines (production has 7). No C4013 or C4133.
(8) STRIP TEST: removing all three (real_vector3d *) casts in add_vectors3d and both typed-view casts leaves input_update EXACT, so every cast is byte-inert.
(9) fake_match_scan on the patched actors.c: 0 leads.
(10) objdiff 3.3.1 on a mini-project scores the candidate 75/76 raw. Code is 18686 of 19130 (+3048 over production's 15638) and data is 2288/2288. The only row under 100% is the pre-existing _actor_handle_unit_effect at 96.8. build/semantic_report.json accepted_ledger credits it (semantic-coff) before revoke_incomplete_units runs (checked in project_x86.py and audit_object_admission.py), so the Matching label holds without a semantic_matches entry.
(11) parked.json loses only the 2 actors entries (86 to 84), the version is unchanged and the JSON is valid. config.json is a one-line NonMatching-to-Matching change and is valid. There are no actors rejections in object_admission_rejections or semantic_credit_rejections.
(12) Every changed construct was re-read against the /Od readout of 0x4486c0 and 0x44dfe0; I regenerated the readouts and they are identical to the worker's. The swarm arm is a direct static call to 0x44bb30, then add_vectors3d, with a short counter. The tail is if/else with a single exit. The parent is taken with object_get (push -1) into -0x24, with a pure-copy view at -0x3c. The gunner store precedes the call whose result goes to -0x44. The != arm comes first. The prevehicle encounter is at -0x58. The stored flag is read after the 4 stores. The mismatch flag is at -0x61, with stores at 0x448da3 and 0x448dc6 and a jmp over the else-if. The threat loop is object_get into -0x6c with a view at -0x70 and two stores. normalize3d is a separate statement. In spawn, the ternary temp at -0xe8 goes to -0x10; there are 3 real_random_range calls, a real call to point_from_line3d at 0x42e0d0, a word counter, and the SD1 declaration order. The full named-local inventory equals final.c's declaration order.
(13) I checked the rewrites for semantic equivalence and house rules. Two hand expansions are removed. There is no new prototype, header, flag or symbols.json change. Names are descriptive and follow codebase convention. The removed macros were TU-local.
(14) Assert-anchor drift, from odbuild file and the HCEX line table: +13 at January 2733 and +21 at 3313-3436.
(15) The pass-1 reviewer's amended/actors_admission_combined.patch (096b9dff) is the exact concatenation of the three primary patches, and git apply --check passes on it.

## issues
Nothing blocks approval.

Integrator notes:
(a) The patch content lines are CRLF. Apply them with plain `git apply` to the CRLF working tree; `git apply --cached/--index` fails against the LF index.
(b) input_update exactness depends on the declared-local set, through the x87 phase at cross_product3d site 2. final.c uses the complete /Od named-local inventory with no selection among features. r2w1b found a different exact subset, but it contradicts /Od in several places (long counter, no firing-variant local, no parent view, keeps the alias), so final.c is the principled landing. Residual authenticity risk: assert-anchor drift is +13 at January 2733 (actor_general_update) and +21 at 3313 (input_update). The HCEX line table cannot place the 8 later-source lines in between, so some may be inside input_update. This is disclosed and acceptable under the /Od evidence standard.
(c) In spawn, January's bytes (0x35-0x5c) compute the swarm/actor ternary but then reload unit.actor_index for actor_get. Swarm members have actor_index NONE (asserts 1301/1302/870/871), so a swarm-member source would call actor_get(NONE). This is a latent January defect, and production's OR-chain behaves the same way, so the patch adds no new behaviour. The owner may want a BUG comment.
(d) _add_vectors3d's January provider is actor_combat, which is RESERVED for the Codex packet. Re-run provider_link for actors if that packet changes actor_combat's COMDAT emission.
(e) Run the full ninja build and stable diff. No header is touched, so only actors.obj can change. Do not gate actors with --forbid-emitted-symbol _point_from_line3d.
(f) r2w1b_actors_c_v2.patch is superseded. It differs only by a byte-inert declaration order that contradicts /Od.
(g) The MANIFEST says HEAD b7c1b7d9 touches only scratch files. It actually adds one docs file, but source/, config/ and tools/ are unchanged, so the patches are unaffected.
(h) Incident: two reviewers worked in this slug at the same time. My odproj/ run at 09:02 overwrote the files pass 1's mkodproj.py had written to the same directory. Both runs give the same numbers. No tracked file was touched.
