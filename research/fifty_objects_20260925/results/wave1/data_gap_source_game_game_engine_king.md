# source/game/game_engine_king

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/king/cand.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/king/game/game_engine_king.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/patches/king.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/fix_king.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/semantic_data_matches_additions.json"
]

## production_changes
(A) Case A credit, +136 B. Append the _king_engine entry from scratch/w/data_gap/semantic_data_matches_additions.json to config/semantic_data_matches.json:
  unit source/game/game_engine_king
  symbol _king_engine
  allow_incomplete_unit true
  measurements size 136, relocation_count 23, normalized_sha256 4009f5b6117e9f00cb001a23b9a56b9dc51a694206c442ce715e2bed2d5465a7
(B) Optional zero-credit ownership correction, matching the oddball/slayer precedent:
  - git apply scratch/w/data_gap/patches/king.patch:
    - the 22 engine-table functions become static;
    - their prototypes move from game_engine_king.h into the .c prototype block as static prototypes;
    - `static struct king_globals king_globals = { 0 };` replaces the public definition;
    - `extern struct king_globals king_globals;` is removed from the header.
  - Add , "static": true to symbols.json lines 2302, 2305-2312, 2314, 2316-2320, 2323-2326, 2328-2330 (the 22 functions) and 22989 (_king_globals). The exact names are in scratch/w/data_gap/fix_king.txt and the edits are in patches/symbols_json_all.patch.
(C) Keep the status NonMatching.

## evidence
The .data _king_engine section (136 B, 23 relocations) resolves identically under coff_compare, with equal flags c0400040. January's name relocation targets '??_C@_04PJOEONHN@king?$AA@', which is UNDEF in January and folded to game_engine. That triggers the objdiff 3.3.1 '$' defect, and the section scores 98.25%.

The code is 29/29 strict exact. The base object_audit shows PASS.

cachebeta publics lack the 22 table functions and _king_globals, and 0 UNDEF references exist anywhere. game_engine_king.h is included only by game_engine_king.c.

The ownership candidate compiles with the shadow header through sgate.py at 29/29 EXACT, and the only warning is the pre-existing C4700.

Against a scratch csplit with 23 static flags (only this object changes): object_audit PASS at 39/39 symbols, the _king_engine and _king_globals sections are equal, and PDB storage shows 0 disagreements.

The objdiff mini report is unchanged.

## blockers
The object cannot be Matching. The surplus _distance3d COMDAT is DIFFERENT from January's selected action_vehicle copy (x87 order), and provider_link fails with LNK2005 against the action_vehicle NODUP hand copy. This is pre-existing and board-wide: 18 objects are affected and none of them is Matching. The data entry therefore needs allow_incomplete_unit.

## reopen_criteria
Flip to Matching after distance3d's COMDAT equals January's selected copy, or the owner rules on the action_vehicle NODUP provider.

## task notes
Expected credit: +1,973 data bytes.
- +1,773 comes from the three Case A single-section entries (periodic 96, king 136, object_lights 1,541); scratch/tools/progress_calc.py takes data from 2,579,142 to 2,580,915, with 38 credits and 0 revoked.
- +200 is path_structure_bsp's ordinary objdiff credit after the Case B reconstruction.
- Code is unchanged and there are no function regressions.

The Case A entries and the ownership/source patches are independent. The entries verify against today's production objects, and I re-checked their snapshots under the proposed symbols/storage changes; they are unchanged.

Integrator order: apply the patches, then csplit plus ninja, then rerun the verifier and progress. The combined symbols.json edits (34 ops, scratch/w/data_gap/fix_all.txt) have been tested with a scratch csplit (cfg_all_split). They change exactly 4 split objects out of 833.

Tool caveat for other workers: tools/campaign/gate.py copies --source to scratch/_gate_PID.c, so a shadow header placed beside the candidate is silently ignored. scratch/w/data_gap/sgate.py compiles in place and honours shadow headers; I confirmed this with a #error test.

Code under-credit is already handled: objdiff scores periodic_function_build_table, transition_function_build_table, bitmap_copy and bitmap_draw_line below 100%, but all four are strict exact and already credited through build/semantic_report.json hidden_exact.

Systemic blocker found: 18 base objects emit a header-inline _distance3d whose bytes differ from January's selected action_vehicle copy. It also fails pair-link against action_vehicle's NODUP hand copy, and none of those objects is Matching. This alone stops king and object_lights from becoming complete objects.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/LEDGER.md
