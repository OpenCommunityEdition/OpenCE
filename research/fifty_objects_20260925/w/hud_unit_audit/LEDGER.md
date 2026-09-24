# hud_unit whole-object admission audit (wave 3) - LEDGER

Slug: scratch/w/hud_unit_audit/. Base tree 954eebd2. Unit source/interface/hud_unit.

## S0 baseline
- gate.py source/interface/hud_unit --all (production source): == exact 22 residual 0 unwritten 0.

## F1 names/storage evidence
- cachebeta publics (scratch/tools/cachebeta_publics.txt): _unit_hud_shield_meter_mapper_init public at 0xC65B0; NO public at 0xC6590/0xC65A0, none for _get_hud_state/_hud_update_unit_local_player/_initialize_hud_state/_unit_hud_globals; every other hud_unit external fn IS public.
- Sept-2001 maps (atlas 7eacac85 cachebeta xbe, 6455066 cache.exe): sequence _unit_hud_outline_mapper_tick(0x10) _unit_hud_shield_meter_mapper_tick(0x10) _unit_hud_shield_meter_mapper_init(0x10) _initialize_hud_state(0x40) _get_hud_state(0x70) _hud_initialize_unit_interface(0x40) _new_map(0xf0)...: size+order bijection with January 0xc6590..; cache.exe decorations @..@0 = zero params.
- January atlas 4cc87b45 (exact tier) anchors: _get_hud_state 0x4c6600, _hud_update_unit_local_player 0x4c6b80 (= Jan 0xc6600/0xc6b80) -> offset rule holds for hud_unit.
- HCEX_Release.pdb -sym: 'static void unit_hud_outline_mapper_tick()' @0x16D65D0, 'static void unit_hud_shield_meter_mapper_tick()' @0x16D65D8, unit_hud_shield_meter_mapper_init (external) @0x16D65E0 - same order; 'static struct unit_hud_state * get_hud_state(short)'; 'static void hud_update_unit_local_player(short)'.
- HCEX.pdb (SHIP) has none of these 5 (inlined/removed).
- Image scan (cachebeta.exe as data): NO absolute VA (0x4c6590/0x4c65a0/0x4c65b0) and NO E8/E9 rel32 references to any of the three mapper fns anywhere in .text. No split object references them by name.
- /Od build: hud_unit functions at 0x632500 (get_hud_state) .. 0x633240; no mapper functions present (5-byte empties at 0x6325a0/0x6325b0 are dispose_from_old_map/dispose, called from hud.c-like 0x621300/0x621330).


## P1 lab A (labA_static_nocaller.c): both ticks renamed + static, no caller
- hypothesis: VC7 discards an unreferenced static (progress_bar law) -> the static spelling alone cannot reproduce January's two emitted-but-non-public 16-B ret sections.
- result: CONFIRMED. labA.obj has NO symbol for either tick (only _unit_hud_shield_meter_mapper_init); gate 20 exact / 2 unwritten.
- control csplit: scratch/w/hud_unit_audit/split_control == build/split byte-for-byte (833/833 files).


