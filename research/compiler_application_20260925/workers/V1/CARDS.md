# V1 (Q11 verifier review packet) cards and outcomes

## 2026-09-26 10:53:59 -0700 - V1-C01 outcomes (lab run1 + discrimination disc1, snapshot bb57160f, report sha256 afc46aff...)
Evidence: scratch/campaign/workers/V1/lab/run1/lab_summary.json, scratch/campaign/workers/V1/lab/disc1/discrimination.json.
- P1 CONFIRMED: STOCK + production + hs subset (11 .rdata literals dropped, legacy padded 44 B) credits +54,780;
  still +54,780 with one dropped literal's byte flipped in a scratch copy of our hs.obj (H1, H1c).
- P2 CONFIRMED: DV2 (fc53d5f6) credits the same subset WITHOUT extent_model (+54,780, also corrupted) and rejects it
  WITH extent_model (coverage) (H2, H2c, H2m).
- P3 CONFIRMED: V1 rejects the subset in both forms (H3, H3c, H3m): "does not cover every target .rdata section".
- P4 CONFIRMED: V1 + production == STOCK + production (0 unit changes, notes identical and in order).
- P5 CONFIRMED: V1 + production + V1 hs/actions entries: Halo data 2,587,011 -> 2,644,195 (+57,184); only hs
  (+54,780) and actions (+2,404) change; code/functions/complete units unchanged; admission audit 10/0/1/0 both.
- P6 CONFIRMED: V1 + entries as proposed (no surplus) fails closed: "leaves rebuilt sections undeclared".
- P7 CONFIRMED: January-vs-January (base_path := split object, surplus []) credits +54,780 and +2,404; the same
  control keeping the rebuilt surplus lists is rejected (surplus owner absent).
- P8 CONFIRMED: shell_xbox and editor_flying_camera pass the new target-coverage check (B == A).
- P9 CONFIRMED: regression_gate._exception_records raises KeyError('symbol') for hs, actions AND shell_xbox
  (pre-existing for every grouped entry).
- P10 PARTLY REFUTED as worded: 810/833 units bind; the 23 others fail only with "unsupported long COFF data section
  name /nnn" (libraries + linker_common), i.e. the model refuses them by design; there is NO size mismatch anywhere.
- Discrimination (50 fixture scenarios): V1 meets all 50. STOCK credits 3 negatives (missing_member, both subset
  forms). DV2 credits 23 negatives (see discrimination.json). 16 single-check V1 mutants: every one is killed.
- Padding accounting: hs .rdata 909 sections 49,626 raw + 1,658 alignment padding = 51,284; hs .data 3,496 raw;
  actions .rdata 45 sections 2,322 raw + 66 padding = 2,388; actions .data 16. Surplus not credited: hs 20 sections
  94 B, actions 15 sections 70 B.
- Surplus finding: hs's 20 surplus literals are all January UNDEFINED externals. actions' 15 surplus: 13 are
  January undefined externals; __real@3f800000 and __real@3f1a36e2e0000000 are NOT referenced by January's
  actions.obj at all - ours references them only from its surplus _normalize2d COMDAT (.text section 26).

## 2026-09-26 11:05:01 -0700 - final re-measurement (snapshot HEAD 91f4824b, 10:54:49; report sha256 d8906c9c...)
The lane HEAD moved during the work (bb57160f -> 91f4824b -> ea3b848e; the lane was also rebased). tools/semantic_progress.py
(blob 9a7a5129), tools/test_semantic_progress.py (c4d1608c) and config/semantic_data_matches.json (ef4ed4f0, 47 entries
after Q12) are identical at 91f4824b and ea3b848e, so the patches apply to both. Our hs.obj changed between the two
snapshots (e4b7d2e7 -> 9fbcff1a, hs_runtime header commit); all 910 pinned hs member snapshots still verify.
- Lab (evidence/lab_summary.json): A stock+production 2,588,903 Halo data; B V1+production identical (0 unit changes,
  46 notes identical, parks 75, revocations 0, ownership 1); D V1+production+hs+actions 2,646,087 (+57,184; hs 18 ->
  54,798, actions 0 -> 2,404); All data 2,595,217 -> 2,652,401; code 1,591,710 / 7,461 fns / 389 complete unchanged.
  E/E2 stock fail closed; F DV2+proposed = +57,184; G V1+proposed (no surplus) fail closed; H1/H1c/H2/H2c subset
  credit +54,780 under stock/DV2-legacy; H2m/H3/H3c/H3m rejected. Admission audit 11/0/1/0 on both manifests.
- NEW (not predicted): real-scorer January-vs-January (evidence/jj_objdiff331_*.json, objdiff 3.3.1 sha1 3130e428):
  hs self-diff .data 99.52 / .rdata 99.91 -> V1 credits exactly the scorer's gap 54,780; actions self-diff has .rdata
  at 100% and only .data (16 B) below, so the full 46-member actions entry is REJECTED ("does not cover the reported
  unmatched sections": it would re-cover an already-matched section) and the .data-only scoped entry credits exactly 16.
- Tests: module 28 -> 116 passed; full tools/ suite 854 passed / 312 skipped (stock) -> 942 passed / 312 skipped (V1),
  0 failed either way; ProductionSemanticDataManifestTests PASSED (not skipped) on the production manifest and on the
  production + V1 hs/actions manifest.
- Discrimination (evidence/discrimination.json): 50 scenarios; V1 meets all; stock credits 3 negatives; DV2 credits 23;
  16 V1 single-check mutants all killed.
- Patches regenerated and applied (git apply outside any repository) to the HEAD blobs: results byte-identical to the
  tested files (after CRLF normalisation).
