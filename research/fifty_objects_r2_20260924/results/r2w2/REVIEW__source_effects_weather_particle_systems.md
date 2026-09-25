# REVIEW source/effects/weather_particle_systems

approve: True

amended_patch: None

## per_object
[
 {
  "unit": "source/effects/weather_particle_systems patch 01 (one_over_char_max static -> external)",
  "approve": true,
  "issues": "None blocking. cachebeta public 0x25AAFC, HCEX Global/exported, January split storage 2. Byte- and credit-neutral: 0 section diffs, 1 storage bit. object_audit symbols go from 29/30 to 30/30 and pdb_storage from 2 to 1. The hs precedent is overstated (hs had a cross-object consumer), but the public=external rule, the ui_widget _one_over_255 no-consumer precedent and HCEX cover it. No symbols.json change is needed."
 },
 {
  "unit": "source/effects/weather_particle_systems patch 02 (weather_particle_system_globals external -> static + symbols.json row 22981)",
  "approve": true,
  "issues": "Optional. It must land atomically with the symbols.json row and a split regeneration; either half alone fails object_audit. Evidence: absent from cachebeta publics (neighbours are public) and HCEX File Static, stronger than cdc8ebd3. It reverses f7afb704's circular choice. With the emulated split, pdb_storage is 0 and audit shows 30/30 symbols with 0 differing; combined with the round-1 owner item it gives object_audit PASS, 20/20. Advisory: the pre-existing = { 0 } becomes audit-inert once the variable is static; the precedent keeps it. The edited lines are LF in a CRLF worktree after git apply; staging normalises this."
 }
]

## checks
All checks were run independently in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_weather_particle_systems\ (see REVIEW.md). I claimed the unit, then released it. No tracked file was edited and nothing ran ninja, configure or a git mutation. The tree is clean apart from the pre-existing untracked research/ folder.

(1) Patches. I rebuilt the candidates from the patch files, not from the worker's .c files: git show HEAD into an isolated autocrlf=true repo, then applied 01 and then 02. The results equal the worker's files: candA.c = cand.c, candB.c = candB.c, symbols_B.json = emu/config/symbols.json. Against production, only l.309 and l.313 of the source and l.22981 of symbols.json change. git apply --check is clean at HEAD 434f0151 for 01, for 02 and for 01+02; the HEAD blobs d2aae8ff / f9abb17d are the patches' index bases.

(2) gate --all. prod, candA and candB give identical rows: 19 EXACT plus _weather_particle_system_render 1680 residual [sha].

(3) objcmp (sections by section_infos_equal and raw bytes, plus the full symbol table):
- build/base vs prod: 0/0.
- prod vs candA: 0 section diffs, 1 symbol diff (_one_over_char_max storage 3->2).
- candA vs candB: 0 section diffs, 1 symbol diff (_weather_particle_system_globals storage 2->3).

(4) Emulated split, my own csplit run. The control config reproduces build/split for all 833 objects. cfg_B (row 22981 plus "static": true) changes only the weather object, by one storage bit, with 30/30 sections identical.

(5) object_audit (copy with a parametric split root):
- Against build/split: prod FAIL(2); candA FAIL(1), render only, 30 symbols with 0 differing; candB FAIL(2) on globals.
- Against emu split_B: prod FAIL(3); candA FAIL(2) on globals; candB FAIL(1), render only, 0 differing.
- This proves 02 is atomic.
- All 10 data sections are ok in every run.

(6) pdb_storage: prod 2, candA 1, candB vs emu split 0.

(7) surplus_identity is unchanged: 13/14 IDENTICAL; _real_local_random DIFFERENT, as before.

(8) provider_link on prod, candA and candB gives the same FAIL(1) on _real_local_random vs effects.obj, in both orders. With --baseline, all three PASS.

(9) Symbol census across 621 base and 833 split objects: weather is the only definer of either symbol, and nothing else references them. A relocation scan of the neighbours (_player_effect_globals, _cheat_strings, _debug_*) finds only addend 0, so there is no aliased reference into weather's globals. No January relocation targets _one_over_char_max (defined but unused, as in our source).

