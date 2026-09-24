# review source/interface/interface
approve=True

## checks
All checks are independent re-runs. Evidence is in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review_interface/ (REVIEW.md, audit_*.txt, rlab*.c, wA.c/wB.c, rfsA*.c, sA_noproto.c/sB_noproto.c, odproj/).
1. Provenance: diffed production interface.c against cand_interface.c. The difference is exactly production.patch, and the function body is unchanged. `git apply --check` (non-mutating) passes for all three patches. Applying them to scratch copies reproduces cand_interface.c and cand_interface_variantA.h byte for byte, with CRLF kept.
2. Storage evidence:
   - cachebeta_publics.txt: _interface_get_weapon_hud_index, _render_debug_profile and _interface_splitscreen_render are absent. All six _profile_* data symbols are present.
   - pdb_storage.py reproduces all 9 disagreements.
   - HCEX DIA2Dump: interface_get_weapon_hud_index has "static function: true" / `static long ...(float *)`, and interface_splitscreen_render is static. The control, interface_draw_screen, is not static, so the tool does tell them apart. render_debug_profile is absent from HCEX, so only cachebeta attests it.
3. Gate: `gate.py --source cand_interface.c --all --out review_interface/cand.obj` gives exact 18/18 (Variant B, original header). A Variant A build (header without the prototype, abs include) also gives 18/18. coff_compare: weapon_hud_index is strict EQUAL to split (448/20/0673f867...). The only .text section that differs between base and cand is that function. _sine/_cosine are identical, and .text emission order is unchanged.
4. object_audit on cand.obj: every January-owned section is ok. The only symbol DIFFs are storage 2/3 on the 3 functions. I also audited a scratch copy of the split with those 3 storage bytes flipped to 3 (simulating symbols_static.patch plus csplit regen): 34/34 symbols match and the result is OBJECT AUDIT: PASS. symbols.json lines 3535/3539/3540 are the right entries, the patch applies to a scratch copy, the JSON still parses, and no other split object references the three symbols.
5. Data: .data (100632 B, 18 relocs) is strict EQUAL in both cand and base, and cand storage now equals split. All 10 .rdata literals are EQUAL.
6. Surplus: surplus_identity shows sine/cosine IDENTICAL to actor_combat. provider_link on cand.obj passes for all 17 surplus symbols (SELECTED-PROVIDER LINK: PASS).
7. objdiff 3.3.1 (build/tools) mini-project: cand 5695 B and 17/18 functions; prod 5251 B and 16/18. The only function below 100% is stall_tick at 67.27%.
8. Warnings: cand at /W3 matches production. /W4 adds exactly one C4211 "redefined extern to static". Variant A has no C4211.
9. Strip tests:
   - Removing `static` from the definition gives the production residual. The keyword is attested January storage, not decoration.
   - The static prototype in the .c file does no work: removing it gives 18/18 in both Variant A and Variant B.
   - The render_debug_profile / splitscreen_render `static` changes and the profile_* changes are bytes-inert storage corrections.
10. Blast radius:
   - Variant A with a byte-preserving include swap: rasterizer_frame_statistics_draw goes from EXACT to residual [sha]; production is 10/10.
   - Oracle only, not for landing: +1 extern-long dummy stays residual, +3 gives EXACT. That confirms a pure declaration-count tie.
   - No header includes interface.h, so the 18 direct consumers are the full set.
11. Mechanism lab:
   - rlab15 (copy of the worker's lab15) reproduces the effect: static gives an integer copy, extern gives x87.
   - New shapes rlab1/rlab2 (local real set to 0.0, conditionally assigned from a call or *g(), then *out = local): static and extern compile IDENTICALLY with x87.
12. fake_match_scan: 0 leads on cand and on production.
13. Semantic credit: build/semantic_report.json already has interface:_render_debug_profile_stall_tick in hidden_exact, semantic_exact and accepted_ledger, with proof_sources semantic-coff. Running semantic_progress.apply_semantic_accepted_ledger on the current report.json credits it (+271 B, +1 function).

## issues
The verdict holds: FUNCTIONS_EXACT_OBJECT_BLOCKED. Every January function is strict EXACT. Admission is correctly held for the owner's header ruling:
- Variant A (prototype removed from interface.h) regresses rasterizer_frame_statistics_draw (4176 B) through a pure declaration-count tie. I verified this.
- Variant B (header kept) is an extern-then-static redeclaration: C89 6.1.2.2 calls it undefined behaviour, and MSVC reports C4211 at /W4. I verified this too.
- Moving a `static` prototype into the shared header would be no cleaner.

Four amendments are needed before the integrator acts:
(a) Drop proposed item (5), the config/semantic_matches.json entry for _render_debug_profile_stall_tick. The claim behind it is false. build/semantic_report.json already accepts that function through accepted_ledger (proof source semantic-coff), and project_x86.py and audit_object_admission.py both apply that ledger. The function is therefore already credited at 100% (+271 B, +1 function). Only raw objdiff shows 67.27%. The entry would be a harmless duplicate. Its reason text also pins $L label numbers (29019..., 29348, 29353); these shift in the candidate (29021..., 29350, 29355).
(b) The ledger's wording overstates the mechanism. "Storage class decides the copy template" and "VC7 uses x87 exactly when direct->indirect" are not general laws. My counterexamples rlab1/rlab2 compile static and extern identically, both with x87. The candidate is justified by the attested storage class plus measured strict exactness, not by that generalised claim. Record it as shape-dependent.
(c) If the owner picks Variant A, the header-removal check of the other 17 interface.h consumers must be redone at section-byte level. The worker compared per-function rows only, as their own ledger notes.
(d) When landing:
   - apply symbols_static.patch in place (never re-serialize symbols.json);
   - regenerate csplit and run a full ninja, because a split-only regen leaves a stale base;
   - retire the parked.json entry source/interface/interface:_interface_get_weapon_hud_index in the same change, or the parked function's moved bytes fail PROGRESS.

production_changes if the owner rules: scratch/w/interface/production.patch, plus scratch/w/interface/production_variantA_header.patch only for Variant A, plus scratch/w/interface/symbols_static.patch, then the park retirement. Do not apply item (5). Review notes: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review_interface/REVIEW.md
