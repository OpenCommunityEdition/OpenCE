# rasterizer_xbox_profile wave-3 ledger (slug scratch/w/rasterizer_xbox_profile/)

Base tree 954eebd2. Task: whole-object admission of source/rasterizer/xbox/rasterizer_xbox_profile (16/16 exact).
Rejection entry (_D3DSIMPLERENDERSTATEENCODE, candidate-only-comdat-owner) names: (1) 3 stock SDK SELECT_ANY tables,
(2) shared window-state declaration ownership (local global_window_parameters extern),
(3) three consumer-local Profile API declaration pairs (rasterizer_transparent_geometry.c, xbox/rasterizer_xbox_models.c,
xbox/rasterizer_xbox_detail_objects.c). Plus objdiff 132 B .data gap.

## Inputs read
- WORKER_BRIEF.md s.0-8, campaign_house_rules.md, claude_fifty_objects_20260925.md (held items).
- All 7 docs/object_matching_logs/rasterizer_xbox_profile_obj_* ledgers; rejection entry; no parks for the unit.
- Precedent: w/collision_debug/LEDGER.md (3 D3D tables identical + pair-link 22/22), w/review_admit2 + w/review2_units:
  57-58 Matching objects already carry the same 3 D3D tables as surplus => systemic.
- semantic_data_matches.json currently has NO rasterizer_xbox_profile entry (the 20260904 ledger's allow_incomplete entry
  is not present at 954eebd2) -> to verify in report.json.

## F1 - baseline (production source, gate.py --out base.obj)
- gate: 16 exact / 0 residual / 0 unwritten. base.obj == build/base (differs only in COFF timestamp byte 5).
- object_audit base.obj: every January-owned section ok, 67 January symbols 0 differ, OBJECT AUDIT: PASS (audit_base.txt).
  Candidate-only: .drectve, 7 literals (water, clear, message, global_d3d_device, 3 __real) + 3 D3D tables.

## F2 - SDK-table surplus identity + provider link (the rejection's first item)
- surplus_all.py (every candidate-only EXTERNAL def vs every January split definer, raw+flags+section_infos_equal):
  10 rows, problems 0 (surplus_all_base.txt). D3DTEXTUREDIRECTENCODE 16 == headers; D3DSIMPLERENDERSTATEENCODE 328
  == progress_bar; D3DPRIMITIVETOVERTEXCOUNT 88 == rasterizer_xbox_decals; all flags equal (0x40301040/0x40401040).
  Literals water==game_globals, clear==ai_debug, message==network_connection, global_d3d_device==rasterizer_xbox,
  __real@bf800000==actor_moving, __real@00000000==action_charge, __real@447a0000==actor_firing_position.
- provider_link.py base.obj: 10/10 PASS both orders, SELECTED-PROVIDER LINK: PASS (provider_link_base.txt).
- surplus_identity.py: 0 candidate-only code COMDATs.
- d3d_census.py (d3d_census.txt): 133 build/base objects define the tables, all SELECT_ANY (sel=2);
  61 Matching objects already carry ALL 3 as surplus (incl. collision_debug, units, files_windows, event_manager,
  biped_limp_noodle). January split definers encode sel=1, but split_sel_census.txt shows csplit writes sel=1 for
  ALL 176 non-literal .rdata COMDATs (and every .text COMDAT) => the split's selection byte is a csplit class
  encoding, not provenance. Stock SDK source: xbox/include/D3D8.h:319-325 `#define D3DCONST extern CONST
  DECLSPEC_SELECTANY` with the SDK comment that selectany lets the linker drop duplicates / unreferenced copies.
- Verdict: the SDK-table item meets exactly the collision_debug (b9a8d587) standard. The literal reopen wording
  ("successful ordinary canonical link with the real dependency/import closure") is NOT achievable in this repo
  (no import libs; full split union leaves 171 unresolved - 20260906 ledger) and no Matching object has it; the lane
  admits on identity + pair-link in both orders.

## F3 - objdiff .data gap (132 B)
- build/report.json: .data 132 at 68.686874% (matched_data 2102/2234). Reproduced in own one-unit 3.3.1 projects
  objproj_nc (incomplete) and objproj_c (complete=true): identical, .data 68.686874 both.
