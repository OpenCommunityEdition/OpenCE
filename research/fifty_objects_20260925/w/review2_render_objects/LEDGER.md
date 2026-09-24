# LEDGER - review2_render_objects (adversarial review of bounding_sphere packet: render_objects OBJECT_COMPLETE_CANDIDATE)
Base tree 931ed8dc. Reviewer dir scratch/w/review2_render_objects/.

## V0 - patch/packaging checks
- All 5 patches `git apply --check` clean against HEAD 931ed8dc, individually and as a set.
- final/*.c/.h equal HEAD + patch hunks (diff --strip-trailing-cr shows exactly the patch hunks); CRLF kept.
- config/symbols.json copy: exactly one line differs (23251), CRLF kept, byte length +59.
- Own csplit (build/tools/csplit.exe, cfg = HEAD config + worker symbols.json) over ALL 833 objects:
  ONLY source/render/render_objects.obj differs from build/split. Control csplit with HEAD config: 833/833 identical
  to build/split (build/split is current).

## V1 - independent shadow-tree compile + board sweep
- rvcomp.py (own harness: build.ninja cflags + exact source path, cwd=shadow tree). Control tree0 (plain copy of
  source/): render_objects, action_vehicle, object_lights objects IDENTICAL to build/base (all sections except
  .debug$S which holds the /Fo path, raw relocations, raw symbol table incl. aux).
- treeF = tree0 + the 4 final files. render_objects vs my scratch split: 22/22 EXACT. vs production split: 21/22
  (_object_get_cached_render_lighting reloc-identity: name only) - as claimed.
- gate.py --source final/render_objects.c (PRODUCTION headers) vs build/split: 21/22, same single residual.
- Board sweep (sweep.py, 592 cl edges; 211 libs/ edges not in the shadow tree - libs do not include objects.h):
  381 raw-identical, 66 raw-different. rowcmp.py (section_infos_equal per January function and per owner-keyed
  section, base vs cand): ZERO row changes anywhere except render_objects _object_get_cached_render_lighting
  residual->EXACT (vs scratch split). Content changes only in: 7 literal users (actor_moving, ai_debug,
  hud_nav_points, motion_sensor, bipeds, vehicles + render_objects) = _object_get_bounding_sphere + literal swap;
  physics _physics_update_old and collisions _collision_move_point (both residual before and after);
  render_objects _object_get_cached_render_lighting. The other ~55 raw diffs are internal $L label renumbering only.

## V2 - admission checks on the candidate (mirror root mroot/: unmodified copies of object_audit/surplus_identity/
## provider_link/pdb_storage + coff_compare; build/split = my split_all; build/base = build/base + treeF objects)
- object_audit render_objects: PASS, 41/41 January symbols; .bss 1150/1150 owned by ?lighting_storage@... at 0.
- surplus_identity: 9 candidate-only code COMDATs, 0 not identical (_object_get_bounding_sphere IDENTICAL to action_vehicle).
- provider_link (candidate render_objects x candidate action_vehicle + base providers): PASS, 26 symbols both orders.
- pdb_storage: 41 split symbols, 0 disagreements.
- Controls in the REAL tree: provider_link base render_objects -> FAIL LNK2005 _object_get_bounding_sphere both orders;
  candidate render_objects x PRODUCTION action_vehicle -> same FAIL. => action_vehicle patch is mandatory, packet atomic.
- action_vehicle with packet: object_audit PASS, provider_link PASS; only raw change = COMDAT selection 1 (NODUP) -> 2 (ANY)
  for _object_get_bounding_sphere (comdat_sel.py). The 7 other users pair-link PASS with it (--symbols=_object_get_bounding_sphere).
- objdiff 3.3.1 mini project (proj/): production AND candidate both code 5437/5437, data 4678/4678, .bss/.data/.rdata/.text
  100% => the packet moves NO objdiff bytes for render_objects; the gain is the strict object admission only.
- HCEX.pdb (my DIA2Dump run, hcex_ogcrl.txt): `Data: static, [016A13B0], Static Local, Type: struct render_lighting,
  lighting_storage` inside object_get_cached_render_lighting; -sym lighting_storage finds no global; render_object_globals
  is `File Static`. cachebeta publics: no lighting_storage; January relocations to it only from
  _object_get_cached_render_lighting (3 DIR32); no January/base object references _lighting_storage from outside.
- fake_match_scan on the 4 final files: 0 leads (base also 0).
- Interaction probe (lab): collisions 'broad route' (current real_math.h with valid_real_plane3d inline after
  valid_real_normal3d, prototype removed; production collisions.c) -> collisions 20/20 WITHOUT and WITH the packet.
  The packet's literal-count shift does not break the held January-like collisions closure.

## V3 - strip tests, controls, objdiff sweep
- ST1 production objects.h + final render_objects.c (gate.py obj): surplus _object_get_bounding_sphere DIFFERENT
  (c:\ literal) -> objects.h edit is load-bearing for Rule 6 identity (it selects a literal VALUE, not codegen).
- ST2 treeF with production object_lights.c (no define): object_lights 43/43 but object_audit MISSING January-owned
  33-byte c:\ literal -> the per-TU define preserves the status quo of object_lights data.
- Control tree0 compiles of shaders/units/collisions IDENTICAL to base -> the $L renumbering is packet-caused.
- objdiff 3.3.1 over the 66 raw-changed units (proj_sweep/): matched code/data/functions unchanged everywhere;
  fuzzy only: collisions 99.54777 -> 99.541794, physics 87.95672 -> 87.95467 (residual functions, not exactness).
- Warnings unchanged (render_objects 0/0, action_vehicle 0/0, object_lights 1/1 pre-existing C4133).
- VERDICT: APPROVE render_objects as OBJECT_COMPLETE_CANDIDATE conditional on atomic application of all 5 patches +
  csplit re-run. REVIEW.md written.
