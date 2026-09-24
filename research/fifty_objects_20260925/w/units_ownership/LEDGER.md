# units_ownership LEDGER (wave 2, base 931ed8dc)

Task: whole-object admission of source/units/units (189/189 strict exact).

## F1 baseline audit (production build/base, 931ed8dc)
- object_audit: every January-owned SECTION ok; FAIL(47) = 47 SYMBOL storage rows, all
  "split 2 / ours 3" (split says external only because symbols.json lacks "static": true).
  audit_base.txt.
- addr_storage.py (NEW, address-keyed, RVA == file_offset, anchored by
  _unit_persistent_control 0x197a70 and _magic_base_animation_seat_index 0x315c08):
  all 144 external split symbols have a cachebeta public OF THE SAME NAME at their address
  (1 static row already static). All 47 audit rows + _code_001a0cf0 + _code_001a1f50 have
  NO public at their address -> January file-static. $L7450/$L7451/$L8061 are local labels.
- _code_001a0cf0 and _code_001a1f50: ours EXTERNAL (source non-static), January static.
- strict report: 189/192 functions; the 3 misses are the csplit-exposed jump-table labels
  $L7451/$L7450 (owner _unit_animation_busy) and $L8061 (owner _unit_update): need
  owner_function semantic_matches entries (precedent: objects $L11841 etc).
- data 5188/6752: only .data (1564 B, owner _magic_base_animation_seat_index, 7 relocs) is
  uncredited (98.98%); coff_compare equal (sha 6ed61c39...). Split string targets are UNDEF
  (asleep/alert/crouch/flee/flaming selected elsewhere); '$'-literal objdiff defect class.

## F2 emulation control
- csplit control run with an unchanged copy of config/ reproduces build/split byte-for-byte
  (units.obj identical; whole split tree compared). Method is sound.

## F3 authentic names (evidence, no invention)
- HCEX_Release.pdb, compiland ..\build\x360\RELEASE_CACHE\halo\units.obj (DIA2Dump -compiland,
  saved hcex_release_units_compiland.txt) lists every units function incl. the small static
  animation predicates that HCEX.pdb (SHIP, -Ob2) inlined. Its order between unit_animation_busy
  and unit_animation_start_action is EXACTLY January's 8 helpers:
  overlay_action_loops(0x20) state_loops(0x64) weapon_ik(0x64) vehicle_ik(0x68)
  aiming_screen(0x30) state_get_aiming_screen_index(0x64) state_can_be_entered_without_animation(0x64)
  compute_interpolation_frame_count(0x5c)  <->  Jan 0x198050 0x198070 0x1980d0 0x198130 0x198170
  0x198190 0x1981f0 0x198230; signatures (animation*)x5, (short), (short), (short,short) match ours;
  semantic state lists match (state_loops 15 one-shot states identical; vehicle_ik 8; can_be_entered 6).
- Sept-2001 cache/cachebeta MAP ground truth (atlas 6455066.. / 7eacac..) has
  busy, state_loops, weapon_ik, vehicle_ik, state_get_aiming_screen_index in the same order.
- January atlas (4cc87b45.., exact tier) names 16 more by address; unit_ping_animation (0x1a0cf0)
  is fuzzy tier but HCEX_Release params 9/9 identical (unit_index, died, feign_death, died_flying,
  ignore_hard_pings, force_hard_pings, angle, damage_part, alignment_vector).
- unit_first_free_weapon_index / unit_set_animation / unit_get_current_weapon_label: HCEX_Release
  static with identical signatures and order (drop_current_weapon -> get_current_weapon_label).
- Full rename map: renames.txt (25). No collisions anywhere in source/ or xbox/.
- cachebeta publics contain NONE of the 25 names (all file-static in January).

## F4 emulated split (scratch/w/units_ownership/config/symbols.json, 49 in-place line edits)
- 25 renames (+ "static": true) and 24 "static": true additions (21 fns + 3 data:
  _unit_update_section, ?last_time@?HC@??unit_update@@9@9, _unit_globals).
- csplit -> scratch/w/units_ownership/split: ONLY source/units/units.obj differs from the control
  split (832 other objects byte-identical) - no blast radius.

## P1 hypothesis
- Renaming is text-only (declared-name COUNT unchanged, so the name-count law predicts inert);
  `static` on unit_ping_animation/unit_add_initial_weapons is January truth (no cachebeta public at
  0x1a0cf0/0x1a1f50). Risk: a static may change the private ABI or C2 definition-order deferral;
  would show as residuals in the callee or callers (unit_place, unit_new, unit_damage_aftermath).

## P1 result (units_cand_static.c -> cand_static.obj)
- vs EMULATED split: gate_split 189/189 EXACT; object_audit_split: 321 January symbols, 0 differ,
  every January-owned section ok -> OBJECT AUDIT: PASS. Address storage check: all 144 externals
  = cachebeta publics, all 50 statics absent from publics; only $L7450/$L7451/$L8061 (local labels).
