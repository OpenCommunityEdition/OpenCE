# source/rasterizer/xbox/rasterizer_xbox_dynavobgeom

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\admission_packet.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\rasterizer_xbox_dynavobgeom.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\config.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\object_admission_rejections.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\audit_final_regen.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\gate_final_regen.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\surplus_final.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\provider_link_final.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\od_unlit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\dynavobgeom\\hcex_compiland.txt"
]

## production_changes
Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\dynavobgeom\admission_packet.patch. It concatenates the four patches below. Each passes `git apply --check` at 954eebd2, and the .c post-image is cmp-equal to cand_final.c.

(1) rasterizer_xbox_dynavobgeom.c.patch (.c only, no header touched):
- Add #include "rasterizer/rasterizer_debug_options.h", "rasterizer/rasterizer_model_types.h" and "rasterizer_xbox_pixel_shader.h".
- Delete the caller-local views: struct rasterizer_dynamic_geometry_debug_options, struct rasterizer_dynamic_geometry_window_parameters, the local struct pixel_shader_definition copy (and its size assert), and struct shader_effect_sort_bias_prefix.
- Globals: `extern struct rasterizer_window_begin_parameters global_window_parameters;` (the genuine rasterizer.h type). The redundant pixel_shader extern goes (rasterizer_xbox.h declares it). The file-scope latch `boolean reported_too_many_transparent_geometry_groups = FALSE;` is deleted.
- Fields and enums: rasterizer_debug_options.dynamic_screen_geometry, .dynamic_unlit_geometry and .stats. The enum `_rasterizer_statistics_mode_enabled` is renamed `_rasterizer_statistics_mode_geometry` (=2). SHADER_GET_EFFECT now casts to the genuine `struct shader_effect_definition`.
- The transparent group's opaque `short effect_type; byte reserved16[0x26];` becomes `struct render_model_effect effect;`, and the store is `group->effect.type = _render_model_effect_type_none;` (new TU enum, =0).
- `_rasterizer_dynamic_unlit_geometry_draw`:
  - Uses the /Od-attested helper calls `vector_from_points3d(&global_window_parameters.camera.position, centroid, &forward);` and `group->z_sort = -dot_product3d(&global_window_parameters.camera.forward, &forward);`. These replace the hand-expanded subtraction and dot product.
  - Local names come from HCEX and the /Od RTC records: `real_vector3d forward; real_plane3d zero_plane;`.
  - HCEX's function-static latch is `static boolean warned = FALSE;`, declared after the automatic locals.
- The header symbol-list comment line becomes `?warned@?1??_rasterizer_dynamic_unlit_geometry_draw@@9@9`.

(2) symbols.json.patch: 10 lines edited in place, CRLF kept, no re-serialisation. The full copy is scratch/w/dynavobgeom/config/symbols.json.
- Line 23161 becomes `{ "file_offset": 4610582, "flags": 0, "name": "?warned@?1??_rasterizer_dynamic_unlit_geometry_draw@@9@9", "static": true },`.
- `, "static": true` is appended to lines 5705, 5706, 5711, 5713, 5714, 5715, 5716, 5717 and 5718. These are the nine D3DINLINE wrappers at 1370176, 1370608, 1370992, 1371584, 1371680, 1371696, 1371728, 1371744 and 1371760.

(3) config.json.patch: line 497 of rasterizer_xbox_dynavobgeom.c goes from "NonMatching" to "Matching".

(4) object_admission_rejections.json.patch: removes the `struct rasterizer_dynamic_geometry_debug_options` source-layout-incomplete entry.

The .c patch and the symbols.json rows must land together. Against the current split the unlit draw is reloc-identity only; against the regenerated split it is EXACT. Then regenerate the split: csplit changes ONLY rasterizer_xbox_dynavobgeom.obj. Run the full ninja and stable sweep. No shared header changed, so there is no consumer list.