- coff: data_entry.py equal=true, size 132, 29 relocs, all resolve to image addresses, sha b577924a..., flags
  c0400040 both, owner storage 3/3 (data_entry.txt).
- Cause proof under the FROZEN 3.3.1 scorer: dollar_rename.py replaces '$'->'_' in ??_C@ names identically in
  target and ours (39 names each): .data 68.686874 -> 100.0, unit data 2234/2234 (objproj_nodollar).
  Diagnostic only (not credited): objdiff 3.6.0 on the same objects scores .data 100.0 (objproj_v36).
  => the gap is the '$'-literal relocation-name defect; a pinned single-section entry is the right handling.

## F4 - window-state declaration ownership (the rejection's second item) - evidence
- profile.c:238 carries a consumer-local `extern struct rasterizer_window_begin_parameters global_window_parameters;`.
- January: `_global_window_parameters` is a COMMON record in build/split/source/linker_common.obj (section 65), inside
  the contiguous rasterizer_xbox.c cluster (texture_table 60, texturestagestate_table 61, renderstate_table 62,
  global_d3d_caps 63, pixel_shader 64, global_window_parameters 65, global_frame_parameters 66) => tentatively defined
  by rasterizer_xbox.c (COMMON-pool ordering law). HCEX.pdb: Global `global_window_parameters`, type
  `struct rasterizer_window_begin_parameters`, 0x258 bytes, contributed by "* Linker *" (COMMON there too).
- Our tree: no TU defines it; 23 TUs declare local externs; 11 use INCOMPATIBLE partial views (different struct tags)
  -> any owner-header extern is a C2371 in every conflicting TU that includes that header.
- include_closure.py (CL /Zs /showIncludes, 453 Halo units, 0 errors) + consumers.py:
    rasterizer.h: 52 consumers, conflicting 6: rasterizer, rasterizer_debug, rasterizer_lights, xbox_decals,
      xbox_dynavobgeom, xbox_widgets.
    rasterizer_xbox.h: 15 consumers, conflicting 8 (adds xbox_debug, xbox_lights, xbox_shadows; drops rasterizer_debug).
    rasterizer_xbox_internal.h: 6 consumers, conflicting 1 (rasterizer.c); profile does not include it.
- Owner choice: rasterizer.h. It defines the type, declares rasterizer_window_begin(parameters), and already carries
  the extern of global_frame_parameters, the adjacent member of the same January rasterizer_xbox.c COMMON cluster.
- Layout check (render_cameras.h): camera@0x8 (position 0x8, forward 0x14, viewport_bounds 0x34), frustum@0x5C
  (world_to_view 0x6C, view_to_world 0xA0, projection_matrix 0x1A0), fog@0x1E8 (atmospheric_maximum_density 0x1F8):
  every field of the 6 conflicting rasterizer.h views maps onto a genuine named field (to confirm by offsetof probe).

## P1 - owner = rasterizer.h (shadow-tree A/B sweep, all 453 Halo units)
- Harness: shadow_build.py (copies source/ to shadow/, compiles every unit with build.ninja flags, /I remapped;
  compare = per-section section_infos_equal+flags by owner key + symbol table). Pristine shadow (objA) vs build/base:
  0 section/symbol diffs in 453 units (d3d_intimacy has no .c) => harness reproduces production.
- offsetof probe (probe_offsets.c): camera.position 0x8, camera.forward 0x14, camera.viewport_bounds 0x34,
  frustum.world_to_view 0x6C, frustum.view_to_world 0xA0, frustum.projection_matrix 0x1A0,
  fog.atmospheric_maximum_density 0x1F8, sizeof 0x258 (== HCEX 0x258). All partial-view fields are genuine fields.
- Patch (apply_window_owner.py, all keys): rasterizer.h extern next to global_frame_parameters; profile local extern
  removed; rasterizer.c / rasterizer_debug / rasterizer_lights / xbox_decals / xbox_dynavobgeom / xbox_widgets
  partial views + their local externs removed, accesses renamed to camera.* / frustum.* / fog.*.
