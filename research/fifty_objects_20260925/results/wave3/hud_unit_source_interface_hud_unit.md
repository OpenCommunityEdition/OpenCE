# source/interface/hud_unit

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\config\\object_admission_rejections.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\patches\\01_hud_unit_c_names_static_calls_and_od_arms.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\patches\\01alt_hud_unit_c_minimal_names_static_calls.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\patches\\02_symbols_json_hud_unit_names_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\patches\\03_config_json_hud_unit_matching.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_unit_audit\\patches\\04_admission_rejections_retire_hud_unit_fast_ftol.patch"
]

## production_changes
All patches are under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_unit_audit\patches\. Each one passes `git apply --check` against 954eebd2, alone and all together. Applying them to copies of the tracked files gives results byte-identical to the slug files. Apply 01 (or 01alt) + 02 + 03 + 04.

01_hud_unit_c_names_static_calls_and_od_arms.patch (recommended; the result equals cand_final.c) changes source/interface/hud_unit.c:
(a) `void code_000c6590(void)` becomes `static void unit_hud_outline_mapper_tick(void)` and `void code_000c65a0(void)` becomes `static void unit_hud_shield_meter_mapper_tick(void)`. Both move under a `private code` marker, and a `public code` marker is added before unit_hud_shield_meter_mapper_init.
(b) hud_render_unit_interface: right after the 0x209 player->local_player_index==render.local_player_index assert and before the `if`, it adds `unit_hud_shield_meter_mapper_tick();` then `unit_hud_outline_mapper_tick();` (the /Od order).
(c) hud_update_unit_local_player: `goto update_finished` and its label are removed and replaced by the /Od per-arm shape. The <15 case becomes `fade_time = 0.0f;` with an `else { vitality; fade +=; last_shield_hit_time = game_time_get(); }` arm. Then `else if (last_shield_vitality < unit->object.shield_vitality) { vitality; fade=-1; last_shield_hit_time = game_time_get(); }` and `else { vitality; if (fade>0) fade +=; last_shield_hit_time = game_time_get(); }`. January's operand order `last < unit` is kept.
(d) The stale header symbol listing is updated: _code_000c6590/_code_000c65a0/_code_000c65c0/_data_002e4c68 become the real names.

01alt_hud_unit_c_minimal_names_static_calls.patch (= cand.c) is (a)+(b)+(d) only, without (c). Use it if the integrator wants no change to an already-exact function body.

02_symbols_json_hud_unit_names_static.patch edits 4 lines in place in config/symbols.json. The full edited copy is scratch/w/hud_unit_audit/config/symbols.json (CRLF preserved).
- L3425 812432: `_code_000c6590` becomes `_unit_hud_outline_mapper_tick`, "static": true
- L3426 812448: `_code_000c65a0` becomes `_unit_hud_shield_meter_mapper_tick`, "static": true
- L3429 812544: `_get_hud_state` gets "static": true
- L3442 813952: `_hud_update_unit_local_player` gets "static": true
L3452 (819008) is hud_weapon's own static get_hud_state and stays unchanged.

03_config_json_hud_unit_matching.patch: config.json line 644, hud_unit.c index 269, NonMatching becomes Matching.

04_admission_rejections_retire_hud_unit_fast_ftol.patch removes the source/interface/hud_unit _fast_ftol entry.

No header changes, so no consumer-TU sweep is needed beyond the standard full ninja. The csplit-only regen changes only build/split/source/interface/hud_unit.obj (832/833 split files byte-identical to control). The rename-stable diff will show 2 name changes at 0xC6590/0xC65A0 and 0 byte changes. Keep the existing semantic_matches entry for _hud_render_damage_indicators: objdiff 3.3.1 still reports 95.85 for it even with complete=true.

## evidence
Baseline: 22/22 EXACT, object_audit FAIL(2).

Storage and names. cachebeta publics exist for mapper_init (0xC65B0) and every other hud_unit external. There is no public at 0xC6590 or 0xC65A0 and none for get_hud_state, hud_update_unit_local_player, initialize_hud_state or unit_hud_globals, so all of these are January file-statics. HCEX_Release.pdb (-ps 0x16D65C0) lists, in January's order, `static void unit_hud_outline_mapper_tick()`, `static void unit_hud_shield_meter_mapper_tick()`, unit_hud_shield_meter_mapper_init (external), static initialize_hud_state, `static get_hud_state(short)`, and `static void hud_update_unit_local_player(short)`. Both Sept-2001 maps (atlas 7eacac85 cachebeta xbe, 6455066 cache.exe, @..@0 = no parameters) give the same sequence with January's sizes (0x10, 0x10, 0x10, 0x40, 0x70, 0x40, 0xf0). The January atlas exact tier anchors the offset: 0x4c6600 get_hud_state lib hud_unit.obj, and 0x4c7f40 get_hud_state lib hud_weapon.obj, so symbols.json L3429 is hud_unit's and L3452 is hud_weapon's (already static).

Why the calls are needed. Lab A (static, no caller): VC7 discards both ticks, so January must have referenced them. The image has no absolute VA and no E8/E9 reference to any of the 3 mapper functions. In the /Od build, hud_unit functions are laid out alphabetically, and 0x6363b0/c0/d0 are outline_tick, shield_meter_init and shield_meter_tick. init is called from hud_initialize_unit_interface. shield_meter_tick (0x63329d) and then outline_tick (0x6332a2) are called in hud_render_unit_interface right after the local_player_index assert and before the `if`. Lab C: with those calls, both statics are emitted (storage 3, 16 B) and render stays EXACT 3504.

