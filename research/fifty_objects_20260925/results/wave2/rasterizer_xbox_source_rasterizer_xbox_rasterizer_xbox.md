# source/rasterizer/xbox/rasterizer_xbox

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\cand_L.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\hdr\\rasterizer_xbox_internal.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\patches\\rasterizer_xbox.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\patches\\rasterizer_xbox_internal.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\patches\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\patches\\config.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\patches\\followup_rasterizer_xbox_debug_symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_audit\\cand_F.c"
]

## production_changes
Apply 4 unified diffs. All pass git apply --check against 931ed8dc and together; post-images verified byte-identical to the candidate files:
(1) C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_audit\patches\rasterizer_xbox.c.patch (post-image = scratch\w\rasterizer_xbox_audit\cand_L.c).
  a. Delete the invented struct rasterizer_xbox_d3d_globals and its 23 '#define member rasterizer_xbox_d3d_globals.member' redirects. Replace them with 23 separate `static ... = NULL / = { 0 }` file statics in the old member order.
  b. Make d3d_palette and rasterizer_state_cache static.
  c. Remove the invented _framebuffer_blend_state_* enum. Replace the static const long framebuffer_blend_function_states[3][9] with three FUNCTION-LOCAL tables in rasterizer_set_framebuffer_blend_function: `static const unsigned long srcblend_table / destblend_table / blendop_table[NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS + 1]`.
  d. Replace IDirect3DDevice8_SetFlickerFilter(global_d3d_device, X) / IDirect3DDevice8_SetSoftDisplayFilter(global_d3d_device, FALSE) with D3DDevice_SetFlickerFilter(X) / D3DDevice_SetSoftDisplayFilter(FALSE).
  e. Rename the private initializer to static rasterizer_filthy_bitmap_defaults_initialize, with a static prototype in the .c prototypes block. The January assert message text is kept verbatim.
  f. Update the header symbol-listing comment and the d3d_palette comment.
(2) patches\rasterizer_xbox_internal.h.patch: remove the extern prototype of the (now static) filthy initializer.
(3) patches\symbols.json.patch. Full edited copy: scratch\w\rasterizer_xbox_audit\config\symbols.json; all edits are in-place lines, never re-serialised.
  - Add "static": true to the 20 XDK wrapper rows 5495-5497, 5502-5507, 5512, 5517, 5518, 5520, 5525, 5533-5535, 5568, 5570, 5571.
  - Line 5578: rename to _rasterizer_filthy_bitmap_defaults_initialize and make it static.
  - Line 22724 _rasterizer_state_cache and line 23139 _d3d_palette: make static.
  - Line 19676 _framebuffer_blend_function_states becomes 3 static rows: 2670508 ?srcblend_table@?1??rasterizer_set_framebuffer_blend_function@@9@9, 2670544 ?destblend_table@..., 2670580 ?blendop_table@...
  - Line 23137 _rasterizer_xbox_d3d_globals becomes 23 static rows at 4579368 + {0, 2112, 2116, 2120, 2124, 2132, ..., 2208, 2212}, named _node_matrix_constants ... _global_d3d_surface_render_primary_copy.
(4) patches\config.json.patch: rasterizer_xbox.c index 128 status NonMatching -> Matching (config copy at scratch\w\rasterizer_xbox_audit\config\config.json).
The integrator must regenerate the split (csplit), which changes only build/split/source/rasterizer/xbox/rasterizer_xbox.obj.
Optional, independent follow-up for another unit (verified): patches\followup_rasterizer_xbox_debug_symbols.json.patch.

## evidence
Every wave-1 claim was re-verified at 931ed8dc. The ledger is C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_audit\LEDGER.md.

(a) cachebeta.pdb publics have nothing in 0x45E028..0x45E8CF, 0x45E8D4, .rdata 0x28BFAC..0x28C017 or .data 0x30CEEC. A scan of all 833 January split objects finds no UNDEF reference to any of the 4 non-public names. So all of them are file-static.

(b) January's .rdata strings name 14 of the 23 members BARE. The aggregate plus #define redirects is the known fake-stringification pattern.

(c) .rdata lab rerun with production flags. One [3][9] array gets align 8 (0x40400040). Three long[9] arrays get align 4 (0x40300040), which is January. Offsets follow definition order, so the tables are defined in the order source, destination, operation.

(d) NEW: HCEX.pdb compiland rasterizer_dx9.obj lists srcblend_table, destblend_table and blendop_table as Static Local `const unsigned long[9]` of rasterizer_set_framebuffer_blend_function. HCEX -lines shows a ~45-line code-free gap after the first declaration. The later /Od build's assert strings name the same three tables, which sit adjacent to the function's own strings. That gives authentic names, scope and type.

