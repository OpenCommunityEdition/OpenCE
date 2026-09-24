# review source/units/units
approve=False

## per_object
[
 {
  "unit": "source/units/units",
  "approve": false,
  "issues": "As submitted: blocked by the misleading, unattested static name _unit_verify_inventory at 0x19ef10. January atlas exact tier and the Sept-2001 cachebeta map (same 0x90 extent) both say unit_vectors_are_valid, and patch 02 edits that very symbols.json line. A false HCEX comment also sits above the renamed unit_add_initial_weapons. Every byte, storage, provider, data and objdiff 3.3.1 strict-emulation claim was reproduced independently: 189/189 EXACT on the emulated split, audit PASS, storage 144/50/0, provider_link PASS, strict 192/192 with data 6752/6752. Approve with the amendment: scratch/w/review2_units/patches/01_units_c_names_static.AMENDED.patch and 02_symbols_json_names_static.AMENDED.patch (or the worker's 01 and 02 plus 07_incremental_on_top_of_worker_01_02.patch), together with the worker's 03, 05 and 06. The amendment is name- and comment-only, keeps the line count, and was fully re-verified."
 }
]

## checks
All checks were run on my own artifacts under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_units\ (evidence: LEDGER.md and REVIEW.md).

(1) Patches apply cleanly.
- git apply --check (also with --whitespace=error-all) passes on 931ed8dc for all five worker patches, one at a time and together.
- Applying them to a temp copy reproduces candidate_units.c and the four config files byte for byte, CRLF kept.
- units.c: 286 changed lines, line count unchanged at 11700. The changes are only the 25 renames, `static` on 2 functions and the header-comment listing.
- symbols.json: exactly 49 in-place edits.

(2) csplit emulation.
- Control run with an unchanged config reproduces build/split exactly (833 objects).
- With the candidate config, only units.obj differs.

(3) Function gates.
- gate.py against the production split: production 189/189; candidate 142/22/25 (renames only).
- Own gate_emul.py against the emulated split: 189/189 EXACT (54,864 B). Storage matches: 142 external, 47 static.

(4) object_audit against the emulated split: PASS, 321 symbols, 0 differ.

(5) Candidate vs production object.
- 447/447 sections are identical except .debug$S.
- Relocations are equal after applying the rename map.
- Symbols are equal apart from the renames and the two external-to-static changes.

(6) Storage against cachebeta_publics.txt (own address- and name-keyed script).
- All 144 externals are public under the same name at the same RVA.
- All 50 statics have no public, either at their RVA or by name.
- Name-based pdb_storage: 0 disagreements (production has 49).

(7) Surplus.
- provider_link: all 97 surplus externals PASS in both orders.
- surplus_identity: 31/31 helper COMDATs IDENTICAL to January's copy.
- D3D tables: 57 objects already marked Matching emit the same 3 SDK tables as surplus, so they are admissible by precedent.

(8) _point_from_line3d.
- January has it UNDEF, with relocations at +265 and +325 in _unit_cause_continuous_melee_damage; ours has the same two.
- Our 48-byte COMDAT is identical to action_charge's.
- Owner ruling 20260921 #1 (Lane B handoff, section 3) names units explicitly.
- Keeping the rejection produces completion-label-contradicted.

(9) Data. section_info_resolved is equal on both the production and emulated splits and under both symbols.json versions: 1564 / 7 / 6ed61c39..., matching patch 05.

(10) objdiff-cli 3.3.1 on my own single-unit projects.
- The production control equals the build/report.json units entry byte for byte.
- With complete=true, the three $L labels score 100 (the same holds for the 9 $L labels in 4 units already marked complete).
- Strict pipeline: 192/192, 53478/53478, 6752/6752, nothing revoked or contradicted.

(11) Other gates.
- No units parks and no symbol_ownership entries.
- The only units semantic_matches entry (_unit_is_busy) is not renamed.
- None of the other 1,452 split/base objects references any old or new name.
- No name collisions and no publics for the new names.
- fake_match_scan: 0 leads.

