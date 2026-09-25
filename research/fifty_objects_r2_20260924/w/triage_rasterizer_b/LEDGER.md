# Triage LEDGER: family rasterizer_b (round 2)

Base f6d00a8c. Objects: rasterizer_xbox_environment_fog, rasterizer_xbox_environment,
rasterizer_xbox_models, rasterizer_xbox, rasterizer_xbox_profile, bitmaps/bitmap_drawing.
Read-only triage; no candidate source edits; no claims.

## Probe log
- P0: read WORKER_BRIEF_R2.md; dumped hints.json packets for the six units.
- P1 rasterizer_xbox: 95/95 exact; audit FAIL(22) = invented rasterizer_xbox_d3d_globals aggregate + storage rows.
  Round-1 owner queue item 2 (names_xbox_hs_scen RULING.md) is apply-ready; only blocker = 9 non-first-party
  .bss static names. New-evidence search: January-hash atlas + Sept-2001 map-tier atlas (645506/7eacac) list only
  functions for rasterizer_xbox.obj (no .bss names); /Od build (PC DX9, global_render_targets[] array) has no
  node_matrix_constants / secondary_z / bitmap_dimensions / *_copy strings. => no first-party names; stays OWNER.
- P2 bitmap_drawing: 7/7 exact; only blocker = unreferenced 16-byte initialised .data prefix (ff x10, 00 x6) before
  public translation_table (cachebeta contribs prove ownership). New-evidence search: HCEX.pdb globals (-g) have no
  translation_table/bevel/bitmap_draw names (only global_post_combat_translation_table, unrelated); all 11 atlas files
  (incl. Sept-2001 map tier) carry only bitmap_drawing FUNCTION names; /Od str "translation_table" none.
  => reopen criterion (first-party name+type) still unmet; held by 2026-09-20 unnamed-global ruling => OWNER.
- P3 rasterizer_xbox_profile: 16/16 exact, audit PASS, data 132 B = '$' scorer defect (pinned entry ready).
  Round-1 reviewer (REVIEW__...profile.md) rejected admission on B1 invented TU-private aggregates (pads; loose-static
  de-aggregation exact except unattested 4-byte .bss +0x45c and needs descriptive names = held class), B2 NONE-branch
  "%s -- tell Bernie!" varargs defect (BUG class, /Od 0x8004c0 attested), B3 3 load-bearing volatiles. Canonical
  f6d00a8c has NOT landed the zero-credit 01-06/08 packet (profile.c:238 still consumer-local extern). => OWNER.
- P4 env spot_light_begin: /Od build fn 0x7e4590 (rasterizer_dx9_environment_specular_light.c; asserts light_index /
  light->radius>0.0f) was NEVER read by any env ledger (grep 7e4590 over docs/ + research/: 0 hits). Readout:
  locals light ptr [-8], gel index [-0x48], real_matrix4x3 at [-0x40..-0xc] (forward -0x3c, left -0x30, up -0x24),
  radius [-0x4c], radius*0.5f named local [-0x50], cone_scale [-0x54], byte [-1]=1 (DX9 'failed' flag).
  Statement shape: matrix.forward=light->forward; matrix.up=light->up; cross_product3d(&light->forward,&light->up,
  &matrix.left); normalize3d(&matrix.left) as a SEPARATE statement (fstp st(0) discard) - NOT nested as in n3a s1.c.
  cone_scale = 1.0f/(radius - half); constant[0].l = 0.5f/radius.
- P5 LAB (premise check, lab-only files scratch/w/triage_rasterizer_b/lab/, generator mk_spot.py; never landing):
  spliced only the spot-light body into the f6d00a8c TU. gate: all four 960/64 [sha], TU 42/2/0, point guard pass.
  alndiff vs January:  s1 (n3a shape) 28 hunks incl. extra fxch @0x1ea + whole tail cascade;
  od_nohalf (separate normalize3d stmt, no half local) == s1 (normalize nesting INERT);
  od / od_half_only (/Od-attested named radius*0.5f local, cone=1/(radius-half), .l = -cone_scale*half):
  248==248 insns, fxch + entire tail cascade GONE; remaining = 3 cross_product3d j/k commutative load flips
  (0xf3..0x116) + eax/ecx/edx rotation in light->definition/position copy (0x12d..0x154) + lea ecx/edx @0x1b5
  (likely downstream of the x87 last-use order). Other hunks are split-vs-defined literal display only.