(10) HCEX DIA2Dump re-run: one_over_char_max is Global const float, PublicSymbol, export is DATA; weather_particle_system_globals is File Static. cachebeta publics: _one_over_char_max is public at 0x25AAFC; the globals symbol is absent while its neighbours 0x43D588/89 are public.

(11) /W3: 15 warnings with an identical set for all three sources. fake_match_scan: 0 leads.

(12) objdiff 3.3.1 (sha1 3130e428) mini project, 6 units (prod/candA/candB vs build/split and vs emu): all give 4114/5779 code, 19/20 functions, 949/949 data; render 84.86602 everywhere. Credit-neutral.

(13) Precedent audit. I read 6e3e2d35 and its round-1 hs storage review, cdc8ebd3 (collision_bsp), 4f4c8502 (ui_widget _one_over_255 and the origin of weather's static), donor 3e84a6de, f7afb704, and the 0907 weather delete ledger.

(14) Future-closure lab: round-1 production.patch plus 02 gives 20/20 EXACT and object_audit PASS against the emulated split. Strip test of the pre-existing `= { 0 }` on the now-static globals: it is inert (see issues).

(15) EOL simulation (iso2): with a CRLF worktree, staging normalises to LF blobs and git diff --cached --check is clean.

(16) Rebased the round-1 owner-item body patch and verified it: it applies after 01 and after 01+02, and reproduces the lab files exactly.

## issues
No blocking issues. Both patches are zero credit; the object stays NonMatching on the render class-I owner item and on the _real_local_random provider (real_math is reserved for Codex).

Precedent check (special focus): the manifest overstates the hs 6e3e2d35 analogy. The hs _hs_type_*_default flips had an extra leg weather lacks: January's hs_runtime.obj imports all 39 as UNDEF, a cross-object consumer. The landing still holds without that leg:
- the brief s3/s4.3 public=external rule alone is sufficient (S_PUB32 exists only for externals; 1,709 unreferenced externals are public);
- canonical 4f4c8502 landed the exact no-consumer analogue, `real const _one_over_255` in ui_widget.c;
- HCEX independently records one_over_char_max as Global and exported;
- the weather `static` came from donor 3e84a6de with no storage justification, before the cachebeta rule (2535f91a).
So the precedent is not stretched.

02 (optional) has stronger evidence than its precedent cdc8ebd3, since it has cachebeta absence plus HCEX File Static. It reverses f7afb704's circular split-following choice, which the 0907 ledger had already flagged as unproven. It must land atomically with symbols.json row 22981 and a split regeneration; either half alone fails object_audit (measured).

Advisory, non-blocking:
(a) Once the globals are static, the pre-existing `= { 0 }` no longer affects audit or credit. The strip lab keeps 19 EXACT and 30/30 symbols; only our internal section order changes. Lane C added it to keep an uninitialised external out of COMMON, which no longer applies. The cdc8ebd3 precedent kept the identical form, so I do not require removing it.
(b) The MANIFEST says "CRLF kept" for the emu symbols.json, but the edited line is LF in the worker's copy and in any CRLF worktree after git apply. Harmless: the canonical blobs are pure LF, staging normalises, and git diff --cached --check is clean.
(c) Integration hazard: after 01 lands, research/fifty_objects_20260925/w/weather_particle_systems/production.patch no longer applies, because its first hunk is 01. A verified body-only rebase for the owner item is at C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_weather_particle_systems\r1_render_body_after_01.patch. It applies after 01 and after 01+02 and reproduces 20/20 EXACT; it is not part of this packet.

No amended patch is needed. Land C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weather_particle_systems\patches\01_one_over_char_max_external.patch as-is. Optionally land 02_weather_particle_system_globals_static.patch as-is, as one atomic commit with the split regenerated. Full review: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_weather_particle_systems\REVIEW.md
