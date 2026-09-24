# LEDGER - owner_queue/objlights_render_debug (wave 5, base cdc8ebd3, Halo objects 379)

Items: (1) object_lights - surplus _object_get_bounding_sphere copy with January's per-TU __FILE__ literal + data_gap packet;
(2) render_debug - helper/vehicle patch (a)-(c) + held (d) invented render_debug_globals_definition aggregate, (e) six
consumer-local `extern boolean debug_*` with no definer.

## Step 0 - state
- git status clean at cdc8ebd3 (0 entries). Brief read (sections 0, 9, 10).
- Since 931ed8dc: render_debug.c and units/vehicle_datum.h UNCHANGED; object_lights.c/objects.h/action_vehicle.c changed
  only in b807f9f6 (OBJECTS_H_FILE packet landed); config/symbols.json changed by b807f9f6, b9a8d587, f7cd2e72, 6e3e2d35,
  466698b8, cdc8ebd3 (patches touching symbols.json must be regenerated).

## Step 1 - tooling + controls (all outputs under this slug)
- tools/: tgate.py (shadow-TREE gate, from w/bounding_sphere), object_audit_split.py (SPLIT_ROOT; ROOT=cwd), symfix.py
  (symbols.json line surgery + csplit into <cfg>_split, slug-local), surplus_cand.py, pdb_storage_cand.py, objcmp.py,
  provider_link_cand.py (--provider=<unit>::<obj>), splitcmp.py (sha census of two split roots). All read before use.
- CONTROL split: symfix cfg_ctrl with EMPTY fixes -> symbols.json copy byte-identical to config/, csplit -> 833 objects,
  0 differ from build/split => build/split is current for cdc8ebd3 and the csplit emulation is faithful.
- CONTROL shadow tree tree0 (copy of source/ at cdc8ebd3): object_lights 43/43 EXACT, render_debug 36/36 EXACT; objcmp vs
  build/base: both IDENTICAL => build/base is current for these units and tgate reproduces production.
- `git apply --check` at cdc8ebd3: review_admit2 render_debug_genuine_helpers_and_vehicle.patch CLEAN (render_debug.c and
  vehicle_datum.h unchanged since 931ed8dc); data_gap patches/object_lights.patch CLEAN; data_gap symbols_json_all.patch
  FAILS (symbols.json moved: 6 commits) -> regenerate as line surgery on cdc8ebd3.

## Step 2 - ITEM 1 object_lights rebased + measured (treeOL = tree0 + data_gap object_lights.patch via git apply --directory)
- data_gap patches/object_lights.patch applies at cdc8ebd3 (all 8 hunks offset +4: the landed b807f9f6 OBJECTS_H_FILE
  define). No content change needed. symbols.json: 4 x "static": true line surgery (cfg_OL, symfix.py) re-done on cdc8ebd3.
- Emulated split cfg_OL_split: csplit changes ONLY source/objects/object_lights.obj (splitcmp 1/833).
- tgate treeOL object_lights vs cfg_OL_split: 43/43 EXACT (gate_OL_object_lights.txt). Warning C4133 at line 855 is the
  pre-existing production warning at line 853 (+2 lines).
- object_audit_split (SPLIT_ROOT=cfg_OL_split): PASS, 71 January symbols 0 differ (audit_OL_object_lights.txt).
  Control: production object vs cfg_OL_split = FAIL(4) on exactly the 4 storage rows (3/2).
- pdb_storage_cand: 71 split symbols, 0 disagreements with cachebeta publics.
- Header consumers of object_lights_rendering.h (render.c 13/13, render_objects.c 22/22): objcmp vs build/base IDENTICAL.
- surplus_cand: 12 candidate-only code COMDATs, 11 IDENTICAL to January's selected copies; _object_get_bounding_sphere
  DIFFERENT vs action_vehicle: size 144/144, normalized sha EQUAL, 9/9 relocs, ONLY rows @37 and @76 (DIR32) differ:
  "..\objects\objects.h" (selected) vs "c:\halo\source\objects\objects.h" (ours) (bsphere_selected_vs_OL.txt).
- provider_link_cand: SELECTED-PROVIDER LINK PASS (all 33 surplus names incl. _object_get_bounding_sphere vs
  action_vehicle, both orders; selection ANY both sides) (plink_OL_object_lights.txt).