- P6 LAB M8 ORACLE on lab_od (dead `long lab_mod8_K = 0;` at function-body top, oracle only, never landing):
  uninitialised dummies = inert (not IL-visible). Initialised: D=0 15 hunks, D=1 15, D=2 10 (ALL THREE cross_product3d
  j/k x87 flips now match January), D=3 14, D=4 13, D=5..8 15. => the x87 residual IS count-governed (M8 family; the
  'j/k never move' clause does not hold for cross_product3d's mixed bare/offset products). Remaining at D=2:
  eax/ecx/edx rotation in the definition/position copy (0x12d..0x154) + lea ecx/edx (0x1b5/0x1bd).
- P7 LAB: flat `real vertex_constants[20]` (exact sibling diffuse_light_begin house style; /Od writes a flat array)
  shifts the M8 key by 3 (odf: D=5 closes the x87 flips; D=2 no longer does). Rotation hunk (0x12d) persists in
  EVERY variant (od D=0..8, 10, 18, dummies inside block, real-typed dummies, flat array D=0..7): the rotation is NOT
  M8-count-governed. Residual after /Od half-local + M8 = 1 register-rotation family (4 hunks + lea ecx/edx).
- P8 env surplus _normalize3d: board census (norm_census.py) 59/60 build/base definers are IDENTICAL to January's
  selected action_charge copy; env is the ONLY different one (its TU defines REAL_MATH_EXTERNAL_SCALE_VECTOR3D, so its
  normalize3d body calls _scale_vector3d). LAB lab_extnorm.c (+#define/#undef REAL_MATH_EXTERNAL_NORMALIZE3D, precedent
  source/ai/ai.c:201/244): gate 42 exact / 2 residual unchanged, _normalize3d COMDAT no longer emitted (January env
  references _normalize3d as an external too). => a Rule-6-clean fix for the env surplus exists.
- P9 env lightmap_draw: astra 0920 packet (docs/.../astra_90pct_research7_environment_*.md) has a STRICT EXACT candidate
  (4016/248 sha 7818e559) with /Od-attested shared 1.0 intensity factor; HOLD only on the duplicated stage-0
  MAG/MIN/MIP per-arm blocks (common-tail ablation = [sha] at +0x3e1..0x406). New witness search: Oct-12-2001 2276betaP.xbe
  (research/oct-betaP.xbe, asserts present) lightmap_draw located via 'illumination->primary_animation_period' ref
  0x1619a8, stage-0 region 0x1612be..0x161356 is IDENTICAL to January's merged tail (same jmp-into-tail @0x1612f6,
  same `xor ecx,ecx; mov edx,0xd; push 2` filter scheduling). 2276P/2276defaultP/default.xbe are assert-free release
  builds. => no per-branch witness exists in any Xbox build we hold; lightmap_draw stays an OWNER/reviewer-standard item.
- P10 models: round-1 ledger (research/fifty_objects_20260925/w/rasterizer_xbox_models/LEDGER.md) F1 = zero-code
  conditional region around the self-illumination subtract site (if(TRUE)/if(shader_model) equivalents; January
  condition unrecoverable), + /Od casts in non-exact caller, + H3/M8 ties (0x1da, 0x5f9, 0xa17, 0xdf9, 0xf6d),
  env shader needs shared-header set_real_vector4d (P1 class) + ALPHATESTENABLE/edi-ebx ties, + TU-local struct
  views/C4013 debt. New-witness search: Oct-2001 2276betaP model_draw (fn 0x16c5a0, same +0x218/+0x5af assert
  layout as January) has the identical untested subtract(0x13090)/periodic/point_from_line3d(0x12f80) sequence at
  0x16cb62..0x16cba4 => no recoverable condition. => OWNER (F1 + P1), low probability.
- P11 env_fog _real_local_random surplus: census 1 same / 3 diff vs January-selected effects.obj NODUP copy
  (weather_particle_systems, breakable_surfaces, env_fog). Bytes: effects `add esp,4; ret` vs header inline `pop ecx; ret`.
  LAB lab_fog_rlr.c (TU-local REAL_MATH_EXTERNAL_REAL_LOCAL_RANDOM + result-local body = effects.c:1398 spelling =
  header sibling real_local_random_range spelling): emitted COMDAT section_infos_equal to effects' January copy (TRUE);
  env_fog 26 exact kept; screen_begin moves only by 2 x87 operand swaps (M8 key shift). => a real_math.h packet
  (real_local_random with `real result= ...; return result;`) would retire the NODUP hand copy + 3 Rule-6 surpluses,
  but it is a shared-header body respelling (LAW I canary lottery; real_math reserved to Codex packet) => owner/integrator.
