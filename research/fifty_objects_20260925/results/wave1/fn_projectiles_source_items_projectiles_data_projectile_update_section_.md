# source/items/projectiles:.data(_projectile_update_section)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\projectiles\\semantic_data_entry.json"
]

## production_changes
Append the JSON object in scratch/w/projectiles/semantic_data_entry.json to config/semantic_data_matches.json. It is a single-section entry: unit source/items/projectiles, symbol _projectile_update_section, allow_incomplete_unit true (same as the leaf_map precedent), measurements size 1548, relocation_count 6, normalized_sha256 7bfaf5fb0b0c9e0593153885a81788bc8b118a9651a60a8a94e5ffd9fd203cf2. If you prefer not to use allow_incomplete_unit, hold the entry until the object is complete. It credits exactly the 1548 unmatched data bytes (total_data 2092 minus matched 544).

## evidence
The January .data section [54] (owners _projectile_update_section @0 and _effect_marker_names @1528, both static, flags 0xc0400040, 6 DIR32 relocations) matches both build/base and the candidate. section_infos_equal returns True, and section_info_resolved against the symbols.json image addresses gives an identical snapshot. object_audit shows .data 1548/1548 with flags and alignment equal. pdb_storage reports 0 disagreements. Cause of the undercredit: January's split leaves the folded literals ??_C@_06IKLLLDHL@normal?$AA@ and ??_C@_07GFBFDLBM@gravity?$AA@ undefined (relocations @1528 and @1544) while our object defines them, and objdiff 3.3.1 has the known '$'-literal name defect. The rdata section is already at 100%.

## blockers
None for the data itself. The only open question is the integrator's choice between allow_incomplete_unit now and waiting for object completion.

## reopen_criteria
Re-measure the snapshot if the .data owners, string literals or symbols.json addresses change.

## task notes
No tracked files were edited and no ninja, configure or git-mutating commands were run; only `git apply --check` was used, and it is read-only. The mechanism is new: the x87-versus-integer emission order after an inlined point_from_line3d depends on whether a C parenthesis node wraps the t argument's load. The paren node survives a copy through a local, and it cannot be reached by casts, local-count changes (M8 or name-count) or constant folds. This probably explains the ai_debug `ai_debug_attractor_label_height` precedent too, and similar held paren sites such as collisions.c:1572, players.c:3037 and bipeds.c:4029. The board scan (scratch/w/projectiles/scan_copy.py, output scan_copy.txt) lists 19 January 'COPY,x87,filler' sites against 58 'COPY,filler,x87' sites, which gives a census of where paren-shaped sources may hide. The provider-link failures (3 NODUP providers) and the non-identical surplus _distance3d already exist in production base and block whole-object admission independently of _projectile_new.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\projectiles\LEDGER.md