- fake_match_scan: 0 leads (candidate and production object_lights.c/.h).
- Semantic data: semsnap (verifier's section_info_resolved) with cfg_OL symbols: _lights_section 1541 B / 1 reloc /
  527b43a1... target==base, identical to data_gap's measurement. Mini objdiff 3.3.1 (proj_OL, candidate vs cfg_OL_split):
  code 12294/12294, 43/43 fns, data 1104/2645 (.data 1541 @99.74%) - identical to production (proj_OL_prod).
  verify_semantic_entry.py (runs tools.semantic_progress.apply_semantic_data_matches on the mini project):
  YES entry (no allow_incomplete_unit) + complete=True -> +1541, data 2645/2645; NO entry (allow_incomplete_unit) ->
  +1541; negative control YES entry without complete -> SemanticProgressError (fail-closed as expected).

## Step 3 - ITEM 1 first-party evidence re-verified at cdc8ebd3
- Census build/split: c:\ objects.h literal DEF only in object_lights (storage 2, sec 45) and referenced by NO relocation
  (relrefs.py: 0 hits in January object_lights); ..\ literal DEF action_vehicle, UNDEF render_objects;
  _object_get_bounding_sphere DEF action_vehicle, UNDEF in the other 8 users (census_bsphere_split.txt).
- NEW: board orphan-literal census (orphan_literals.py over all 833 split objects, 9,420 ??_C@ literals): exactly ONE
  January string literal has no referrer anywhere - the c:\ objects.h literal in object_lights (orphan_literals_split.txt).
  So it is unique and its only possible referrer is object_lights' own discarded COMDAT copy.
- NEW: link-order consistency: action_vehicle code sits at image file offset ~0x9100 (module 461), object_lights at
  0x128730 (module 152); image order = descending module index, so action_vehicle precedes object_lights in link order
  and the linker's pick-first ANY selection keeps action_vehicle's "..\" copy - exactly January's selected copy.
- January object_lights also UNDEFs _normalize3d and 18 literals/reals that our copy emits as identical COMDATs.

## Step 4 - (e)-class externs ALSO present in object_lights (found while auditing item 1)
- object_lights.c declares `extern boolean debug_lights; extern boolean debug_object_lights; extern struct data_array
  *light_data; extern struct cluster_partition light_cluster_partition; extern short debug_rasterizer_light_count;` with
  no definer anywhere in the tree, next to its own tentative `struct lights_globals lights_globals;`.
- January pool (linker_common) records 108..113 = debug_lights, debug_object_lights, debug_rasterizer_light_count,
  lights_globals (ours: object_lights COMMON 848), light_cluster_partition, light_data - one contiguous cluster.
  Same class exists in ADMITTED objects: render_objects.c `extern boolean debug_objects; extern short
  debug_rasterizer_light_count;` (Matching, admitted b9a8d587), scenario.c `extern boolean debug_sound_environment;`.

## Step 5 - ITEM 2 render_debug (a)-(c) rebased + measured (treeRD = tree0 + review_admit2 patch, unchanged)
- Patch applies unchanged at cdc8ebd3 (render_debug.c, vehicle_datum.h untouched since 931ed8dc).
- tgate render_debug 36/36 EXACT; object_audit PASS (63 January symbols, 0 differ); pdb_storage 0 disagreements.
- surplus_cand: 12 candidate-only COMDATs, 0 not identical (new: _point_from_line3d == action_charge selected copy;
  production has 11). provider_link_cand: SELECTED-PROVIDER LINK PASS.
- Rule 6 exception conditions for the new _point_from_line3d COMDAT: identical to selected copy YES; callers
  (render_debug_vector/tick/line_offset) strictly exact YES; sweep: render_debug.c is TU-local and vehicle_datum.h's
  only includers (actor_moving, physics, vehicles; no header includes it) are objcmp-IDENTICAL to build/base;
  selected-provider link PASS. January render_debug holds no out-of-line reference (all-inlined class, ruling #5).
- Mini objdiff 3.3.1: identical to production (code 8235/9448, 34/36 fns: _render_debug_add_cache_entry 93.6 and
  _render_debug 94.2 are the known objdiff under-credit rows already in semantic_report hidden_exact; data 30236/30236).
- fake_match_scan 0 leads.

## Step 6 - (e) six extern boolean debug_*: a real definer EXISTS (first-party evidence)
- January: the six are linker_common COMMON records 50..55 (1 byte each); only referencers render_debug.obj and
  hs_globals_external.obj (census_debug6_split.txt). No definer anywhere in our tree (same census on build/base).
- Pool oracle (pool_oracle.py, all 242 records): 105 single-referencer records, 102 in ascending module order
  (3 violations: wind_globals, 2 XAPI/CRT). hs_globals_external (module 285) references 81 pool records and is the
  bracket-admissible owner of NONE of them -> it defines no COMMON. The six sit between render_objects(84)'s cluster
  (debug_objects, cached_object_render_states) and render(90)'s _render; the only referencer in range: render_debug(87).
- 2001 Bungie linker maps (earlier-map-archives; COMMON rows read "<common>", no owner column):
  * 2001-08-15 cachebeta.map: 5 toggles (input,bsp,structure,player,camera) between cached_object_render_states and
    _render; render_debug.obj has render_debug_player/camera/structure/bsp/input but NO render_debug_structure_decals.
  * 2001-09-25 cachebeta.map: 6 toggles incl. _debug_permanent_decals (new, head of cluster) AND render_debug.obj now
    has _render_debug_structure_decals (its only user) -> the toggle arrives with its render_debug user.
  * 2001-09-25 cache.map (retail): render_debug.obj NOT linked (0 rows); render.obj and render_objects.obj linked;
    the pool goes debug_objects, cached_object_render_states, _render - all six toggles ABSENT. Other hs toggles of
    linked TUs (debug_objects, debug_sprites, debug_lights, debug_point_physics, debug_damage) persist in retail.
    => the six exist iff render_debug.obj is linked.
- Later first-party /Od build (halo_cache_symbols.exe, data only; od_hsglobals.py reads the hs_global_external table
  records by name string): debug_permanent_decals 0xf0afb0, debug_camera b1, debug_player b2, debug_structure b3,
  debug_bsp b6, debug_input b7 - INTERLEAVED byte-by-byte with render_debug's own cache variables at 0xf0afb4 (short),
  b8, bc, c0 (referenced only by render_debug cache fns 0x845810/0x845c00/0x846350); each toggle is read by exactly
  one render_debug function (0x84a1f0, 0x847660, 0x848d00, 0x84a0e0, 0x847210, 0x8481c0). A linker COMMON pool cannot
  interleave inside one object's .bss -> in the /Od build render_debug.obj itself holds the six.
- HCEX.pdb: debug_bsp/camera/input/permanent_decals/player/structure absent (2011 build); HCEX pools its own COMMONs in
  a "* Linker *" contribution (light_data, debug_rasterizer_light_count, players_globals...) -> no compiland data.
- Resolution form (evidence-determined, not invented): the six as bare TENTATIVE definitions in render_debug.c
  (boolean debug_bsp; ...) - exactly the objects.c precedent (boolean debug_objects_physics; etc., Matching) and the
  batch-4b players COMMON landing. Measured (treeRDe): 36/36; objcmp vs (a)-(c) candidate IDENTICAL; only delta = six
  external section-0 symbols value 0 -> 1 (COMMON, size = January record size); .debug$S differs by the path string only.
  linkprobe: + January linker_common.obj both orders 0 LNK2005/LNK1169; + base hs_globals_external both orders: 0
  duplicate, all six resolved (control production: 12 unresolved lines for the six).
- Same class in object_lights (5 externs; pool cluster 108-113 around its own tentative lights_globals; /Od:
  debug_object_lights 0xded688 and debug_lights 0xded689 sit between object_lights-only addresses 0xded684 and
  lights_game_globals 0xded68c). Measured (treeOLe): 43/43 vs cfg_OL_split, objcmp IDENTICAL to item-1 candidate,
  COMMON sizes 1,1,2,12,4 = January record sizes; linkprobe with January linker_common and with base render_objects +
  hs_globals_external: 0 duplicates, all resolved.
- Precedent note: this class did NOT block render_objects' admission (b9a8d587: extern boolean debug_objects;
  extern short debug_rasterizer_light_count; no definer) nor scenario (Matching, extern boolean debug_sound_environment;).
  Board: 18 of 81 hs-referenced pool records are tentatively defined in our tree; 181 of 242 pool records have no definer.

## Step 7 - (d) invented aggregate: LAB (never lands)
- lab/bss_decl_order.c vs lab/bss_uninit.c (plain CL /O2 /Oy-): seven separate ZERO-INITIALISED statics in the
  aggregate's member order give strings@0, entries@0x400, game_time@0x7400, entry_count@0x7404, string_offset@0x7408,
  entry_overflow_reported@0x740A, string_overflow_reported@0x740B, .bss 29708 = January exactly; the uninitialised
  variant is name-hash ordered (29714 B) -> wrong.
- Whole-TU lab (make_lab_d.py on the (a)-(c) candidate; placeholder names render_debug_cache_<member>; aggregate, its
  8 offset checks and both opaque pad members removed): vs build/split 33/36 (3 residuals are reloc NAME only: sha equal,
  same section-relative destinations; January's .bss owner symbol is _render_debug_globals) -> with a lab symbols.json
  (rename _render_debug_globals -> first static, add 6 static rows at +0x400,+0x7400,+0x7404,+0x7408,+0x740A,+0x740B;
  csplit changes only render_debug.obj): 36/36 EXACT, object_audit PASS (69 January symbols, 0 differ). Uninitialised
  variant: 36/36 by gate (name-matched relocs) but object_audit FAIL (7): .bss 29715 and every offset moved.
  Mini objdiff 3.3.1 identical to production (34/36 under-credit rows unchanged, data 100%).
- /Od corroboration: the hs toggles debug_bsp/debug_input (0xf0afb6/b7) sit INSIDE the span of the cache variables
  (0xf0afb4..0xf0afc0) -> the later first-party build had separate variables, not a struct.
- => the aggregate is NOT load-bearing; the separate-statics form is exact with ANY names in declaration order; the open
  question is only the names (no first-party source has them: HCEX has no render_debug data, 2001 maps list no
  render_debug data rows, cachebeta publics lack them) and the = 0 initialisers (VC7 declaration-order law, the
  periodic_functions P2 precedent).

## Step 8 - admission hygiene scan of both final objects
- render_debug /W3: 6 x C4013 implicit declarations (ai_debug_render [ai/ai_debug.h not included],
  collision_debug_render [physics/collision_debug.h not included], render_debug_object_damage, texture_cache_debug_render,
  render_debug_recording, render_debug_fog_planes [no header prototype anywhere]). Present in production too; precedent:
  render.c (Matching) has 2 C4013. Not touched here (header edits = declaration-count risk).
- object_lights /W3: no C4013; pre-existing C4133 (line 853/855) and C4244s.
- Stale "symbols in this file" header comment with _code_/_bss_ placeholders in render_debug.c (comment only; admitted
  render_objects/path carry the same kind of stale block).

## Step 9 - final combined tree from the patch files only (treeFINAL = cdc8ebd3 source+config + 01,02,03_YES,04_YES,05,06,07)
- git apply --check at repo root: each patch alone, YES set (01-05), YES+E set (01-07), NO set (01,02,03_NO_ALT,05,06,07)
  all CLEAN; git status stayed clean (0 entries).
- treeFINAL files == the measured trees byte-for-byte (config/symbols.json == cfg_OL, semantic/config == cfg_edit YES,
  object_lights.c/.h == treeOLe, render_debug.c/vehicle_datum.h == treeRDe).
- tgate vs cfg_OL_split: object_lights 43/43, render 13/13, render_objects 22/22, render_debug 36/36, action_vehicle 17/17,
  actor_moving 31/36, physics 13/17, vehicles 37/39 (the last three = production rows; objcmp IDENTICAL to build/base for
  render, render_objects, actor_moving, physics, vehicles, action_vehicle).
- object_audit: object_lights PASS 71/0, render_debug PASS 63/0. pdb_storage 0/0. surplus: object_lights 12 (1 = the
  ruled bounding sphere), render_debug 12 (0). provider_link PASS both. fake_match_scan 0 leads (4 files).

## Step 10 - packaging
- All generated patches normalised to LF (the repo index is LF, worktree CRLF via core.autocrlf=true; 05 was already
  LF). Re-checked: each alone, YES set, YES+E set, NO set, 01-08 together -> git apply --check CLEAN at cdc8ebd3.
- Sequential `git apply` of 01..07 in a throwaway autocrlf=true repo (git archive of the 7 paths at cdc8ebd3, inside
  this slug, deleted afterwards) reproduces treeFINAL byte-for-byte; the lab D1 pair applies after 05+06.
  Quirk recorded: one-invocation `git apply --check 05 06 D1` at the real repo root reports a spurious failure at
  render_debug.c:248 (same-path chaining under autocrlf); one-call-per-patch is clean.
- 03_YES reason text finalised (no placeholder); both entries re-verified through apply_semantic_data_matches (+1541).
- 08_IF_D_RULED_render_debug_status_matching.patch added (config.json render_debug -> Matching) for a (d) yes.
- D1 lab patch now carries its disclosure comment; re-gated 36/36 + audit PASS (69/0) vs lab split; fake scan 0.
- /Od re-check: "D:\P4\Halo1\source\objects/objects.h" referenced twice by 0x443f50 (object_get_bounding_sphere asserts).
- Deliverables: patches/ (01..08), APPLY_ORDER.txt, MEASURED.md, RULING.md, symbols.json (edited copy == 02 applied),
  lab/ (D1 option + layout labs), final/ (gate_all, audits, surplus, plink).
- git status of the worktree: clean (0 entries) throughout.
