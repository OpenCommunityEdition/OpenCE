# source/rasterizer/xbox/rasterizer_xbox_debug (admit source/rasterizer/xbox/rasterizer_xbox_debug)

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_rasterizer_xbox_debug\\patches\\01_symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_rasterizer_xbox_debug\\cfg_P\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_rasterizer_xbox_debug\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_rasterizer_xbox_debug\\cand_P.obj"
]

## production_changes
One file only: config/symbols.json. Patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_rasterizer_xbox_debug\patches\01_symbols.json.patch (index f9abb17d..d7e57cfc). The body is the research independent patch unchanged; I only added a git header. It edits rows 5602, 5604, 5606 and 5608-5612 in place, appending `, "static": true` to _D3DDevice_SetRenderState, _IDirect3DDevice8_SetRenderState@12, _SetVertexShaderConstant@16, _SetVertexData4f@24, _SetVertexData2s@16, _SetVertexDataColor@12, _Begin@8 and _End@4. CRLF is preserved; nothing is renamed, reordered or re-serialised. There is no source, header or config.json change: the unit is already Matching at config index 126. After applying, regenerate the split (csplit). git apply --check is clean at 434f0151. The post-image (patch -p1 on a copy) is cmp-identical to cfg_P/symbols.json.

## evidence
PRECEDENT: commit 05255584 says '13 wrapper rows go static'. Its hs_runtime entry says '12 typecast converters are static per cachebeta publics'. The precedent manifest's storage basis is 'No wrapper name is in the cachebeta.pdb publics ... so all 13 are file-static'. I checked each condition. C1: all 8 are stock XDK D3DINLINE wrappers; xbox/include/D3D8-Xbox.h:21 has `#define D3DINLINE static __forceinline`, and lines 1476/1497/1708/1739/1789/1790/1793-1795 define them, so the C storage is static. C2: in cachebeta.pdb publics the RVA equals file_offset (control: _rasterizer_debug_drawing_end at 0x149AE0). The object range holds exactly 10 publics, all Halo functions. There is no public at any of the 8 wrapper addresses and no public by any of the 8 names. C3: the in-place row form matches 05255584 and the neighbouring active_camouflage and decals rows for the same names, which are already static. C4: csplit emulation shows split_ctl equals build/split (833/833), and split_P differs from it in 1 object, rasterizer_xbox_debug.obj. Within that object the 24 sections are byte- and relocation-identical, and only the 8 storage classes flip from 2 to 3. C5: gate --all with the _point_from_line3d guard gives 18/18 EXACT against both build/split and split_P. The candidate is a verbatim copy of the production source; its objects match production in all 44 sections except .debug$S (embedded path) and in all 126 symbols. C6: object_audit goes from FAIL(8) (production vs build/split) to PASS (24 sections, 24 symbols, 0 differ). pdb_storage goes from 8 disagreements to 0. surplus_identity finds 0 candidate-only COMDATs. provider_link passes 7/7 in both orders, with no new surplus against the production baseline. Data: 6 .rdata (292 B) are ok under coff_compare, and the tinfo --data listing is identical. C7: objdiff 3.3.1 (sha1 3130e428) gives 3108/3108 code, 18/18 functions and 292/292 data before and after, so the change is credit-neutral. Cross-object census: no object references any of the 8 names as undefined, and no production object defines any of them as external, so no other unit loses a provider. Differences from the precedent: none in the construct; this is a strict subset (storage rows only, no rename, no source call and so no disclosure comment). The atlas Sept-2001 map tier has no wrapper records for this object; I disclosed this as neutral. Artifacts: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_rasterizer_xbox_debug\{MANIFEST.md, LEDGER.md, publics_by_address.txt, gate_cand_vs_splitP.txt, object_audit_cand_vs_splitP.txt, object_audit_prod_vs_buildsplit.txt, pdb_storage_cand_vs_splitP.txt, provider_link_cand_vs_splitP.txt, surplus_identity_cand_vs_splitP.txt, splitdiff_P.txt, split_ctl_vs_P_detail.txt, name_census.txt, objdiff_report.txt}.

## blockers
None. The Halo object count does not change because the unit is already labelled Matching. The gain is that the label becomes truthful (object_audit FAIL(8) -> PASS, pdb_storage 8 -> 0).

## reopen_criteria
Only if a January-dated first-party source showed any of the 8 wrappers as external. That would contradict both the cachebeta publics and the header's static storage.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_rasterizer_xbox_debug\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_rasterizer_xbox_debug\LEDGER.md: R0 packet check through R5 close. The claim was taken (exit 0) and released (exit 0). git status shows no tracked modification.
