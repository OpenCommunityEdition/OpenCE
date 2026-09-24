# source/game/game_engine_king

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/c2/game_engine_king.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/c2/game/game_engine_king.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/king_source.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/king_source_struct_move_ALT.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/symbols_king.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/config_status_king.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/semantic_data_matches_king.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/symbol_ownership_king_OPTIONAL.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/config/config.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/config/semantic_data_matches.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/config/symbol_ownership.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/semantic_data_matches_entry.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/c1/game_engine_king.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/c4/game_engine_king.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/split/source/game/game_engine_king.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/LEDGER.md"
]

## production_changes
Apply these together. All patches are under C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/patches/. `git apply --check` passes on each one and on the combined set.

(1) Source: king_source.patch (RECOMMENDED; this is candidate c2). It edits source/game/game_engine_king.c and .h:
- the 22 engine-table callbacks become `static` (king_engine_dispose, _initialize_for_new_map, _dispose_from_old_map, _player_added, _game_ending, _game_starting, _statistics_append, _handle_client_message, _handle_server_message, _pregame_post_rasterize, _post_rasterize, _player_update, _player_damaged_player, _player_killed_player, _display_score, _prespawn_player_update, _goal_matches_player, _update, king_get_score, king_get_score_string, king_get_score_header_string, king_get_team_score_string);
- `struct king_globals king_globals = { 0 };` becomes `static struct king_globals king_globals = { 0 };`;
- game_engine_king.h loses those 22 prototypes and `extern struct king_globals king_globals;`.
No static prototypes are added. This follows the oddball precedent, and none of the 22 is forward-referenced in the TU: they are only named by the king_engine table at the bottom of the file.

Byte-identical alternatives; pick exactly ONE source patch:
- scratch/w/data_gap/patches/king.patch (= c1). Same change, plus 22 static prototypes in the .c prototype block.
- king_source_struct_move_ALT.patch (= c4). c2 plus struct king_globals and its 4 verify typedefs moved from the header into the .c structures block, as oddball/ctf do.

(2) config/symbols.json: symbols_king.patch. Full edited copy: scratch/w/king/config/symbols.json. It makes 23 in-place line edits, each appending `, "static": true` to the existing line, with no reorder or re-serialise:
- lines 2302, 2305-2312, 2314, 2316-2320, 2323-2326, 2328-2330 (the 22 functions above);
- line 22992 (_king_globals).

(3) config/config.json: config_status_king.patch. Line 678, source/game/game_engine_king.c status NonMatching -> Matching.

(4) config/semantic_data_matches.json: semantic_data_matches_king.patch. Full copy: scratch/w/king/config/semantic_data_matches.json. Standalone entry: scratch/w/king/semantic_data_matches_entry.json. It appends one single-section entry: unit source/game/game_engine_king, symbol _king_engine, measurements size 136, relocation_count 23, normalized_sha256 4009f5b6117e9f00cb001a23b9a56b9dc51a694206c442ce715e2bed2d5465a7. It has NO allow_incomplete_unit. Do NOT use the king entry in scratch/w/data_gap/semantic_data_matches_additions.json: its allow_incomplete_unit flag and its _distance3d NODUP reason went stale after Layer 2.

(5) OPTIONAL: config/symbol_ownership.json via symbol_ownership_king_OPTIONAL.patch. Full copy: scratch/w/king/config/symbol_ownership.json. It adds a .bss snapshot pinning the three file-static King globals (_king_globals@0, _king_engine_hill_count@428, _king_engine_hills@432, all storage 3; 560 B), following the objects precedent.

Header-edit flag for the integrator's full sweep: game_engine_king.h is included only by game_engine_king.c. No object in build/base (621) or build/split (833) has an UNDEF reference to any of the 23 symbols. game_engine_list.c declares king_engine itself (a local extern) and does not include the header. The integrator's full ninja, stable 8,245-function diff, parks, admission audit, fake scan and pytest are still required.

## evidence
Storage truth, from scratch/tools/cachebeta_publics.txt:
- The only publics in this TU are _king_engine, _render_dynamic_quad and _render_dynamic_quad_initialize.
- The 22 table callbacks and _king_globals are absent, so they are file-static. pdb_storage at production shows 23 disagreements, all 'split 2 ours 2 PDB-public False'.
- There are 0 UNDEF references in any January or production object.

