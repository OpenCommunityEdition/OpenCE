# review_r2w2_vehicles LEDGER
- Claimed source/units/vehicles as review_r2w2_vehicles_rv1 (review only; no landing probes).
- Read: brief s0-9; worker MANIFEST/LEDGER; ruling #1 text (HANDOFF :65-68, :156-162); house-rules rule-6 exception;
  the 4 precedent diffs; 05255584; round-1 REVIEW__source_units_vehicles; structural owner packet s1; lane B manifest l.1119-1120.
- apply-check OK. Reproduced cand.c e6710a63 and symbols c602dcdc outside the repo.
- gate: base 37/2, cand 37/2 (no verdict change). objcmp base -> cand: +_point_from_line3d and the fighter only.
- Reloc census and 17-object census confirmed. lab_nofighter: the COMDAT is anchored by the exact _vehicle_update.
- csplit emulation split_R: only vehicles.obj differs, 1 storage row.
- object_audit FAIL(2) residual-only; pdb_storage 1 -> 0; surplus_identity 0 not identical; provider_link PASS (85 rows).
- objdiff 3.3.1 mini-report: no function lost; fighter fuzzy up; data 1744/1744.
- /W3 27 == 27; fake scan 0; /Od 0x8fd970 attests both pfl3d calls and scale_vector3d (optional 03 verified inert).
- VERDICT: APPROVE 01+02; 03 optional, verified.