- P12 LAB: M8 oracle (D=1..7 initialised dead locals, function top) on round-1 fog diag candidate_diag_DE_R3.c
  (gates [sha] 3904 at f6d00a8c, 197 alndiff hunks): INSENSITIVE (197 hunks every D). Fog's x87/order residuals are not
  M8-count ties. H3 oracle (1..16 pre-function externs) on spot odz_d2: rotation invariant; /Od declaration order
  (light_matrix before gel) invariant (odo_d0..7 same pattern as od).
- P13 LAB spot rotation probes (on odz_d2): position copies before radius -> emission follows source (no interleave),
  refuted (January interleaves => radius first, = /Od order); radius operand swap inert; a `definition` pointer local
  -> 242 insns (January reloads [esi] at 0xb5 and 0x12d) refuted. Rotation cause still unidentified.
- P14 rasterizer_xbox names: HCEX.pdb -t dump gives the target enum (two copies, values 0..9):
  render_d3d, render_primary, render_secondary, shadow_primary, shadow_secondary, motion_sensor_primary,
  sun_glow_primary, sun_glow_secondary, water, z (NUMBER_OF_RASTERIZER_TARGETS=10; also _primary=1/_secondary=2).
  January's 8th target (tree: descriptive `_rasterizer_target_render_primary_copy`, after water) is the render-primary
  descriptor with Data = global_d3d_surface_render_primary_z->Data, i.e. the Z buffer viewed as a target = HCEX's last
  target `_rasterizer_target_z`. This upgrades owner-queue names #8/#9 (global_d3d_{texture,surface}_render_primary_copy,
  'INVENTED _copy suffix') to HCEX-enum convention stems (e.g. global_d3d_{texture,surface}_z) and the tree enum
  `_rasterizer_target_water_bumpmap` -> HCEX `_rasterizer_target_water`. Still convention-derived, not bare first-party
  names => item stays OWNER; node_matrix_constants (#1) remains invented. No /Od string for the z target.
- P15 env admission debt census: env.c defines 17 TU-local structs (20 reserved spans; e.g. rasterizer_environment_debug_options
  view of rasterizer_debug_options, point_light_definition, a TU-local rasterizer_light_submit_parameters that is ALSO
  defined separately in object_lights.c and rasterizer_lights.c with different definition-pointer types) + 4 consumer-local
  externs (rasterizer_debug_options, pixel_shader, rasterizer_lights, global_window_parameters). env_fog: 12 structs /
  6 spans / 2 externs; models: 16 / 26 / 4. Brief s9: caller-local struct views with opaque spans block admission even when
  bytes match => every T3 object here also needs a genuine-shared-declaration packet (dynavobgeom precedent).
  Lab sweep files pruned (kept lab_s1/od/od_half_only/od_nohalf/odf/odz_d2/posfirst/radswap/extnorm*/fog_rlr + mk_spot.py).

## FINAL TRIAGE (2026-09-24)
- rasterizer_xbox            OWNER  (owner-queue item 2; P14 upgrades #8/#9 to HCEX-enum stem `z`, still convention)
- rasterizer_xbox_profile    OWNER  (review3 B1 invented aggregates/+0x45c, B2 NONE-branch BUG, B3 volatiles)
- bitmap_drawing             OWNER  (unnamed 16-byte .data prefix; no first-party name/type anywhere: P2)
- rasterizer_xbox_environment ATTACK spot_light_begin (P4-P7,P13: /Od 0x7e4590 half-radius local + M8 D=2 closes all
  cross_product3d flips; residual = one register rotation); object also needs lightmap_draw owner call (P9), 8 static
  storage rows + static permutation index (measured 42 kept), REAL_MATH_EXTERNAL_NORMALIZE3D (P8), TU-view packet (P15).
- rasterizer_xbox_environment_fog RETIRE (round-1 P0-P22 + P12 M8-insensitive; needs D/E/R3 rulings AND unsolved frame+4 /
  orders); object-level: 7 static rows ready, _real_local_random fix route P11 (header packet).
- rasterizer_xbox_models     RETIRE (F1 invented zero-code condition, Oct-2001 build has no test either (P10); env shader
  needs P1-class set_real_vector4d header move; H3/M8 ties; TU-view debt).