(12) House rules and strip test.
- The only non-rename construct is `static`. It does not change code bytes (production without it is 189/189) and is January's storage truth, so it is not decoration.
- No header edits and no owner-gated classes.

(13) Name evidence.
- Checked every rename against the January atlas (4cc87b45), both Sept-2001 maps (7eacac85 and 64550664), HCEX_Release (re-queried with DIA2Dump) and HCEX.pdb.
- The atlas anchor check over the units range agrees on 123 of 124 names. The one disagreement is the defect below.

(14) Amendment built and fully re-verified.
- csplit: only units.obj changes.
- 189/189 EXACT; audit PASS; storage 144/50/0; pdb_storage 0; provider_link PASS; fake scan 0.
- Strict emulation 192/192, data 6752/6752.
- Byte-identical to the candidate except one symbol name.
- The amended patch set applies on 931ed8dc and reproduces the files byte for byte.

## issues
BLOCKING, but fixable with a verified amendment.

1. Wrong function name at 0x19ef10: `_unit_verify_inventory` (config/symbols.json line 7004, which patch 02 itself edits to add static).
- The function checks that facing/aiming/looking vectors and forward/up axes are valid. It has nothing to do with inventory.
- First-party evidence says the name is unit_vectors_are_valid:
  - The January atlas 4cc87b45 (2002-01-14 cachebeta_exe) names 0x59ef10 `_unit_vectors_are_valid` at the exact tier.
  - The Sept-2001 cachebeta_xbe map 7eacac85 (ground truth) has unit_vectors_are_valid at 0x1ada40, directly followed by unit_verify_vectors at 0x1adad0. The gap is 0x90, the same as January's 0x19ef10 -> 0x19efa0.
  - The 1749betap and 2002 cachebeta_xbe atlases agree.
- `unit_verify_inventory` appears in no map, atlas, HCEX PDB or doc. HCEX and HCEX_Release have neither name because the function is debug-only.
- Admitting the object as submitted would freeze a misleading name, against house rules 7 and 21. It also contradicts the packet's own claim that its names are first-party.

2. False comment directly above the renamed unit_add_initial_weapons (candidate line 2485): "HCEX identifies this private helper as unit_create_initial_weapons".
- HCEX.pdb, HCEX_Release.pdb and both Sept-2001 maps all name it unit_add_initial_weapons.

FIX (verified, all under scratch/w/review2_units/):
- Use patches/01_units_c_names_static.AMENDED.patch and patches/02_symbols_json_names_static.AMENDED.patch in place of the worker's 01 and 02. Alternatively, apply the worker's 01 and 02 followed by patches/07_incremental_on_top_of_worker_01_02.patch.
- units.c: rename unit_verify_inventory to unit_vectors_are_valid at 5 sites (including the header listing). Rewrite the comment in place, 2 lines for 2 lines, so line count stays 11700 and assert line literals do not move.
- symbols.json line 7004: rename to "_unit_vectors_are_valid" and keep "static": true.
- Full amended files: amend/units.c and amend/config/symbols.json.
- The worker's 03, 05 and 06 stay as they are. Do not apply optional/04 (not needed: the three $L labels already score 100 once the unit is marked complete).
- With the amendment, every gate passes (see checks_run point 14) and I would approve.

Non-blocking notes:
- The worker says 16 renames are atlas exact-tier; the count is 15, and unit_ping_animation is fuzzy tier. It is still confirmed by both Sept-2001 maps and HCEX_Release.
- Follow-up: HCEX names the struct `euler_axis_plan` where ours has `unit_acceleration_plan`.
- Follow-up: unit_animation_state_get_aiming_screen_index returns long in ours but short in HCEX (pre-existing; bytes are exact).
- Pre-existing single-line static prototypes at units.c:1010 and 1026 break house rule 2 (one parameter per line). They were not introduced by this packet.
