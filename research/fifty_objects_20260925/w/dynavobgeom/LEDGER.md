# dynavobgeom whole-object admission (wave 3) - LEDGER

Unit: source/rasterizer/xbox/rasterizer_xbox_dynavobgeom (17/17 exact at 954eebd2).
Rejection: config/object_admission_rejections.json `struct rasterizer_dynamic_geometry_debug_options`
(source-layout-incomplete): debug-options + frame-statistics caller-local views, window-global partial
type, surplus `_real_alpha_to_pixel32`, stock SDK tables.

## Read before probing
- WORKER_BRIEF 0-8, house rules, lane log (held items), 3 dynavobgeom ledgers (0821 trio, 0903 opus
  reconciliation, 0904 bss owner audit), rejection entry. No parked.json entry for the unit.
- State at 954eebd2: frame statistics ALREADY come from the shared owner header
  rasterizer/rasterizer_frame_statistics.h (struct rasterizer_frame_statistics_globals, extern
  rasterizer_frame_statistics) - no caller-local frame-stat view remains in the .c.
  Remaining views: `struct rasterizer_dynamic_geometry_debug_options` (opaque reserved00[2],
  reserved04[0x1C], reserved23) and `struct rasterizer_dynamic_geometry_window_parameters` (pad06 + camera).
- Genuine owners: rasterizer/rasterizer_debug_options.h `struct rasterizer_debug_options_definition`
  + `extern ... rasterizer_debug_options` (fields stats@0x02, dynamic_unlit_geometry@0x20,
  dynamic_lit_geometry@0x21, dynamic_screen_geometry@0x22); rasterizer.h
  `struct rasterizer_window_begin_parameters` (rasterizer_target, window_index, has_mirror,
  suppress_clear, camera, frustum, fog, screen_flash, screen_effect).

## Probes

### P0 baseline (base.c = production copy)
- gate: 17/17 EXACT. objeq base.obj == build/base production obj (49 sections IDENTICAL).
- object_audit: all January sections ok; 9 symbol DIFFs = the 9 D3DINLINE wrappers storage split 2 / ours 3.
  pdb_storage: the 9 wrappers are absent from cachebeta publics (=> file-static; ours already 3; symbols.json lacks
  "static": true - same class as the b9a8d587 wrapper rows). ALSO `_reported_too_many_transparent_geometry_groups`
  split 2 / ours 2 / PDB-public False: no cachebeta public at 0x00465A16 => January storage is STATIC, the 0904
  audit's "external" was the symbols.json default, not January truth.
- HCEX (DIA2Dump -compiland rasterizer_dx9_dynavobgeom.obj -> hcex_compiland.txt): in
  `_rasterizer_dynamic_unlit_geometry_draw`: `Static Local, Type: unsigned char, warned` and
  `Local, Type: struct real_plane3d, zero_plane`. => the latch is HCEX's function-static `warned`.
- surplus (candidate-only): code COMDATs _real_alpha_to_pixel32 (112) + _real_argb_color_to_pixel32 (320)
  IDENTICAL to January's selected copies (active_camouflage / hud_draw); SDK tables D3DPRIMITIVETOVERTEXCOUNT (88),
  D3DSIMPLERENDERSTATEENCODE (328), D3DTEXTUREDIRECTENCODE (16); 7 literals; 5 __real. provider_link: 17/17 PASS
  both orders, SELECTED-PROVIDER LINK: PASS.
- January's own `__rasterizer_psuedo_dynamic_screen_quad_draw` REL32-calls `_real_alpha_to_pixel32` (@05f1) and
  `_real_argb_color_to_pixel32` (7 sites): January's TU referenced the header __inline out of line, so it emitted a
  COMDAT the linker folded into the selected copy (COMDAT folding invisible law).
- window global owner: linker_common COMMON slot 65 (_global_window_parameters, 600 B) sits between
  _pixel_shader (64) and _global_frame_parameters (66) after the D3D state tables/_global_d3d_caps => owner
  rasterizer_xbox.c (which assigns it in window_begin); its header rasterizer_xbox.h already hosts
  `extern struct pixel_shader_definition pixel_shader;`.

