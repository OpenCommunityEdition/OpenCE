# REVIEW source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps

approve: True

amended_patch: None

## checks
Worktree 5d3ca708, clean before and after (only the untracked research/ that was already there). I claimed the unit through claim.py and released it at the end. No tracked file was edited. All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_rasterizer_xbox_hardware_bitmaps\ (LEDGER.md, MANIFEST.md, bodyid.txt, splitcmp.txt, splitdetail.txt, gate_prod.txt, gate_cand.txt, objcmp_prod_cand.txt, battery.txt, parkcheck.txt, objdiff/report.json).

1. git apply --check on patch 20 at 5d3ca708: rc 0. The blob indexes match HEAD (symbols.json 885b3b3d, .c a62f75f5).
2. Name evidence, checked independently:
   - Sept-2001 beta cachebeta.map (ts 3bb0dabe) has _rasterizer_bitmap_cm_changed at 0001:00155dd0, size 0x210. Its neighbours 2d_changed (0x1c0) and 3d_changed (0x210) line up with January's 0x157d50 and 0x157f10, which already carry those names.
   - The Aug-2001 map (ts 3b7b3218) has the same name at 0x13b980, size 0x210.
   - The Sept-2001 RETAIL cache.map has @rasterizer_bitmap_cm_changed@4.
   - The January atlas 4cc87b45 has _rasterizer_bitmap_cm_changed at 0x558120, tier exact, between 3d_changed and delete.
   - The current name cube_map_changed appears in no atlas build.
   - cachebeta.pdb publics: neither name is public, so the "static": true flag is right. HCEX.pdb has neither name.
3. Body identity (bodyid.py, with the January split's 26 relocation fields masked):
   - The Sept beta xbe body matches January on all 528 bytes (195/195 instructions).
   - The Aug xbe differs in 3 bytes, which are the assert line immediate (0x110 against January's 0x114).
4. Callers (E8 scan): the only caller is _rasterizer_bitmap_changed, in both the Sept xbe (0x16707d) and the January exe (0x5583cd).
5. January strings: no literal names either spelling. The cm abbreviation for cube map appears in January assert text (default_cm_hardware_format, 3 times) and in the Sept _bitmap_cm_* names.
6. Reference sweep (grep outside scratch):
   - The only references are symbols.json:5810, the .c prototype/definition/call, and one historical docs ledger row.
   - No reference in parked.json, semantic matches, ownership, rejections, tools or tests.
   - The unit's park is _rasterizer_bitmap_new, which this rename does not touch.
   - The patch updates all 3 code references plus the listing-comment line.
7. Emulated split (build/tools/csplit.exe, slug config copy, slug output):
   - The control split equals build/split on 833/833 objects.
   - The candidate split differs only in hardware_bitmaps.obj, and objcmp with the rename map says IDENTICAL (32 sections, 96 symbols, storage 3 kept).
8. Gate:
   - Production against build/split: 18 exact, 1 residual.
   - Candidate (patched .c against split_cand via with_split.py): 18 exact, 1 residual, rows identical apart from the name. cm_changed is EXACT 528.
   - build/base matches my production compile, and the production and candidate compiles are IDENTICAL apart from the name (46 sections, 114 symbols).
   - The parked _rasterizer_bitmap_new measurements are unchanged (target 041cef84, base 71946946, 400 bytes, 26 relocations).
9. Section-4 battery, production against candidate:
   - object_audit: FAIL(1) on both sides, the existing park only; 0 of 32 symbols differ.
   - pdb_storage: 0 disagreements on both sides.
   - surplus_identity: 0/0 on both sides.
   - provider_link: PASS in both link orders on both sides (5 surplus items that were already there).
10. objdiff 3.3.1 (sha1 3130e428), mini report: production and candidate are both 1875/2263 code, 18/19 functions, 1584/1584 data, 99.99379%.
11. Strip test and /Od declaration order do not apply: the change is identifiers only, with no new construct, cast, scope or declaration.
12. House rules: rule 7 is met, since a first-party name replaces an invented descriptive one. Brief section 8 lets a first-party name bypass the owner gate on descriptive names. Nothing matches a held class.
13. Sibling overlap:
   - Patch 19's symbols.json hunk (5783-5789) and patch 20's (5807-5813) do not overlap.
   - The hardware_geometry 02_symbols patch, whose context includes line 5810, is already in 5d3ca708, so there is no textual conflict.

## issues
No blocking issues. I approve patch 20 as written; no amendment is needed. It earns zero credit because it changes a name only.

Notes for the integrator:
(a) The per-function diff in stable_verdicts will show one name swap: _rasterizer_bitmap_cube_map_changed goes away and _rasterizer_bitmap_cm_changed appears, both EXACT at 528 bytes. This is not a regression, and the exact count stays at 18/19. csplit has to be regenerated together with the .c edit, as with the other rename packets.
(b) docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj.md:65 still lists the old name in its historical exact table. You may want to add a rename note to your ledger. This is not a code reference and nothing reads it.

Problems that were already there and that the patch neither adds nor fixes:
- Rule-9 prototypes declared locally in this consumer file for texture_cache_bitmap_delete and rasterizer_error, plus the local extern global_d3d_device.
- The symbol-listing comment still shows the _code_ placeholders for 0x157d50 and 0x157f10 (2d/3d_changed) and for the D3D thunks. After the patch, the 0x158120 row is the only named row in that block.

The unit's only remaining blocker is the _rasterizer_bitmap_new park, which is unchanged.
