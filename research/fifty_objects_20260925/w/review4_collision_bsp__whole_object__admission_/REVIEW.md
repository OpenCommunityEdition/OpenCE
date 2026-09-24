# Review 4: collision_bsp whole-object admission (ADMISSION_FIX_PROPOSED)
Reviewer slug. Base 16542e46 (git status clean at start).
Proposal: `static` on collision_bsp_usage_times (production.patch) + symbols.json line 23117 `"static": true`.

## Log
- V1 gate st1.c (worker candidate copied): exact 27 / residual 3 / unwritten 0 (pill3d sha, sphere3d sha, surface_test_sphere size 848!=880).
  base.c (HEAD source) gate identical 27/3. st1.obj vs base.obj: 69/69 sections section_infos_equal; the ONLY symbol-table
  difference is _collision_bsp_usage_times storage 2 -> 3. (build/base obj vs my base.obj: only .debug$S path differs.)
- V2 git apply --check production.patch vs HEAD 16542e46 (clean tree): OK; index b4438fa9 == HEAD blob; CRLF round-trip
  (HEAD + static) == st1.c byte-for-byte.
- V3 symbols.json: my own in-place edit of HEAD config/symbols.json line 23117 (CRLF kept, +16 bytes) is byte-identical to
  the worker's config/symbols.json copy.
- V4 emulated csplit (build/tools/csplit.exe -> slug): split_ctrl == build/split 833/833; split_emu differs ONLY in
  source/physics/collision_bsp.obj; there: 35/35 sections equal, 93 symbols, sole change _collision_bsp_usage_times 2 -> 3.
- V5 object_audit (slug copy, SPLIT_ROOT): st1 vs split_emu: 35 January symbols 0 differ, only DIFF = the 3 residual .text;
  base vs split_emu: +1 storage DIFF (3/2); st1 vs split_ctrl: +1 storage DIFF (2/3). => the edit pair is required together.
- V6 storage truth: cachebeta publics lack _collision_bsp_usage_times; the collision_bsp compiland is present (external
  _collision_bsp_test_sphere/_vector/_pill public; statics bsp3d_*/collision_surface_test_sphere absent - same oracle holds
  in this TU). Sibling collision_usage `_collision_usage_times` is already static in symbols.json and also non-public.
  PDB-storage check (candidate vs split_emu): 0 disagreements (production: 1).
- V7 references: relocs.json targets 0x456EB0..0x456EBC come ONLY from _collision_bsp_test_vector (+0,+4) and
  _collision_bsp_test_sphere (+8,+0xC); no build/split object other than collision_bsp names the symbol; grep of source/
  (all dirs, incl. paths with spaces) and headers: only collision_bsp.c. No extern declaration anywhere.
- V8 objdiff-cli 3.3.1 single-unit projects: emu (split_emu vs st1) and ctrl (split_ctrl vs base) both code 7134/10292,
  data 176/176 (100%), 27/30 fns; equals build/report.json.
- V9 surplus_identity (slug copy on st1.obj): 21 candidate-only code COMDATs, 0 not identical.
- V10 provider_link st1.obj (temp under slug): FAIL (2): _cross_product2d vs actor_combat (held P1) and
  _plane2d_distance_to_point vs geometry NODUP hand copy - both pre-existing, both disclosed by the worker; unchanged by
  the storage edit (the storage edit touches no external surplus).
- HCEX 2011 collision_bsp compiland (DIA2Dump -compiland full module path -> hcex_compiland_collision_bsp.txt): no
  file-scope data at all (timing removed by 2011); HCEX neither confirms nor contradicts; atlas has functions only.
- V11 /W3 /Zs census (slug tools/w3.py): base.c 22 warnings, st1.c 22; multisets identical (no new C4013 etc.).
- V12 STRIP TEST on the one construct left (`= { 0 }` on a now-static object): st1_noinit.c (static, no initialiser)
  gates 27/3 with identical function hashes and passes the owner-keyed object_audit vs split_emu (0 symbol diffs); only the
  .bss section index moves (3 -> 68). => the initialiser is NOT load-bearing for any credited byte (no fake-matching
  concern); it is pre-existing production text, and the minimal one-keyword diff is preferred. Sibling precedent:
  collisions.c `static struct collision_usage_times collision_usage_times;` (already static, symbols.json static).
- V13 parked.json: the 3 collision_bsp parks (surface_test_sphere, bsp3d_sphere, bsp3d_pill) keep identical
  size/reloc/normalized_sha256 on BOTH sides (split_ctrl == split_emu == manifest target; base == st1 == manifest base)
  => no stale-park PROGRESS failure. config/object_admission_rejections.json: no collision_bsp entry.
  config/semantic_data_matches.json / semantic_matches.json / symbol_ownership.json: no entry names the symbol.
- V14 (outside this proposal, informational only) worker's geometry_provider_repair.patch: git apply --check OK; round-trip
  == worker g1.c; geometry gate 27/3 identical rows; all 61 geometry .text hashes unchanged; `_plane2d_distance_to_point`
  section_infos_equal to January in head and g1; pair link st1.obj+g1.obj 0 LNK2005 in both orders, control
  st1.obj+production geometry.obj 1 LNK2005 (discriminating). NOT approved by this review (separate unit, needs its own
  review/sweep); it only removes one of the two provider-link blockers anyway (cross_product2d P1 stays held).

## Pre-existing, not introduced by this diff (disclosure for the eventual whole-object admission)
- `struct collision_bsp_usage_times {vector; sphere}` vs two loose statics is undecidable from bytes: LARGE_INTEGER forces
  8-byte .bss alignment in both shapes, no assert stringifies it, HCEX 2011 has no timing data. Layout agrees with January
  relocs (vector +0/+4 from test_vector, sphere +8/+0xC from test_sphere). Mirrors the admitted sibling idiom in collisions.c.
  No pad members (law j). The name is pre-existing (2026-09-04 ledger) and not first-party attested; this diff adds no name.

## Verdict: APPROVE the storage packet (production.patch + symbols.json line 23117 op + csplit-only regen of collision_bsp)
- Evidence-backed (PDB-public oracle, image relocations, sibling precedent), byte-inert for every section, closes the only
  symbol disagreement (35/35 symbols vs emulated split), data 176/176 stays 100%, parks stay valid, warnings unchanged,
  no header, no other consumer, no new name/aggregate/COMDAT.
- The object remains NOT complete: 3 residual .text (surface_test_sphere owner-held SSE __asm; bsp3d sphere/pill parity
  residuals) and 2 selected-provider LNK2005 (cross_product2d = held P1; plane2d_distance_to_point = geometry NODUP).
- Integrator: apply production.patch; in config/symbols.json edit line 23117 in place (append `, "static": true` before
  ` }`, CRLF kept, no re-serialisation) - equals scratch/w/collision_bsp/config/symbols.json; csplit-only regen; expect
  build/split/source/physics/collision_bsp.obj sha256 35f9f040674643c5... (emulated), all other 832 split objects unchanged.
