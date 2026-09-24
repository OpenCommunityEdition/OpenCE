# source/units/units

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\candidate_units.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\config\\semantic_data_matches.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\config\\object_admission_rejections.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\patches\\01_units_c_names_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\patches\\02_symbols_json_names_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\patches\\03_config_json_units_matching.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\patches\\05_semantic_data_matches_units_data.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\patches\\06_admission_rejections_retire_units.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\units_ownership\\LEDGER.md"
]

## production_changes
Five patches in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\units_ownership\patches\. Each passes `git apply --check` against 931ed8dc, and applying all five together reproduces the full candidate copies byte for byte (CRLF kept).
(1) 01_units_c_names_static.patch changes source/units/units.c; the full file is scratch/w/units_ownership/candidate_units.c. It renames 25 `code_` placeholders in word-boundary form (map in renames.txt): 00197e30 unit_euler_axis_doplan, 00197f90 unit_animation_state_interruptable, 00198050 unit_animation_overlay_action_loops, 00198070 unit_animation_state_loops, 001980d0 unit_animation_weapon_ik, 00198130 unit_animation_vehicle_ik, 00198170 unit_animation_aiming_screen, 00198190 unit_animation_state_get_aiming_screen_index, 001981f0 unit_animation_state_can_be_entered_without_animation, 00198230 unit_animation_compute_interpolation_frame_count, 00198400 unit_animation_start_overlay_action, 00198e40 unit_animation_impulse_get_index, 0019a170 unit_update_driver_and_gunner, 0019a640 unit_first_free_weapon_index, 0019b0b0 unit_set_animation, 0019b160 unit_animation_update, 0019b410 unit_drop_grenades, 0019b600 unit_melee_sound, 0019bf70 unit_euler_axis_buildplan, 0019c460 unit_euler_axis_couple, 0019dff0 unit_get_current_weapon_label, 0019ea70 unit_align_facing, 001a0cf0 unit_ping_animation, 001a1f50 unit_add_initial_weapons and 001a2030 unit_adjust_for_seat_change. It adds `static` to unit_ping_animation (prototype and definition) and unit_add_initial_weapons. It also replaces the 45 stale `_code_` entries in the header comment listing with the current names; line count is unchanged.
(2) 02_symbols_json_names_static.patch makes 49 in-place line edits to config/symbols.json; the full copy is scratch/w/units_ownership/config/symbols.json. The 25 renamed entries also get `"static": true`. Another 24 entries get `"static": true` only: 21 functions (lines 6867, 6891, 6940-6945, 6948-6950, 6955, 6972, 6975, 6987, 6991, 7003-7005, 7014, 7022) plus _unit_update_section (22779), ?last_time@?HC@??unit_update@@9@9 (22780) and _unit_globals (23283). The $L entries are left as they are, following the objects precedent.
(3) 03_config_json_units_matching.patch changes source/units/units.c from NonMatching to Matching in config/config.json.
(5) 05_semantic_data_matches_units_data.patch adds a single-section entry to config/semantic_data_matches.json: symbol _magic_base_animation_seat_index, measurements size 1564, relocation_count 7, normalized_sha256 6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7. This is the '$'-literal defect class.
(6) 06_admission_rejections_retire_units.patch deletes the units _point_from_line3d entry from config/object_admission_rejections.json (14 -> 13 entries).
patches/optional/04 holds $L owner_function semantic_matches entries. They are not required and are not recommended.

## evidence
All 47 object_audit diffs are symbol storage rows (split 2, ours 3). January-owned sections were already all ok. A new address-keyed check (addr_storage.py; RVA == file_offset, anchored at 0x197a70 and 0x315c08) finds all 144 external split symbols as cachebeta publics under the same name at the same address. The 47 symbols plus _code_001a0cf0 and _code_001a1f50 have no public at their address, so January had them file-static; this also covers the _unit_weapon_next_index PDB disagreement. None of the 49 is referenced by any of 1,452 other split or base objects.

