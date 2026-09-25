# review_critic_3_ai_debug LEDGER (adversarial review of critic_3 patches 01/02, source/ai/ai_debug)

Worktree HEAD 5d3ca708 (task text said 7b9de585; 7b9de585..5d3ca708 = one docs ledger per critic_3). Slug normalised to
review_critic_3_ai_debug (orchestrator slug string was garbled with critic_3's unit list). Claimed source/ai/ai_debug.

## R1 patch application (read-only against the tree)
- `git apply --check critic_3/patches/01_ai_debug.patch` at HEAD: OK. Outside any repo (scratchpad copy of HEAD blobs):
  01 then 02 apply cleanly. GNU patch 01(+02) onto `git show HEAD:` copies -> t01/, t02/; t01 ai_debug.c == critic
  shadow_ALLxs (CR-insensitive). symbols.json: exactly 32 rows change (21 rename+static in 01, 11 static-only in 02).
- Zero `code_` identifiers remain in ai_debug.c after 01.

## R2 name evidence (independent)
- Sept-2001 cachebeta.map (2001-09-25) ai_debug.obj, Static-symbols section: all 21 Sept names are statics; in-order
  pairing between shared public anchors is UNIQUE (each gap has equal counts) and all 20 sizes equal except 0x38280
  (Jan 0x150 vs Sept 0xf0).
- January atlas 4cc87b45 (keyed by cachebeta.exe; lib ai_debug.obj): the proposed name at the exact January address for
  19 of 21 rows (task text said 18) - atlas_check.txt. The 2 without a January-exe record: render_spatial_effects is
  'exact' in the 2002-01-14 cachebeta_xbe atlas; get_this_actor is 'exact' in the Aug-2001 1749betap atlas + Sept map.
- January split relocations: 0x38280 calls local_player_exists, observer_get_camera, display_assert, system_exit,
  director_get_perspective, local_player_get_player_index, datum_get, collision_test_vector, object_try_and_get...,
  object_get_and_verify_type; its only caller is 0x39e10 (select_this_actor). Matches critic callgraph.
- cachebeta.pdb publics: no public at any of the 28 function RVAs or the 4 data RVAs, and no public named with any new
  name (pubcheck.py). January exe literals: ai_render_spatial_effects / ai_render_vehicles_enterable /
  ai_render_paths_nodes / ai_render_paths_failed / "ai_debug_lineofsight: overflowed ray|point buffer" etc. support,
  none contradicts.
- HCEX.pdb / HCEX_Release.pdb: no ai_debug functions at all (debug code compiled out) - no evidence either way.
- Removed stub provenance: added 2026-07-13 (37e96a1c, "// TODO: finish", community skeleton; punpckhdq is a
  skeleton reconstruction, not first-party) - removing it discards nothing authentic.

## R3 emulated split (own csplit runs, build/tools/csplit.exe -i cachebeta.exe -p cfg -o split)
- split_ctl == build/split (833/833). split_01, split_02 vs ctl: only source/ai/ai_debug.obj differs.
- objdetail.py: 01 = 21 renames + storage 2->3, section bytes identical, relocation rows identical except the target
  symbol's storage; 02 adds 11 storage 2->3. relocname.py: NO relocation symbol name changes with 02 (critic's
  "csplit spells 4 relocations as firing_position_color_count+N" is coff_compare's _defined_noncode_destination
  normalisation (nearest EXTERNAL anchor), not csplit output; both sides normalise the same way).

## R4 gate / objcmp / objdiff
- gate prod 59/1, 01 59/1, 02 59/1; per-function rows identical under the rename map (only residual
  _ai_debug_render_actor 24976 vs 25008, unchanged).
- objcmp prod.obj vs c01.obj: all non-.debug$S sections identical except $L label numbering (uniform -35) in 2 .text
  relocation lists; lab/stubless.c (HEAD minus stub+prototype ONLY) vs prod reproduces the $L shift; stubless vs
  c01 identical (rename-mapped). c01.obj == c02.obj (same source).
- objdiff 3.3.1 (sha1 3130e428): p/a/b all 16446/42523 code, 58/61 fns, 5818/5874 data, 98.09388; per-function
  measures identical under the rename map.

## R5 audit battery (with_split.py redirect, tools unmodified)
- object_audit FAIL(34) -> FAIL(13) [01] -> FAIL(2) [01+02: render_actor residual + render_actor_jmptable MISSING].
- pdb_storage 33 -> 12 -> 1 (jmptable). surplus_identity 19 candidate-only COMDATs, 0 not identical (all states).
  provider_link PASS (both orders, all states).
- CL /Zs /W3: candidate removes the stub's 7 C4101 unreferenced-local warnings; no new warning.

## R6 defect found and amended
- The stub removal left its leading comment `/* Render the path-node overlays selected by the caller. */` orphaned
  directly above `ai_debug_render_surface` (mis-describes that function). Amended: comment moved above the real
  `ai_debug_render_path_nodes` definition (01_ai_debug_amended.patch). t01a gate 59/1 rows identical; c01a.obj ==
  c01.obj except .debug$S; battery identical; amended 01 passes git apply --check at HEAD and 02 applies after it.
