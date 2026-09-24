# rasterizer_xbox_models wave-4 worker ledger (lane claude/fifty-objects-20260925, base 16542e46)

Targets: `__rasterizer_model_draw` (5168, park tu-context-optimization) and
`_rasterizer_model_draw_environment_shader` (3104, park instruction-scheduling).

## Baseline (base.c = production copy)
- gate --all: exact 12, residual 2 (draw [relocs 348!=350, sha]; env [sha]); submit now EXACT (landed since
  the w1/w3 ledgers). Frames equal (draw 0xa4, env equal; not in frame_census).
- draw reloc multiset diff: January has +1 `_subtract_vectors3d` call and +1 `_point_from_line3d` call
  (both at the self-illumination lerp, 0x5d2 / 0x605); nothing else differs.
- env reloc multiset: identical.
- real_regions draw: 56 REAL of 286 (base_draw_real.txt).

## Prior negatives read (do not repeat)
opus_reconciliation, opus5_100k_waveC (d1/d2 subtract placement, env e1..e_c1), opus5_150k_w1 (e2..e5),
opus5_150k_w3 (s1..s3b, pr1..pr14, b1..b6, q1), astra research16/wave13, lane B/C handoffs, shadows w2
ledger (CALL-RESULT ALIAS SCOPE law; January models +0x12bd has the same shape).

## Evidence pass
E1. Board census (refcensus.py): `_subtract_vectors3d` is referenced out of line ONLY by action_charge
    (definer) and rasterizer_xbox_models (1 ref, draw 0x5d2). `_point_from_line3d` UNDEF in models (1 ref),
    `_plane3d_distance_to_point` UNDEF in models (1 ref = draw fog block; ours already out of line there).
E2. January draw 0x5c2..0x609: `lea ecx,[ebp-0x48]; push ecx; lea edi,[esi+0x78]; lea edx,[esi+0x84]; push edi;
    push edx; call _subtract_vectors3d` then `push &self_illumination_color([ebp-0x20]); push ecx(slot);
    <periodic arg>; call periodic_function_evaluate; add esp,4; fstp [esp]; push &gradient; push edi;
    call _point_from_line3d` => source:
      subtract_vectors3d(&upper, &lower, &gradient);
      point_from_line3d(&lower, &gradient, periodic_function_evaluate(fn, t/period + phase), &color);
    (fraction is a direct argument, not a named local.)
E3. /Od 0x7f6710 (dx9 later build) at 0x7f704f..0x7f70cd: IDENTICAL call shape, subtract(0x432a30) with
    upper (+0x84), lower (+0x78), &[ebp-0x84]; point_from_line3d(0x42e0d0)(lower, &[ebp-0x84],
    periodic(...), &[ebp-0x34]). RTC names: [ebp-0x84] `gradient_color` (12), [ebp-0x34]
    `self_illumination_color`, [ebp-0x90] `seed`. Phase is a `?:` (temp [ebp-0x270]) into [ebp-0x98].
    Assert line spacing period->red = 12 lines in BOTH January (864->876) and /Od (2126->2138);
    red->external = 11 in both.
E4. /Od block structure: seed/phase/gradient_color/external_color are declared AFTER all function-scope
    locals ([ebp-0x84..-0x9c], after the -0x5d byte), inside the later build's profile block that
    ends before the diffuse_change_color code (0x7f7512).
E5. January frame slots (slotmap.py): gradient_color [ebp-0x48] SHARES with cc1 (fog block);
    seed [ebp-0x3c] SHARES with vertex_shader_work; phase at its own [ebp-0x54].
    Ours: seed alone at [ebp-0xa4] (bottom), phase shares -0x3c with vertex_shader_work,
    specular/vertex constants shifted by 4. By the escape-scope law, seed and gradient_color (both
    address-taken, reach out-of-line calls) must be declared in a block that closes before
    `vertex_shader_work = ...` => January-side attestation of the /Od block (E4).
E6. HCEX rasterizer_dx9_models: real_rgb_color is a UNION {n[3]; red,green,blue} (no vector member),
    so the /Od subtract/point calls on colours need pointer casts in the original too.