## evidence
Checks, all run on cand_final.obj (the regenerated split is csplit run with the slug's config):
- gate_r against the regenerated split: 17/17 EXACT. object_audit (audit_r) against the regenerated split: 31/31 January sections ok, 31 January symbols with 0 differences, OBJECT AUDIT: PASS.
- Against the unmodified split: 16/17. The only residual is `__rasterizer_dynamic_unlit_geometry_draw` [reloc-identity], which is the latch-rename naming gap.
- The regenerated split was byte-compared with build/split: only dynavobgeom.obj differs.
- Storage agrees with the cachebeta publics: regenerated split 18 symbols and candidate 25 symbols, 0 disagreements.

Why each change:
- Latch storage:
  - cachebeta has no public at 0x00465A16, so January's latch was file-static. The 0904 audit's external storage came from the symbols.json default.
  - HCEX rasterizer_dx9_dynavobgeom has `Static Local unsigned char warned` in this function. The /Od build has the static byte at 0xe0d37c.
- D3D wrappers: the nine wrappers are absent from the publics. This is the same fix class as b9a8d587's wrapper rows.
- Helper calls in the unlit draw:
  - /Od 0x7d82d0 calls vector_from_points3d (0x42e3a0) with camera.position, centroid and &forward before new_group.
  - It calls dot_product3d (0x42dd80) with camera.forward and &forward, then negates.
  - Its RTC records name `forward` (12 bytes) and `zero_plane` (16 bytes).
  - The assert-line deltas (4, 2) match January's 38/42/44.
- Window global: slot 65 of linker_common (600 bytes) sits between _pixel_shader and _global_frame_parameters, so its owner is rasterizer_xbox.c.
- Byte-inertness: every step except the latch rename is IDENTICAL under objeq to the previous step (cand1 through cand6). The shared-type swaps use the same offsets: shader_effect_definition.flags at 0x28, render_model_effect.type at 0x14.

Surplus (surplus_all, problems 0):
- 19 candidate-only external definitions, all IDENTICAL (raw bytes and section flags) to January's selected copies:
  - code COMDATs: _real_alpha_to_pixel32 (active_camouflage), _real_argb_color_to_pixel32 (hud_draw), _dot_product3d (action_charge), _vector_from_points3d (action_alert)
  - SDK tables: D3DPRIMITIVETOVERTEXCOUNT 88, D3DSIMPLERENDERSTATEENCODE 328, D3DTEXTUREDIRECTENCODE 16
  - 7 string literals and 5 __real constants
- provider_link: 19/19 PASS in both link orders; SELECTED-PROVIDER LINK: PASS.
- nodup_census reports no NODUP definer for any of the four code COMDATs or the three tables.

Folded-inline COMDAT exception for real_alpha_to_pixel32:
- January's own split object has UNDEF references to _real_alpha_to_pixel32, _real_argb_color_to_pixel32 and _D3DSIMPLERENDERSTATEENCODE. Its exact quad draw calls the first two out of line (@05f1 and 7 sites).
- So January's translation unit emitted these COMDATs and the linker folded them into the selected copies.
- The calling function is strictly exact, the copy is identical, and both link orders pass.

The two unreferenced SDK tables are D3D8.h D3DCONST data that every D3D8.h translation unit emits (133 definers). This matches the collision_debug precedent (b9a8d587).

Other checks:
- objdiff 3.3.1 one-unit report: data 525/525; code 2159/5112 with 16/17 functions, the same figures as production build/report.json (the quad-draw scorer under-credit already present there).
- fake_match_scan: 0 leads.
- /W3: no warnings in the unit; 12 warnings in shared headers, the same count as the unchanged base.

## blockers
None found inside the unit. Disclosed shared-owner debt that is not a caller-local opaque view:

(a) global_window_parameters remains a consumer-local extern. It uses the genuine complete type, as the Matching active_camouflage, draw_primitives, screen_effect, transparent_geometry and water units already do.
- Moving the extern into the owner header rasterizer_xbox.h would fail to compile (C2371) in seven includers that still use partial views: rasterizer.c, rasterizer_lights.c, rasterizer_xbox_debug.c, rasterizer_xbox_decals.c, rasterizer_xbox_lights.c, rasterizer_xbox_shadows.c and rasterizer_xbox_widgets.c.
- Three non-includers also keep their own views: rasterizer_debug.c, rasterizer_xbox_motion_sensor.c and rasterizer_xbox_text.c.
- The rasterizer_xbox_profile ledger treats this as a separate global-owner migration.

(b) struct transparent_geometry_group stays local to the file. No shared definition exists: eight other files define it locally, and the owner's own copy is opaque. After this patch it has no opaque span.

(c) struct rasterizer_meter_parameters is a complete local copy, identical to the one in hud_draw.c.

A reviewer or owner who wants (a) resolved before admission would turn this into a multi-file packet.

## reopen_criteria
If a reviewer rejects this, reopen for either of these:
(1) the global_window_parameters owner migration: convert the 10 view consumers listed under blockers to struct rasterizer_window_begin_parameters, then add the extern to rasterizer_xbox.h next to pixel_shader, with a full consumer sweep;
(2) first-party naming for the /Od-attested named effect-shader local ([ebp-0x3c] in /Od 0x7d82d0), which would replace the local SHADER_GET_EFFECT macro. That change would be cosmetic; the bytes are already exact.

## task notes
- **The rejection text is partly stale.** The frame-statistics view was already replaced by the shared rasterizer_frame_statistics.h owner in a34d984c.
- **Three blockers not named in the rejection are also fixed.** The object could not otherwise be admitted:
  - the hand-expanded vector_from_points3d / dot_product3d in the unlit draw (wave-2 law g; /Od 0x7d82d0 calls both);
  - the shader_effect prefix view;
  - the latch storage: January had it static (no cachebeta public), and HCEX makes it a function-static named `warned`.
- **Latch naming follows a precedent.** The function-static symbols.json name follows render_objects' `?lighting_storage@?1??object_get_cached_render_lighting@@9@9` (b807f9f6).
- **Integrator order.**
  1. Apply the .c and symbols.json patches together.
  2. Regenerate the split. csplit changes only this object: the latch name and storage, the relocation target name in the unlit draw, and the storage of the nine wrappers.
  3. Run a full ninja and the stable sweep. Expect +0 functions and 0 regressions: the unit stays 17/17 and gains two identical SELECT_ANY helpers (_dot_product3d, _vector_from_points3d).
  4. Flip config.json and retire the rejection.
- **Scorer-only effect, not counted.** objdiff 3.3.1 under-credits the psuedo quad draw at 97.74% in production today. Flipping the status to Matching may add scorer-only $L-label credit.
- **No held item was re-probed. No tracked file was edited.**
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\dynavobgeom\LEDGER.md