- vs production split: 142/22/25 as expected (names only).
- `static` on unit_ping_animation / unit_add_initial_weapons is byte-inert (no private-ABI change).

## P2 final candidate (candidate_units.c = P1 + stale header-comment listing renamed; cand.obj)
- cand.obj sections byte/reloc-identical to cand_static.obj (447/447) - comment-only delta.
- vs emulated split: 189/189 EXACT; object_audit_split PASS (321 symbols, 0 differ).
- provider_link (cand.obj): 97 surplus externals (31 helper COMDATs, 3 D3D SDK tables, literals,
  floats) all PASS both orders; surplus_identity: 31/31 IDENTICAL to January's selected copies.
- _point_from_line3d: January units.obj holds it UNDEF (storage 2, section 0) with 2 relocations
  from _unit_cause_continuous_melee_damage (same 2 in ours); our 48-B COMDAT section_infos_equal to
  the action_charge split copy (sha 9b763841..., flags 60501020) -> owner ruling 20260921 #1 names
  units explicitly: rejection entry is retired.
- .data (_magic_base_animation_seat_index): section_info_resolved equal on BOTH production and
  emulated splits; 7 relocs resolve to the same image addresses; measurements
  1564 / 7 / 6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7.

## P3 frozen-scorer emulation (objdiff-cli 3.3.1 on single-unit projects, mk_objproj.py)
- control objproj_prod reproduces build/report.json for units exactly (169/192 raw, 45285, 5188).
- objproj_cand (emulated split + cand.obj, complete=true): raw 172/192; the three $L labels score
  100 raw once the unit is complete (objproj_prod_c control shows the same on production objects;
  objproj_cand_nc without complete -> None). So NO $L semantic_matches entries are required.
- strict_emulate.py (repo pipeline: audit_semantic_matches -> rejections -> matches -> accepted
  ledger -> data matches -> revoke -> classify) with config_min (only the data entry + retired
  rejection): 192/192 functions, 53478/53478 code, 6752/6752 data, revoked [], contradicted [].
- control config_keeprej (rejection kept): completion-label-contradicted -> retirement required.

## Classification of the 47 object_audit diffs (all: split 2 / ours 3, January truth = static)
- 21 already-named file-static functions: _base_seat_label_get _base_weapon_label_get
  _seat_label_to_base_seat_index _unit_animation_busy _unit_animation_set_state
  _unit_can_play_animation_impulse _unit_cause_continuous_melee_damage _unit_drop_item
  _unit_get_seat_label _unit_get_weapon _unit_integrated_night_vision_is_active
  _unit_ready_desired_weapon _unit_refresh_illumination _unit_running_blind _unit_seat_update
  _unit_set_or_test_seat_and_weapon_label _unit_throw_grenade_move_to_hand
  _unit_throw_grenade_release _unit_verify_inventory _unit_verify_vectors _unit_weapon_next_index
  -> symbols.json "static": true (source already static; HCEX_Release also shows most as static).
- 23 static _code_ placeholders -> authentic rename + "static": true (symbols.json + source).
- 3 data: _unit_update_section, ?last_time@?HC@??unit_update@@9@9, _unit_globals ->
  "static": true (HCEX lists unit_globals as File Static).
- Plus the 2 pdb_storage-only rows _code_001a0cf0/_code_001a1f50 (ours external): source `static`
  + rename (unit_ping_animation / unit_add_initial_weapons) + "static": true. Byte-inert.
- No other object (1,452 split/base objects scanned) references any of the 49 symbols.

## PRODUCTION CHANGES (patches/*.patch, all git apply --check clean, verified byte-equal together)
1 source/units/units.c: 25 renames (+2 static) + stale header-comment listing -> candidate_units.c
2 config/symbols.json: 49 in-place line edits -> config/symbols.json (full copy)
3 config/config.json: units.c NonMatching -> Matching
5 config/semantic_data_matches.json: single-section entry _magic_base_animation_seat_index
  (1564 / 7 / 6ed61c39...)
6 config/object_admission_rejections.json: retire units _point_from_line3d entry
optional/04: $L owner_function entries - NOT needed (objdiff scores them 100 once complete).

## Result
OBJECT_COMPLETE_CANDIDATE. Expected integrated delta (frozen 3.3.1 emulation): units strict
189->192 functions (+48 B, the $L labels), data 5188->6752 (+1,564), object complete (+1).
Integrator: full ninja regenerates build/split from the edited symbols.json (only units.obj
changes); rename-stable diff will show 25 name changes at identical addresses, 0 byte changes.
Non-blocking follow-ups: HCEX names the plan struct `euler_axis_plan` (ours
unit_acceleration_plan); HCEX param names differ in order for
unit_animation_compute_interpolation_frame_count / unit_melee_sound (later build; January
positional semantics are fixed by exact bytes) - not changed.