### P1 cand1: genuine shared types (hypothesis: type/field-name swaps with identical offsets are byte-inert;
risk = declared-name-count ties from the extra includes)
- + #include "rasterizer/rasterizer_debug_options.h" (struct rasterizer_debug_options_definition + extern);
  fields .dynamic_screen_geometry (0x22), .dynamic_unlit_geometry (0x20), .stats (0x02); local enum renamed
  `_rasterizer_statistics_mode_enabled` -> `_rasterizer_statistics_mode_geometry` (value 2 = geometry in
  rasterizer_frame_statistics.c's enum none/objects/geometry/profile/memory; same name used by decals/lights).
- + #include "rasterizer_xbox_pixel_shader.h" (the shared complete pixel_shader_definition, identical fields);
  local struct copy + size assert + redundant `extern ... pixel_shader` removed (rasterizer_xbox.h declares it).
- window view removed; `extern struct rasterizer_window_begin_parameters global_window_parameters;` (genuine
  complete rasterizer.h type; camera at 0x8 by natural alignment).
- RESULT: 17/17 EXACT; objeq cand1.obj == base.obj IDENTICAL (49 sections, symbol table equal).

### P2 cand2 = cand1 + HCEX static local latch (hypothesis: HCEX `Static Local unsigned char warned` +
cachebeta no-public at 0x465A16 => January had a function-static; storage/name change only, bytes inert)
- source: file-scope `boolean reported_too_many_transparent_geometry_groups = FALSE;` removed; in
  _rasterizer_dynamic_unlit_geometry_draw `static boolean warned = FALSE;` (first local) and the HCEX local name
  `zero_plane` for the zeroed plane (was `plane`). Header comment symbol list updated.
