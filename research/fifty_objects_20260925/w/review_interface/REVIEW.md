# review_interface REVIEW (started Wed Sep 23 19:58:26 PDT 2026)

## R1 candidate provenance
- diff production interface.c vs cand_interface.c == production.patch exactly (static proto+def for get_weapon_hud_index,
  render_debug_profile, interface_splitscreen_render; six profile_* data defs non-static). Body unchanged.
- cand_interface_variantA.h == interface.h minus the public prototype. cand_interface.c compiles against the ORIGINAL
  interface.h => the "final candidate" is Variant B (extern header proto then static definition).
- No other source file references the 3 functions or the six profile_* data (grep; profile.c hits are enum constants).

## R2 storage evidence (independent)
- cachebeta_publics.txt: _interface_get_weapon_hud_index, _render_debug_profile, _interface_splitscreen_render ABSENT;
  all six _profile_* data PRESENT. pdb_storage.py reproduces the worker's 9 disagreements.
- HCEX DIA2Dump: interface_get_weapon_hud_index "static function: true" / `static long ...(float *)`;
  interface_splitscreen_render static true; control interface_draw_screen NOT static (tool discriminates).
  render_debug_profile absent from HCEX (single-attested by cachebeta only).

## R3 independent gate + audit (Variant B = cand_interface.c with original interface.h)
- gate.py --source cand_interface.c --all --out review_interface/cand.obj: == exact 18 residual 0 (matches gate_after.txt).
- object_audit cand: every January-owned section ok (29 rows); symbol DIFFs only storage 2/3 on the 3 functions
  (split=2 because symbols.json lacks static). audit_base vs audit_cand diff: weapon_hud_index DIFF->ok, six profile_*
  storage DIFFs gone, 3 function storage DIFFs added; surplus list byte-identical to base listing (17 rows).
  => the extern-then-static definition IS emitted as storage 3 (MSVC honours the later static).
- .data (100632 B, 18 relocs): coff_compare section_infos_equal TRUE vs split for cand and base; cand storage now 2 = split.
  All 10 .rdata literals EQUAL.
- symbols.json: lines 3535/3539/3540 are exactly the three entries; symbols_static.patch applies cleanly to a scratch copy,
  JSON still parses, only those 3 lines change. No other split object references the three symbols (grep over build/split).
- Simulated post-csplit split (scratch copy of split obj with storage byte 2->3 on exactly the 3 symbols; tracked split untouched):
  object_audit (scratch copy of the script pointed at the sim split) => 34 January symbols, 0 differ, OBJECT AUDIT: PASS.
- stall_tick: strict EQUAL vs split in base and cand (272/19/b5fe1f5e...). Split section has no $L symbols; ours carries 7
  compiler-local $L labels (label NUMBERS shift between prod $L29019.. and cand $L29021..; reason text must not pin numbers).
- objdiff 3.3.1 (build/tools/objdiff-cli.exe, my own mini-project review_interface/odproj): cand matched_code 5695,
  17/18 fns, data 100960/100960; prod 5251 16/18. Only <100% in cand: stall_tick 67.27273.

## R4 mechanism lab (independent)
- rlab15 (copy of worker lab15) reproduces: static k2s int copy (and custom EAX param convention), extern k2e x87.
- rlab1/rlab2 (NEW shapes: local real initialised 0.0, conditionally assigned from h() / *g(), then *out = local):
  static s_get and extern e_get compile IDENTICALLY (fld [ebp-4]; fstp [eax]) => "storage class decides the copy
  template" is NOT a general law; it is shape-dependent. The candidate stands on the attested storage + measured exactness,
  not on the generalised lab claim. Ledger wording overgeneralises (documentation defect, not a source defect).

## R5 surplus / warnings / Variant A
- surplus_identity (base): sine/cosine IDENTICAL to actor_combat. provider_link on review cand.obj: all 17 surplus PASS,
  SELECTED-PROVIDER LINK: PASS. cand vs base: _sine/_cosine section_infos_equal True; the ONLY .text section differing
  base->cand is _interface_get_weapon_hud_index (render_debug_profile/splitscreen_render bytes unchanged by static).
