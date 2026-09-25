# critic_r2 LEDGER (read-only completeness critic, round 2)

## C1 battery (current tree 5d3ca708, build/base from R2-2 ninja 11:07)
- Ran object_audit/pdb_storage/surplus_identity/provider_link on all 83 incomplete source/ units -> battery/*.txt, table.txt.
- 83 incomplete (excl. linker_common) = 468-384-1. Consistent with ledger.

## C2 bitmap_drawing 16-byte prefix: 2001 builds (NEW check, negative)
- Sept-2001 cachebeta.xbe (.data va 0x2c6280 = map sec 8 base): translation_table 0x2eae88; 16 bytes before = ff*10 00*6 (same as January).
- Aug-2001 1749betaP.xbe (.data va 0x2a0080): translation_table 0x2c4c18; same 16-byte prefix.
- 4-byte absolute-reference scan over every XBE section: 0 refs to the prefix in BOTH builds; exactly 1 .text ref to translation_table each.
- => prefix was already dead in Aug-2001; no first-party user/name recoverable from these builds. Route stays OWNER/hold.

## C3 Sept-2001 raw linker maps vs the atlas (NEW: atlas is lossy)
- Raw 2001-09-25 cachebeta.map has 11,172 function records; atlas 7eacac85 map tier has 8,568 (one per address).
  Section-1 coverage: publics 4,655/5,629 in atlas, statics 1,975/3,115 in atlas. ~2,100 .text names exist only in the raw map
  (e.g. _bink_playback_update__internal 0001:001b3670, _profile_internal_step 0001:0007d260, _get_next_available_set_array_index 0001:0006f980).
- lab/map_align.py aligns raw-map functions to January split functions per incomplete unit by name anchors + equal size.
  Output lab/map_align.txt: 22 incomplete units carry invented/descriptive/placeholder names where the Sept-2001 map
  has a first-party name with identical size between the same anchors.

## C4 bink_playback: _code_001b5850 = bink_decompress_audio_frame (first-party) -- removes the owner NAMING item
- Sept-2001 cachebeta.map Static symbols: 0001:001b3090 _bink_decompress_audio_frame (0x10) between _bink_free@4 (001b2fd0) and _bink_draw_frame (001b30a0).
  January: 0x1b5850 (0x10) between _bink_free@4 (0x1b5790) and _bink_draw_frame (0x1b5860). Same spacing. Atlas 7eacac85 also has it (0x1c4090) - R2 triage/review missed it.
- Retail 2001-09-25 cache.map: @bink_decompress_audio_frame@0 at 0x54d870, bytes c3 90.., 0 E8 calls, 0 absolute refs. Sept xbe: c3, 0 refs. => empty static, never called out of line in any build.
- LAB (claimed critic_r2): static + call placed before bink_decompress_video_frame() in bink_playback_render -> _code_001b5850 EXACT (emitted), 23/24 EXACT unchanged, provider_link PASS; static + unused -> NOT emitted (UNWRITTEN). Confirms static-COMDAT law; matches the R2-1 hardware_geometry precedent (emitted never-referenced static + first-party map name; call position byte-inert, disclosed).
- Remaining bink blockers after this: only the q3 ruling (_bink_query_analog_controller_buttons) + symbols.json row 0x1b5850 -> _bink_decompress_audio_frame "static": true.
- Also hygiene: _bink_playback_idle (0x1b5e30, 0xa0) = Sept _bink_playback_update__internal (001b3670, 0xa0).

## C5 hs_compile storage vs cachebeta PDB (NEW; no lane packeted it)
- PDB publics: _whitespace_characters [0x269F98], _eol_characters [0x269F9C], _hs_type_primitive_parsers [0x269FA0] are PUBLIC (addresses == symbols.json file_offset 2531224/2531228/2531232), but symbols.json rows 16255-16257 carry "static": true and hs_compile.c:634-636 declares them static. _hs_compile_finish is NOT public (static in January) but ours is external (hs_library_internal_compile.h:32 prototype). No other split object references any of the four (storage_check.txt ext-refs 0).
- LAB (claimed/released critic_r2): tables non-static -> 61/64 unchanged, only storage differs; + static hs_compile_finish -> still 61/64, every section section_infos_equal to baseline; only 4 storage rows change.
- Packet needed: drop static on 3 tables, static hs_compile_finish with a TU-local prototype (remove from hs_library_internal_compile.h, which hs.c also includes -> declaration-count sweep needed), symbols.json: remove static on 16255-16257, add static on _hs_compile_finish row, csplit regen.
- Does NOT complete hs_compile (3 residuals; _hs_parse_boolean held bug). Hygiene/audit-blocker only.

## C6 First-party Sept-2001 names never applied (atlas AND raw map)
- lab/map_align.py + atlas cross-check: 72 unique-size name alignments in 21 incomplete units; 57 of them are ALREADY in atlas 7eacac85 (R2 used the atlas only for texture_cache/hardware_geometry/hardware_bitmaps/weapons), 15 exist only in the raw map.
- Includes 20 house-rule-violating _code_ placeholders in ai_debug (e.g. _code_00037890 -> _ai_debug_render_path_line 0x80, _code_00039990 -> _ai_debug_render_path_node 0x350) and bink _code_001b5850 (C4).
- Residual functions whose first-party name differs: transport_endpoint_set_winsock _transport_endpoint_set_compare_entries = _poll_ep_array_compare_proc (0x30); transport_endpoint_winsock _connect_endpoint_process@4 = _connect_async_thread_proc@4 (0x130); progress_bar _draw_gravy_layer = _draw_layer_int (0x160).
- Byte-inert hygiene except where a placeholder is a completion blocker (bink).

## C7 Cross-build residual census (Sept-2001 cachebeta.xbe + Aug-2001 1749betaP.xbe, raw-map located)
- lab/xbuild_run.py -> lab/xbuild.txt (128 residuals). Relocation-masked capstone instruction streams.
- 84 residuals: Sept == January exactly (source stable since Sept 2001).
- _solo_level_select_list_update_displayed_items: Sept == OURS (the single `mov edi,[ebp+8]` placement), i.e. the Sept-2001 compile reproduces our form; corroborates R2-3's pure TU-count tie reading (body correct).
- Sept differs from January (source/context changed Sept->Jan) for e.g. _sphere_intersects_cluster_portal (Sept frame 0x408 vs Jan 0x414: the 12-byte point local + fchs point computation were ADDED after Sept), _actor_combat_plan_grenade_trajectory (js .91), hud_weapon trio, ui_widget five, virtual_keyboard two, main_update_time.

## C8 Admission/metric consistency
- collision_features (Codex-reserved) carries an ACTIVE object_admission_rejections entry (candidate-only-comdat-owner _projection_from_vector3d, "reverses the x/y dominant-axis choice") that gate_r2b2/admission.log still reports as rejected; but current surplus_identity says _projection_from_vector3d IDENTICAL to decals and provider_link PASS. Stale rejection -> Codex cross_product2d repair alone will NOT flip collision_features; the rejection must be retired (reopen criteria appear met).
- matching_audit.txt: of 384 Matching Halo objects, 74 PHONY-only (audit tool artifact), 77 audit FAIL + PDB disagreements, 71 audit PASS but PDB storage disagreements. The R2 admission battery (audit PASS + pdb 0) is stricter than what ~148 existing Matching objects satisfy.

## C9 Caveat on Sept-2001 names
- rasterizer_xbox: Sept map _rasterizer_filthy_bitmap_defaults_initialize vs January's own error literal "### ERROR rasterizer_filthy_bitmap_default_initialize failed" (rasterizer_xbox.c:3676, January .rdata) -> January text wins; Sept names can be stale. Any rename packet must check January strings/asserts first.
- progress_bar: Sept map also shows the emitted-never-called statics pattern (_draw_layer 0xe10d0, _draw_initial_layer 0xe1130, _this_is_awful 0xe0820: 0 E8 calls, 0 abs refs) = January _draw_gravy_screen_layer/_draw_gravy_base_layer/_progress_bar_alias_surface_texture. Sept _draw_layer_int == January _draw_gravy_layer (instruction-identical), so no new lever for that residual.
- bitmap_drawing prefix, periodic/king Case-A data entries (96 B / 136 B, round-1 research/fifty_objects_20260925/w/data_gap) remain unlanded; data credit only.
