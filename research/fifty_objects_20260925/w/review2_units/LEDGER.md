# review2_units LEDGER (adversarial review of units_ownership OBJECT_COMPLETE_CANDIDATE, base 931ed8dc)

## R1 patch application (931ed8dc)
- git apply --check: all 5 patches OK individually and together.
- Applied in a temp copy (outside repo): units.c, symbols.json, config.json, semantic_data_matches.json,
  object_admission_rejections.json all cmp-identical to the worker's candidate files.
- units.c diff: 286 changed lines, line count unchanged (11700); only identifier renames (25 names),
  `static` added to unit_ping_animation (proto+def) and unit_add_initial_weapons (def), and the
  header comment listing (45 stale `_code_` entries -> current names). No logic change.
- symbols.json diff: exactly 49 lines changed in place (23846 lines both), format matches existing
  `"static": true` entries.
- Old names outside units.c/symbols.json: only docs/ ledgers and research/PROMPT_* (history). New names: no
  collisions in source/ xbox/ config/; none in cachebeta_publics.txt.

## R2 independent emulated split
- csplit.exe with unchanged config copy -> split_ctrl == build/split (diff -rq empty, 833 objs).
- csplit with candidate symbols.json+config.json -> split_emul: ONLY source/units/units.obj differs.
- gate.py (production split): prod 189/189; candidate 142/22/25 (names only, expected).
- gate_emul.py (own script, gate.py rule) candidate vs split_emul: 189/189 EXACT, 54,864 B;
  storage 142 ext/ext, 47 static/static.
- object_audit (SPLIT_ROOT=split_emul): 321 January symbols, 0 differ, all sections ok -> PASS.
  Surplus: .drectve, 25 string literals, 3 D3D tables, 38 __real, 31 helper COMDATs.
## R3 byte identity / storage / surplus
- cmp_prod_cand.py: prod.obj (current units.c) vs cand.obj: 447/447 sections, only .debug$S differs; all
  relocations equal after rename map; 1262 symbols equal modulo rename, plus _code_001a0cf0/_code_001a1f50
  ext->static. Candidate is a names/storage-only change.
- storage_check.py (own, address+name keyed vs cachebeta_publics.txt, candidate symbols.json):
  144 externals all public at same RVA under same name; 50 statics absent at RVA and by name; 0 disagree.
- pdb_storage (name-based) on split_emul + cand.obj: 0 disagreements (production: 49).
- provider_link cand.obj: 97 surplus externals, all PASS both orders (SELECTED-PROVIDER LINK: PASS).
- surplus_identity vs cand.obj: 31/31 helper COMDATs IDENTICAL to the single January definer.
- D3D tables: census shows 57 Matching objects already emit the same 3 SDK tables as surplus
  (objects, files_windows, scenario, ...); rasterizer_xbox_profile's rejection keyed on
  _D3DSIMPLERENDERSTATEENCODE also cites other source-owner debt. Not a units blocker by precedent.
- .data entry: section_info_resolved equal (prod split/base and emul split/cand, both symbols.json);
  1564 / 7 / 6ed61c39... matches patch 05 exactly.
## R4 frozen-scorer emulation (own projects, objdiff-cli 3.3.1)
- objproj_prod unit == build/report.json unit (byte-equal JSON): 169/192, data 5188.
- complete=true (prod or cand): $L7451/$L7450/$L8061 -> 100 raw; without complete -> None.
- strict pipeline (worker's strict_emulate.py, reviewed; same order as project_x86.calculate_progress):
  config_final -> 192/192, code 53478/53478, data 6752/6752, revoked [], contradicted [].
  keep-rejection control -> completion-label-contradicted. Production control -> 189/192, 5188.
- No units entries in parked.json / symbol_ownership.json; only semantic_matches entry is _unit_is_busy
  (not renamed).
## R5 name evidence (atlas 4cc87b45 = 2002-01-14 cachebeta_exe; maps 7eacac85/64550664 = Sept-2001)
- Anchor check over units range: 123 of 124 atlas-covered existing names agree with our symbols.json.
- 15 renames are atlas exact-tier at off+0x400000 (worker said 16; unit_ping_animation is fuzzy) ;
  23 of 25 new names appear in both Sept-2001 maps or HCEX_Release; the 4 not in any map
  (overlay_action_loops, aiming_screen, can_be_entered_without_animation,
  compute_interpolation_frame_count) are HCEX_Release statics in exactly January's order with
  matching signatures (verified in hcex_release_units_compiland.txt lines 333-397).
- unit_add_initial_weapons: both Sept-2001 maps AND HCEX_Release/HCEX name it unit_add_initial_weapons.
  The pre-existing comment above it (candidate line 2485) says "HCEX identifies this private helper as
  unit_create_initial_weapons" - that is FALSE (stale) and now sits directly above the renamed fn.
- **DEFECT (pre-existing, but frozen by this admission):** 0x19ef10 is named `_unit_verify_inventory`
  (symbols.json line 7004, edited by patch 02 to add static). The body validates facing/aiming/looking
  vectors + forward/up axes (no inventory). January atlas exact tier: `_unit_vectors_are_valid`;
  Sept-2001 cachebeta_xbe MAP (ground truth) lists unit_vectors_are_valid right before
  unit_verify_vectors; also 1749betap/2002 xbe atlases. `unit_verify_inventory` appears in NO map,
  atlas, HCEX or doc. House rule 7 (authentic/descriptive) violated by a misleading name.
- 1452 other split/base objects: no symbol references any old/new changed name (incl. unit_vectors_are_valid).
- 7eacac85 (Sept-2001 cachebeta_xbe MAP): unit_vectors_are_valid 0x1ada40 -> unit_verify_vectors 0x1adad0
  (gap 0x90); January 0x19ef10 -> 0x19efa0 (gap 0x90): same extent. HCEX.pdb/HCEX_Release.pdb carry
  neither name (debug-only code), so the map + atlas are the only first-party evidence, and both say
  unit_vectors_are_valid.
- HCEX_Release.pdb re-queried with DIA2Dump: unit_animation_aiming_screen and
  unit_animation_compute_interpolation_frame_count present (worker evidence is genuine).
## R6 amendment (review2_units/amend/, patches/*.AMENDED.patch, patches/07_incremental...)
- units.c: unit_verify_inventory -> unit_vectors_are_valid (5 sites incl. header listing); the false
  comment above unit_add_initial_weapons rewritten in place (2 lines -> 2 lines; line count 11700 kept,
  so assert __LINE__ literals cannot move). symbols.json line 7004 renamed (static kept).
- csplit(config_amend): only units.obj differs from control. gate_emul amend.obj: 189/189 EXACT,
  storage 142/47 equal. object_audit PASS (321 symbols, 0 differ). amend.obj vs cand.obj: 0 non-debug
  section diffs, only the one symbol name differs. storage_check 144/50/0; pdb_storage 0;
  provider_link PASS (97); fake_match_scan 0 leads.
- objdiff 3.3.1 + strict pipeline on amend: 192/192, 53478/53478, 6752/6752, revoked [], contradicted [].
- git apply --check (and --whitespace=error-all): AMENDED 01+02 with worker 03/05/06 apply together on
  931ed8dc and reproduce amend files byte-for-byte (CRLF kept). 07 applies on top of the worker set.
## VERDICT
approve=false AS SUBMITTED (misleading pre-existing static name frozen by the admission, plus a false
comment next to a renamed fn); every byte/storage/scorer/provider claim independently reproduced.
With the AMENDED 01/02 patches (or worker set + 07) the object is admissible.
