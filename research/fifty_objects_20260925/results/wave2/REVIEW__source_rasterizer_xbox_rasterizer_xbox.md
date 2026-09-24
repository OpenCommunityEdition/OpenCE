# review source/rasterizer/xbox/rasterizer_xbox
approve=False

## per_object
[
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox (4 patches: rasterizer_xbox.c, rasterizer_xbox_internal.h, symbols.json, config.json -> Matching)",
  "approve": false,
  "issues": "Technically verified: 95/95 EXACT against an independently emulated split; object_audit PASS 216/216; no PDB storage disagreement by address; surplus identical and provider_link PASS; header consumers identical; the patches apply cleanly. Blocked only on an owner ruling: the admission needs 9 unauthenticated descriptive .bss static names in symbols.json, the class held in the 2026-09-15 env_fog/models ruling and the 2026-09-24 path_obstacle_avoidance admission hold. Apply as-is if the owner says yes."
 },
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_debug (follow-up patches/followup_rasterizer_xbox_debug_symbols.json.patch)",
  "approve": true,
  "issues": "None. A storage-only change: 8 XDK wrapper rows become \"static\": true, and none of those offsets is a cachebeta public. It applies standalone and on top of the main symbols patch. csplit changes only rasterizer_xbox_debug.obj. object_audit goes FAIL(8) -> PASS, the gate stays 18/18, and objdiff 3.3.1 measures and per-function % are identical (score-inert). The object is already Matching."
 }
]

## checks
All run at 931ed8dc with the reviewer's own tooling in scratch/w/review2_rasterizer_xbox/ (evidence in LEDGER.md and REVIEW.md).
(1) git apply --check: all 4 patches pass, individually and together, plus the follow-up. Applied outside the repo, the post-images are cmp-identical to cand_L.c, hdr/rasterizer_xbox_internal.h, config/symbols.json and config/config.json. The patched source is all CRLF; git diff --check is clean.
(2) csplit with the patched config (split_R): of 833 objects, only rasterizer_xbox.obj differs. A control csplit of the unmodified config equals build/split for all 833.
(3) rgate.py (gate.py plus --split-root/--tmpdir only): cand_L with the PATCHED header gives 95/95 EXACT against split_R, and the _point_from_line3d guard passes. A shadow '#error' control proves the patched header was the one compiled. With the unpatched header the object bytes are identical, and my object equals the worker's obj/cand_L.obj.
(4) object_audit (split override): PASS, 216/216 January symbols, all sections ok (.bss 2230 8-aligned, .data 6, blend .rdata 108 4-aligned). Production scores FAIL(22).
(5) Storage checked by ADDRESS against cachebeta_publics.txt: no public at 0x45E028..0x45E8CF, 0x45E8D4, .rdata 0x28BFAC..0x28C017, .data 0x30CEEC, the filthy function 0x146650, or any of the 20 wrapper offsets. The object_audit storage rows cannot prove this on their own, because the split's storage class comes from symbols.json.
(6) Surplus: my surplus_r.py (code and data) finds 15 candidate-only externals, all IDENTICAL to January's selected definers, and no static surplus. provider_link.py with the candidate obj: PASS 15/15.
(7) objdiff 3.3.1 on my own project: production and candidate measure identically (data 9048/9048; the same 4 pre-existing code under-credits). audit_object_admission on the current strict report already lists the unit as a candidate with function_gap 0 and data_gap 0.
(8) Header blast radius: 7 includers, both include spellings, each proven with a control header. All compile to IDENTICAL section bytes and owner section_infos, with the same gate counts. Only the internal $L ordinals shift. objdiff per-function % is identical for environment, transparent_geometry and models.
(9) /W3 /Zs gives the same 17 pre-existing warnings as production. fake_match_scan on cand_L: 0 leads.
(10) STRIP S1, re-run myself (23 zero-initialisers removed): the gate stays 95/95, but object_audit fails 23 rows on name-hash .bss order, and the attested names alone contradict January's order. So the initialisers are layout-attested.
(11) Evidence re-checks:
- (b) All 14 bare names appear in January strings; none of the 9 others appears in January strings, /Od strings (the later build uses global_render_targets[]) or the HCEX compiland.
- (d) I re-dumped the HCEX rasterizer_dx9 compiland myself (identical to the worker's). It gives Static Local const unsigned long[9] for srcblend/destblend/blendop, in src/dst/op order. The /Od srcblend_table/blendop_table asserts are referenced only by fn 0x7ca560.
- (e) January relocations +2164/+2170 go to _D3DDevice_SetFlickerFilter@4/_SetSoftDisplayFilter@4, and January has no wrapper copy of either.
- (g) The January atlas (4cc87b45, at 0x546650) and the 2001-09-25 map both use the plural name, as does HCEX.
(12) No leftover references to the removed names in source/config/tools/tests. No parks or admission rejections for the unit; the semantic_matches entry name is unchanged.
(13) Follow-up rasterizer_xbox_debug patch: applies standalone and on top of the main patch; csplit changes only rasterizer_xbox_debug.obj; audit FAIL(8) -> PASS; gate 18/18; none of the 8 offsets is a public; objdiff measures are identical before and after.

## issues
OWNER RULING REQUIRED (policy, not technical). Everything technical checks out, but the whole-object admission depends on the .bss split adding 9 descriptive static names to symbols.json. No first-party source (January strings, the /Od build, HCEX) attests any of them:
_node_matrix_constants, _bitmap_dimensions_non_blocking, _bitmap_dimensions, _global_d3d_texture_render_primary, _global_d3d_surface_render_primary, _global_d3d_texture_render_secondary_z, _global_d3d_surface_render_secondary_z, _global_d3d_texture_render_primary_copy, _global_d3d_surface_render_primary_copy.

The owner has held this class before:
- The 2026-09-15 ruling (docs/object_matching_logs/opus5_250k_house_clean_checkpoint_20260915.md, section 2) kept both descriptive .bss static splits, env_fog and models ('owner ruling on 5 descriptive names'). It said not to re-ask without new evidence such as authenticated names.
- Canonical halo_only_object_closures_20260924.md did not admit path_obstacle_avoidance because its 'BSS relabel lacks authenticated private identities'.
- The only counter-precedent is collision_debug 5671843f, a descriptive 3-way split that landed for function credit only; that object was not marked Matching.

The new evidence here (a bare-name string proof that the aggregate plus #define redirects is invented) authenticates 14 of the 23 names, not the other 9. Proposed owner question: 'Admit rasterizer_xbox as Matching with 14 string-authenticated plus 9 descriptive zero-initialised .bss file statics replacing the invented rasterizer_xbox_d3d_globals aggregate? Names are layout-inert: initialised statics follow declaration order.' On a yes, apply the 4 patches exactly as provided (verified) and regenerate the split.

Non-blocking notes:
(i) The header symbol-listing comment still carries stale _code_XXXXXXXX entries. They predate this patch.
(ii) If the owner declines, a storage-only zero-credit subset could land instead: the 20 static wrapper rows, static d3d_palette and rasterizer_state_cache, the HCEX function-local blend tables, the direct D3DDevice_SetFlickerFilter/SetSoftDisplayFilter calls, and the static rasterizer_filthy_bitmap_defaults_initialize. It would leave out the .bss split and the Matching flip. No patches exist for that subset and I have not verified it.
(iii) Claim (i), HCEX 'dimensions' refuted, was not re-run. It does not affect bytes.

Review notes: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_rasterizer_xbox\REVIEW.md; ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_rasterizer_xbox\LEDGER.md.