Names come from first-party evidence and none were invented. The HCEX_Release.pdb units compiland (hcex_release_units_compiland.txt) lists the eight small static animation predicates in exactly January's address order, with matching signatures and identical state lists. The Sept-2001 map ground truth in the atlas has busy, state_loops, weapon_ik, vehicle_ik and get_aiming_screen_index in the same order. The January-hash atlas (exact tier) names 16 functions by address. unit_ping_animation is fuzzy tier in the atlas, but its 9 HCEX parameters are identical to ours. None of the 25 names is a cachebeta public, and none collides with any name in source/ or xbox/.

Emulation: csplit on an unchanged copy of config/ reproduces all 833 build/split objects byte for byte. With the edited symbols.json, only units.obj changes. Against that split, the candidate is 189/189 EXACT. object_audit reports PASS with 321 symbols and 0 differing. The name-based PDB storage check finds 0 disagreements.

provider_link passes in both orders for all 97 candidate-only surplus externals: 31 helper COMDATs, the 3 D3D SDK tables (which admitted objects and files_windows also emit), literals and floats. surplus_identity finds the 31 helper COMDATs identical to January's selected copies.

_point_from_line3d: January's units.obj holds it as an undefined external with 2 relocations from _unit_cause_continuous_melee_damage. Our 48-byte COMDAT compares section_infos_equal to the action_charge split copy (sha 9b763841...). Owner ruling 20260921 #1 names units explicitly.

.data: section_info_resolved is equal on both the production and emulated splits, and all 7 relocations resolve to the same image addresses.

Frozen-scorer emulation (objdiff-cli 3.3.1 on single-unit projects plus the repository's own strict pipeline in strict_emulate.py): the production control reproduces build/report.json exactly (169/192 raw; strict 189/192, 5188 data). The candidate reaches 192/192 functions, 53478/53478 code and 6752/6752 data, with nothing revoked and nothing contradicted. The three $L labels score 100 raw once the unit is marked complete. The control that keeps the rejection gives completion-label-contradicted, which is why retiring it is required. fake_match_scan: 0 leads. The candidate's code sections are byte-identical to production apart from names.

## blockers
None found. Two follow-ups are optional and not blocking: HCEX names the plan struct euler_axis_plan while the source uses unit_acceleration_plan; and HCEX_Release gives different parameter-name orders for unit_animation_compute_interpolation_frame_count and unit_melee_sound. That is a later build, and exact bytes fix January's argument positions, so neither was changed.

## reopen_criteria
Reopen if the integrated full ninja shows any of the following. (a) A units.obj split that differs from scratch/w/units_ownership/split/source/units/units.obj; the edited symbols.json should reproduce it byte for byte. (b) Any units function that is not strict exact under the regenerated split. (c) apply_semantic_data_matches rejecting the pinned snapshot. (d) A provider_link LNK2005 caused by other lanes' header changes. If the owner rejects the 'descriptive' profile-section name unit_update_section, note that it is file-static with no first-party record.

## task notes
Integrator order: apply patches 01, 02, 03, 05 and 06, then run a full ninja. The ninja run re-splits from the edited symbols.json (only units.obj changes) and regenerates the report and semantic report. Expected result: units becomes Matching and complete, strict 192/192 functions (+48 code bytes from the three $L labels), data 5188 -> 6752 (+1,564), Halo objects +1. The rename-stable diff should show 25 name changes at unchanged addresses and no byte changes.

Evidence files in scratch/w/units_ownership/:
- hcex_release_units_compiland.txt: first-party names.
- addr_storage.py and addr_storage_{base,cand}.txt: storage checked by address.
- audit_base.txt and audit_cand.txt: object_audit before and after.
- gate_cand_newsplit.txt: 189/189 against the emulated split.
- provider_link_cand.txt: surplus provider links.
- objproj_*/ and strict_emulate.py: frozen-scorer emulation and controls.
- split_ctrl/ and split/: control and emulated csplit outputs.

Tools used: csplit and objdiff-cli 3.3.1, both from build/tools, run only on inputs and outputs in scratch. No tracked files were edited and no ninja, configure or git-mutating commands were run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\units_ownership\LEDGER.md