Lab E (/Od per-arm stores instead of the goto): all 41 sections identical. Lab G (/Od operand order `unit > last`) breaks update_local_player, so January's `last < unit` is kept.

Final candidate against the emulated split (csplit -i cachebeta.exe -p slug/config; the control split equals build/split for 833/833 files):
- gate_split: exact 22, residual 0, unwritten 0.
- object_audit (SPLIT_ROOT): every January-owned section ok; 30 January symbols, 0 differ; OBJECT AUDIT PASS.
- Address-keyed cachebeta storage check: all 24 symbols agree.
- cmp_objs: all 41 production sections byte- and relocation-identical in cand_final.obj.
- Surplus: 1 _fast_ftol plus 10 pooled literals. provider_link PASS in both orders. _fast_ftol is section_infos_equal to January's selected actor_combat copy.

_fast_ftol veto retirement (the reopen criterion's second branch is met):
- base actor_combat now emits the genuine cseries.h `__inline fast_ftol` as a SELECT_ANY COMDAT.
- nodup_census: 25 base definers, 0 NODUP. All 25 base copies, plus cand and cand_final, are identical to January's selected copy.
- Ordinary VC7 Link.Exe probe (/DLL /NOENTRY /FORCE:UNRESOLVED /INCLUDE /EXPORT /MAP): cand_final or base hud_unit linked with base actor_combat in both orders gives rc=0, no LNK2005/LNK1169/LNK4006, and exactly one _fast_ftol in the map.
- Control against the split's NODUP actor_combat reproduces LNK2005 + LNK1169, the 2026-08-31 failure, so the probe discriminates.
- The renderer-strict clause is also met. Precedent: player_rumble_obj_fast_ftol_owner_reaudit_20260919.md; 8 Matching objects already carry the identical surplus.

Frozen-scorer emulation: objdiff 3.3.1 single-unit projects; the control reproduces build/report.json (5491/6176, 152/152, 21/22). The repo strict pipeline with the rejection retired and complete=true gives 22/22 functions, 6176/6176 code, 152/152 data, and candidates/contradicted/rejected/revoked all empty. With the rejection kept it gives completion-label-contradicted. fake_match_scan: 0 leads.

## blockers
None of the audit checks fail. One item needs reviewer or owner judgment and is disclosed, not failing: both hud_render_unit_interface and hud_update_unit_local_player still use the inherited, previously reviewed stack-sentinel spelling with `goto corrupt_stack_found/stack_buffer_checked` (6ad1964c, 20260904 renderer review).

Lab F shows this code is byte-equivalent to `short corrupt_index = check_stack_buffer(stack_buffer);` with hud_draw.c's helper inlined. The emitted 48-B copy is identical to January's selected hud_draw _check_stack_buffer, and no split object references that function by relocation. In the current tree check_stack_buffer is a plain hud_draw.c function, not a header helper, so rule 6 and law (g) do not formally apply. The Sept-2001 map, however, tags get_return_eip as an inline COMDAT ('i hud_draw.obj') with check_stack_buffer's 48-B gap beside it. If reviewers treat the loop as a hand-expanded helper, the fix is a hud_draw.h __inline packet across hud_draw, hud_unit, hud_nav_points, hud_weapon, motion_sensor and every hud_draw.h consumer, with a declaration-count sweep. That packet is not proposed here.

Other inherited items (non-blocking by precedent): file-scope `extern` declarations of hud.c's hud_scripted_globals and hud_globals (45 of 375 Matching units have such externs, units and render_objects included), and TU-local hud tag type definitions (complete layouts, no opaque spans).

## reopen_criteria
Nothing to reopen if admitted. If a reviewer rejects the stack-sentinel spelling, reopen via a hud_draw.h `__inline short check_stack_buffer(long *buffer)` owner packet (lab F already shows the hud_unit side is byte-inert, and its copy is identical to January's). Such a packet needs January or first-party evidence that the helper was a header inline, plus a full declaration-count sweep. If patch 01 (c) is rejected, use 01alt: identical bytes, no goto change. Optional and not included: the /Od-attested `unit_hud_shield_meter_mapper_init();` after the initializer assert (lab D, byte-inert). Out of scope: the hud_draw _fast_ftol rejection has the same retirement evidence, but hud_draw is not otherwise complete.

## task notes
Tools written in the slug:
- object_audit_split.py: object_audit with SPLIT_ROOT.
- gate_split.py and addr_storage.py: from units_ownership.
- strict_emulate.py and mk_objproj.py: adapted to hud_unit.
- link_probe.py: VC7 Link.Exe pair link in both orders with /MAP.
- ftol_census.py, cmp_objs.py, syms.py, odfuncs.py: /Od function-start lister (alphabetical layout evidence).
- ppc_refs2.py: HCEX_release.exe PPC branch and reference scan; file offset == RVA.

Evidence outputs:
- audit_cand_final_emulated.txt
- gate_cand_final_emusplit.txt
- addr_storage_cand_final.txt
- provider_link_cand_final.txt
- link_probe_*.txt
- ftol_census.txt
- strict_emulate_cand_final.txt
- od_render_633240.txt
- od_update_local_635cd0.txt
- hcex_release_ps.txt

The emulated split is at scratch/w/hud_unit_audit/split and the control split at split_control. No tracked file was edited. No ninja, configure or git-mutating command was run. The only executables run were the project's own csplit, objdiff-cli 3.3.1 and VC7 Link.Exe. cachebeta.exe, halo_cache_symbols.exe and HCEX_release.exe were read as data only.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_unit_audit\LEDGER.md
