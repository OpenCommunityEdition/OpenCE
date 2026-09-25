# r2w2_actions ledger (round 2, wave 2) - ADMISSION of zero-credit fuzzy re-baseline for source/ai/actions

Tree HEAD 434f0151 (task said 05255584; 434f0151 = 05255584 + R2 ledger commit; actions.c blob 2c996930 = patch base).
Claimed source/ai/actions as r2w2_actions.
Packet under audit: research/fifty_objects_20260925/w/actions/production_fuzzy.patch (git apply --check: clean;
parked.json hunks at offset -19 lines).
Precedent: house rule 11 + user clarification 2026-09-07 (docs/campaign_house_rules.md:17, :55-61).
Precedent audit: scratch/w/precedent_audit/LEDGER.md:43 "production_fuzzy.patch zero-credit covered (alt if macro refused)".

## Step 1: rebuild at the current tree
- prod.c = git show HEAD:source/ai/actions.c (blob 2c996930 = patch index base). cand_fuzzy.c = prod.c + actions.c hunk of
  production_fuzzy.patch (GNU patch, clean).
- gate prod.c: `== exact 62 residual 1` (vehicle_entry 960 [sha]); gate cand_fuzzy.c: `== exact 62 residual 1`;
  per-function rows (sorted) IDENTICAL between prod and cand (no swaps; 62 EXACT rows the same names).
- alndiff vehicle_entry: prod 300 vs 301 insns, 2 families (0x17b transposition + false-epilogue `mov dl,1..mov al,dl`);
  cand 300/300, ONLY the 0x17b transposition (`mov [ebp-4],ecx` before vs after `fmul st(1)`), plus the literal
  symbol-vs-defined rendering of __real@42c80000 at 0x153 (same in both, normalization artefact).
## Step 2: /Od re-attestation (odbuild fn 0x42abb0 -> od_42abb0.txt; rtc)
- guard: 0x42ac35 `jmp 0x42b158` is the if-body of `(flee && panic>0) || obey` = explicit `goto result_exit;` trampoline
  (a structured `if (!(..))` would jump straight past; the separate jmp is the goto statement).
- body: `last == NONE` je body / `last + 45 >= now` jge 0x42b158 = `if (last == NONE || last + 45 < now) {..}`.
- store order: best_distance(-0x18), attempt(-0x1c), continue(-0x20), best_vehicle_index(-0x24)=NONE, then last_check=now.
- prop loop: single prop_iterator_next call, back edge 0x42ae1b -> 0x42acb6 = `while ((prop = next(&it)) != NULL)`.
- prop arm: best_vehicle_index stored first (0x42adda), then mulss square, 8.0f, 10.0f.
- scripted arm: `best == NONE && certain_combat_timer >= 60`; `short slot` (word -0x58); enterable -0x5c; `vehicle` local -0x60;
  distance_squared3d(&position, &body_position); `short index` (-0x80); ai_index -0x84; low-16 equality = DATUM_INDEX_TO_ABSOLUTE_INDEX.
- tail: `if (best != NONE && action_vehicle_setup_impromptu(..)) { actor_action_change(.., 9, &new_state_data); result = TRUE; }`
  and ONE epilogue (return result).
- local declaration order (frame): actor, definition, current_time, result, best_distance_squared, attempt_distance,
  continue_distance, best_vehicle_index ... new_state_data = candidate declaration order.
