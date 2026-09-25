# REVIEW - source/objects/object_lights (r2w2_object_lights, ADMISSION_FIX_PROPOSED)

Reviewer slug: scratch/w/review_r2w2_object_lights (claim review_r2w2_object_lights_rv1, released at end).
Tree: HEAD 434f0151 (= 05255584 + lane ledger), clean except the pre-existing untracked research dir.

## Verdict: APPROVE (01 + 02 + csplit regen, then 03_NO_ALT). No amendment needed.

Credit: 0 new code bytes; +1,541 data (03_NO_ALT, allow_incomplete_unit). Object stays NonMatching (held Q1).

## What I re-measured independently (own tools, own csplit, own shadow tree)

| check | result |
|---|---|
| git apply --check at HEAD | 01, 02, 03 each CLEAN; all three together CLEAN; index lines = HEAD blobs; LF |
| post-images | byte-identical to the worker's tree/ and cfg_OL/ |
| csplit emulation | verbatim config == build/split 833/833; with 02 only object_lights.obj changes |
| gate (cand vs emulated split) | 43/43 EXACT, 12,608 padded (shadow compile, build.ninja cflags); gate.py --source 43/43, same object |
| objcmp prod vs cand | all 109 sections identical (except .debug$S path); only 4 symbol storage rows 2 -> 3 |
| object_audit | cand vs split_OL PASS, 71 symbols, 0 differ; prod vs split_OL FAIL(4); cand vs build/split FAIL(4) |
| pdb_storage | production 4 disagreements = exactly the 4 helpers; candidate storage = PDB truth |
| outside references | 0 UNDEF references to the 4 names in all build/split and build/base objects |
| surplus_identity | 12 COMDATs, 11 IDENTICAL; _object_get_bounding_sphere DIFFERENT (pre-existing held Q1) |
| provider_link | 33 surplus names PASS in both orders |
| header consumers | render 13/13, render_objects 22/22, objects identical to build/base |
| data | _lights_section resolved infos EQUAL (both symbol maps); 1541 / 1 reloc / sha 527b43a1...f71 = entry |
| verifier | +1541 (2645/2645) on build/report.json and on a mini objdiff 3.3.1 cand report; no opt-in -> fail-closed |
| admission audit | 9/0/2/0 -> 10/0/2/0 (disclosed side effect) |
| fake_match_scan | 0 leads; /W3 and /W4 warning sets identical to production |

## Strip tests
- Calls removed (static helper kept): 42/43, `_should_render_lights` UNWRITTEN. The calls are load-bearing for a
  January-owned section AND /Od-attested (0x78dab0, only callers diffuse 0x78c6c0 / specular 0x78c950, each right
  after its *_lights_begin call). They replace a hand-expanded helper (rule 13). Not decoration.
- Static prototype removed from the .c block: 43/43, object identical. The prototype is byte-inert (not a filler);
  it keeps the name's prior declaration position and follows rule 9. Acceptable either way.

## Precedent audit (read the original commits)
- P1 storage per cachebeta publics: 05255584 (12 hs_runtime converters static, object still blocked), marker_cluster
  2026-08-31 correction, units / ui_widget_event_handler_functions. All conditions hold. The earlier "external" claims
  (3bfde7bd / 20260903 reconciliation, 20260829 light_attenuation ledger) read csplit's default class for a `_code_`
  placeholder; 3bfde7bd only renamed `_code_00128a10`. Not independent January evidence.
- NEW first-party corroboration (not in the worker packet): in the /Od build, in-TU calls to January-public functions
  go through incremental-link thunks (light_disconnect_from_map, light_reconnect_to_map), while every direct in-TU
  call/address targets a January static. All four helpers are direct: should_render_lights 0x78dab0,
  light_attenuation 0x787b80 (called from 0x78798e), cluster_get_first_light 0x787810 and cluster_get_next_light
  0x787840 (pushed as direct addresses in lights_preprocess_scene). HCEX marks the cluster pair static.
- P2 allow_incomplete_unit single-section data entry: leaf_map ae12a3c1, editor_flying_camera 5019c186, objects
  84414a1a, race/CTF 20260902 ("established CTF policy"). Single symbol, pinned measurements, sole unmatched data,
  re-verified on every report. The round-1 owner queue s4 lists 01+02 (+ optional 03_NO_ALT) as the landing on the
  NO path, so it is independent of Q1. Not stretched.

## Integrator notes
- Land 01 and 02 together (then csplit regen); 03_NO_ALT after them (its reason text states their result).
- If the owner later answers Q1 YES: replace 03_NO_ALT with round-1 03_YES and add 04_YES.
- Optional doc hygiene (not required): the 20260903 reconciliation and 20260829 light_attenuation ledgers call these
  helpers "external"; a one-line correction note pointing at this packet would keep the ledger set consistent.
- 02 stacks textually with the in-flight symbols patches of rasterizer_xbox_debug, vehicles, weapons and
  weather_particle_systems.
