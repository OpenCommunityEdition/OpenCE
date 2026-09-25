# LEDGER - record-count SYNTHESIS (slug record_synthesis) - LAB ONLY

Tree: 7b9de585 (HEAD moved from 42125a52 by the integrator; git status shows only untracked research/). No tracked edits.
Claim: source/interface/ui_widget_game_data_input_functions (record-synthesis, lab only). math/real_math RESERVED (Codex).
Inputs: record_census (census.json), real_math_h_recovery (real_math_jan.h, jan.py), canary_frame_statistics (PA packet).
Goal: ONE genuine configuration whose record shifts close count-sensitive residual ties with ZERO board-wide loss.

## Step 0 - reading (done)
- WORKER_BRIEF_R2 0-9; canary LEDGER F1-F9 + MANIFEST + lab/board/window/grid/netcount/mkvar/rerun_packet;
  record_census LEDGER/MANIFEST; real_math_h_recovery LEDGER/MANIFEST/january_real_math_map.md.
- Closable targets by count: (a) ui_widget_gdif::_solo_level_select_list_update_displayed_items 704 B needs +2/+3 mod 64
  (TU total count; also works inside solo's own body); (b) collisions::_collision_move_point 4752 B via the canary PA packet
  (genuine real_math.h bodies; RESERVED -> after Codex). The 7 other count-coupled residuals never reach January.

## Step 1 - harness (slab.py, units_list.py) + control
- slab.py = canary lab.py/board.py fingerprint (cwd=<tree>, build.ninja flags, strict section_infos_equal vs build/split)
  with the unit list fixed for all three ninja rule forms (`obj: cl src`, `obj: cl $\n src`, `obj: $\n cl src`):
  454 units (canary 447 missed 7 incl. ui_widget_gdif). All outputs asserted under this slug.
- sh/ = copy of source/ @ 7b9de585 (source identical to 42125a52: `git diff --stat 42125a52 7b9de585 -- source config tools` empty).
- CONTROL: sweep(sh) vs fingerprint of build/base/*.obj: 454 units, 0 failed, 7251 EXACT, gained 0 / lost 0 /
  drift 0 / data drift 0. ui_widget_gdif rows: 44 EXACT, mgd 44f3a050 + solo f227fef9 residual (== r1 park bytes).
- HCEX.pdb -sf dumped to hcex_sf.txt (2011 compiland include lists; e.g. ui_widget_gdif compiland opens 120 files incl.
  cseries\errors.h; later revision - names/includes only).

## Step 2 - solo counter (win.py) + ui_widget_gdif candidate repairs (ugd.py) - production vs January order
- win.py: K=0..63 `extern long synth_dummy_N;` at the TOP of the .c, lab copy in the unit dir of the (slug) tree.
  Control sh: solo {2,3}, mutliplayer_settings (ms) exact {0-14,17-21,24-63}, 3wide {0-17,48-63} == census.
- r1 lane packet R (claude-fifty-objects-20260925 slug ui_widget_game_data_input_functions final.c/final.h: January
  definition order from assert lines + deferred emission, static callbacks/prototypes/table (cachebeta publics absent,
  HCEX static), static locals displayed_servers/build_number_string (HCEX Static Local, .bss name-hash order), header
  trimmed). The production .c/.h are unchanged since r1's HEAD, but r1's final.c predates the landed game_variant
  bodies of get_editable_playlist_profile_display_name / multiplayer_edit_profile_set_ruleset_textbox_string_index:
  ugd.py PORTS r1 (those two bodies = current production). Rm = R + r1's mgd join-store tail.
- Simulated csplit renames (slab.RENAMES; r1 symbols.json.new): _displayed_servers -> ?displayed_servers@?1??server_list_menu_update@@9@9,
  _ui_widget_game_data_build_version_wide_string -> ?build_number_string@?1??set_textbox_to_build_number@@9@9.
- MEASURED (rows + windows; K = dummies needed at top for the function to be exact):
  | variant | rows (renames on for R*) | solo K | ms exact K | 3wide exact K | net vs base (solo) |
  | base | 44/2 | 2-3 | 0-14,17-21,24-63 | 0-17,48-63 | 0 |
  | R  | 42/4: LOSES ms + 3wide (count shift), solo+mgd resid | 37-38 | 1-55,58-62 | 28-61 | -35 (Jan order) |
  | Rm | same as R (mgd 32d299eb, still resid) | 37-38 | same | same | -35 |
  | V (6 .c verify typedefs removed) | 44/2 | 18-19 | shifted +16 | shifted +16 | -16 |
  | E (#include "cseries/errors.h") | 43/3: LOSES 3wide | 22-23 | shifted +20 | shifted +20 | +44 |
  | RV | 42/4 (ms,3wide exact? see sweep) | 53-54 | 0-7,10-14,17-63 | 0-13,44-63 | R-16 |
  | RVE | 44/2 w/ renames | 9-10 | 0-27,30-34,37-63 | 0-33 | R-16+44 |
  Additivity holds exactly (V, E are uniform shifts before every function in both orders).
- KEY: r1's packet R is NO LONGER zero-loss at 7b9de585: in January order ms and 3wide are exact only for
  R-relative K in {28-55,58-61}; K=0 breaks both (r1 measured 44/2 at its older base e9e62b78; headers moved since).

## Step 3 - canary PA packet re-verified on 7b9de585 with the COMPLETE 454-unit harness
- pkt/ = copies of canary packet/A_*.patch (+ lab/effects_owner_gated.patch as B1); all apply cleanly (patch -p1 --binary).
- PA  (rlr vp vpd vpc swcw bdB): 454 units, 0 failed, 7252 EXACT: GAINED physics/collisions::_collision_move_point 4752,
  LOST 0, data drift 0; drift = the known surplus COMDATs (collisions/render_cameras _valid_real_plane3d 5a34cbcd,
  breakable/weather _real_local_random -> a8ad3cb3) + parked __rasterizer_model_draw 89b0d7ea -> cbfa8585.
- PAE (PA + B1 effects owner-gated half): same gain, 0 lost, + effects _real_local_random_range 7ed2e50e surplus.
  => canary result holds on the current base INCLUDING the 7 units the canary harness skipped.

## Step 4 - more genuine levers + the solo arithmetic (combos.py, jtree.py, tuwin.py, run_configs.py)
- dupcensus/proto_scan over the ui_widget_gdif TU (pre/ugd.i): only ONE duplicate prototype in a Halo header outside
  cseries: player_ui.h player_ui_get_single_player_local_player_controller at l.34-35 and l.64-65 (verbatim).
  PU = remove l.34-35: it is the LATER copy (677d0982 2026-08-30 vs cc7b74b7 2026-08-20) AND the out-of-order one
  (player_ui.c defines it between get_last_single_player_level_played and local_player_joined_multiplayer_game, i.e.
  where l.64-65 sits) -> copy choice is NOT count-driven. Only ONE unnamed-parameter prototype: main.h
  main_vertical_blank_interrupt_handler(unsigned long) (main.c: `unsigned long context`) -> MN names it.
- MEASURED: PU net -2 (solo K 4-5), whole board 454 units gained 0 / lost 0 / drift 0. MN net +1 (solo K 1-2),
  whole board gained 0 / lost 0 / drift 0. JAN tier (real_math_jan.h default) in this TU: +7 (solo K 59-60).
  PA in this TU: net 0 (rlr +1, vp +1, swcw -2).
- combos.py (additive arithmetic over measured levers V -16, E +44, PU -2, MN +1, SWCW -2, JAN +7, LATER +3, bases
  prod/R): the ONLY solo-closing sets with ms+3wide kept are {JAN, PU, SWCW} (net +3, production order) and three
  R-based sets that need JAN+LATER. With PA (net 0) and without the JAN tier NOTHING closes solo.
- J0 = JAN bodies+companions (spl1 spl3 rlr vp vip2 vrv2 vrn2 + vpd vip2c vrv2c vrn2c vpc) + swcw + bdB;
  J1 = J0 + PU. Whole board (454): J0 gained 0 / lost 5; J1 GAINED solo 704 / lost 5 (8,144 B):
  bitmap_utilities::_bitmap_2d_alpha_bleed 560, decals::_decals_delete_permanent_from_cluster 400,
  network_client_message_handler::_network_game_client_handle_message 1088, frame_statistics draw 4176,
  units::_unit_preprocess_node_orientations 1920. collision_move_point NOT gained (35a38557: JAN nets +5 in its TU).
  => prediction confirmed: the complete JAN tier + swcw + PU puts the gdif TU on solo's window (+3).
- tuwin.py joint windows of the lost TUs under J1 (K top dummies = extra genuine records needed): frame_statistics {59}
  (= exactly -5), bitmap_utilities {45-49,53-60}, decals {24-44,54-63}, network_client_message_handler {1-63},
  units {1-2,6-13,16,19,26-61}. frame_statistics needs EXACTLY -5 (width 1); its measured genuine levers are vt -17
  (all 6 typedefs), stack_walk (already in), LATER +3; the recovery lane's "+12 residual" is the same wall.
- run_configs.py (whole board, vs sh): u_MN 0/0; C1 = PA+PU+MN, C2 = PA+PU+MN+R+V, C3 = PA+PU+R+V,
  C4 = PA+B1+PU+MN+R+V: every one GAINED _collision_move_point 4752, LOST 0, data drift 0 (R renames simulated).
  gdif solo nets: C1 -1, C2 +12, C3 +11, C4 +12 -> solo stays residual.

## Step 5 - more lever classes checked (missing_protos.py, proto_scan.py, hcex_counts.py)
- Missing owner-header prototypes (January publics defined in the owner .c, declared nowhere in the TU): hud_messaging.h
  10 (cost 29; the declarations live as CONSUMER-LOCAL prototypes in hs.c / game_engine.c), main.h 6 (cost 21; 4 live
  in the campaign-invented main_runtime.h / main_internal.h), game_engine.h 46, players.h 18, real_math.h 5 (broad
  hosts). Adding HM (+29) to combos.py: still no solo-closing zero-loss set without the JAN tier. Owner-header
  consolidation is a genuine class but every partial application is a count-selected subset.
- HCEX include graph (hcex_sf.txt / hcex_file_counts.txt): cseries/errors.h is opened by 329/329 cseries.h compilands
  (so the 2011 cseries.h pulls errors.h; ours does not - 115 TUs include it locally = precedent for E);
  ui_widget_definitions.h, rasterizer_frame_statistics.h, sort.h, main_runtime.h, rasterizer_debug_options.h,
  bitmaps_internal.h do not exist in HCEX (campaign-invented; each costs +1 file record per includer). HCEX widget-type
  enum = _widget_type_container.._custom + NUMBER_OF_UI_WIDGET_TYPES (ours: TU-local invented _ui_widget_type_*).
- collisions object under PA: 20/20 code, scratch/tools/object_audit.py (read first; read-only) OBJECT AUDIT: PASS
  (surplus = the pre-existing header-inline COMDAT set + the new _valid_real_plane3d, == January's selected copy per canary).

## Step 6 - post-Codex lookup (codex_lookup.py -> codex_lookup.txt)
- For Codex's net record change N (mod 64) in the ui_widget_gdif TU, the measured non-real_math levers that close solo
  with ms+3wide kept: N=+1 MN; N=+2/+3 nothing needed; N=+3 MN+PU; N=+4/+5 PU; N=+8..+12 R+E+V(+PU/MN);
  N=+17..+25 V/E families; N=-28..-23 R or E+V families; N=-12..-4 R+V / R+E families; all other N: nothing.
  (J1 is the N=+5 case: JAN tier + swcw = +5, PU closes solo - measured.)
- Where two or more sets work, choosing one is a COUNT-SELECTED owner decision; PU and MN are independently desirable
  repairs, so N in {+1, +3, +4, +5} would close solo with genuine-only hygiene (still disclose: the timing of landing
  PU/MN is then count-driven).

## CONCLUSIONS
1. Nothing fully principled closes solo today. With PA (net 0 in its TU) and every measured non-real_math genuine lever
   (R, V, E, PU, MN, HM) no zero-loss combination reaches solo's +2/+3 window. The only measured closure (J1: complete
   January-attested real_math.h JAN tier + swcw + PU) loses 5 canaries in 5 other TUs; frame_statistics then needs
   EXACTLY -5 records (width-1 window) and no principled lever set gives -5 (vt = -17, LATER +3, header merges -1 each).
2. The canary PA packet is re-verified on 7b9de585 over all 454 units: +_collision_move_point 4752 B, 0 lost, data 0;
   collisions 20/20 + object audit PASS. Still needs the count-driven duplicate-copy owner ruling (bdB vs bdI,
   swcw vs swcs) + COMDAT admission + parked model_draw re-baseline. It is the best landable-after-Codex packet.
3. NEW WARNING: r1's ui_widget_gdif storage/order packet R (object-audit repair) is no longer zero-loss at this base:
   alone it loses mutliplayer_settings 1424 + 3wide 1232 (count shift). R+V and R+E(+V) are zero-loss (measured); the
   compensator choice is count-driven (owner decision).
4. PU (player_ui.h duplicate, copy fixed by definition order) and MN (main.h parameter name) are zero-loss genuine
   hygiene (0 gain); C1 = PA+PU+MN and C2/C3/C4 (+R+V, +B1) all measured +4752 / 0 lost.

## Step 7 - packets written + cleanup
- packet/: PU_player_ui.h.patch, MN_main.h.patch, RV_ui_widget_game_data_input_functions.{c,h}.patch,
  E_ui_widget_game_data_input_functions.c.patch, R_symbols_json_ops.txt (41 static flags + 2 static-local renames,
  re-based on config/symbols.json @ 7b9de585 line numbers). Verified: PU/MN/RV patches applied with patch -p1 --binary
  to a pristine sh copy reproduce the lab variant files byte-for-byte. pkt/ = canary PA patch copies (+ B1).
- Cleanup: var/ trees removed (rebuild with ugd.py / jtree.py / run_configs.py), large DIA dumps deleted
  (regenerate: DIA2Dump -sf / -t HCEX.pdb > hcex_sf.txt / hcex_types.txt; hcex_file_counts.txt and
  hcex_widget_type_enum_excerpt.txt kept). sweeps/*.json kept (per-function fingerprints of every configuration).
