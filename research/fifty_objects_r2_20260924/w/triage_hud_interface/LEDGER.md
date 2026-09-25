# Triage ledger: family hud_interface (round 2)
Base f6d00a8c. Read-only triage; no candidate source edits, no claims.
Objects: interface, hud_nav_points, progress_bar, virtual_keyboard, ui_widget_game_data_input_functions, ui_widget, draw_string, hud_draw.

## Production object_audit (f6d00a8c) for all 8
- interface FAIL(7): weapon_hud_index DIFF + 6 profile_* symbols storage 2(Jan)/3(ours).
- hud_nav_points FAIL(1): custom_render_nav_point only; symbols 0 differ.
- progress_bar FAIL(25): draw_gravy_layer 352/336 + 24 D3D wrapper symbols storage 2/3.
- virtual_keyboard FAIL(2): render_internal 1616/1584, select 1088/1088.
- ui_widget_game_data_input_functions FAIL(4): 2 fns + .bss offsets displayed_servers/build_version_wide_string swapped (0/128 vs 40/0).
- ui_widget FAIL(6): 6 fns; symbols 0 differ.
- hud_draw FAIL(2): _fast_ftol_C MISSING (section+symbol).
- draw_string FAIL(1): bitmap_draw_string 304/240.

## hud_draw
- P0 census of non-16 code sections: build/split has 2 (hud_draw _fast_ftol_C 56; ai _ai_handle_spatial_effect 1130); build/base has 1 (ai 1130).
  ai_handle_spatial_effect is inside `#pragma optimize("", off)` in ai.c -> /Od-region functions are not nop-padded. Same mechanism already found by
  hud_draw_obj_opus5_150k_w1_20260914 sec.4 (v2.c pragma-bracketed fast_ftol_C = 56/0 EXACT, meter stays exact, unit 23/23 in whole-TU gate).
  That route is the HELD "hud_draw optimizer pragma" (brief sec.2). n1/n4 ledgers: /O2 asm body = 64 + inlines into hud_draw_meter (1312 -> 1536).
  => OWNER (pragma ruling). No new first-party evidence removes the ruling need. _fast_ftol rejection text is stale (round1 provider_repair: conflict gone).

## interface
- Round1 (wave1 fn_interface + REVIEW approve=True): 18/18 strict with `static` interface_get_weapon_hud_index (HCEX static + absent from cachebeta publics),
  profile_* six made external (cachebeta PUBLIC, verified here: publics lines 3088/6329/11269/13562/14961/17325), render_debug_profile/splitscreen_render static
  + symbols.json static flags. Blocker = interface.h prototype: Variant A (remove) regresses rasterizer_frame_statistics_draw 4176 B by pure decl-count tie
  (only net 0 works, -3..+44 tested); Variant B keeps extern proto over static def (C4211 /W4, C89 UB). Not in owner queue table; not landed (prod audit FAIL(7)).
- Checked here: genuine missing interface.h protos = interface_dispose, interface_dispose_from_old_map (imported by Jan game.obj), interface_draw_fullscreen_overlays
  (render.obj) -> all `void f(void)` weight 2; interface_get_rgb_color PUBLIC but no January importer (3 params). No genuine weight-3/weight-1 declaration exists:
  -3 + 2a + 5b has no zero solution. profile_* have no January importers (no header extern evidence). => OWNER (Variant A/B pick). ATTACK only via rfs-prefix
  duplicate-declaration cleanup (declaration-count class, disclosure-heavy) - not recommended as primary.

## hud_nav_points
- dis_range at 0x520-0x5a8: only diff = January `push 0` (override_scale 0.0f) scheduled before `mov eax,[ebp-0x10]; mov ecx,[ebp-0x14]`; ours loads first. Pure scheduler order.
- Round1 fn2 result: >30 prior shapes + 19 lab diagnostics + /Od-faithful od1 all give identical 8-byte residual; only known mechanisms (reg conflict, tail-merge IL boundary)
  are absent in January (no partner path). Stop rule fired. Review4 lists extra admission blockers (consumer-local protos for objects.h/game_engine.h/units.h fns,
  opaque-span struct views hud_globals_definition/hud_nav_object_datum, float z_offset/vertical_offset). => RETIRE (function) with hygiene debt noted.

## progress_bar
- draw_gravy_layer 352/336: frame identical 0x1c, reloc multiset 22/22 identical; gap = January keeps rect corners as live values (carrier+dead spill homes),
  ours forwards rect.* memory operands. n1 ledger: 6 shapes identical (named corners, pointer alias, nested block, rotation, aggregate copy, inlined helper);
  round1 wave1: ~25 more perturbations (definition order, K&R/void* protos, casts, arrays, unions, __inline, register, dead stores...) all collapse; lab-only
  route (remove no-capture summary of generate_gravy_rect) has no source cause and still allocates reversed. => RETIRE.
- Storage debt (independent of the function): 24 D3D wrapper copies at progress_bar offsets (e.g. 855312 _D3DDevice_SetRenderState, 856416 SetRenderState@12)
  lack "static": true in symbols.json while every other object's copies carry it (symbols.json 5591/5594/5620...) and PDB publics lack them -> in-place
  static flags + csplit regen. 5 PDB-static Halo fns (draw_fullscreen_blur, set_blur_texture_coordinates, draw_gravy_screen_layer, progress_bar_screen_initialized,
  progress_bar_rendering_enabled) + atlas names draw_layer_int/do_convoluation_coords/draw_fade_layer/progress_bar_create_mask_texture; draw_gravy_screen_layer
  static drops its emission (no in-TU user). Also _code_000d16d0 rename owner-gated.