## F2 /Od call-site attestation (halo_cache_symbols.exe, data only)
- /Od hud_unit functions are laid out ALPHABETICALLY (get_hud_state 0x632500, hud_dispose_unit_interface 0x6325a0, ..._from_old_map 0x6325b0, hud_fix_unit_data 0x6325c0, hud_initialize_unit_interface 0x632680, ..._for_new_map 0x632780, hud_play_unit_sounds 0x632870, hud_render_damage_indicators 0x632cf0, hud_render_unit_interface 0x633240, ..., hud_update_unit_local_player 0x635cd0, initialize_hud_state 0x635fc0 (callers new_map+render), scripted_hud_* x6 0x636050..0x636320, then THREE 5-byte empties 0x6363b0/0x6363c0/0x6363d0 = unit_hud_outline_mapper_tick / unit_hud_shield_meter_mapper_init / unit_hud_shield_meter_mapper_tick (alphabetical).
- callers: 0x6363c0 (shield_meter_mapper_init) <- 0x6326c6 in hud_initialize_unit_interface, right after the unit_hud_globals assert. 0x6363d0 (shield_meter_mapper_tick) <- 0x63329d and 0x6363b0 (outline_mapper_tick) <- 0x6332a2, both in hud_render_unit_interface IMMEDIATELY after the player->local_player_index==render.local_player_index assert and before the if(local_player_index==... && unit_index!=NONE). Order: shield_meter tick first, then outline tick.
- consistency: alphabetical ordering predicts init is the middle one (0x6363c0) and it is exactly the one called from the initializer.
- assert anchors: January 0x106/0x107 (get_hud_state), 0x110 (initialize), 0x209 (render); /Od 0x10b/0x10c, 0x11e, 0x228 -> later revision with drift (+5, +0xe, +0x1f): /Od is location/topology evidence only.


## P2 lab C (labC_static_render_calls.c): static ticks + /Od-attested calls in render after the assert (shield tick, then outline tick)
- hypothesis: an empty static called once is inlined to nothing at the site, yet VC7 still emits the out-of-line COMDAT (progress_bar precedent) -> reproduces January's emitted non-public statics with render unchanged.
- result: CONFIRMED. labC.obj: _unit_hud_outline_mapper_tick sec6 storage3 16B, _unit_hud_shield_meter_mapper_tick sec8 storage3 16B, mapper_init sec10 external; gate vs PRODUCTION split 20 exact + 2 'unwritten' (names only), _hud_render_unit_interface EXACT 3504 (calls byte-inert at the site).
## P3 lab D (= labC + /Od-attested unit_hud_shield_meter_mapper_init() call after the initializer assert)
- result: identical symbol set; gate 20 exact + 2 names-only; initializer EXACT 64 (inert).


## P4 final candidate cand.c (labC + stale header-listing names + private/public section markers)
- gate vs production split: 20 exact + 2 names-only 'unwritten' (expected: production split still says _code_000c6590/_code_000c65a0).
- emulated split (config/symbols.json 4 in-place line edits, csplit -> scratch/w/hud_unit_audit/split; only source/interface/hud_unit.obj differs from the control split, 832/833 identical):
  object_audit_split (object_audit.py with SPLIT_ROOT) -> every January-owned section ok (30 incl. 25 .text), 30 January symbols, 0 differ -> OBJECT AUDIT: PASS. (scratch/w/hud_unit_audit/audit_cand_emulated.txt)

- cmp_objs: all 41 production sections byte+reloc identical in cand.obj (2 renamed owners); _fast_ftol cand copy section_infos_equal to January actor_combat selected copy (flags 0x60501020 both).
- surplus_identity (production): _fast_ftol IDENTICAL vs actor_combat; provider_link cand.obj: 11 surplus externals PASS both orders (SELECTED-PROVIDER LINK: PASS).


## F3 admission scan (law g) on cand.c
- fake_match_scan cand.c: 0 review leads. No float/pragma/volatile/asm/register/opaque-unknown spans. File-scope 'extern' of hud.c globals (hud_scripted_globals, hud_globals) and TU-local hud tag type definitions are inherited; 45 of 375 Matching units carry file-scope extern variable declarations (units, render_objects incl.) -> not an admission blocker class.
- gotos: stack-sentinel labels (corrupt_stack_found/stack_buffer_checked, reviewed pattern shared with hud_nav_points/hud_weapon/motion_sensor, commit 6ad1964c) and 'goto update_finished' (fresh-hit early exit, 6ad1964c) - the latter is NOT /Od-attested: /Od 0x635cd0 has per-arm 'last_shield_hit_time = game_time_get()' stores (0x635e30/0x635e71/0x635ec4) and the fade=0 arm falls to the cinematic tail with no goto. Held-item class 'invented goto' -> probe.
## P5 lab E (labE_od_arms.c = cand + /Od per-arm shape, goto removed)
- hypothesis: VC7 cross-jumps the three identical 'call game_time_get; mov [hud_state+0xc],eax' arm tails into January's single common store block, so the /Od shape reproduces January without the goto.
- result: CONFIRMED: _hud_update_unit_local_player EXACT 544; whole object cmp_objs vs cand.obj below.


## F4 stack-sentinel provenance (admission-risk scan)
- January hud_draw.obj defines PUBLIC _get_return_eip (0xC08E0, 16 B) and _check_stack_buffer (0xC08F0, 48 B, 0 relocations to it anywhere in any split object incl. hud_draw itself). Sept-2001 cachebeta map (atlas 7eacac85) tags _get_return_eip 'i hud_draw.obj' (INLINE COMDAT copy selected from hud_draw) with an unlisted 48-B gap right after it (0xcff20 -> next 0xcff60) = check_stack_buffer's size.
- /Od build: no stack sentinel anywhere (strings absent) -> no /Od attestation either way. HCEX/HCEX_Release: no check_stack_buffer/get_return_eip.
## P6 lab F (labF_inline_check.c = labE + LAB-LOCAL __inline check_stack_buffer copy of hud_draw.c's body; both sentinel sites -> 'short corrupt_index = check_stack_buffer(stack_buffer);', no gotos left)
- hypothesis: hud_unit's hand-written goto loop is byte-equivalent to an inlined call of January's check_stack_buffer helper (ESI=-1 on loop exit / mov esi,eax on hit == inlined return-value merge).
- result: CONFIRMED byte-equivalence: all 41 labE sections identical (render 3504 + update_local 544 EXACT) + one extra 48-B ANY COMDAT _check_stack_buffer.

## F5 which _get_hud_state is which
- symbols.json line 3429 (file_offset 812544 = 0xC6600) = hud_unit's get_hud_state: January atlas 4cc87b45 exact tier 0x4c6600 lib hud_unit.obj; Sept maps 0xd5c20 hud_unit.obj; inside hud_unit range 0xC6590..0xC7E70.
- symbols.json line 3452 (819008 = 0xC7F40) = hud_weapon's OWN file-static get_hud_state: atlas 0x4c7f40 lib hud_weapon.obj; hud_weapon.c:530 static; gate hud_weapon _get_hud_state EXACT 112; already 'static': true. Both are January file-statics (no cachebeta public at either address). Only line 3429 needs the flag.
## P7 final candidate cand_final.c (= cand.c + labE /Od per-arm shape, no 'goto update_finished')
- gate vs emulated split (gate_split.py): == exact 22 residual 0 unwritten 0; storages 3/3 for the 5 statics.
- object_audit_split: 30 January symbols 0 differ, every section ok -> PASS (scratch/w/hud_unit_audit/audit_cand_final_emulated.txt).
- addr_storage (cachebeta publics AT ADDRESS, SYMS=slug symbols.json): all 24 non-literal symbols agree (externals have a same-name public at their address; statics none) -> scratch/w/hud_unit_audit/addr_storage_cand_final.txt.

## P8 lab G (cand_final + /Od operand order 'unit->object.shield_vitality > hud_state->last_shield_vitality' in the else-if)
- hypothesis: /Od loads [unit+0xa0] first -> maybe January's operand order too.
- result: REFUTED: _hud_update_unit_local_player residual (x87 compare order is January-byte-determined). Keep January's 'last_shield_vitality < unit->...shield_vitality'. /Od is topology evidence only (per-arm stores, no goto); operand order comes from January bytes.

## F6 _fast_ftol rejection: reopen criterion MET (branch 2: ordinary base/base link with compatible authenticated canonical owner)
- build/base actor_combat.obj _fast_ftol = genuine cseries.h '__inline long fast_ftol' (cseries.h:309; no hand copy), COMDAT selection 2 (ANY); nodup_census: 25 base definers, 0 NODUP.
- ftol_census: all 25 base copies + cand.obj + cand_final.obj section_infos_equal to January's selected build/split actor_combat copy (flags 0x60501020).
- link_probe.py (xbox/bin/vc7 Link.Exe, /DLL /NOENTRY /NODEFAULTLIB /FORCE:UNRESOLVED /INCLUDE:_fast_ftol /EXPORT:fast_ftol /MAP): cand_final.obj + base actor_combat.obj both orders rc=0, no LNK2005/LNK1169/LNK4006, map shows exactly ONE _fast_ftol (first input selected). Same for production base hud_unit.obj.
- CONTROL: same probe against build/split actor_combat.obj (NODUP representation) -> LNK2005 + LNK4006 + LNK1169 in both orders = the 2026-08-31 failure that created the veto. Probe discriminates.
- provider_link.py cand_final.obj: 11/11 surplus PASS both orders.
- Precedent: player_rumble_obj_fast_ftol_owner_reaudit_20260919.md retired the identical veto on the identical evidence; 8 Matching objects already carry the identical ANY _fast_ftol surplus (bitmap_utilities, bitmaps, player_rumble, hud, model_animations, rasterizer_xbox_transparent_geometry, rasterizer_xbox_widgets, structure_detail_objects).
- The veto's other clause ('renderer must independently become strict') is also met: _hud_render_unit_interface EXACT 3504/102.
- strict_emulate (repo pipeline on single-unit objdiff 3.3.1 reports; control objproj_prod == build/report.json 5491/6176, 152/152, 21/22):
  cand_final + emulated split, complete=True, rejection RETIRED: strict 22/22 fns, 6176/6176 code, 152/152 data; candidates [] contradicted [] rejected [] revoked [].
  same with rejection KEPT (config_keeprej): 'completion-label-contradicted' -> retirement is required for admission.

## F7 residual reviewer-judgment item (not a failing check): stack-sentinel gotos
- render + update_local_player keep the inherited, previously reviewed (6ad1964c, 20260904 renderer review) sentinel spelling with 'goto corrupt_stack_found / stack_buffer_checked'. Lab F proved it byte-equivalent to 'short corrupt_index = check_stack_buffer(stack_buffer);' with hud_draw.c's helper inlined (emitted copy identical to January hud_draw's selected 48-B _check_stack_buffer). In the current tree check_stack_buffer is a plain hud_draw.c function (no header), so this is not a hand-expanded HEADER helper under the current rules; but Sept-2001 map tags get_return_eip as 'i hud_draw.obj' (inline COMDAT) with check_stack_buffer's 48-B gap beside it, so January may have had both as header inlines. Converting would be a hud_draw.h header packet (hud_draw + hud_unit + hud_nav_points + hud_weapon + motion_sensor + every hud_draw.h consumer, declaration-count sweep) - owner/reviewer decision, NOT proposed here.
- Not included: /Od-attested unit_hud_shield_meter_mapper_init() call in hud_initialize_unit_interface (lab D, byte-inert, non-load-bearing; /Od assert drift +0xe, later build added code after it).

## RESULT
OBJECT_COMPLETE_CANDIDATE. patches/01 (or 01alt minimal) + 02 + 03 + 04, all git apply --check clean individually and together; applied results byte-identical to the slug files.
