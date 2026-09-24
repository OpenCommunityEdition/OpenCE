# REVIEW3 rasterizer_xbox_dynavobgeom (adversarial review of OBJECT_COMPLETE_CANDIDATE) - LEDGER

Base 954eebd2. Worker slug scratch/w/dynavobgeom (cand_final.c).

## R0 patch mechanics
- base.c == working tree .c (cmp). All 4 patches + admission_packet.patch: git apply --check OK at current tree.
- Applied admission_packet.patch to a copy tree (applytree/): .c post-image cmp == cand_final.c;
  symbols.json post-image cmp == worker's config/symbols.json; symbols.json diff = 10 lines changed.

## R1 independent gates (cand.c = copy of worker cand_final.c)
- gate.py vs CURRENT split: 16/17; `__rasterizer_dynamic_unlit_geometry_draw` residual [reloc-identity] (latch rename).
- base (tree .c) gate: 17/17.
- csplit regen with patched symbols.json+config.json -> R/split: 833 objs, byte-cmp vs build/split: ONLY
  rasterizer_xbox_dynavobgeom.obj differs. Control: csplit with unmodified config == build/split (0 diffs).
- gate_r (GATE_SPLIT_ROOT=R/split): 17/17 EXACT.
- audit_r (AUDIT_SPLIT_ROOT=R/split): 31 sections ok, 31 January symbols 0 differ, OBJECT AUDIT: PASS.
  Surplus: 4 code COMDATs (_dot_product3d 32, _real_alpha_to_pixel32 112, _real_argb_color_to_pixel32 320,
  _vector_from_points3d 48), 3 SDK tables, 7 literals, 5 __real, .drectve.
- surplus_check.py (own script: every candidate-only EXTERNAL def vs every January split definer, section_infos_equal +
  raw bytes + section flags): 19 surplus, 19 IDENTICAL, problems 0. January object UNDEF-references
  _real_alpha_to_pixel32, _real_argb_color_to_pixel32, _D3DSIMPLERENDERSTATEENCODE, 3 __real, 2 literals; NOT
  _dot_product3d / _vector_from_points3d / D3DPRIMITIVETOVERTEXCOUNT / D3DTEXTUREDIRECTENCODE / 5 helper-assert literals.
- provider_link cand_r.obj: 19/19 PASS, SELECTED-PROVIDER LINK: PASS.

