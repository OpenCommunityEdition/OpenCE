# source/items/projectiles (admit source/items/projectiles)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/packet/semantic_data_matches.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/packet/semantic_data_entry.json",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/packet/semantic_data_matches.json",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/MANIFEST.md",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/LEDGER.md"
]

## production_changes
Config only; the integrator applies it. Append one entry to config/semantic_data_matches.json with C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/packet/semantic_data_matches.patch (LF; git apply --check passes on the worktree and with --cached; a throwaway autocrlf=true apply gives uniform CRLF output). The entry follows the leaf_map form exactly. Fields: unit source/items/projectiles, symbol _projectile_update_section, allow_incomplete_unit true, reason, measurements {size 1548, relocation_count 6, normalized_sha256 7bfaf5fb0b0c9e0593153885a81788bc8b118a9651a60a8a94e5ffd9fd203cf2}. There are no source, header or symbols.json changes. Credit is +1,548 data bytes only (halobetacache 2,583,826 -> 2,585,374). There is no code or function credit, the object is not completed, and it stays at 29/30. The entry is also valid if the round-1 zero-credit rule-6 production.patch lands: that candidate still compiles to 29/1 with an identical .data section.

## evidence
Snapshot re-measured at HEAD. split sec#54, build/base sec#3 and a fresh gate compile agree: .data 1548 B, flags 0xc0400040, 6 DIR32 relocations, sha 7bfaf5fb (identical to round 1). The owners are _projectile_update_section@0 and _effect_marker_names@1528, both static. section_infos_equal is True and the section_info_resolved dict (the verifier's check) is identical. At @1528 and @1544, January's split leaves the pooled normal/gravity literals undefined, while ours defines select-any copies. Both resolve to the same image addresses, and the literal payloads are byte-identical to January's providers (game, particles). In report.json, total_data 2092 minus matched 544 leaves 1548 unmatched, equal to the entry; .rdata is 100.0 and the unit has no complete metadata. Cause proof with a mini objdiff-cli 3.3.1 report: January vs itself scores .data 99.7416, January vs production 99.096275 (the report.json value), and both objects with '$' renamed to '_' identically score 100.0, so the '$'-literal defect is the only cause. I ran configure.py progress in memory with the real pass order: it adds one verified data exception (+1548), leaves code, functions and complete_units unchanged (502, category 384), changes no other unit, revokes nothing, and parked validation passes (83 active, 0 stale). Negative controls: a tampered sha, a wrong relocation count and a missing allow_incomplete_unit each fail closed. Section-4 audit on production: gate --all gives 29 EXACT plus the parked _projectile_new residual. object_audit's only DIFF is _projectile_new; .data is ok; 55 January symbols, 0 differ. pdb_storage: 0 disagreements. surplus_identity: 25 COMDATs, 0 not identical. provider_link passes in both orders for all 56 surplus names. No rejections exist. symbols.json is unchanged, so no emulated split was needed. All 10 precedent conditions are checked one by one in MANIFEST.md (C1-C10), with quotes from the ten_more_halo_objects_20260923.md l.38-45 ruling, reconciliation l.55-56 and race doc l.21-22.

## blockers
None for the data entry. The object stays incomplete because _projectile_new (432 B) is a parked residual. It closes only with an outer parenthesis on the initial_velocity load, which is held by the owner under the strip-test ruling (round-1 lab_pfull/lab_localrhsparen; TRIAGE decision OWNER).

## reopen_criteria
Re-measure the snapshot if the .data owners, the string literals or the symbols.json image addresses change; the verifier fails closed on drift. If _projectile_new closes under an owner ruling, the unit becomes complete and allow_incomplete_unit becomes redundant but harmless.

## manifest
C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/MANIFEST.md

ledger: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/LEDGER.md