- RESULT objA vs objB: 452 units section-identical INCLUDING all 6 converted TUs and profile; ONE regression:
  rasterizer_frame_statistics `_rasterizer_frame_statistics_draw` (EXACT 4176 at base) -> DIFF. frame_statistics
  never names global_window_parameters: this is the known declared-name-count canary (+1 name in rasterizer.h).
  => rasterizer.h (and rasterizer_xbox.h, also included by frame_statistics) are REJECTED as landing sites
  (would cost a 4,176 B exact function; no filler compensation allowed).
- (harness fix) build.ninja wrapped `build $` lines were missed: ninja_units.py now parses 467 source units (466 with a
  .c); include closure + objA rebuilt: objA vs build/base = 0 section/symbol diffs over 466 units.
  Recomputed consumers: rasterizer.h 58 (same 6 conflicts), rasterizer_xbox.h 21 (8 conflicts),
  rasterizer_xbox_internal.h 8 (rasterizer.c the only conflict).

## P2 - owner = rasterizer_xbox_internal.h (hypothesis: the backend header of the January COMMON owner
## rasterizer_xbox.c, which already declares `_rasterizer_window_begin(struct rasterizer_window_begin_parameters const *)`,
## the writer of global_window_parameters (rasterizer_xbox.c:1801); frame_statistics does NOT include it)
- Edits (apply_window_owner.py --only internal,profile_internal,rasterizer): internal.h gets
  `extern struct rasterizer_window_begin_parameters global_window_parameters;` before #endif; profile.c includes
  "rasterizer_xbox_internal.h" after "rasterizer_xbox.h" and drops its local extern; rasterizer.c drops the partial
  `struct rasterizer_window_parameters` + its extern and uses camera.forward / camera.position (6 sites).
- RESULT objA vs objB, all 466 Halo units: 0 section/symbol diffs (profile 16/16 sections identical, rasterizer.c,
  frame_statistics, xbox_texture_cache, units all identical). ZERO regressions.

## P4 - Profile API consumer pair (rejection's third item). transparent_geometry and models were already migrated
## (7d3c53c8 rasterizer_xbox_state.h; models includes rasterizer_xbox.h). Only xbox_detail_objects.c:266-270 remains.
- P4a: detail_objects includes "rasterizer/xbox/rasterizer_xbox.h" and drops EVERY local declaration that header owns
  (rasterizer_error, rasterizer_profile_end/begin, rasterizer_set_pixel_shader, rasterizer_set_texture [local had a
  wrong `void` return; owner returns union point2d *], rasterizer_set_vertex_shader_permutation, externs
  global_d3d_device + pixel_shader); `21` -> `_rasterizer_profile_detail_objects` (enum value 21, rule 16).
  gate 20/20 EXACT; object section-identical to production (objdiff2.py).
- P4c: narrow rasterizer_xbox_state.h instead: 20/20, section-identical. P4e (enum only): 20/20, identical.
- Preferred: P4a (genuine public owner header; removes 8 consumer-local declarations, not just the pair).