Compiles (scratch/w/king/sgate.py compiles in place with the shadow header; a #error probe proved the header is used):
- c1, c2, c3 and c4 are each 29/29 EXACT, against both build/split and the emulated split.
- The official gate.py --source run on the c2 .c is also 29/29.
- objcmp against production: 59 owner-keyed sections, 0 byte/relocation/flag differences. Only the 23 symbol storage rows change (2 -> 3), so the surplus is unchanged.

Emulated split (scratch/w/king/symfix.py + csplit):
- The 23 line edits change only game_engine_king.obj among the 833 split objects.
- object_audit against the emulated split: PASS for c1, c2, c3 and c4 (39 January symbols, 0 differ; every January section ok).
- Control: the production object against the emulated split FAILS with 23 storage mismatches.
- pdb_storage against the emulated split: 0 disagreements (the control has 23).

Surplus:
- surplus_identity: 10 code COMDATs, 0 not identical. _distance3d is now IDENTICAL to action_vehicle's copy.
- provider_link on the c2 object: SELECTED-PROVIDER LINK PASS for all 22 surplus rows, in both orders.

Data:
- section_info_resolved(_king_engine) target == base under both the current and the edited symbols.json, and with c1 and c2: 136 B, 23 relocations, sha 4009f5b6...65a7.
- relrows: all 23 relocations OK. The only raw difference is the 'king' literal ??_C@_04PJOEONHN@king?$AA@, which is UNDEF in January (folded to game_engine) but a defined COMDAT in ours. This is the objdiff 3.3.1 '$' defect, and the section is the entire 136 B unmatched (.data 98.24561%).

Full chain (scratch/w/king/emu_progress.py: rejections, semantic matches, accepted ledger, data matches, ownership snapshots, revoke, parks, audit_object_admission):
- A mini objdiff 3.3.1 report gives king rows identical to production, and adds complete 4865/876/1 when marked complete.
- EMU vs BASE: Halo complete units 363 -> 364; complete_code +4,865; complete_data +876; matched data +136 (the _king_engine credit, with no incomplete opt-in).
- King .bss snapshot validated; revoked []; parks 93/0/0; admission 15/0/7/0 with 0 contradicted. The same result holds for the c1 and c4 objects.

Other checks:
- fake_match_scan: 0 leads on all candidate .c/.h files.
- No king entries in parked.json, object_admission_rejections.json or semantic_matches.json.
- The full ledger is in scratch/w/king/LEDGER.md.

## blockers
None for admission. The choice between the three byte-identical source forms is a house-style decision: c2 has no prototypes (recommended, oddball precedent); c1 adds static prototypes (the data_gap king.patch); c4 also moves the struct into the .c. Bytes cannot tell them apart.

## reopen_criteria
Reopen only if one of these happens:
- the integrator's full ninja or stable diff shows any king row change;
- the provider link for any surplus helper regresses after a later real_math.h or provider edit;
- objdiff is upgraded to 3.6.0, in which case the '$' defect entry may become redundant and must be re-measured, not deleted blindly.

## task notes
1. Why the admission is now possible. The data_gap worker's king data entry carried allow_incomplete_unit because of a _distance3d NODUP provider conflict. Layer 2 fixed that conflict: provider_link now PASSES and surplus_identity reports 0 non-identical COMDATs. The object therefore completes as Matching, and the replacement entry needs no incomplete opt-in.

2. Precedent debt outside this unit (not changed). Two already-Matching precedent objects have the same storage gap that king had:
- slayer `_slayer_globals` is external but absent from cachebeta publics.
- ctf `_ctf_state_message_update_warning` is external but not public; its prototype is in game_engine_runtime.h.
Both have 0 UNDEF references in any January or production object. This is a candidate for a follow-up zero-credit storage correction.

3. Emission-order evidence (recorded only; no change proposed). January's .text section order is the HCEA engine-table order. Each static helper is defined just before its first use. initialize_for_new_map, post_rasterize and update are deferred to the end: each calls a helper that is defined later (wave-1 law a). Section order is not part of strict exactness or object_audit.

4. Housekeeping. The emulated split was pruned to the king object after the 833-object census, which showed only game_engine_king.obj changes. No tracked file was edited. No ninja, configure or git-mutating command was run.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/king/LEDGER.md
