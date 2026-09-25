# LEDGER - full-period record-count census (slug record_census) - LAB ONLY

Tree: 42125a52 (canonical f6d00a8c + R2 batches 1-2), clean at start. No claims taken (read-only census).
Task: for all 86 board units, K = 0..63 dummy file-scope declarations (`extern long census_dummy_NNN;`, 1 record each)
inserted at ONE fixed position (immediately after the last leading #include, before the TU's own constants/structs/
globals), compiled via tools/campaign/gate.py --source <copy> --all; per-function EXACT sets over the full period.
Dummies are an ORACLE only; nothing here lands. real_math.h is RESERVED (Codex); no real_math proposals.

## Step 0 - reading (done)
- WORKER_BRIEF_R2 sections 0-9; canary lane LEDGER F1-F9, MANIFEST, lab.py/window.py/netcount.py/board.py/canaries.txt/
  consumer_sweep_list.txt. gate.py read in full: writes only scratch/_gate_<pid>.{c,obj} (PID-isolated) + --out copy.
- Known windows to validate against: frame_statistics draw {0} only (F1, both top and after-globals positions);
  ui_widget_game_data_input_functions solo: "2-name window at production+16/17" (r2w1 triage).

## Step 1 - harness (inspoint.py, census.py, validate_gate.py)
- inspoint.py: anchor = last #include before the first C code line that follows the first #include (continuation
  lines of #define skipped; code before the first #include allowed - game.c `struct game_options;`, ui_widget);
  moved past any enclosing #if (none needed on the 86 units). Report: inspoint_report.txt (no MOVED, no WARN;
  vehicles/physics have mid-file #includes of datum headers AFTER the anchor - dummies precede them, fine).
- Dummy = `extern long census_dummy_NNN;` inserted at the start of the line after the anchor.
- census.py compiles with a FAITHFUL COPY of gate.py (verbatim flag parse, command line, /I<unit dir>, newline
  handling, fn_syms, section_infos_equal) but PID-isolated temporaries live in tmp/ under this slug and are deleted
  (real gate.py leaves scratch/_gate_<pid>.* outside the slug, 1,602 already there; the brief forbids deleting them).
- validate_gate.py (real gate.py --source copy --all --out vs census compile, same code): frame_statistics K=0/1,
  ui_widget_gdif K=0/16, saved_game_files K=5, collisions K=3, hs K=0: per-function status AND section_info
  (size, normalized sha, relocs) identical for all 600 rows. TOTAL MISMATCHES 0.
- Position check vs canary F3: frame_statistics K=1 after-include draw sha 7aafc0 == canary's +1 fingerprint at EVERY
  pre-globals position (top of TU etc.) -> after-include position is in draw's counted region.

## Step 2 - validation windows (results/validate.jsonl, K=0..63, after_include)
- rasterizer_frame_statistics::_rasterizer_frame_statistics_draw EXACT at K={0} only (23 distinct shas over 64 K):
  reproduces canary F1 (period 64, window 1). HARNESS VALIDATED.
