# REVIEW source/items/weapons

approve: True

amended_patch: 

## per_object
null

## checks
All checks were independent, at HEAD 434f0151. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_weapons\ (REVIEW.md). git status is unchanged; the only untracked entry is the pre-existing research/fifty_objects_r2_20260924/.
(1) Patch application:
- `git apply --check -v --whitespace=error` is clean for 01, for 02, and for both together.
- GNU patch on the HEAD blobs reproduces the worker's cand.c and final/{weapons.c,symbols.json} (CR normalised).
- The blob hashes match the patch index lines (72e3597->c0baa42, f9abb17->17bd4c8).
(2) Source state: the other 10 functions are already static on both prototype and definition. Only finish_reload was external.
(3) Strip test / byte-inertness, with my own byte+reloc comparer:
- build/base vs head.obj: 0 section diffs and 0 symbol diffs.
- head.obj vs cand.obj: 0 section diffs; the only symbol diff is finish_reload storage 2->3.
(4) Split emulation: I ran csplit myself with my own copies of the config.
- split_ctl equals build/split for all 833 objects.
- split_s differs only in items/weapons.obj, by 11 bytes. These are exactly the 11 storage classes (2->3); all 97 sections are equal.
- All 79 function fingerprints (section_infos_equal) are unchanged.
(5) gate --all gives identical rows for cand vs split, cand vs split_s and head vs split_s: exact 78, residual 1 (_weapon_place [sha]).
(6) object_audit:
- production: FAIL(11);
- cand vs current split: FAIL(12);
- head vs split_s: FAIL(2);
- cand vs split_s: FAIL(1), .text _weapon_place only. 97 January symbols, 0 differ. 17 .rdata sections and .data are all ok.
This confirms the two patches must land together.
(7) pdb_storage drops from 12 disagreements to 1 (_data_00307140, the held naming item B).
(8) cachebeta publics:
- All 11 are absent, both by name and at their RVAs.
- A post-patch census of every symbols.json row in 0xEA400-0xECF00 finds every external row is a public and every static row is not, so the packet is complete for functions.
(9) Cross-TU references: my own raw scan for the exact names finds 0 hits outside weapons.obj across 1,454 build/split and build/base objects. Source, headers and the other config files also have 0 hits.
(10) Sept-2001 cachebeta.map (sha256 b63d3090, verified): all 11 are under 'Static symbols' (l.20331-20358). The controls are public (l.2299-2316).
(11) HCEX DIA2Dump, my own runs:
- finish_reload, set_state and reset are 'static function: true';
- weapon_busy has no record;
- the control weapon_place is a public.
(12) surplus_identity: 17 COMDATs, all identical, and the output is identical to production.
(13) provider_link against split_s: all 31 rows PASS in both orders, identical to production. `--baseline` finds no new surplus.
(14) The semantic_matches pin for _weapon_set_state (sha 667efc08) holds for split_ctl, split_s and cand. The verifier does not depend on storage class.
(15) objdiff 3.3.1 (sha1 3130e428, my own mini project): 4 pairings, each 13277/17595 code, 75/79 functions, 2052/2052 data. Per-function rows are identical and match build/report.json.
(16) /W3: 20 lines for both head and cand, identical apart from the file name.
(17) fake_match_scan: 0 leads for both.
(18) Precedent audit. I read the original commits:
- f04103c2 is an owner commit. It set symbols.json "static": true on the action_flee/guard private functions, citing "January cachebeta.pdb publics, no cross-TU refs". It also dropped static on aim_assist where January has the function public.
- 05255584: hs_runtime converters made static per publics on a NonMatching object, at zero credit. The same commit flagged 13 hardware_geometry rows static.
- 7979cf8f: the IsBusy wrappers were flagged static.
- 9f74e993 had already flagged weapons' own start_reload and state_next static.

## issues
No defect found; every measured claim reproduced independently. The precedent is not stretched:
- Target-side symbols.json static flags per cachebeta publics with no cross-TU references are canonical owner practice (f04103c2, 7979cf8f, 05255584, 9f74e993). Source-side static per publics is also canonical (05255584 hs_runtime, on an object that stays NonMatching).
- The joint finish_reload edit is the union of the two directions f04103c2 itself applied.
- All seven stated conditions hold: absent from publics; 0 cross-TU references; byte-inert; exact form; the split change is confined to weapons.obj; surplus and provider links unchanged; not a held class.
House rules are clean: it is a storage keyword only, with no new scope or declaration, and the strip test restores production bytes.
Integrator notes:
- Patches 01 and 02 MUST land in the same commit, followed by a full ninja so the split regenerates. Either patch alone makes object_audit worse or leaves it incomplete: FAIL(12) with the source patch only, FAIL(2) with symbols.json only.
- The credit is zero. The object stays NonMatching on the owner-held _weapon_place class-F item and the _data_00307140 naming item B, both untouched.
- Housekeeping, not caused by this packet: scratch/ holds stale scratch/_gate_<pid>.c/.obj files from the gate.py runs of all workers.