- vs CURRENT split: 16/17, unlit draw residual [reloc-identity] only (naming gap: ours `?warned@?1??...@@9@9` st 3).
- symbols.json (slug copy scratch/w/dynavobgeom/config/symbols.json, byte-level in-place, CRLF kept, 10 lines):
  line 23161 -> `"name": "?warned@?1??_rasterizer_dynamic_unlit_geometry_draw@@9@9", "static": true` and
  "static": true on the 9 D3DINLINE wrapper rows 5705, 5706, 5711, 5713-5718 (absent from cachebeta publics;
  same fix class as b9a8d587's wrapper rows).
- csplit regen (build/tools/csplit.exe -i cachebeta.exe -p slug/config -o slug/split): rc 0, 833 objs; byte cmp vs
  build/split: ONLY rasterizer_xbox_dynavobgeom.obj differs (latch rename+storage, reloc target name in the unlit
  draw, 9 wrapper storage classes).
- gate_r (GATE_SPLIT_ROOT=slug/split): 17/17 EXACT. audit_r: 31 January symbols, 0 differ, OBJECT AUDIT: PASS.
- surplus_all (every candidate-only external def vs every January split definer): 17 rows, problems 0; all
  IDENTICAL (section_infos_equal + raw + flags): 2 code COMDATs, 3 SDK tables, 7 literals, 5 __real.
- provider_link cand2.obj: 17/17 PASS (both orders each), SELECTED-PROVIDER LINK: PASS.
  nodup_census: _real_alpha_to_pixel32 (6 definers), _real_argb_color_to_pixel32 (3), the 3 SDK tables (133 each):
  NODUP [] for all.
- January's split object itself has UNDEF refs to _real_alpha_to_pixel32, _real_argb_color_to_pixel32,
  _D3DSIMPLERENDERSTATEENCODE and every surplus literal: its exact bodies used them out of line, i.e. January's TU
  emitted these COMDATs and the linker folded them into the selected copies. D3DPRIMITIVETOVERTEXCOUNT and
  D3DTEXTUREDIRECTENCODE are unreferenced D3D8.h `D3DCONST` tables (D3D8.h:327/367) every D3D8.h TU emits
  (133 base definers), exactly the collision_debug (b9a8d587) precedent.

### Scan for remaining caller-local views (wave-2 law g)
- `struct transparent_geometry_group` (TU-local, 0xA0): has an OPAQUE span `short effect_type; byte reserved16[0x26]`.
  No shared header defines the group (8 other TUs define it locally; the owner .c's own copy is opaque). BUT the
  0x14..0x3B span is the complete shared `struct render_model_effect` (rasterizer/rasterizer_model_types.h, 0x28,
  first field `short type`), which the admitted Matching active_camouflage uses in its group copy
  (`struct render_model_effect effect;`). -> P3.
- `struct shader_effect_sort_bias_prefix` { struct shader shader; word flags; }: prefix view of the effect shader
  (no opaque span); no shared shader_effect type exists in any header (grep) -> disclosed, unchanged.

### P3 cand3: transparent group opaque span -> shared `struct render_model_effect effect;`
(+ #include "rasterizer/rasterizer_model_types.h"; `group->effect.type = 0`). Hypothesis: identical layout
(0x14..0x3B, type at +0), byte-inert. RESULT: 17/17 (regen split); objeq cand3 == cand2 IDENTICAL.

### P4 latch placement / controls (all vs regen split, all objeq IDENTICAL to cand3, 17/17)
- cand4: `static boolean warned = FALSE;` declared AFTER the automatics (HCEX record order: zero_plane, then the
  static local warned). Chosen (bytes indifferent).
- ctl_uninit: `static boolean warned;` - identical (control only; `= FALSE` kept per repo style of every other
  one-shot report latch, e.g. environment `warned`, render_objects `reported_rendered_object_overflow`).
- cand4_enum: `group->effect.type = _render_model_effect_type_none;` with TU enum
  `_render_model_effect_type_none = 0` (the name active_camouflage/xbox_transparent_geometry use) - identical. Chosen.
- /W3 (warn_gate): base 12 / cand4_enum 12 warnings, all in cseries.h:45 and real_math.h C4244; ZERO in the TU.

### P5 /Od readout of the unlit draw (odbuild fn 0x7d82d0 -> od_unlit.txt; later dx9 port, shape evidence only)
- assert lines 0x84/0x88/0x8a = deltas 4,2 like January's 38/42/44; then (later-only "shader" assert) centroid assert.
- `call 0x42e3a0(0xdfb058 = camera.position, centroid, &[ebp-0x1c])` = vector_from_points3d BEFORE new_group;
  `call 0x42dd80(0xdfb064 = camera.forward, &[ebp-0x1c]); fstp; xorps sign` = z_sort = -dot_product3d(...).
  RTC names: [ebp-0x1c] size 12 `forward`, [ebp-0x34] size 16 `zero_plane`; static byte 0xe0d37c = the latch.
  => production's `relative_centroid.x = centroid->x - ...` x3 and `-(fk*rz + fj*ry + fi*rx)` are HAND-EXPANDED
  real_math.h helpers (wave-2 law g / rule 6).
- cand5 = cand4_enum + `real_vector3d forward;` + `vector_from_points3d(&global_window_parameters.camera.position,
  centroid, &forward);` + `group->z_sort = -dot_product3d(&global_window_parameters.camera.forward, &forward);`
  RESULT: 17/17 EXACT (regen split). New surplus COMDATs _dot_product3d (32) / _vector_from_points3d (48):
  IDENTICAL raw+flags to January's selected copies (action_charge / action_alert); provider_link --baseline
  cand4_enum: both PASS; nodup_census NODUP [] (72 / 77 definers). January's unlit draw has no out-of-line
  reference to them (all inlined) - same as collision_debug's magnitude3d/arccosine (admitted b9a8d587).

### P6 cand6: `struct shader_effect_sort_bias_prefix` view -> genuine shaders/shader_definitions.h
`struct shader_effect_definition` (already included; `unsigned short flags` at 0x28 = the /Od `movzx word [eax+0x28];
and 1`). RESULT: 17/17, objeq cand6 == cand5 IDENTICAL.

## FINAL candidate = scratch/w/dynavobgeom/cand_final.c (== cand6.c)
Admission chain (cand_final.obj; "regen" = csplit with slug config/symbols.json):
- gate vs CURRENT split: 16/17 (unlit draw [reloc-identity] = the latch rename; lands WITH symbols.json).
- gate_r vs regen split: 17/17 EXACT (gate_final_regen.txt); cand_final_r.obj objeq IDENTICAL to cand_final.obj.
- audit_r vs regen split: 31 January sections ok, 31 January symbols 0 differ, OBJECT AUDIT: PASS (audit_final_regen.txt).
- storage vs cachebeta publics: regen split 18 symbols / candidate 25 symbols, 0 disagreements.
- surplus_all: 19 candidate-only external defs, problems 0, all IDENTICAL raw+flags (surplus_final.txt):
  code _real_alpha_to_pixel32 112, _real_argb_color_to_pixel32 320, _dot_product3d 32, _vector_from_points3d 48;
  SDK D3DPRIMITIVETOVERTEXCOUNT 88, D3DSIMPLERENDERSTATEENCODE 328, D3DTEXTUREDIRECTENCODE 16; 7 ??_C literals; 5 __real.
- provider_link: 19/19 PASS (both orders each), SELECTED-PROVIDER LINK: PASS (provider_link_final.txt).
  nodup_census NODUP [] for all 4 code COMDATs and 3 tables.
- objdiff 3.3.1 one-unit (od_proj, cand_final.obj vs regen split): data 525/525 (.bss/.rdata 100), code 2159/5112,
  16/17 fns = IDENTICAL to production build/report.json (psuedo quad draw 97.74 objdiff under-credit, scorer-only;
  strict coff_compare EXACT).
- fake_match_scan: 0 leads. /W3: 0 TU warnings (12 shared-header warnings, same as base).
- Blast radius: .c-only edit (no header touched) + 10 symbols.json lines whose csplit regen changes ONLY
  rasterizer_xbox_dynavobgeom.obj.
Patches (git apply --check OK at 954eebd2; .c post-image cmp-equal to cand_final.c):
  rasterizer_xbox_dynavobgeom.c.patch, symbols.json.patch (+ full copy config/symbols.json), config.json.patch
  (status Matching), object_admission_rejections.json.patch (retire entry), admission_packet.patch (all four).

## Disclosed, not changed (reopen items; none is an opaque caller-local view of a shared global)
- `extern struct rasterizer_window_begin_parameters global_window_parameters;` stays a consumer-local extern of the
  GENUINE complete type (same as Matching active_camouflage, draw_primitives, screen_effect, transparent_geometry,
  water, rasterizer_text, rasterizer_transparent_geometry). Moving the extern into the owner header
  rasterizer_xbox.h (owner rasterizer_xbox.c by COMMON slot order) is a separate global-owner migration: it breaks
  (C2371, different tags) the rasterizer_xbox.h includers rasterizer.c, rasterizer_lights.c, rasterizer_xbox_debug.c,
  rasterizer_xbox_decals.c, rasterizer_xbox_lights.c, rasterizer_xbox_shadows.c, rasterizer_xbox_widgets.c, which
  still use partial views; non-includers rasterizer_debug.c, rasterizer_xbox_motion_sensor.c, rasterizer_xbox_text.c
  also keep views.
- `struct transparent_geometry_group` stays TU-local (no shared definition exists; 8 other TUs define it locally and
  the owner's own copy is opaque); after P3 it has no opaque span (only natural-alignment pads pad12/pad66/pad9E).
- `struct rasterizer_meter_parameters` is complete and TU-local (identical copy in hud_draw.c; rasterizer.h types
  the field as void *). TU-local enums (_shader_type_effect, blend functions, stats mode) have no shared owner.
- /Od dx9 port names for the quad-draw locals (vsh_constants__screenproj[2], psh_constants, tint_colors,
  geometry_offset) are a different platform body; not applied.
- /Od readouts NOT applied (production spelling already exact; no first-party name): (a) the dx9 /Od stores the
  shader_get_and_verify_type result to [ebp-0x3c] and reloads it (a named local; models.c's inline cast
  expression at /Od 0x7042f7 has no spill) - name unrecoverable (not address-taken, HCEX lists only zero_plane),
  so the TU macro SHADER_GET_EFFECT (now on the genuine struct shader_effect_definition) is kept; (b) the /Od
  largest-triangle update goes through a ?: temp ([ebp-0x44], MAX-like), January's bytes show a conditional store
  (`jle; mov [largest], eax`) that production's `if` reproduces exactly.
- Stop rule: nothing residual; all remaining items are board-wide shared-owner debt outside this unit.