(e) January keeps an unreferenced out-of-line copy of EVERY D3DINLINE wrapper the TU uses (57). Production adds only SetFlickerFilter and SetSoftDisplayFilter copies. January relocations +2164/+2170 call _D3DDevice_*@4 directly, so January spelled the calls directly.

(f) Storage precedent. 37 sibling wrapper rows of this object are already "static": true (20260906 SDK packet). The admitted rasterizer_xbox_decals audits PASS with static wrapper rows. rasterizer_xbox_debug (admitted 20260902) still FAILs object_audit on the same 8-row issue; a verified follow-up fix is provided.

(g) Filthy initializer name. The 2001-09-25 cachebeta.xbe LINKER MAP (atlas tier 'map', same position in rasterizer_xbox.obj) and HCEX.pdb 2011 both say rasterizer_filthy_bitmap_defaults_initialize (static). The singular form appears only in January's assert message text. This supersedes wave-1's 'keep singular'.

(h) Strip test on the '= NULL / = {0}' initialisers: removing them keeps 95/95 but the audit FAILs (23): the statics fall into VC7 name-hash order. January's declaration order therefore attests zero-initialised statics.

(i) HCEX's static-local 'point2d dimensions' spelling for the two bitmap-dimension statics was measured (D_u, D_z, D_uN, D_uN_ord) and cannot give January's .bss order. It is REFUTED, so they stay descriptive file statics.

Final measurements:
- cand_L vs the csplit-emulated split_L: gate 95/95 EXACT; _point_from_line3d guard passed.
- object_audit PASS, 216/216 symbols; all sections equal, including .rdata align 4 and .bss align 8.
- pdb_storage: 0 disagreements.
- 15 surplus externals ALL IDENTICAL to January's selected copies; provider_link PASS; no static code surplus.
- objdiff 3.3.1 on its own project: data 100% (9048/9048), code measures identical to production.
- csplit: only rasterizer_xbox.obj differs (833 objects compared).
- The 7 other rasterizer_xbox_internal.h includers compile IDENTICAL with the shadow header, which a control header proved was used.
- /W3 shows no new warnings; fake_match_scan reports 0 leads.
- cand_F (file-scope tables) is also 95/95 and audit PASS, but HCEX scope evidence contradicts it.

## blockers
None found. Owner-visible disclosures:
(1) 9 of the 23 separated statics keep their pre-existing descriptive names; no first-party name exists (HCEX and the /Od build are dx9 rewrites): node_matrix_constants, bitmap_dimensions_non_blocking, bitmap_dimensions, global_d3d_texture_render_primary, global_d3d_surface_render_primary, global_d3d_texture_render_secondary_z, global_d3d_surface_render_secondary_z, global_d3d_texture_render_primary_copy, global_d3d_surface_render_primary_copy.
(2) The filthy initializer is renamed to the plural symbol form, while January's assert message keeps the singular.
(3) The zero initialisers are layout-attested, not decoration.

## reopen_criteria
If the owner rejects the plural rename, drop step (5b) in mk_cand.py and the filthy replace_line in mk_config.py. The result stays 95/95 with audit PASS; the name changes only in symbols.json and the source. If the owner prefers file-scope tables, cand_F with config_F is exact and passes the audit. Revisit the 9 descriptive names if a January-era map or PDB with data statics turns up.

## task notes
No tracked file was edited; git status is clean apart from untracked files. Generators, all under scratch\w\rasterizer_xbox_audit\:
- mk_cand.py and mk_config.py (L|F): candidates and config copies.
- Emulated splits in split_L and split_F, produced with build/tools/csplit.exe -i cachebeta.exe.
- lab_gate.py (gate.py plus --tmpdir/--split-root) and audit2.py (object_audit.py plus AUDIT_SPLIT_ROOT).
- surplus_check.py (surplus_identity logic on a candidate object) and pdb_storage_cand.py.
- od_report.py (objdiff 3.3.1 report on a private project).
- mk_dims.py plus lab/ for the refuted HCEX 'dimensions' layout lab.
- hcex_rasterizer_dx9.txt (DIA2Dump -compiland dump).

Wave-1 errors corrected:
- The wave-1 patch files had b/ paths that git apply could not use; they are regenerated here.
- '.rdata reverse definition order' was a wording error: offsets actually follow definition order.
- The descriptive table names and the 'keep singular' filthy name are superseded by HCEX and the 2001-09-25 linker-map evidence.
- The decals NODUP blocker is gone after Layer 1.

Cross-unit follow-up for the integrator: rasterizer_xbox_debug (already Matching) fails today's object_audit with 8 non-static XDK wrapper rows. followup_rasterizer_xbox_debug_symbols.json.patch fixes it; it applies with or without the main symbols patch, changes only that unit's split, and gives audit PASS and gate 18/18.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_audit\LEDGER.md
