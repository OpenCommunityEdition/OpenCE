# critic_3 LEDGER (R2 completeness critic: Sept-2001 first-party names route)

Worktree HEAD 5d3ca708 (task named 7b9de585; git diff 7b9de585..5d3ca708 = one docs ledger only). Label/slug: critic_3
(the orchestrator's computed label/slug string was garbled with the unit list; normalised to `critic_3`).

## L0 claims
- Claimed (critic_3): ai_debug + 19 others. source/hs/hs HELD by critic_2_hs_compile (exit 3) -> hs rows read-only, reported BLOCKED.

## L1 premise measurement (read-only)
- align.py: raw Sept-2001 cachebeta.map (and Aug-2001 1749betaP map) aligned per unit by name anchors, in-order pairing
  between anchors (align.txt/json). provenance.py: January atlas (4cc87b45, 2002-01-14 cachebeta.exe) name at the
  January address, HCEX.pdb, January exe strings (provenance.txt/json). xcmp.py: relocation-masked instruction streams,
  January split function vs EVERY unmatched Sept function of the same object (xcmp.txt/json).
- Result: every ai_debug pairing except _code_00038280 is Sept-IDENTICAL or >=0.93 (and the best match by a wide margin);
  most other rows are IDENTICAL instruction streams. No January string contradicts a proposed Sept name
  (bare-token search of cachebeta.exe; stems searched too).

## L2 attestation per row (read-only)
- xcmp.txt: January split fn vs Sept xbe fn (relocation-masked capstone streams). IDENTICAL for 66 of the final 75 rows (rows_table.md);
  near-identical best-by-far for _code_000383d0 (0.936), _code_00038ad0 (0.976), _code_00038de0 (0.933), _code_00039e10 (0.973),
  _code_00039e80 (0.981), network_connection_notify_traffic_event (0.957), hill_points_to_2d (0.929), progress_bar_setup (0.968).
- _code_00038280 -> _ai_debug_get_this_actor: size 0x150 vs Sept 0xf0 (ratio 0.36) BUT callgraph.txt: January's call list is
  Sept's list with a prefix `local_player_exists` + assert (display_assert/system_exit) and otherwise the same 7 callees in order;
  sole caller in both builds is _ai_debug_select_this_actor (IDENT 0.973); in-order position between the same anchors
  (_ai_debug_describe_actor.._ai_debug_vocalize); Aug-2001 map has the same name/size. => grown by an assert; name attested.
- callgraph.txt: callee sequences SAME for every row whose Sept callees resolve in section 1 (DIFF rows differ only in
  unresolved Sept D3D/XAPI library addresses, or saved_game_files' enumerate_saved_game_file -> append_entry_to_mapfile callee);
  in-object callers agree for every row (the DIFF caller rows are function-pointer uses, not E8 calls).
- cachebeta.pdb publics: NONE of the January addresses is public (all January file statics) -> no January-own name exists;
  the Sept map (same lineage, 4 months earlier, byte-identical bodies) is the nearest first-party name.
- January string check (cachebeta.exe): no literal names any current or Sept name. Stems: "timesection" asserts
  (parent_timesection/child_timesection) and "lens_flare_parameters" asserts/data-array name SUPPORT the Sept vocabulary;
  "default_cm_hardware_format" (January assert text) supports Sept `_rasterizer_bitmap_cm_changed`; "glowy halo gravy" is where
  the invented _draw_gravy_* names came from (Sept names are draw_layer*).
- players: HCEX.pdb (2011) has `create_weapon` in d:\projects\code\hcex\sources\game\players.c = Sept name; the current
  `unit_create_starting_weapon` is a C++-mangled atlas PROPAGATION (tier exact, other lineage) that contradicts the Sept 2001
  retail cache.exe MAP ground truth at the same function (_create_weapon). Same for main `sort_controllers_ascending` and
  set_winsock `poll_endpoint_array_compare_proc` (atlas propagation) vs Sept map `_sort_desired_local_player_controllers` /
  `_poll_ep_array_compare_proc`.
- hs radiosity evaluators: Sept map `_radiosity_hack_{start,save,find_point}_evaluate` while Sept xbe ALREADY carried the hs
  command strings radiosity_start/save/debug_point (so January's identical strings do not contradict). hs HELD by
  critic_2_hs_compile -> not probed.
- Rows NOT packeted: _IDirect3DResource8_Release@4 (SDK wrapper, header-order lever territory), saved_game_files
  _enumerate_mapfile_begin/_read_next_entry_in_mapfile/_open_/_close_mapfile (module restructured Sept->Jan, ratios 0.67-0.97,
  pairing not unique), progress_bar tga*/load_loading_texture (January-only code), periodic (same name, only decoration).

## L3 emulated split + candidate compile (tag R: 71 renames in 19 units, ai_debug rows also "static": true)
- mkcand.py R: shadow_R/source (19 .c + progress_bar.h + saved_game_files.h), cfg_R (config copy, 71 in-place symbols.json
  name edits; the 21 ai_debug rows also gain "static": true). ai_debug: the dead never-called stub
  `ai_debug_render_path_nodes` (+ prototype) removed because code_0003a910 takes that first-party name (w2 ledger proposal).
- csplit control: split_ctl == build/split 833/833. split_R vs split_ctl: exactly the 19 owner objects differ
  (splitdiff_R.txt); splitdetail_R.txt: every section byte- and relocation-identical modulo the rename map, symbols differ only
  by the renamed names + 21 ai_debug storage 2->3. No other object references any renamed symbol.
- run_gate_R.txt: production (gate vs build/split) vs candidate (gate vs split_R, CL=/I shadow): per-function rows SAME in all
  19 units; objcmp (all non-.debug$S sections bytes + relocations + symbols, names mapped) IDENTICAL for 18 units; ai_debug
  differs ONLY in $L local-label numbering (1270 symbols, same sections/relocs) = the removed stub's labels shifting the counter.
- consumers_R.txt: the 8 other includers of progress_bar.h / saved_game_files.h compile to identical objects with the shadowed
  headers (claimed critic_3 verification-only).

## L4 audit battery (battery_R.txt, battery/*.txt; with_split.py redirect, tools unmodified)
- ai_debug: object_audit FAIL(34) -> FAIL(13); pdb_storage 33 -> 12 disagreements (the 21 renamed rows now static in the
  split, as the source and cachebeta publics say). surplus 19/0 not identical and provider_link PASS unchanged.
- Optional Rxs (11 more ai_debug rows "static": true: 7 functions + 4 .data): object_audit FAIL(2) (= _ai_debug_render_actor
  residual + _ai_debug_render_actor_jmptable MISSING), pdb_storage 1 (jmptable). csplit then spells the 4 static .data
  relocations as `_global_ai_debug_firing_position_color_count+N` instead of the member names (splitdetail PROBLEM rows are
  exactly those 4 relocation spellings); gate rows SAME, objdiff 3.3.1 unchanged (16446/42523 code, 58/61 fns, 5818/5874 data).
- 18 other units: object_audit / pdb_storage / surplus_identity / provider_link results IDENTICAL prod vs R (renames of
  statics cannot move storage).
- objdiff 3.3.1 (sha1 3130e428) mini report objdiff_proj/: all 19 units identical measures prod-vs-split_ctl and
  cand-vs-split_R (category totals 126848/190434 code, 666/708 fns, 1568316/1570488 data both sides).
- Label-shift attribution: lab/ai_debug_stubless.c (production minus the dead stub only) vs production = the same $L shift;
  stubless vs renamed candidate = IDENTICAL. Renames are inert; only dead-stub removal renumbers local labels.

## L5 dependent config found by grep of config/ and tools/
- config/parked.json keys `_transport_endpoint_set_compare_entries` and `_connect_endpoint_process@4` (parked residuals) ->
  parked_functions.validate needs one report function by that name: the parked.json "function" value MUST be renamed in the
  same commit (measurements unchanged: byte-identical). render_cameras frustum park mentions the old helper name in prose only.
- config/object_admission_rejections.json: rasterizer_xbox_profile entry keys `_rasterizer_profile_globals`; the old helper
  name appears in prose only (unaffected).
- tools: docstring mentions only (context_diff_oracle, emission_shape_oracle, real_regions, c2_diag_map); the pinned manual
  audit tools/audit/progress_bar_base_layer_runtime.py uses role labels on pinned historical objects (fixture-free tests).

## L6 hs (claim released by critic_2 later; claimed critic_3)
- tag H: 3 renames; split_H differs in hs.obj only; splitdetail OK; gate 447/1 rows SAME; objcmp IDENTICAL.
- Combined with critic_2's hs_library_internal_compile.h (patch/b): prod+c2hdr vs H+c2hdr IDENTICAL; prod vs prod+c2hdr
  IDENTICAL. Independent of that packet.
- hs pairing proven by DATA, not only order: Sept xbe .rdata hs_function_definition structs (string pointer at 0x26f89c/
  0x26f8b8/0x26f8d4) point to _radiosity_hack_start/_save/_find_point_evaluate for "radiosity_start"/"radiosity_save"/
  "radiosity_debug_point"; January split _hs_radiosity_{start,save,debug_point}_definition relocate to 0xb14a0/c0/e0.

## L7 progress_bar D3D wrapper (Sept first-party name + header-order law)
- Sept map names the 0x10 thunk at the January slot (between _D3DTexture_UnlockRect@8 and _IDirect3DTexture8_GetLevelDesc@12)
  `_IDirect3DTexture8_Release@4`; w1 20260914 had proposed the same from the COMDAT header-order law (D3D8.h line 2107 vs
  Resource8 line 2020). texture0/1 are `IDirect3DTexture8 *`, so the natural call is cast-free.
- tag PB: 3 call sites `IDirect3DResource8_Release((D3DResource *)progress_bar_mode.textureN)` ->
  `IDirect3DTexture8_Release(progress_bar_mode.textureN)` + symbols.json 3639 rename. split_PB differs in progress_bar.obj
  only; gate rows SAME (`_IDirect3DTexture8_Release@4` EXACT 16); all 100 non-debug sections identical keyed by owner; the only
  change is COMDAT emission ORDER, which now equals January's (UnlockRect -> Texture8_Release -> GetLevelDesc; production had
  the Release thunk after Device8_End). Audit/pdb/surplus/link unchanged (the wrapper keeps the pre-existing class-wide
  storage 2/3 disagreement shared by all static D3D thunks in this object).

## L8 final combined state (ALLxs = all 20 packets + optional 02)
- Sequential GNU-patch application of 01,02,03..21 onto production copies reproduces shadow_ALLxs + cfg_ALLxs exactly
  (CR-insensitive; patches are LF like the index, worktree is autocrlf); each of 01,03..21 passes `git apply --check` alone
  at 5d3ca708.
- split_ALLxs vs split_ctl: 20 objects differ, all rename/storage only (ai_debug also the 4 xs relocation spellings).
- run_gate_ALLxs.txt: rows SAME in all 20 units; objcmp IDENTICAL in 18 (ai_debug: $L numbering; progress_bar: one COMDAT
  order swap, keyed compare identical). consumers_ALLxs: 8/8 identical.
- objdiff 3.3.1 objdiff_final/summary.txt: all 20 units identical measures (152096/215763 code, 1113/1156 fns,
  1568334/1625286 data both sides). => zero credit change, as expected for names.
- Renamed parked entries: split_ALLxs + candidate measurements equal the recorded parked measurements under the new names.

## L9 outcome
- Packets: patches/01..21 (+ optional 02). Zero credit; byte-inert (objdiff identical, gate rows identical). ai_debug is the
  only unit whose audit moves (placeholder names were a house-rule-7 violation; storage agrees with PDB). No object completes:
  every unit keeps its pre-existing residual/data/storage blockers (MANIFEST.md "Not packeted" + per-unit notes).
- Stop rule: route is names-only; no further probes are meaningful. Reopen: a newer first-party map (e.g. a Dec-2001/Jan-2002
  map) or a January literal that contradicts any Sept name here.
