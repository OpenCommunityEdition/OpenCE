# MANIFEST - critic_3: first-party Sept-2001 names for placeholder/invented function names (round 2)

- Base/tip: worktree HEAD 5d3ca708 (task named 7b9de585; 7b9de585..5d3ca708 changes one docs ledger only). `git status`
  clean except the pre-existing untracked research/ dir, at start and end. No tracked file edited; no ninja/configure/git
  mutation. Label/slug `critic_3` (the orchestrator's computed label string was garbled with the unit list).
- Claims (critic_3): the 20 owner units below + 8 header consumers (verification only). source/hs/hs was HELD by
  critic_2_hs_compile at first (exit 3); claimed after its release. All released at the end.
- Credit: ZERO. Names are byte-inert: objdiff 3.3.1 (sha1 3130e428) measures identical for all 20 units before/after
  (objdiff_final/summary.txt). What changes: house-rule 7 compliance (ai_debug's 21 `code_<address>` statics disappear),
  first-party names replace invented ones, ai_debug storage agreement (audit FAIL 34 -> 13, or -> 2 with optional 02),
  progress_bar's D3D thunk COMDAT order now equals January's.

## Evidence (per row: rows_table.md; tools align.py, xcmp.py, callgraph.py, provenance.py)

1. Raw Sept-2001 `cachebeta.map` (2001-09-25, Static symbols section) aligned to the January split by name anchors;
   unmatched functions paired IN ORDER between identical anchors, sizes equal (align.txt). Aug-2001 map agrees where present.
2. Relocation-masked instruction streams, January split function vs every unmatched Sept function of the same object
   (xcmp.txt): 66 of 75 packeted rows are instruction-IDENTICAL to their Sept counterpart; 8 are best-by-far at
   0.93-0.98 (5 ai_debug, network_connection log_traffic_event, king point3d_to_point2d, progress_bar make_stuff_ready);
   `_code_00038280` is 0.36 (see 5).
3. Call graphs (callgraph.txt): callee sequences identical wherever Sept callees resolve (differences only at unresolved Sept
   D3D/XAPI library addresses); in-object callers identical.
4. cachebeta.pdb publics: none of the January addresses is public -> January had file statics with no surviving January
   name; the Sept map (same lineage, 4 months earlier, identical bodies) is the nearest first-party name. January string check
   (cachebeta.exe): no literal contradicts any Sept name; the "timesection" and "lens_flare_parameters" assert texts and the
   `default_cm_hardware_format` assert text support the Sept vocabulary. (Contrast the rasterizer_xbox caveat, where a January
   error literal contradicts the Sept name: no such row here.)
5. `_code_00038280` -> `_ai_debug_get_this_actor`: 0x150 vs Sept 0xf0; January = Sept callee list with a prefixed
   `local_player_exists` assert, same 7 callees in order, sole caller `_ai_debug_select_this_actor` in both builds, same
   anchor slot, Aug-2001 map same name.
6. hs radiosity evaluators: Sept .rdata hs_function_definition structs point "radiosity_start/save/debug_point" at
   `_radiosity_hack_{start,save,find_point}_evaluate`; January's definitions point at 0xb14a0/c0/e0.
7. players `create_weapon`: also HCEX.pdb 2011 (`d:\projects\code\hcex\sources\game\players.c`). The current
   `unit_create_starting_weapon`, main `sort_controllers_ascending` and set_winsock `poll_endpoint_array_compare_proc` are
   C++-mangled atlas PROPAGATIONS from another lineage that contradict the Sept retail cache.exe MAP ground truth.
8. progress_bar D3D thunk: Sept names the slot `_IDirect3DTexture8_Release@4` (w1 20260914 header-order proposal agrees);
   texture0/1 are `IDirect3DTexture8 *`, so the call drops three casts; our COMDAT order then equals January's.

## Patches (patches/, LF like the index; each of 01,03..21 passes `git apply --check` at 5d3ca708; 02 applies after 01)

| patch | files | content |
|---|---|---|
| 01_ai_debug | ai_debug.c, symbols.json | 21 `code_<addr>` statics -> Sept names (source + symbol-listing comment); dead never-called `ai_debug_render_path_nodes` stub (+prototype) removed because 0x3a910 takes that name (w2 20260914 proposal); 21 symbols.json rows renamed AND `"static": true` |
| 02_ai_debug_extra_static (OPTIONAL, after 01) | symbols.json | `"static": true` on 11 more ai_debug rows (7 fns + 4 .data) that are static in source and absent from cachebeta publics |
| 03 transport_endpoint_winsock | .c, symbols.json, parked.json | 4 renames; parked.json `_connect_endpoint_process@4` -> `_connect_async_thread_proc@4` (+ evidence note). MUST land together (parked validation keys on the name) |
| 04 transport_endpoint_set_winsock | .c, symbols.json, parked.json | 2 renames; parked.json `_transport_endpoint_set_compare_entries` -> `_poll_ep_array_compare_proc` (+ note). MUST land together |
| 05 profile | .c, symbols.json | 11 renames (timesection vocabulary) |
| 06 progress_bar | .c, progress_bar.h, symbols.json | 10 renames (incl. public `progress_bar_screen_initialized` -> `progress_bar_is_stuff_ready` in the header) + `IDirect3DTexture8_Release` call fix + symbols.json 3639 |
| 07 network_connection | .c, symbols.json | 4 renames |
| 08 saved_game_files | .c, saved_game_files.h, symbols.json | 4 IDENTICAL-body renames only (public `saved_game_files_enumerate_default_files` -> `enumerate_default_profiles` in the header) |
| 09 rasterizer_xbox_profile | .c, symbols.json | 4 renames (`profile_assert`, `callback_function`, `frame_callback_function`, `rasterizer_profile_enabled`) |
| 10 xbox_sound_cache | .c, symbols.json | `cache_block_get_sound_permutation_name` |
| 11 king | .c, symbols.json | `point3d_to_point2d` |
| 12 players | .c, symbols.json | `create_weapon` |
| 13 hs | hs.c, symbols.json | 3 `radiosity_hack_*_evaluate` (independent of critic_2's hs_library_internal_compile.h packet: combined objcmp IDENTICAL) |
| 14 hs_runtime | .c, symbols.json | `script_error` |
| 15 main | .c, symbols.json | `sort_desired_local_player_controllers` |
| 16 decals | .c, symbols.json | `decal_check` (NOTE: decals is closed for this lane for residual work; this is a name only, independent patch - integrator may defer) |
| 17 rasterizer_lights | .c, symbols.json | `lens_flare_parameters_get` |
| 18 dynavobgeom | .c, symbols.json | `submit_screen_vertex` |
| 19 environment_fog | .c, symbols.json | `rasterizer_environment_fog_screen_is_active` |
| 20 hardware_bitmaps | .c, symbols.json | `rasterizer_bitmap_cm_changed` |
| 21 render_cameras | .c, symbols.json | `render_camera_warn_once` (also proposed by opus5 w1/w3 20260914) |

After applying: csplit regen (ninja). symbols.json edits are in-place line edits (order untouched).

## Measurements (all slug-local; with_split.py open() redirect for split emulation; tools unmodified)

- csplit control split_ctl == build/split (833/833). Final split_ALLxs vs split_ctl: exactly the 20 owner objects differ;
  splitdetail: every section byte- and relocation-identical modulo the rename map; symbol changes = renames + ai_debug
  storage 2->3 (21, +11 with 02). With 02 csplit spells 4 static .data relocations as
  `_global_ai_debug_firing_position_color_count+N` (gate/objdiff/audit unaffected).
- Gate (run_gate_ALLxs.txt): per-function rows identical to production in all 20 units (ai_debug 59/1, hs 447/1, ...).
- objcmp production vs candidate (all non-.debug$S sections: bytes, relocations, symbols; names mapped): IDENTICAL in 18;
  ai_debug differs only in `$L` local-label numbering, attributed solely to the dead-stub removal (lab/ai_debug_stubless.c
  reproduces it; stubless vs renamed = IDENTICAL); progress_bar differs only by one COMDAT order swap (keyed compare
  IDENTICAL; new order = January's).
- Header consumers of progress_bar.h / saved_game_files.h (game, marketing_and_strategic_business_development, player_ui,
  ui_widget, virtual_keyboard, render, player_profile, playlist_profile): objects IDENTICAL with the shadowed headers.
- Section-4 battery (battery/, battery_R.txt, battery_ALLxs.txt): ai_debug object_audit FAIL(34) -> FAIL(13) [01] ->
  FAIL(2) [01+02; remaining = `_ai_debug_render_actor` residual + `_ai_debug_render_actor_jmptable` MISSING]; pdb_storage
  33 -> 12 -> 1. All other units: audit/pdb/surplus/provider_link results identical to production.
- Parked entries renamed in 03/04: target and candidate measurements under the new names equal the recorded ones.
- Not run: full ninja, pytest suite, board-wide stable_verdicts (integrator; no tracked edits allowed here).

## House-rule review
- Only identifiers/comments change (+ progress_bar cast removal, + ai_debug dead stub removal). Authentic names chosen
  over descriptive ones per rule 7, including the odd authentic `this_is_awful` and the misspelt `do_convoluation_coords`
  (both Sept statics, instruction-identical bodies). No filler, no new declarations, no casts added.
- Follow-ups noticed, not done: profile's `struct profile_timer` vs Sept/January `timesection` vocabulary; progress_bar /
  saved_game_files / xbox_sound_cache functions that are public in our source but static in January (storage packets that
  are NOT byte-inert: making never-called ones static drops them); ai_debug jmptable ownership.

## Not packeted (with reason)
- saved_game_files `_enumerate_mapfile_begin`, `_read_next_entry_in_mapfile`, `_open_/_close_mapfile_for_reading`,
  `_add_new_entry_to_mapfile`: module restructured Sept->Jan (0.67-0.97 similarity, reordered), pairing not unique.
- progress_bar `tgaLoad*`, `progress_bar_load_loading_texture`: January-only code (PDB-public tga* already named).
- periodic_functions: the only map difference is decoration (`@...@4` fastcall in January): nothing to rename.
- hs `_scripted_foley_predict_evaluate`: January-only.