## virtual_keyboard
- select (1088/1088, T-like): round1 decoded tail-merge survivor = first predecessor in switch-END list; January's single range check + fallthrough default
  forbids every END-as-jump structure (nested switch / if-else wrapper add a compare: s3b). e1-e30 labs + corpus + /Od 0x68ef60 same order; stop rule fired.
- render_internal (1616 vs 1584, 6 independent residual classes incl. -1 web split, key-IV base, loop-2 rotation, caret reload, font-header reload,
  csplit reloc alias _item_update_section=_keyboard_rect+352 fixable in relocs.json only after exact). /Od 0x68fa00 is a redesign (no key grid).
  => RETIRE both (no new first-party evidence; reopen criteria need C2 trace).

## ui_widget_game_data_input_functions
- mgd: alndiff = one 0x11-byte return-arm placement (machine arm 0x22 after player arm) + teams join-store (fixed by round1 mgd_a: 122/122 insns). >30 shapes
  across 3 lanes (round1 LEDGER line 33/50), 73-analog corpus scan, K=0..63 oracle never exact, /Od has no mgd (assert string absent). => RETIRE.
- solo: TU name-count tie (period 64, 2-wide window at prod+16/17); body correct; needs January's genuine header set. Round1 r3 packet (January definition
  order, static callbacks/table/caches per HCEX+cachebeta, header trimmed) fixes all 165 symbols incl. .bss displayed_servers order, byte-neutral, but does
  not hit the count window. => RETIRE (object cannot complete while mgd is a tie); r3 storage packet is zero-credit hygiene.

## ui_widget (6 residuals)
- Structural lane REPORT (C:/halo-worktrees/claude-structural-20260923/scratch/lane/w/interface__ui_widget/REPORT.md) already used /Od for all six:
  * process_one_event_recursive: /Od 0x668e50 P2/P3 -> only diffs = 2 dpad_event_times sites (lea [edx+ecx*4-8] + storage:2160 vs ours folded 2128);
    exact 95 with the held `_dpad_event_times` .bss split (symbols.json insert + static array). Name not first-party: HCEX _widget_globals has none,
    /Od has no dpad code, atlas (research/halo-symbol-atlas halo_ce/*.jsonl, checked here) has no ui_widget data names -> OWNER (invented .bss split name).
    NB: P2 body was not landed in production (prod still 1984/65) - it lands only together with the split.
  * ui_check_for_pause_game: H2/H6 (/Od 0x664d00, ui_widgets_active() helper) 672 [sha], 226/226 insns, esi/edi swap only; 5 shapes; not landed. RETIRE (tie).
  * launch_widget: alndiff here = Jan `test ecx,ecx; push ebx; je; mov ebx,[ecx]; jmp; or ebx,-1` if/else vs ours `or ebx,-1` pre-init; every if/else/?:/arg
    spelling pins -1 (w1, house_clean, 100k, structural L1/L2). RETIRE.
  * display_scenario_help ebx/edi tie (/Od is 2020 rewrite); draw_bitmap_in_rect store scheduling (/Od rewrite); render_recursive R1 slot-home swap (5 shapes). RETIRE.
- PROBE (read-only re-measure): structural-lane candidate.c + the one include line canonical added since (bitmaps/bitmap_color_conversion.h)
  -> scratch/w/triage_hud_interface/ui_widget_structcand.c: gate == exact 96 residual 6 (pause 672 [sha], poe 2016 [sha]). alndiff poe: only the dpad
  site `lea eax,[edx+ecx*4-8]` + storage:2160 vs ours `lea eax,[edx+ecx*4]` + 2128 (same effective address; January's -8 stays in the index because the
  array is a SEPARATE object starting at storage+0x870). Premise of the held .bss split confirmed at f6d00a8c.

## draw_string
- bitmap_draw_string exact (cand1/cand2, 28/28) ONLY with January's NULL-bounds dereference (inverted `if (!bounds)` then bounds-> reads; /Od 0x8b2090
  has the same inverted test + RTC names adjusted_bounds/adjusted_clip). January-bug class (brief sec.8) -> OWNER. Ownership packet (4 HCEX file statics
  replacing invented _draw_string_globals; symbols.json line 23279 still `_draw_string_globals` -> NOT landed; pdb_storage 1 disagreement) is landable at zero credit.

## hud_draw (cont.)
- provider_link PASS (all surplus incl. _fast_ftol vs actor_combat, _square_root vs action_charge), surplus_identity 2/2 IDENTICAL, pdb_storage 0 disagreements.
  So the ONLY blocker is _fast_ftol_C (held pragma) + stale _fast_ftol rejection text.
- Correction to n1 ledger: "zero non-16 COMDATs in build/base (interface, cseries, ai)" is wrong - ai.obj _ai_handle_spatial_effect is 1130 in base; it is the
  only non-16 base section board-wide (1/8918) and it sits inside ai.c's `#pragma optimize("", off)` region. This double-attests the /Od-no-padding mechanism.

## Other storage debt noted (pdb_storage, January static / ours external)
- virtual_keyboard: free_space_in_text_buffer, get_current_character, _virtual_keyboard_globals, _virtual_keyboard_key_layout.
- ui_widget: _global_ui_white_red/green/blue.

## Summary of decisions
hud_draw OWNER (pragma); interface OWNER (header Variant A/B); draw_string OWNER (Jan bug); ui_widget poe OWNER (.bss split name), rest RETIRE;
hud_nav_points RETIRE; progress_bar RETIRE; virtual_keyboard RETIRE; ui_widget_game_data_input_functions RETIRE. No ATTACK route with new evidence found.
