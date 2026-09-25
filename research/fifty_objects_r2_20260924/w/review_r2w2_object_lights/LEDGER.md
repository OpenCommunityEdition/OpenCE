# LEDGER review_r2w2_object_lights (adversarial review of r2w2_object_lights 01+02+03_NO_ALT)
Claimed source/objects/object_lights as review_r2w2_object_lights_rv1 (exit 0).
Read: WORKER_BRIEF_R2 s0-9; worker MANIFEST/LEDGER/patches; triage; round-1 RULING Q1 + owner_queue s4;
precedent commits 05255584 (hs_runtime statics per publics), ae12a3c1 (leaf_map allow_incomplete_unit),
84414a1a (objects allow_incomplete), 5019c186 (editor_flying_camera allow_incomplete); race doc cites
"established CTF policy" of allow_incomplete_unit for '$'-literal data in incomplete units.

## R1 storage facts
- pdb_storage production: 4 disagreements (exactly the 4 helpers). cachebeta publics: none of the 4 present;
  _sample_diffuse_texture 0x128920 and _lights_initialize 0x128A30 public; 0x128A10 has no public row. CONFIRMED.
- Board census: 1,033 external non-public symbols.json fn rows remain; storage reconciliation is incremental (canonical practice).
- Prior ledger object_lights_obj_opus_reconciliation_20260903 said "January COFF storage class 2" for
  should_render_lights: that class is csplit's output from symbols.json (not independent). Worker's rebuttal holds.

## R2 independent re-measurement (all under this slug; no tracked edits)
- git apply --check at HEAD 434f0151: 01, 02, 03_NO_ALT each CLEAN; all three in one call CLEAN. Index lines equal
  HEAD blobs (2553c63c, c863833f, f9abb17d, 77814b3d); my post-images hash to 9e77ec23/527cc0b3/e6ea96b2/3f1a813e and
  are byte-identical to the worker's tree/ and cfg_OL/. Patches are LF.
- Own csplit emulation (build/tools/csplit.exe -i cachebeta.exe -p <cfg> -o <slug>): verbatim config == build/split
  833/833; with 02: only source/objects/object_lights.obj differs.
- Own shadow compile (sgate.py: build.ninja cflags, cwd = shadow root): cand vs split_OL 43/43 EXACT, 12,608 padded;
  prod vs build/split 43/43; prod vs split_OL 43/43. gate.py --source cand.c also 43/43, objcmp == shadow object.
- objcmp prod vs cand: 109/109 sections identical except .debug$S (path only); 311 symbols, only the 4 storage rows 2->3.
- object_audit (redirected to split_OL): cand PASS, 71 January symbols, 0 differ; prod FAIL(4) storage 3/2;
  cand vs unmodified build/split FAIL(4). 01+02 must land together + csplit regen.
- Reference census build/split + build/base: the 4 names are DEF only in object_lights, 0 UNDEF anywhere.
- surplus_identity: 12 COMDATs, 11 IDENTICAL, _object_get_bounding_sphere DIFFERENT (held Q1, pre-existing).
  provider_link cand: 33 surplus names PASS both orders.
- Consumers render 13/13, render_objects 22/22: objcmp == build/base (only .debug$S path).
- Strip tests: (a) no calls (static helper, expanded predicate restored) -> 42/43, _should_render_lights UNWRITTEN
  => calls load-bearing for a January-owned section; (b) static prototype removed from the .c block -> 43/43, objcmp
  identical => the prototype is byte-inert style, not a filler.
- /Od (halo_cache_symbols.exe, data only): 0x78dab0 = should_render_lights (reads [0xded68c]+0, calls 0x59b3a0
  whose ONLY caller it is); callers only 0x78c6eb (diffuse, after 0x825320 begin) and 0x78c97b (specular, after
  0x825a30). January diffuse/specular relocs: begin, lights_game_globals, _game_engine_allow_dynamic_lighting (inlined).
- NEW corroboration (not in worker packet): /Od incremental-link pattern. In-TU calls to January-PUBLIC functions go
  through ILT thunks (light_disconnect_from_map 0x787f90 via 0x407441, light_reconnect_to_map 0x788bf0 via 0x409c19);
  every direct in-TU call/address is a January non-public static. should_render_lights (0x78dab0), light_attenuation
  (0x787b80 = 1 - d*d/(r*r), called direct from 0x78798e), cluster_get_first_light (0x787810) and
  cluster_get_next_light (0x787840, both pushed as direct addresses in lights_preprocess_scene 0x78b3c5/0x78b3ca) are
  all direct => static in the later build too. HCEX: cluster pair "static function: true"; other two absent.
- Data: section_info_resolved(_lights_section) EQUAL with cfg_OL and production symbols; size 1541, 1 reloc
  (type 6 -> 0x267e88), sha 527b43a1...f71 == entry measurements. .data flags c0400040 both; _lights_section static @0,
  4 public object_light_* @1528..1540. report.json: .data 99.7411%, unmatched 1541 == entry size.
- Verifier apply_semantic_data_matches: on build/report.json and on my mini objdiff 3.3.1 (sha1 3130e428) cand report
  (43/43, 12294/12294, data 1104/2645): +1541 -> 2645/2645; without allow_incomplete_unit -> FAIL-CLOSED.
- audit_object_admission: prod 9/0/2/0; with cfg_OL semantic+symbols 10/0/2/0 (object_lights joins candidates, as
  disclosed; the queue already holds other owner-held units without rejection entries).
- fake_match_scan: 0 leads (cand + prod). /W3 identical (14); /W4 identical in the same location (24; the prod-path 25
  is a cwd/path-display artifact reproduced by the unpatched source in a shadow).
- Stacks textually with the other in-flight symbols.json patches (rasterizer_xbox_debug, vehicles, weapons,
  weather_particle_systems).

## R3 precedent audit
- P1 (static per cachebeta publics, zero-credit, object stays blocked): 05255584 hs_runtime converters, action_flee/
  guard, ui_widget_event_handler_functions, units. Every condition holds; prior 'external' claims (3bfde7bd /
  20260903 reconciliation, 20260829 light_attenuation ledger) read csplit's default class of a _code_ placeholder,
  not independent January evidence. 3bfde7bd renamed _code_00128a10 without touching its default storage. Canonical
  2026-08-31 marker_cluster policy correction already established "correct linkage to static even at credit cost".
- P2 (allow_incomplete_unit single-section '$'-literal data entry): leaf_map (ae12a3c1), editor_flying_camera
  (5019c186), objects (84414a1a), race/CTF ("established CTF policy", 20260902). Single symbol, pinned measurements,
  sole unmatched data, verifier re-proves on every report. Not stretched. Round-1 owner queue s4 itself lists 01+02
  (+optional 03_NO_ALT) as the NO-path landing independent of Q1.
- No new scopes/locals: /Od declaration-order check N/A. House rules on changed lines OK (void own line, rule 13
  hand-expansion removed, rule 9 static prototype in the owner .c).
