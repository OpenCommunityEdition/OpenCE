# LEDGER r2w2_projectiles (admission of source/items/projectiles semantic data entry)

- Claimed source/items/projectiles as r2w2_projectiles (tree HEAD 434f0151).
- Build state: HEAD 434f0151 (ledger-only on top of 05255584). build/split (09:24:02) and build/report.json (09:24:05) postdate the
  config/symbols.json + config.json edit (09:23:18); symbols.json diff f6d00a8c..HEAD touches no projectiles/literal names. build/base
  projectiles.obj (07:40) postdates projectiles.c (07:39). A fresh gate compile (prod_head.obj) agrees -> the build is current.
- gate --all (production source): 29 EXACT, residual _projectile_new 432 [sha] (active park, class unclassified). gate_prod_head.txt.
- DATA re-measure at HEAD (measure_data.py -> measure_data_head.txt): split sec#54 / base sec#3 / fresh prod_head.obj: .data 1548 B,
  flags 0xc0400040, 6 DIR32, sha 7bfaf5fb0b0c9e0593153885a81788bc8b118a9651a60a8a94e5ffd9fd203cf2, owners _projectile_update_section@0
  and _effect_marker_names@1528 both static(3). section_infos_equal True and the section_info_resolved dict (the verifier's check)
  identical for split/base/prod_head. Snapshot identical to round 1.
- Raw relocation spelling: @1528 normal / @1544 gravity literals are UNDEF in January's split and defined select-any .rdata in ours;
  both resolve to the same image addresses (0x25ad38, 0x25a8f0). The other 4 literals are defined in both.
- Literal payloads identical to January's selected providers (game.obj 'normal\0', particles.obj 'gravity\0', same flags) -> literal_identity.txt.
- report.json: projectiles total_data 2092, matched 544 -> unmatched 1548 == entry size; .data 99.096275, .rdata 100.0; no 'complete' metadata.
- Packet built (build_packet.py): entry appended as the 41st element in the leaf_map form (unit, symbol, allow_incomplete_unit, reason,
  measurements{size, relocation_count, normalized_sha256}). LF patch packet/semantic_data_matches.patch: git apply --check OK on worktree
  and --cached; throwaway autocrlf=true repo apply gives uniform CRLF == expected.
- Emulated configure.py progress in memory (emulate_progress.py): +1 verified data exception, +1548 data bytes (halobetacache
  2,583,826 -> 2,585,374), code/functions/complete_units unchanged (502 / cat 384), no other unit changes, 0 revocations, parks 83 active, none stale.
- Negative controls (negative_controls.txt): tampered sha, wrong reloc count, missing allow_incomplete_unit all fail closed. Using the
  other owner of the SAME section (_effect_marker_names) also verifies (same section, same base) -> expected, not a defect.
- Section-4 audit on production: object_audit FAIL(1) = only _projectile_new DIFF; .data ok 1548/1548; 55 January symbols 0 differ.
  pdb_storage 0 disagreements. surplus_identity 25 COMDATs 0 not identical (round-1 _distance3d difference is gone at HEAD).
  provider_link PASS both orders for all 56 surplus names incl. the normal/gravity literals (providers game/particles).
- Compatibility: round-1 zero-credit production.patch (rule-6 point_from_line3d call) applied to a CRLF copy -> gate 29/1 (same) and
  the .data section is still identical (cand_prodpatch.obj). The entry holds whether or not that patch lands.
- CAUSE PROOF (objdiff_mini, objdiff-cli 3.3.1 sha1 3130e428): January vs itself .data 99.7416 (scorer cannot credit even identical objects);
  January vs production 99.096275 (reproduces report.json); both objects with '$'->'_' renamed identically in the string table -> .data 100.0.
  The '$'-literal name defect is the sole cause of the undercredit.
- VERDICT: every condition of the leaf_map allow_incomplete_unit precedent holds -> ADMISSION_FIX_PROPOSED (data credit +1548 only).
- Released claim r2w2_projectiles (ADMISSION_FIX_PROPOSED). MANIFEST.md written.