## R2 evidence verification
- cachebeta publics BY ADDRESS (file_offset == RVA here; 50/50 nearby data publics have delta 0): no public in
  0x465980..0x465B00 (latch 0x465A16 static); none at the 9 wrapper offsets; control: 0x14EE80 has
  __rasterizer_dynamic_unlit_geometry_draw. (pdb_storage.py matches by NAME only, so the worker's "no public at
  0x465A16" needed this address check - confirmed.)
- HCEX hcex_compiland.txt: _rasterizer_dynamic_unlit_geometry_draw locals = zero_plane (struct real_plane3d, frame),
  `Static Local unsigned char warned`; forward/group absent (enregistered on PPC) - consistent.
- /Od 0x7d82d0 (od_unlit.txt): vector_from_points3d(0xdfb058, centroid, &[ebp-0x1c]) before new_group;
  dot_product3d(0xdfb064, &[ebp-0x1c]) -> fstp -> xorps sign -> [group+0x78]; RTC: forward 12 @-0x1c, zero_plane 16
  @-0x34; latch byte 0xe0d37c. /Od frame declaration order: [ebp-5] (later-only success byte), group -0xc, forward
  -0x1c, zero_plane -0x34 => candidate order group, forward, zero_plane (+ static) consistent. No new block scope.
- real_math.h: vector_from_points3d(a,b,r): r = b - a => (camera.position, centroid) = centroid - camera: same
  semantics as production's relative_centroid. dot_product3d a.i*b.i+... ; sign -(...) kept.
- symbols.json static-local mangling precedent: 9 existing `?x@?1??fn@@9@9` rows (render_objects lighting_storage etc.).

## R3 probe p_max (hypothesis: /Od 0x7d8604..0x7d8623 shows `cmp tc,[largest]; jle; tmp=tc | tmp=largest;
   largest=tmp` = a ?: / MAX(triangle_count, largest) in the later source; worker's disclosure (b) claims January's
   conditional store is reproduced only by the `if`)
- p_max.c = cand + `largest = MAX(triangle_count, largest);` -> unlit draw EXACT (regen split).
- => worker's (b) is NOT decisive: January bytes accept both spellings; the /Od-attested one is MAX (house rule 18).
- p_max full gate vs regen split: 17/17 EXACT; objeq(cand_r.obj, p_max.obj) IDENTICAL 51 sections => byte-inert.

## R4 other checks
- /W3 /Zs (warn_check.py): base 12, cand 12, p_max 12 warnings, all in cseries.h:45 / real_math.h C4244; 0 in the TU.
- fake_match_scan: cand.c 0 leads, p_max.c 0 leads.
- nodup_census: _dot_product3d 72 / _vector_from_points3d 77 / _real_alpha_to_pixel32 6 / _real_argb_color_to_pixel32 3 /
  3 SDK tables 133 definers each: NODUP [] for all.
- admission audit (tools/audit_object_admission.py) now: unit in `rejected` with function_gap 0 (semantic pipeline
  already credits the objdiff-3.3.1 under-credited quad draw). Simulation with report metadata complete=True +
  patched rejections + patched symbols.json: contradicted 0, revoked 0, rejected 4 (dynavobgeom gone). No Matching
  label contradiction after the flip.
- Repo references to the old latch name: only symbols.json + the .c (models.c has its own
  local_reported_too_many_transparent_geometry_groups at a different offset - unaffected).
- tools/test_rasterizer_frame_statistics_owner.py lists dynavobgeom.obj as a frame-statistics importer; the .c keeps
  rasterizer_frame_statistics.h and its extern - unaffected.
- Window global: consumer-local `extern struct rasterizer_window_begin_parameters global_window_parameters;` (genuine
  complete type) is the exact declaration rasterizer_xbox.c (owner) itself uses (line 886) and 7 Matching objects use
  (active_camouflage, draw_primitives, screen_effect, water, xbox_transparent_geometry, rasterizer_text,
  rasterizer_transparent_geometry). No header declares it. Not a blocker (not an opaque view; not a function prototype).
- transparent_geometry_group after P3 == active_camouflage's copy (Matching) field for field (short vs word pads only).
- Pre-existing quad-draw bare block (not in the diff): strip test (hoist its 5 locals to function scope) -> size
  2976 != 2960 => load-bearing; justified by the escape law (vertex_constants/map_constants reach the out-of-line
  SetVertexShaderConstant wrapper; map_color reaches out-of-line real_argb_color_to_pixel32), accepted since 0903.
  /Od quad draw is the dx9 body (different locals) - cannot attest either way. Advisory only.

## R5 amended packet (reviewer)
- amended_final.c = worker cand_final.c + `largest_dynamic_unlit_triangle_count = MAX(triangle_count, largest...)`
  (/Od 0x7d8604 ?: temp; house rule 18). rasterizer_xbox_dynavobgeom.c.patch regenerated (git apply --check OK);
  symbols/config/rejections patches unchanged (copied). admission_packet_amended.patch = 4 concatenated: applies OK.
- Applied to applytree2: .c == amended_final.c, symbols.json == worker copy.
- gate vs CURRENT split 16/17 (unlit reloc-identity, expected); gate_r vs regen split 17/17; audit_r PASS;
  objeq(cand_r.obj, amended_r.obj) IDENTICAL 51 sections; surplus 19/0 problems; provider_link PASS;
  objdiff 3.3.1 one-unit: data 525/525, 16/17 (quad draw 97.74 = same under-credit as production; semantic pipeline
  credits it: audit function_gap 0).
- Worker ledger inaccuracy (non-blocking): "January's split object has UNDEF refs to ... every surplus literal" - only
  2 of the 7 literals (global_d3d_device, parameters) are UNDEF (all 5 __real are); the 5 helper-assert literals come
  from the folded real_*_to_pixel32 COMDAT bodies. Conclusion unchanged.
VERDICT: APPROVE (amended packet recommended; original packet also admissible - the amendment is byte-inert).

## R6 Layer-2 contingency probe (shadow header, no tracked edit)
- shadow_nomove/math/real_math.h = real_math.h minus lines 678-679 (lock/unlock_global_random_seed prototypes, the
  pending-ruling Layer 2 move); shadow_gate.py puts /I<shadow> first; /showIncludes confirms the shadow real_math.h is
  the FIRST real_math.h opened (later production opens are no-ops under the __REAL_MATH_H guard).
- amended_final.c vs regen split: 17/17 with the move reverted; control (unmodified shadow copy) 17/17; production .c
  vs build/split with the move reverted 17/17 => this admission is NOT contingent on the Layer 2 prototype-move ruling.