- Warnings (wlab.py = worker lab harness + /W flag, MSYS_NO_PATHCONV): cand(B) /W3 only the pre-existing cseries.h C4146
  (same as production); /W4 adds exactly one C4211 "redefined extern to static" at the .c static prototype (line 317).
  Variant A (wA.c = cand with variantA header via abs include) /W4: no C4211; gate 18/18 EXACT.
- rasterizer_frame_statistics with variantA header (byte-preserving include swap, rfsA.c): draw 4176 EXACT->residual [sha]
  (9/10); production 10/10. Oracle (NOT landable): +1 extern long dummy residual, +3 EXACT => pure declaration-count tie. Confirmed.
- Worker's Variant A blast check of the other 17 consumers compared ROWS only (their own note says byte check pending);
  irrelevant while A is blocked, but required before A could ever land.

## R6 item (5) semantic_matches entry for stall_tick -- REDUNDANT / claim inaccurate
- build/semantic_report.json already lists interface:_render_debug_profile_stall_tick in hidden_exact, semantic_exact AND
  accepted_ledger (proof_sources ["semantic-coff"]). tools/semantic_progress.apply_semantic_accepted_ledger on the current
  build/report.json credits it (+271 code bytes, +1 function; interface -> 5522 / 17 fns). project_x86.py and
  audit_object_admission.py both apply that ledger. So the progress pipeline ALREADY gives it 100%; only raw objdiff shows 67%.
  The worker's "needs a semantic_matches.json entry for 100% report credit / with that entry the object reports 18/18" is
  wrong for the credited pipeline; the entry is unnecessary (harmless duplicate). Recommend NOT applying item (5).

## R7 strip tests / patch mechanics / house rules
- Only load-bearing change = `static` on the definition (production without it = residual). `static` is NOT decoration:
  double-attested January storage (cachebeta publics absent + HCEX "static function: true"; control fn not static).
- The .c static prototype is NOT load-bearing: removing it gives 18/18 in both Variant A (sA_noproto) and Variant B (sB_noproto).
  It is ordinary house-style placement beside the existing static render_debug_profile_stall_tick prototype.
- render_debug_profile / interface_splitscreen_render `static` and profile_* non-static: bytes unchanged (only .text diff
  base->cand is weapon_hud_index; .data strict EQUAL); pure storage corrections to January PDB truth. .text emission order
  base==cand.
- git apply --check (non-mutating) OK for all three patches; applying production.patch (+variantA header patch) to scratch
  copies reproduces cand_interface.c / cand_interface_variantA.h byte-exactly (CRLF preserved).
- House rules on the diff: void on own line, one param per line, real, authentic names, prototypes for file-statics in the
  owning .c prototypes section. fake_match_scan: 0 leads (cand and production). No header/config change other than the
  gated interface.h removal (Variant A) and the 3 in-place symbols.json flags.
- Variant B (header kept) = extern-then-static in one TU: C89 6.1.2.2 UB, MSVC C4211 at /W4 (verified). Not self-admissible;
  a `static` prototype moved into the shared header would be equally unclean. Worker correctly escalates to owner.

## VERDICT
Worker's FUNCTIONS_EXACT_OBJECT_BLOCKED is CONFIRMED. All January functions strict EXACT (18/18), sections/data/.rdata ok,
surplus identical + provider links PASS, symbol storage fully matches once symbols_static.patch + csplit regen land
(simulated audit PASS). Object landing correctly held for the owner header ruling (Variant A regresses
rasterizer_frame_statistics_draw by a verified pure declaration-count tie; Variant B carries a UB/C4211 extern->static redeclaration).
Required amendments: (a) DROP item (5) -- stall_tick is already credited via build/semantic_report.json accepted_ledger
(semantic-coff); the "needed for 100%" claim is false; (b) do not record "storage class decides the copy template" as a
general law (rlab1/rlab2 counterexamples); (c) if Variant A is chosen, redo the 17-consumer blast check at section-byte level
(worker compared rows only) and run full ninja after csplit regen; (d) retire the parked.json entry in the same landing
(a parked function whose bytes move fails ninja PROGRESS).