- ui_widget_game_data_input_functions: _solo_level_select_list_update_displayed_items (704, residual) EXACT at K={2,3}
  (2-wide window as r2w1 triage said; triage's "+16/17" was measured at another position/base). Canaries in the
  same TU: _mutliplayer_settings_select_list_update_displayed_items exact {0-14,17-21,24-63}, _player_profile_3wide_list_update
  exact {0-17,48-63} -> both exact at K=2,3 => TU joint window {2,3} for solo (the other residual,
  _multiplayer_game_directions, never exact). Each compile ~0.3 s; 128 compiles in 3 s at 14 workers.

## Step 3 - FULL CENSUS, position after_include (results/raw.jsonl, 86 units x K=0..63 = 5,504 compiles, 0 failures, 91 s)
- windows_after_include.txt lists every function whose status or sha varies with K.
- The after-include position is remarkably INERT: of all functions on the 86 units only 17 vary at all, and 16 of them
  take exactly 2 distinct shas over the whole period (a binary flip).
- Count-sensitive RESIDUAL (non-empty window): ONLY ui_widget_game_data_input_functions::_solo_level_select_list_update_displayed_items
  {2,3} (TU joint window {2,3}).
- Residuals that FLIP sha with K but are never exact at this position (count-coupled, not closable by .c count alone):
  ai::_ai_test_ballistic_line_of_fire 944, bitmap_group::_bitmap_group_add_bitmap 992, decals::_decal_new_from_collision 6176,
  hud_weapon::_render_weapon_hud 2624, collisions::_collision_move_point 4752 (canary packet gain), physics::_physics_update_old 5456,
  rasterizer_xbox_models::__rasterizer_model_draw 5168 (parked).
- Count-sensitive EXACT functions (fragility): ai_communication::_ai_communication_update_speech_timers {0-4,6-20,23,31-63},
  bitmap_drawing::_bitmap_copy {0,3-16,25-31,56-58,63}, decals::_decals_delete_permanent_from_cluster {0-3,28-48,58-63},
  game_engine::_populate_statistic_buffer {0-40,46-63}, ui_widget_gdif mutliplayer_settings {0-14,17-21,24-63} and
  player_profile_3wide {0-17,48-63}, geometry::_get_edge_vertex {0-7,15-56,62-63},
  rasterizer_xbox_models::__rasterizer_model_transparent_geometry_submit {0-11,13-16,25-34,36,58-63}.
- Next: the same census at the TOP of the TU (before all #includes: shifts header records too, canary window.py
  position), because canary F5 showed header-inlining functions are position-sensitive.

## Step 4 - positions 2 and 3 (results/raw.jsonl pos=top 5,504 compiles 86 s; results/raw_before_fn.jsonl 126 residual
##          functions x K=0..63 with the dummies immediately before the residual's own definition, 8,064 compiles 135 s)
- fnpos.py locates all 126 residual definitions uniquely (after the include anchor in every case).
- flipsets.py (sha classes per position): EVERY count-sensitive function on the 86 units has EXACTLY TWO forms over the
  full period at every position (a single binary tie). top == after_include for every function except
  projectiles::_projectile_detonate (EXACT; flips only at top, K=23-37 -> its relevant records include header records).
- before_fn: solo {2,3} again (its tie records are its own). ai_test_ballistic_line_of_fire and render_weapon_hud do NOT
  flip before_fn (their tie involves .c records declared before them); physics_update_old and __rasterizer_model_draw
  flip on a different K set before_fn (records on both sides of the function start).
- The B forms of the flipping residuals are NOT January: collision_move_point B = 2cded988 (January 8b2f2900 needs the
  canary packet's genuine source); __rasterizer_model_draw B = cbfa8585 (the same drift canary F5 saw under the packet).
- Residuals count-sensitive at ANY position: 8 of 126 (solo closes; 7 flip A<->B, both wrong).

## Step 5 - tiers and form distances (tier.py -> results/tiers.json; formdist.py -> results/formdist.txt)
- tier.py = structural_split.py's S1/S2/T logic (reloc-target multiset minus internal $L/$SG/self; real insn count)
  against a fresh K=0 census compile: 126 residuals = S1 26 (48,176 B) / S2 59 (112,800 B) / T 41 (32,880 B).
- Count-sensitive residuals by tier: T: solo (closes), ai_test_ballistic_line_of_fire (flips only);
  S2: bitmap_group_add_bitmap, decal_new_from_collision, collision_move_point; S1: render_weapon_hud, physics_update_old,
  __rasterizer_model_draw. => of the 41 tie-shaped (T) residuals, 2 are count-sensitive at all and 1 closes by count.
- Form distance to January (difflib over capstone text, relocated dwords zeroed; "unmatched January insns in order"):
  solo A 1 / B 0 (EXACT); collision_move_point A 3 / B 2 (B closer; the canary packet's genuine real_math.h body closes it);
  ai_test_ballistic_line_of_fire A 4 / B 10 (A closer; the flip is a second tie, January needs a different one);
  bitmap_group 175/175, decals new_from_collision 774/774, render_weapon_hud 490/491, physics_update_old 610/610,
  model_draw 370/371: structural gaps dominate, the count tie is incidental.

## Step 6 - 64x64 GRID (results/raw_grid.jsonl): K1 dummies at TOP x K2 immediately before the function, for the 8
##          count-sensitive residuals; 32,768 compiles, 0 failures, 530 s
- solo: EXACT at 128 points = exactly the points with (K1+K2) mod 64 in {2,3}: a pure TOTAL-COUNT tie.
- The 7 count-coupled residuals: still EXACTLY 2 forms each over all 4,096 points, never exact.
  Form depends only on K1+K2 (total count) for decal_new_from_collision, collision_move_point, bitmap_group_add_bitmap;
  on the split (records before vs after the function start) for ai_test_ballistic_line_of_fire, render_weapon_hud,
  physics_update_old, __rasterizer_model_draw. Neither the total nor the relative phase reaches January for these.
=> No count configuration (uniform or before/after-function split) closes any residual except solo.
- Next: inside_fn position (k block-scope externs as the first declarations of the function body = shift the
  function's locals/labels relative to its params) for all 126 residuals.

## Step 7 - inside_fn position (results/raw_inside_fn.jsonl; k block-scope `extern long census_in_NNN;` as the first
##          declarations of each residual's body; 126 x 64 = 8,064 compiles, 0 failures, 134 s)
- Sensitive: the same 6 as before_fn, with IDENTICAL flip sets (so the functions' own parameters are not tie
  participants; the relevant records are locals/labels, all after the insertion). solo {2,3} again.
- No residual closes at inside_fn except solo. 120 of 126 residuals are completely count-INERT at every position tried
  (after_include, top, before_fn, inside_fn): one sha over the full period.
- Launched whole-board top-of-TU census (all_units.txt: 446 units with a build/split target) for the fragility map.
- CORRECTION to step 7: residuals count-sensitive at ANY position = 8 (solo + 7 coupled), so 118 of 126 residuals are
  completely count-INERT (one sha over the full period at after_include, top, before_fn and inside_fn).

## Step 8 - WHOLE-BOARD fragility (results/raw_board_top.jsonl, top-of-TU K=0..63 for all 453 units that have a
##          build/split target; 28,992 compiles, 0 failures; board_fragility.py -> results/board_fragility.txt)
- (first pass used the canary's ': cl ' regex and missed 7 units whose ninja rule wraps with `$`, including
  ui_widget_gdif and transport_endpoint_winsock; re-run for all_units_extra.txt appended - now complete.)
- 7,250 functions EXACT at K=0 board-wide; 22 are count-sensitive (all with 2 forms except draw 23, race_touch_flag 3,
  bitmap_3d_smooth 4); 7 break within |shift|<=3: frame_statistics draw (+-1), bitmap_copy (+1,+-2,-3), race
  player_update/touch_flag (-1..-3), units node_orientations (+1..+3), bitmap_2d_alpha_bleed (+2,+3),
  geometry get_edge_vertex (-3). Board-wide tolerated uniform shift (every TU at once): {0} only.
- 8 count-sensitive NON-exact functions board-wide, all on the 86 units (the same 8 as above): no residual on a
  complete object is count-sensitive.

## Step 9 - zero-loss hosts for solo's +2/+3 (incsets.py -> results/incsets.json via CL /E #line records;
##          header_candidates.py -> results/header_candidates_solo.txt)
- ui_widget_gdif opens 73 files. A genuine +2 or +3 (mod 64) records in any of 18 of them loses no currently-exact
  function board-wide (top-of-TU approximation): the .c itself, transport.h, ui_widget.h, ui_widget_definitions.h,
  ui_widget_game_data_input_functions.h, hud_messaging.h, player_ui.h, main.h, network_client_manager.h,
  network_game_globals.h, network_game_manager.h, network_messages.h, player_profile.h, playlist_profile.h,
  text_group.h, unicode.h (+ SDK wchar.h/time.h, not editable).
- Every broad header loses canaries: cseries.h/SDK/float/integer_math/real_math (+2: bitmap_copy, alpha_bleed, draw,
  units; +3: alpha_bleed, draw, units); tag_groups/byte_swapping (alpha_bleed, units; +2 also bitmap_copy);
  game_engine.h (units); players.h/game.h/xtl-family (draw, units); cseries_windows.h (draw).
- Inside the .c: mutliplayer_settings (L3127) and player_profile_3wide (L3434) are both defined BEFORE solo (L3883)
  and both tolerate +2/+3, so any genuine deficit anywhere before solo in the .c is zero-loss in this TU; the tightest
  localisation is the region after player_profile_3wide_list_update (player_profile_1wide_list_update, L3663).

## Conclusions
- Full period, four positions + a 64x64 grid: ONE residual of 126 (0.8%) closes by record count: ui_widget_gdif
  _solo_level_select_list_update_displayed_items, k=+2 (width 2: +2/+3 mod 64), TU-joint window holds at both K.
  It is the only count-sensitive residual in its object (the other residual, _multiplayer_game_directions, is
  count-inert) => NO object completes by count alone.
- 7 more residuals are count-COUPLED (binary tie flips A<->B) but neither form is January at any of 4,096 grid
  points; the 5 S1/S2 ones have large structural gaps; collision_move_point's B form is 2 instructions from January
  (the canary packet closes it with genuine real_math.h bodies); ai_test_ballistic_line_of_fire (T) has A closer.
- Shift distribution: a single data point (k=+2). No shared-header deficit pattern exists to detect: the only
  count-sensitive functions on the 86 units live in 8 TUs, each a single binary tie with its own phase.
- The earlier "2 of 57" figure is confirmed in spirit over the full period: of the 41 T-tier (tie-shaped) residuals,
  2 are count-sensitive at all and 1 closes. The record-count oracle is NOT a primary route.
