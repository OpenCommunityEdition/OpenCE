# A4 ledger (wave 3): rasterizer_xbox + rasterizer_xbox_dynavobgeom admission

Started 2026-09-26 00:32:46 -0700. Worktree C:\halo-worktrees\claude-compiler-application-20260925, HEAD 455dffad.
Rules: AGENT_BRIEF_v2.md + HALO_HOUSE_RULES_20260926.md. Patch-only; config/headers lead-owned.

## Baseline (measured 2026-09-26 00:32:46 -0700)
- rasterizer_xbox: gate 95/95 EXACT; object_audit FAIL(22) = 1 section (.rdata _framebuffer_blend_function_states
  align 3/4) + 21 symbol storage rows (split 2 / ours 3). pdb_storage: 25 disagreements (the 21 + 4 split-2/ours-2
  non-publics: _d3d_palette, _rasterizer_filthy_bitmap_default_initialize, _rasterizer_state_cache,
  _rasterizer_xbox_d3d_globals). Logs: logs/object_audit_rasterizer_xbox_base.txt, logs/pdb_storage_rasterizer_xbox_base.txt
- rasterizer_xbox_dynavobgeom: gate 17/17 EXACT; REJECTED (candidate-only-comdat-owner).

## History read (do not repeat)
- fifty-objects wave1/wave2 (research/fifty_objects_20260925/results/wave{1,2}/*rasterizer_xbox*): full packet
  (23 .bss statics, HCEX static-local blend tables, direct D3DDevice_ flicker calls, static filthy init renamed plural,
  20 static wrapper rows). Reviewer: technically verified; blocked ONLY on owner ruling on 9 descriptive .bss names.
- Owner queue item 2 (docs/object_matching_logs/claude_fifty_objects_20260925_owner_queue.md): unanswered.
- Owner 2026-09-15: prefers no descriptive/invented static names in symbols.json (env_fog/models held).
- dynavobgeom: fifty-objects wave3 APPROVED (amended packet); canonical 641e1466 kept the rejection (SDK-table
  coalescing policy + consumer-local global_window_parameters). Profile rejection: moving the extern to
  rasterizer_xbox_internal.h perturbed parked __rasterizer_model_draw by one instruction.
- January _global_window_parameters is a pooled COMMON record (source/linker_common, 600 B).

## Findings
- [2026-09-26 00:42:13 -0700] F-D1 (dynavobgeom, NEW): object_audit at HEAD is FAIL(2), not the 31/31 the rejection text records.
  Cause: merge artifact of canonical f6d00a8c. Parent dbcea3d6 had the global flag
  'boolean reported_too_many_transparent_geometry_groups = FALSE;' + its uses; donor 4e84c212 replaced it with the
  HCEX static local 'static boolean warned = FALSE;'. The merge kept BOTH: the global line (240) is now dead (no use
  in the TU; models.c uses its own static local_reported_...), emitted as a candidate-only EXTERNAL 1-byte .bss
  definition at +0, pushing ?warned@... to +1 (January: .bss 1 byte, warned at +0). No cachebeta public, no January
  split definer. Fix = delete the dead line (unit-local; no header, no config).
- [2026-09-26 00:42:13 -0700] F-X1: no raw 2001 linker map on disk (grep 'Preferred load address' negative); atlas is function-names only;
  HCEX rasterizer_dx9 compiland statics = previous_stencil_mode, use_fullscreen, safe_video_mode, flip_status,
  InsideScene, anisoFilterMap, static locals srcblend/destblend/blendop_table (src<dest<op addresses) and three
  'union point2d dimensions'. => NO new first-party evidence for the 9 descriptive .bss names.
- [2026-09-26 00:42:13 -0700] F-W1: HCEX global_window_parameters = Global 'struct rasterizer_window_begin_parameters' at 0x18F0C40,
  contribution '* Linker *' 0x258 (COMMON in HCEX too). January pool neighbours: _texture_table,
  _texturestagestate_table, _renderstate_table, _global_d3d_caps, _pixel_shader, _global_window_parameters,
  _global_frame_parameters. rasterizer.h defines struct rasterizer_window_begin_parameters and already carries
  'extern struct rasterizer_frame_begin_parameters global_frame_parameters;' in its globals block.
- [2026-09-26 00:54:35 -0700] F-S1 (SDK tables / helper data, criterion (b)): XDK D3D8.h:325-367 defines D3DPRIMITIVETOVERTEXCOUNT,
  D3DSIMPLERENDERSTATEENCODE, D3DTEXTUREDIRECTENCODE unconditionally at file scope as 'extern CONST
  DECLSPEC_SELECTANY' ahead of the D3DINLINE bodies (D3D8.h:21 D3DINLINE = static __forceinline). January's
  dynavobgeom split contains those D3DINLINE bodies (_D3DDevice_SetRenderState 432 B etc.), so January's TU compiled
  D3D8.h and (same compiler) emitted the same three SELECT_ANY COMDATs, later discarded at link (split cannot show
  per-object COMDATs: comdat-folding-invisible).
  surplus_all.py on D1: 19 candidate-only external definitions, all selection 2 (SELECT_ANY), all section_infos_equal
  + flags equal to January's selected copies. definer_census.py over build/base (621 objects): each SDK table 134
  definers all SELECT_ANY+identical; real_alpha_to_pixel32 6/6, real_argb_color_to_pixel32 3/3, dot_product3d 74/74
  identical; vector_from_points3d 78/79 identical - CROSS-UNIT NOTE: breakable_surfaces (/Ow /QIfist) emits a
  DIFFERENT SELECT_ANY _vector_from_points3d body (normalized sha a947d351 vs January 905f56db); not an A4 unit.
- [2026-09-26 01:08:46 -0700] F-R2 (rasterizer_xbox source-review debt beyond the 22 findings, NOT in the prior packet): TU-local views
  struct rasterizer_debug_options (reserved spans; genuine struct rasterizer_debug_options_definition exists in
  rasterizer_debug_options.h), rasterizer_lights_globals_prefix (prefix view of rasterizer_lights, no header),
  window_globals_prefix (opaque reserved00[8]; no header), rasterizer_xbox_rasterizer_globals (reserved-span view of
  rasterizer_globals accessed through casting macros); consumer-local data externs: global_window_parameters,
  pixel_shader (rasterizer_xbox.h already declares it), global_vector_palette (bitmaps_internal.h declares it),
  global_d3d_caps, renderstate_table, texturestagestate_table, texture_table (no header).
- [2026-09-26 01:23:21 -0700] F-W2 criterion (a) verdict: view reconciliation (W0) is zero-cost and ready (lead-owned, 10 files of other
  units). The single owner declaration costs exact code under every tested placement: rasterizer.h (W1) loses
  _rasterizer_frame_statistics_draw 4176 B (commutative operand-order tie) + parked model_draw drift;
  rasterizer_xbox_internal.h (W2) loses 480 B + 3888 B + drift. => OWNER QUESTION (rule 62 exception needed).
- [2026-09-26 01:28:29 -0700] F-P1 (policy consistency): 15 of the 23 global_window_parameters consumers are already Matching with a
  consumer-local extern (rasterizer, rasterizer_debug, rasterizer_text, rasterizer_transparent_geometry,
  rasterizer_frame_statistics is not a consumer but is Matching, xbox_debug, decals, motion_sensor, shadows, text,
  widgets, xbox_transparent_geometry, active_camouflage, draw_primitives, water, screen_effect); 8 of them declare it
  through INCOMPATIBLE partial views, while dynavobgeom's own extern already has the genuine complete type. Same
  inconsistency class as the SDK tables (134 definers, ~54 Matching emitters). W1 would ALSO un-match Matching
  rasterizer_frame_statistics (-4176 B exact) => net 0 objects for a uniform application.
- [2026-09-26 01:28:29 -0700] F-R3 rasterizer_xbox remaining type debt (family-wide, lead/owner level): rasterizer_lights_globals_prefix is
  HCEX's complete 'struct rasterizer_lights {long light_count; lights[0x80]}' under a local name (MAXIMUM_RENDERED_LIGHTS
  = 128), genuine type TU-local in rasterizer_lights.c/environment.c; window_globals_prefix vs HCEX 'struct window_data
  {hInstance, hWnd, hWndPresentTarget +8, nCmdShow, MainWndProc, class_name[0x40], window_title[0x40]}' (no owner in
  tree); rasterizer_xbox.h prototypes rasterizer_set_model_skinning with struct rasterizer_model_skinning_parameters,
  defined as TU-local copies in 5 .c files (HCEX: struct render_skinning {real_matrix4x3 *node_matrices; short
  node_matrix_count}); rasterizer_xbox_rasterizer_globals cast view of rasterizer.h's incomplete
  rasterizer_globals_definition; consumer-local externs of pooled COMMON globals global_d3d_caps, renderstate_table,
  texturestagestate_table, texture_table, global_window_parameters; global_vector_palette (owner header include flips
  __rasterizer_initialize).

## Final state (2026-09-26 01:38:04 -0700; HEAD 09f5208f, re-verified)
- Unit patches: rasterizer_xbox_dynavobgeom.patch (D1), rasterizer_xbox.patch (R-A + R-B).
- Lead-owned: LEAD_RA_symbols.json.patch, LEAD_RA_rasterizer_xbox_internal.h.patch, LEAD_RA_parked.json.patch,
  LEAD_W0_window_views.patch. Owner-gated: OWNER_W1_window_owner_decl.patch, OWNER_RC_rasterizer_xbox.c.patch,
  OWNER_RC_symbols.json.patch. Conditional: CONDITIONAL_dynavobgeom_retire_rejection.patch,
  CONDITIONAL_dynavobgeom_config_matching.patch. Docs: OWNER_QUESTIONS.md, LEAD_PROPOSALS.md.
- Batteries: logs/battery_D1_dynavobgeom.txt, logs/battery_RARB_rasterizer_xbox.txt,
  logs/battery_RC_rasterizer_xbox_OWNER.txt, logs/W0_w3_fake.txt; sweeps scratch/campaign/workers/A4/sweep_*/sweep.tsv;
  link probes scratch/campaign/workers/A4/link_probe_{D1,units}/.
- Object effect of stage 1: 0 objects, 0 new exact code bytes, 0 exact losses; both units object_audit PASS.
  dynavobgeom becomes admissible only on owner answers OQ-2 (A or B) + OQ-3; rasterizer_xbox needs OQ-1 + OQ-2 + OQ-3
  + OQ-4.
- Failed predictions: W1 (predicted zero exact loss; lost _rasterizer_frame_statistics_draw); W2 (two losses);
  R-B as a whole (bitmaps_internal.h include flips __rasterizer_initialize).