## P3 - optional cleanup: drop the now-redundant compatible local externs in the other internal.h consumers
- rasterizer_transparent_geometry.c, xbox_active_camouflage.c, xbox_environment.c, xbox_models.c,
  xbox_transparent_geometry.c (rasterizer_xbox.c left alone: NonMatching, held .bss item; January's tentative definer).
- RESULT objA vs objC (P2+P3), all 466 units: 0 section/symbol diffs. /W3: profile, rasterizer.c, detail P4a add 0
  warnings (12 shared-header warnings before and after).

## F5 - /Od readout of the profile TU (later PC dx9 build "rasterizer\dx9\rasterizer_dx9_profile.c")
- /Od begin 0x800580, end 0x800770, frame_begin 0x800920, check 0x8004c0, enable 0x800400, query 0x800c20.
- check signature in /Od: [ebp+8]=boolean condition, [ebp+0xC]=short profile, [ebp+0x10]=message (pushes message,
  profile, condition). The NONE branch also pushes profile+message (same authentic format/arg bug as January).
- P5: production order (message, profile, condition) -> /Od order (condition, profile, message) at prototype,
  definition and 9 call sites (make_profile_p5.py): gate 16/16 EXACT, object section-identical, audit PASS.
  January's private ESI/DI contract cannot distinguish; /Od is the only evidence => optional authenticity patch.
- /Od statics: active_profile_index a36d48 / window_index a36d4c (+4, word) - same 4-byte stride as January +8/+12;
  local_profile_enable e0e8c8, profile_flags e0e8cc (+4), callback_errors e0e8d0 (+8, word) = January +1104/+1108/+1112.
  Nothing in /Od reads January's unreferenced +1114..+1119 span (profile_state.reserved06[6]); error_count is elsewhere
  (e0ecfc) in the later build. No new evidence for that span; left as reviewed in 20260903/20260906.

## F6 - volatile strip/add controls (disclosure for the admission reviewer; NOT changed)
- Strip all 3 volatile: 12/16 (callback 192!=272, frame_begin 320!=336, initialize 144!=112, query 256!=288).
  Per qualifier: elapsed_times -> callback/initialize/query residual; callback_errors -> callback/frame_begin;
  start_times -> callback/initialize. All three are load-bearing.
- Add-volatile controls: callback_elapsed_times -> frame_end residual; callback_start_times -> frame_callback residual;
  callback_end_times -> inert. => January's volatile set is exactly the state shared with the per-profile D3D
  callback (rasterizer_profile_callback: start_times, elapsed_times, callback_errors) and NOT the frame-callback
  arrays: a coherent semantic boundary (D3D InsertCallback callbacks run asynchronously), previously reviewed as
  genuine in 20260903/20260906. Reviewer risk only; no source change proposed.

## F7 - group link (stronger than pair links, still NOT a full image link)
- group_link.py: final profile obj + ALL 132 other build/base definers of the 3 SDK tables (incl. January's selected
  headers/progress_bar/xbox_decals) in one Link.Exe run, candidate first and last: 0 LNK2005/LNK1169 naming any SDK
  table (the only duplicate is the unrelated pre-existing `bipeds.obj: _object_get_type already defined in items.obj`);
  2519 unresolved lines (no import libs) => no image. group_link.txt.

## FINAL composed candidate (patches/01..07; candidate_files/ = post-images)
- Composed shadow P2+P3+P5+P4a (+ listing comment) vs pristine: 466/466 Halo units section/symbol-identical (objD, objE).
- final_profile.obj: object_audit PASS (59 January sections ok, 67 symbols 0 differ); pdb_storage 0 disagreements;
  surplus_all 10/10 identical; provider_link 10/10 PASS both orders; fake_match_scan 0 leads (4 changed files).
- objdiff 3.3.1 one-unit project (complete=true) + tools.semantic_progress verifier with ONLY the proposed entry:
  credited +132, revoked [], unit 2511/2511 code, 2234/2234 data, 16/16. Without complete (no allow_incomplete_unit)
  the verifier refuses => entry must land atomically with the config.json status flip and the rejection retirement.
- git apply --check: series 01..07 OK on the real tree; 01,02,03,05,06,07 each OK alone (04 needs 03).
- Shared header 01 consumers (CL /showIncludes closure == git grep): xbox_texture_cache, rasterizer,
  rasterizer_transparent_geometry, rasterizer_xbox, xbox_active_camouflage, xbox_environment, xbox_models,
  xbox_transparent_geometry (+ profile via 03). consumers_rasterizer_xbox_internal_h.txt.
- Patches 01+02 are inseparable (01 alone is C2371 in rasterizer.c). 04 (/Od param order), 05 (redundant externs)
  are optional zero-byte cleanups. 06 is independent (detail_objects already Matching).

## Stop rule
- No further probes: every rejection item is resolved or answered with measured evidence; remaining items are
  reviewer/owner judgements (reopen-criterion wording, owner-header choice disclosure, volatile, reserved06 span).
- (housekeeping) shadow/ + obj dirs + full HCEX dumps deleted after measurement (regenerable: shadow_build.py, include_closure.py, DIA2Dump -c/-f); hcex_contribs_excerpt.txt kept.