- RTC names: iterator(8), vehicle_origin(12), position(12), new_state_data(112) = candidate names.
- Later-build difference noticed: /Od tests `prop->vehicle_index != -1` TWICE (0x42ad1f, 0x42ad2f).
## Probe L1 (lab only, NOT landable): duplicate `prop->vehicle_index != NONE &&` (the /Od double test)
- lab/dupcheck.c: gate 62/1, alndiff identical to cand_fuzzy (only 0x17b transposition). INERT. Not retained.
## Step 3: measurements at 434f0151 (fresh)
- vehicle_entry: target 960/22 sha df9b95a1...; production (build/base == prod.obj, 0 symbol diffs) 960/22 sha 80e7bf25...;
  cand_fuzzy 960/22 sha 7ef7a0e0ac5c66eda94a6d1f28f76ba291e23c27e1d9c10706ffb2822804dd32 (= the packet's re-baseline).
- objdiff 3.3.1 (build/tools/objdiff-cli.exe sha1 3130e428, one-unit project od331/mk.py): vehicle_entry 96.54422 -> 99.319725;
  .text 99.357506 -> 99.49561; matched_code 16333 -> 16333 (ZERO credit change); .data 57.14286 / .rdata 98.38118 unchanged.
  build/report.json has 96.54422 for vehicle_entry (= prod).
- sbs 0x160-0x190 (sbs_0x160.txt): the only difference is `fmul st(1)` vs `mov [ebp-4],ecx` order at 0x17b; the two
  instructions are dependency-free (x87 register op vs integer store of best_vehicle_index), so the candidate is
  January's code on every path.
## Step 4: section-4 audit of cand_fuzzy.obj
- object_audit: FAIL(1) = the parked vehicle_entry only; all 46 January data sections ok; 112 January symbols 0 differ;
  surplus list identical to production (diff of the two audit outputs is empty).
- secdiff prod vs cand: 136/136 sections, only DIFF = vehicle_entry; 351/351 symbols, only compiler-local $L label
  names renumbered (same storage/section/offset).
- pdb_storage (production; candidate symbol table identical): 112 split symbols, 0 disagreements.
- surplus_identity: production 11 COMDATs 0 not identical; candidate (surplus_identity_cand.py, same logic) 11/0.
- provider_link cand_fuzzy.obj: 26 surplus, SELECTED-PROVIDER LINK PASS (both orders); --baseline=prod.obj: no new surplus.
- tinfo --data: candidate == production (only .debug$S differs: source path); data coff-identical to January per object_audit.
- fake_match_scan: prod 0 leads, cand 0 leads. CL /Zs /W3: same 18 diagnostics in both (one site line-shifted by -25),
  no new warning from the changed hunk.
- rejections: none for ai/actions. config status NonMatching (unchanged). Parks: HEAD 83/0/0; packet 83/0/0;
  source-only landing (no park update) would make the park STALE -> both hunks must land together.
## Step 5: packet rebuilt at the current tree
- production_fuzzy_r2.patch = actions.c hunk (2c996930 -> af8cb90c, byte-identical to round-1) + parked.json hunk
  regenerated at 69c454eb (zero offset). Park evidence amended ONLY in its research pointer: round-1 pointed at a donor
  scratch path (scratch/w/actions/cand_exact.c in lane claude/fifty-objects-20260925); now the tracked
  research/fifty_objects_20260925/w/actions/owner_gated_exact.patch + LEDGER.md, plus the inert L1 result.
- git apply --check clean (no offsets); GNU patch round-trip on copies byte-identical to cand_fuzzy.c and parked_r2.json;
  only the vehicle_entry park entry changes (83 entries before/after).
## Step 6: owner-gated option kept alive (HELD, not part of the landing)
- held/owner_gated_exact_on_fuzzy.patch: the macro + one-line square spelling + park removal, layered ON TOP of the fuzzy
  packet (round-1's owner_gated_exact.patch is against the pre-fuzzy tree). Result == round-1 cand_exact.c (mod CR).
  gate 63/63 EXACT, object_audit PASS, parks 82/0/0. So landing the fuzzy now does not foreclose a later owner yes.
## Verdict
FUZZY_IMPROVED / landable zero-credit packet: scratch/w/r2w2_actions/production_fuzzy_r2.patch (both hunks together).
Every condition of rule 11 + the 2026-09-07 clarification holds (MANIFEST.md section 1). No credit; no loss of exactness.
Owner items unchanged: squaring macro (held/owner_gated_exact_on_fuzzy.patch, layered), grouped data verifier.