## Probes (hypothesis -> change -> result)
c1 (E2-E6): /Od-attested form. Self-illumination section in a block: `real_rgb_color gradient_color;
    unsigned long seed= local_parameters->unique_identifier; real self_illumination_phase= TEST_FLAG(..) ?
    0.0f : real_seed_random(&seed);` + assert 864 + `subtract_vectors3d((real_vector3d *)&upper,
    (real_vector3d *)&lower, (real_vector3d *)&gradient_color); point_from_line3d((real_point3d *)&lower,
    (real_vector3d *)&gradient_color, periodic_function_evaluate(..), (real_point3d *)&self_illumination_color);`
    + asserts + external-colour if-block (external_color declared in it). Function-top seed/phase/fraction/
    external_color declarations removed.
    RESULT: draw [size 5184!=5168, relocs 349!=350]; point_from_line3d now out of line (matches);
    phase now at January's [ebp-0x54]; subtract_vectors3d still INLINED (the only reloc-multiset diff).
c2: c1 minus REAL_MATH_EXTERNAL_POINT_FROM_LINE3D -> point_from_line3d ALSO inlined (5200) and submit flips
    to [sha] (declaration-count tie). The switch stays.
Lab (never landable) inline-gating measurements on c1:
  L_if1/2/3 (`if (rasterizer_debug_options.draw_models)` around subtract only): subtract OUT of line.
  => the site is exactly ONE conditional step above subtract's cost (Lane C per-site law).
  L_ret (return; at end of camo branch): inert. L_x1/4/12 (1..12 extra dot_product3d expansions before the
  site): inert (no cumulative budget). L_top/L_bottom (block moved before camera distance / after fog):
  inert (position independent). L_nocast (no pointer casts): inert.
  L_true (`if (TRUE) {subtract}`), L_sizeof (`if (sizeof(a)==sizeof(b))`), L_else (`if (!shader_model) {}
  else {..}`), L_sm (`if (shader_model)`), L_sh (`if (shader)`): ALL give draw [sha] ONLY (5168, 350 relocs,
  subtract out of line, gradient_color lands at January's [ebp-0x48]). VC7 counts a conditional region for
  inline gating BEFORE folding constant conditions / null tests of already-dereferenced pointers.
  L_lp (`if (local_parameters)`, global) keeps a real test (5184/351).
  L_camtrue_v/both (`if (TRUE)` around the CAMERA vector_from_points3d): vector_from_points3d goes out of
  line (January inlines it) => the camera site is at the same boundary; January's extra invisible region
  must START after the camera-distance statement and enclose the subtract site.
FINDING F1: January has a zero-code conditional region enclosing the self-illumination subtract site but not
  the camera site. /Od (0x7f6f33) has exactly such a region at exactly that place in the later build:
  `if (ps_version >= D3DPS_VERSION(1,4) || shader_model->model.detail_mask == _shader_detail_mask_none)`
  wrapping seed/phase/gradient_color/external colour (ends before diffuse_change_color, 0x7f7512); and
  January's frame (E5) independently requires a block there. The January condition is not recoverable from
  bytes (it folds to nothing); natural candidates must be proven before landing (owner-gated otherwise).
c3 (lab scaffold, not landable): c1 with the self-illumination block opened by `if (shader_model)` /
    `if (TRUE)` (the F1 invisible region spanning the /Od block): draw [sha] only; 30 / 34 REAL regions
    (base 56). January's seed/gradient/phase/vertex/specular slots all reproduced.
c4 (evidence: shadows CALL-RESULT ALIAS SCOPE law + January +0x12bd/+0xa88 early constant stores; /Od
    declares the texscale/effect constant arrays and the perpendicular/parallel colours in inner blocks):
    a  vertex+specular block (first pass) + separate two-sided vertex_constants: 21 REAL; camera dot product
       x87 order breaks (0x24f) -> declaration-count/ordinal effect of the extra declaration.
    b  a + perpendicular/parallel inside: 11 REAL (camera 0x24f still broken).
    e  single block from vertex constants through the end of the two-sided pass: 19 REAL.
    d  single block from perpendicular/parallel through the end of the two-sided pass (perp, parallel,
       vertex_constants, specular_constants; no second declaration): 7 REAL, camera region restored.
    Remaining in c4d: plasma fold 0xba; 0x1d5 `mov edi,[ebp+0x10]` placement after the env call;
    0x5f4 `fstp [esp]` vs `fstp [esp+4]` (periodic result into point_from_line3d arg); fog 0xdf9 (cc0 green
    x87 operand order) and 0xf6d (cc1 red operand order).
c5 (/Od fog operand orders cc0 `((1-pff)*atmos + pff*planar)*fog_density`, cc1 `fog_density*atmos`):
    byte-identical to c4d (sha 908e861d5b32) -> source operand order INERT (M8 law confirmed here).
c6 (/Od fog-block declaration order cc0,cc0_error,cc1,camera_plane_distance,pff,fog_density; no fog_plane
    local): WITHOUT the fog_plane pointer local, plane3d_distance_to_point INLINES (5200/354) at any depth
    (L_R3: extra region over the fog block still inlines) -> January's out-of-line call needs a non-constant
    pointer argument; keep `fog_plane` (base). Reordering the fog declarations alone (odp) flips the
    submit tie -> rejected.
c8 = c4d + plasma sub-structure (/Od 0x7f035c `add eax,0x28` into a named pointer): local view split into
    `struct shader_transparent_plasma_properties` (reserved00[4], intensity_exponent_source) inside
    `struct shader_transparent_plasma_definition { struct shader shader; ... plasma; }`;
    `plasma = &((struct shader_transparent_plasma_definition const *)shader_get_and_verify_type(..))->plasma;`
    RESULT: 0xba now EXACT (`add eax,0x28; mov ax,[eax+4]`), submit EXACT, draw [sha] 10 REAL; a new x87
    reassociation diff at 0xa17 (perpendicular.alpha = rf*pb*ta: Jan rf*pb first, ours pb*ta first) appeared
    -> TU declared-name-count tie (the new struct tag/members) — M8/H3 class.
Oracles on c8 (lab only): k=0..16 extern declarations before draw -> k=1,2,3 remove only the 0x1da edi
    region (k>=4 flips submit); unused in-function locals behave the same (name count); 1..7 dead locals in the
    fog block: inert on the fog x87 regions. gradient_color as real_vector3d (no casts): draw byte-identical,
    submit flips -> keep real_rgb_color + casts.
    The x87 operand-order regions (0xa17 perpendicular.alpha reassociation, 0xdf9 cc0.green, 0xf6d cc1.red:
    all offset-0 or first-term effects) are NOT name-count or fog-local-count ties at the probed counts; they
    moved with the plasma edit (IL-temp/ordinal class, M8).
DRAW STATUS: best lab form (c8 + scaffold) = [sha] only, 10 REAL regions, size/relocs/frame/slots exact;
    closing needs F1 (owner/evidence) + the ordinal ties. Moving to the environment shader.

## Environment shader
Evidence: /Od 0x7f2f80 (RTC: forward, reflection_view_perpendicular_color/parallel_color argb 16,
  vsh_constants__texscale 48, vsh_constants__effect 32, cc0 16, cc0_error 12, cc1 12); later build computes
  camera_plane_distance = plane3d_distance_to_point(&fog.plane,&camera.position) and
  camera_distance = dot_product3d(&camera.forward,&forward) as named fog locals, detail scale field read twice.
  January env: plane distance INLINED (direct global address, consistent with the draw finding that a
  constant address inlines and a pointer local does not); January 0x3fc `mov edx,[esi+0xb4]; mov ecx,edx`
  = two inline-parameter pseudos of one load (set_real_vector4d class, w3), stores after perpendicular.
e_a2 (w3 a2 spellings: vector_from_points3d, argb perp/parallel, rgb cc0_error, helpers in PIN): 3104 [sha],
  41 REAL (base 70). +odloc 40. +blk 39. +twice => 3120.
L_h_* (lab, TU-local __inline set_real_vector4d = w3 s1..s3 class, NOT landable as TU-local):
  noblk row0 = 20 REAL / 3120 (w3 s3 reproduced: j reloaded late through x87).
  blk (constants only) row0: copy forms but the vertex stores hoist above perpendicular (not January).
  blkpp (perp/parallel + constants in one block opened after shader_get_and_verify_type) row0: the whole
  0x3f3..0x46b constants window matches January's shape (copy + late stores), 35 REAL / 3120; rows v1,v2,s0,s1
  through the helper: 37-39 REAL.
  Residual +16 bytes = top-of-function ALPHATESTENABLE value: January `movzx edi,byte [esi+0x28]; and edi,ebx`
  (value allocated straight to edi), ours `xor eax,eax; mov al,..; and eax,ebx; mov edi,eax` (w1 recorded the
  (boolean)/raw-flag spellings inert) + downstream register rotations; cc0/cc1 pixel edi/ebx permutation.

## Fuzzy measurement (objdiff-cli 3.3.1 report generate on a private mini-project in odp/; base reproduces the
## park values exactly: draw 95.08453 / env 95.87124 / normalized sha 89b0d7ea.. / 5179768e..)
| variant | draw % | env % | submit | notes |
|---|---:|---:|---|---|
| base | 95.08453 | 95.87124 | 100 | production |
| c8 (lab scaffold, not landable) | 99.57182 | 95.87124 | 100 | draw [sha] only, 10 REAL |
| c9 = c8 with a bare block | 96.832306 | - | [sha] (LOST) | subtract inlined 5184/349 |
| d1 = c1 + constants block (casts) | 96.76907 | - | 100 | new rgb->vector/point casts in a non-exact caller = owner-gated |
| d3 = c1 + plasma | 96.208244 | - | LOST | plasma edit flips submit in bare form |
| f1 = base + draw constants block (d) | 95.703094 | 95.87124 | 100 | cast-free |
| f3 = f1 + self-illum block w/o calls | 96.00069 | - | 100 | size 5184 (tag regresses) |
| e_a2_blk | 95.08453 | 96.38387 | 100 | env |
| L_h_noblk / L_h_blkpp (lab TU-local helper) | 95.076 | 97.88 / 97.39 | 100 | helper not landable (shared header) |
| **g2 = f1 + e_a2 + odloc + env constants block** | **95.71134** | **96.38508** | **100** | BEST landable; 12/14 exact kept |
Oracle: k=1..4 extern declarations before submit restore submit in c9 (name/numbering tie; the plasma edit
  shifts it). The genuine C4013 prototype of rasterizer_frame_statistics_count_static_vertices (owner header
  blocked by the local frame-statistics view, w3 A7) does NOT fix 0x1da and flips submit (lab).

## FINAL (this wave)
- best_candidate.c = g2: gate exact 12 / residual 2 (draw [relocs 348!=350, sha], env [sha]) — tags unchanged,
  frames unchanged (0xa4 / 0x80), identical defined-symbol set and non-code sections vs production.
  draw 95.08453 -> 95.71134 (REAL 56 -> 55), normalized sha 771bf2e799fe91d357ec0dc2e56d91cb8f05be67aad0fc11ea49ffeafc002f26
  env  95.87124 -> 96.38508 (REAL 70 -> 38), normalized sha a4a4ca436d309778ff1ad21a7b5cbe35e4701fc4b4c22e06ae7263d206dfa9bb
  House-rule wins: env hand-expanded vector_from_points3d / plane3d_distance_to_point / dot_product3d replaced by
  the genuine header helpers (/Od 0x7f305a, 0x7f37bb, 0x7f385b), /Od types (argb reflection colours, rgb
  cc0_error), /Od named fog locals; constant arrays block-scoped (CALL-RESULT ALIAS SCOPE law; shadows +
  active_camouflage precedents; January early constant stores at draw +0xa88/+0x12bd).
- owner_F1_draw_lab_scaffold.c = c8 (NOT landable): proves that with one zero-code conditional region around
  the self-illumination block (+ /Od calls, block scopes, plasma sub-structure) draw is [sha]-only (size,
  relocations, frame, every slot exact; 10 REAL regions of H3/M8 ties).
g3 = g2 with `real camera_plane_distance= plane3d_distance_to_point(..)` (house rule 17): byte-identical.
g4 = g3 with `real detail_map_scale= ..` declared+initialised inside the env constants block: byte-identical.
=> best_candidate.c = g4; production.patch regenerated from g4 (git apply --check clean; applying to a slug
   copy reproduces g4 byte-for-byte); fake_match_scan 0 leads; /W3 warning set identical to production
   (pre-existing C4013 rasterizer_frame_statistics_count_static_vertices only). parked.patch re-baselines
   both parks (draw 771bf2e7.. 95.71134; env a4a4ca43.. 96.38508) with appended evidence notes.

## Zero-credit ownership correction (independent of the .c patch)
- pdb_storage: 5 disagreements = the five XDK D3DINLINE wrapper rows of this object (split external, ours static,
  absent from cachebeta publics). Same fix class as dynavobgeom/shadows/rasterizer_xbox wrapper rows.
- config/symbols.json slug copy (in-place, CRLF kept): `"static": true` on lines 5844, 5845, 5847, 5848, 5849
  (_D3DDevice_SetRenderState 1417552, _D3DDevice_SetTextureStageState 1417984, _IDirect3DDevice8_SetRenderState@12
  1419072, _IDirect3DDevice8_SetTextureStageState@16 1419616, _IDirect3DDevice8_SetVertexShaderConstant@16 1419712).
  symbols.patch: git apply --check clean.
- csplit regen (build/tools/csplit.exe -i cachebeta.exe -p slug/cfg -o slug/split): rc 0, 833 objects, ONLY
  rasterizer_xbox_models.obj differs. g4 vs regen split: 12/14 strict EXACT (same two residuals);
  audit_r (AUDIT_SPLIT_ROOT=slug/split): 58/58 January symbols equal, only the two residual .text sections DIFF
  (OBJECT AUDIT FAIL(2)). build/report.json: data 1912/1912 (100%); no admission-rejection entry.

## Object blockers after this wave
1. the two residual functions (draw: F1 + H3/M8 ties; env: set_real_vector4d header-inline question +
   ALPHATESTENABLE allocation + edi/ebx pixel permutation);
2. house-rule debt that blocks admission even if bytes match (brief (g)): TU-local struct views with opaque
   spans (rasterizer_debug_options, rasterizer_model_begin_parameters, shader_*_definition,
   rasterizer_models_frame_statistics vs the owner rasterizer_frame_statistics.h layout), and the C4013
   implicit declaration of rasterizer_frame_statistics_count_static_vertices (owner header include blocked by the
   local frame-statistics view; w3 A7 owner decision).

## Corroboration for F1 (not byte proof)
- Third-party HCEA decompile of HCEX (research/halocea/src/rasterizer_model_draw_model_shader_pp.c:183):
  the self-illumination section (seed, phase, colour lerp, external colour) sits inside
  `if (!shader->model.detail_mask) { ... }`; the /Od dx9 build has `if (ps>=1.4 || detail_mask==none)`.
  Two later lineages both guard exactly this section; January's bytes have NO test there, yet its inliner saw a
  conditional region (F1). The January condition is unrecoverable from bytes (folded/zero-code).

## Do-not-repeat (this wave)
- operand order of the fog cc0/cc1 products (c5: byte-identical); fog-block declaration order (c6 odp: flips
  submit); dropping the draw fog_plane pointer local (plane distance inlines at any depth);
  gradient_color as real_vector3d (draw identical, submit flips); `return;` after the camo branch (inert);
  extra inline expansions / block position for the subtract site (inert); the C4013 frame-statistics prototype
  (does not fix 0x1da, flips submit); dead/unused locals as oracles (only name-count effects);
  env: direct D3DDevice_SetRenderState for ALPHATESTENABLE (inert); env detail scale read twice (3120);
  TU-local set_real_vector4d (lab only; 3120 in every block variant).

## Reopen criteria
- DRAW: (a) owner ruling or first-party evidence for the January form of the zero-code conditional region
  around the self-illumination block (candidate forms measured equivalent: `if (TRUE)`, `if (shader_model)`,
  `if (shader)`; /Od + HCEX lineage guard the same section), together with admitting the /Od-attested
  rgb->vector/point casts for subtract_vectors3d/point_from_line3d (/Od 0x7f704f), then the plasma sub-structure
  (/Od 0x7f035c, fixes 0xba) and the H3/M8 ties that remain in owner_F1_draw_lab_scaffold.c (0x1da edi reload =
  +1..3 pre-function declarations; 0x5f9 fstp order; 0xa17 perpendicular.alpha reassociation; 0xdf9/0xf6d fog
  x87 first-term order). Every draw edit must re-gate submit (the plasma edit and many bare-block forms flip its
  name-count tie; oracle: +1..+4 declarations before submit restores it).
- ENV: a shared-header packet that turns environment_fog's NODUP set_real_vector4d hand copy into a genuine
  header __inline (January flags-32 COMDAT, selected from environment_fog; board sweep required, P1 class until
  the env caller is exact) + the blkpp scope (perp/parallel/constants block, measured to reproduce January's
  0x3f3..0x46b copy+late-store window) + a decoded source cause for the ALPHATESTENABLE value being homed
  straight in edi (movzx edi) and the cc0/cc1 pixel edi/ebx permutation.
- OBJECT: additionally the TU-local struct views -> genuine shared declarations (dynavobgeom precedent) and the
  frame-statistics owner-header conflict (w3 A7).
g5 = g4 with the env fog-block declarations in /Od RTC order (cc0, cc0_error, cc1, camera_plane_distance,
  planar_fog_fraction, camera_distance, fog_density): byte-identical.
g6 = g5 with vertex_shader_permutation declared right after shader_environment (/Od -0xc/-0x10 order):
  byte-identical. FINAL best_candidate.c = g6; production.patch regenerated from g6 (apply --check clean,
  reproduces g6 byte-for-byte; fake scan 0; /W3 warning set unchanged; vs regen split 12/14 + audit FAIL(2)
  = only the two residual .text sections). Draw block declaration order perp, parallel, vertex, specular
  = /Od RTC order (-0x128, -0x140, -0x178, -0x1a0).
