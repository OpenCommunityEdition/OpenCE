# rasterizer_xbox_shadows wave-2 worker ledger (lane claude/fifty-objects-20260925, base 931ed8dc)

Target: `__rasterizer_environment_shadow_model_draw` (688 padded / 48 relocs, park instruction-scheduling).

## Baseline
- gate --all on production: 17 exact / 1 residual (`__rasterizer_environment_shadow_model_draw` [sha]).
- alndiff (production vs January): 200/200 insns; exactly two transpositions:
  * Jan: `mov eax,[esi+0xd8]; fld [esi+0xec]; fmul [esi+0xd8]; mov [ebp-0x30],eax`
    ours: `mov eax,[esi+0xd8]; mov [ebp-0x30],eax; fld; fmul`
  * Jan: `fstp [ebp-0x2c]; lea edx,[ebp-0x20]`  ours: `lea edx,[ebp-0x20]; fstp [ebp-0x2c]`
- frame 0x30 equal; relocations identical (only csplit symbol/defined-noncode naming of the string literals).

## Prior negatives (do NOT repeat; from w2/w3 ledgers)
m1 named product local first (1 transposition left), m2 j-first (worse), m4 real[3][4] (inert),
m5 j = v_scale * vc[0].i (worse), m6 separate u/v locals (worse), m7 local after i-store (=baseline),
m8 m1 with operands commuted (=m1).

## Evidence gathered this wave
- /Od 0x80eba0 (dx9 later build): RTC names the 48-byte aggregate `vsh_constants__texscale`; stores in order
  [0]=scale, [1]=scale*v_scale (movss 0xd8, mulss 0xec), [2..11] constants; &x[4], &x[8] indexed with
  `mov r,4; shl r,2/3` => element size 4 => `real vsh_constants__texscale[12]` in the later build.
- January `__rasterizer_model_draw` (rasterizer_xbox_models.obj +0x12bd) has the IDENTICAL January shape
  (load eax, fld/fmul, store eax, ..., fstp before lea edx); our models reconstruction has the same defect.
- January has `_set_real_vector4d` as a flags-32 COMDAT (selected copy attributed to environment_fog.obj,
  image 0x155820 region; no January object references it out of line) => a shared-header __inline helper.
  Inlining a 4-argument setter evaluates ALL arguments (load i into eax, compute j on x87) before the
  body stores (i store, j fstp) - exactly January's interleave. HYPOTHESIS H1.

## Probe H1 (lab stand-in `__inline set_real_vector4d`, row 0 only) - h1a (v*s), h1b (s*v)
- Hypothesis: a header-inline 4-arg setter evaluates args before body stores -> January's interleave.
- Result: both 688, residual; IDENTICAL to w2 m1: `fld [0xec]; mov eax,[0xd8]; fmul [0xd8]; mov [ebp-0x30],eax`
  and fstp/lea now match. One transposition left (fld hoisted above the integer load). Operand order inert.
- Reading: the shared load(0xd8) is emitted when first DEMANDED; January demands it before fld [0xec]
  (canonical operand order of the fmul differs), ours after. Next: M8/name-count oracle (lab-only fillers).

## Oracle probes (lab only, never landable)
- or_base_1..8 / or_h1a_1..8: 1..8 UNUSED `long` locals at function top -> region unchanged (inert).
- nc_base_1..16 / nc_h1a_1..16: 1..16 extern declarations before the function -> region unchanged (inert).
  => not an M8 / TU-name-count tie at these counts.

## Probes V1-V5, b_sv
- v1: helper for all three rows -> = h1a (1 transposition).  v5: v1 with s*v -> = h1a.
- v2/v3/v4: named `real detail_map_scale` local (with/without helper, i- or j-first) -> x87 routing
  (fld [0xd8]; fst ...), 34-37 insn diffs: named-real routes x87 (known law) -> REFUTED.
- b_sv: baseline with `detail_map_scale * detail_map_v_scale` -> identical to baseline (x87 order
  fld [0xec]; fmul [0xd8] regardless of spelling here; models.c's site gives fld [0xd8] for s*v, so the
  commutative order is context-canonicalised, not spelled).

## KEY FINDING (lab_param_base / lab_param_h1a, lab-only semantics change)
- Replacing the call-result base (`model = shader_get_and_verify_type(...)`) with a PARAMETER copy
  (`model = (...)shader;`, call result discarded) gives January's exact region order in BOTH the plain
  baseline body and the helper body: `mov eax,[esi+0xd8]; fld [esi+0xec]; fmul [esi+0xd8]; mov [ebp-0x30],eax`.
  (rest of the function differs because esi is no longer the call result - lab only.)
- Reading: alias class of the base pointer. A parameter cannot point at this frame's locals; a call
  result may point at an escaped local (vertex_constants escapes to shader_texture_animation_evaluate /
  SetVertexShaderConstant), so the v_scale load cannot be hoisted above the i-store. January hoists it
  although its base IS a call result (mov esi,eax) => in January the vertex-constant stores were not
  aliasable by `model` at that point. Next: test escape/scope forms.

## Probes s_b1 / s_b2 / s_b3 / lab_noescape  ** s_b3 EXACT **
- Hypothesis (from the lab_param finding): VC7's call-result alias set is SCOPE-gated - a local whose
  declaration scope opens AFTER the call that produced `model` cannot be pointed to by `model`, so the
  v_scale load may be hoisted above the vertex_constants[0].i store.
- s_b1: vertex_constants at the top of the `shader->base.type == _shader_type_model` block (scope opens
  BEFORE the call) -> baseline order (4 diffs). s_b2: model + vertex_constants both there -> same.
- s_b3: vertex_constants declared in a bare block `{ real_vector4d vertex_constants[3]; ... }` around its
  stores/animation/SetVertexShaderConstant (scope opens AFTER model = shader_get_and_verify_type)
  -> __rasterizer_environment_shadow_model_draw EXACT 688.
- lab_noescape (vertex_constants never escapes; lab only) -> 592 bytes, stores dead-eliminated (control only).
- Precedent: rasterizer_xbox_active_camouflage.c (landed exact, cfa20176) declares the same
  texture-animation vertex_constants[3] in a bare block after its `model = shader_get_and_verify_type`.

## Form selection (all four EXACT 18/18)
- s_b3_sv: block + explicit stores, product `detail_map_scale * detail_map_v_scale` -> EXACT (operand order inert).
- s_b3_init_vs / s_b3_init_sv: block-scoped AGGREGATE INITIALIZER (camo form) -> EXACT (both operand orders).
- => the load-bearing fact is the declaration SCOPE of the escaped vertex constants (opens after the
  call that produced `model`); statement form and operand order are inert.
- Chosen: aggregate initializer at the head of a bare block (house rule 17; C89 requires the block because
  the initializer reads `model`, which exists only after shader_get_and_verify_type; identical idiom to the
  landed exact rasterizer_xbox_active_camouflage.c:610 block), product in the /Od-attested order
  (0x80ee04: movss [0xd8]; mulss [0xec]).

## Function closed; section-4 audit of cand.c (block-scoped aggregate initializer)
- gate: 18/18 EXACT.
- object_audit FAIL(10):
  * .bss `_rasterizer_environment_shadows_globals` size 74/74 but ALIGN 3 (Jan, 4-byte) vs 4 (ours, 8-byte):
    our invented 72-byte struct is >=64 B => 8-aligned section (aggregate-vs-scalar oracle 2). January's
    .bss holds no >=64-byte object => loose statics.
  * 9 XDK D3DINLINE wrapper symbols storage split 2 / ours 3; pdb_storage: all 9 absent from cachebeta
    publics => file-static; symbols.json rows 5906-5908, 5910, 5911, 5914-5917 lack "static": true
    (same fix the rasterizer_xbox_audit worker applied to its 20 wrapper rows).
- HCEX compiland rasterizer_dx9_shadows.obj (hcex_shadows_compiland.txt): File Statics
  local_shadow_color (union real_rgb_color) @+0, local_parameters @+0x44, shadow_setup @+0x48,
  shadow_used @+0x49, shadow_restored (.data); the 0x38 bytes +0xC..+0x44 are unnamed there
  (= object radius 4 + real_matrix4x3 52). => January: six '= 0' file statics in this declaration order.

## cand_flat.c = cand.c + January's loose file statics (de-aggregation)
- Source: removed the invented `struct rasterizer_environment_shadows_globals`, its offset typedef and the
  `#define local_parameters rasterizer_environment_shadows_globals.local_parameters` bare-spelling hack
  (the banned macro-to-fake-stringified-text pattern; January's assert text "local_parameters" is bare).
  Declared, in January's .bss offset order with '= 0' initialisers (VC7: '= 0' statics = declaration order):
  local_shadow_color (real_rgb_color), local_object_bounding_radius (real), local_shadow_matrix
  (real_matrix4x3), local_parameters (const model-begin parameters *), shadow_setup, shadow_used.
- symbols.json (scratch config/, mk_config.py line surgery): 9 wrapper rows "static": true; the
  `_rasterizer_environment_shadows_globals` row -> 4 rows `_local_shadow_color` +0,
  `_local_object_bounding_radius` +0xC, `_local_shadow_matrix` +0x10, `_local_parameters` +0x44
  (existing `_shadow_setup` +0x48 / `_shadow_used` +0x49 rows unchanged).
- csplit (scratch split/): 833 objects compared, ONLY rasterizer_xbox_shadows.obj differs.
- lab_gate vs scratch split, --forbid-emitted-symbol _point_from_line3d: 18/18 EXACT, guard passed.
- audit2 (object_audit + AUDIT_SPLIT_ROOT): PASS - all 28 January sections equal incl. .bss 74/74 align 3/3,
  33/33 January symbols equal (storage/section/offset).
- Surplus: _dot_product3d (code COMDAT, external), 3 XDK SDK tables, 6 assert literals, 5 __real consts.
- surplus_check (code+data, vs split): 15 candidate-only externals ALL IDENTICAL to January's selected definers.
- provider_link (cand_flat.obj): SELECTED-PROVIDER LINK PASS (15 rows incl. _dot_product3d vs action_charge).
- objdiff 3.3.1 (od_report.py, own project): prod code 4633/5316 17/18 fns, data 403/403;
  cand (vs scratch split) code 5316/5316 18/18, data 403/403 (.bss 74, .data 1, .rdata 328 all 100).
  cand.c alone (vs build/split): also 5316/5316, data 100.
- /Zs /W3 (warn.py): production and candidate have the identical 12 shared-header warnings; no C4013, none new.
- Strip S1 (strip_s1.c: the six '= 0'/NULL/FALSE initialisers removed): gate 18/18 but audit FAIL(7):
  .bss 76 bytes, name-hash order (shadow_used@0, shadow_setup@1, local_object_bounding_radius@4, ...).
  => the zero initialisers are layout-attested (VC7 '= 0' = declaration order), not decoration.

## Controls
- ctl_flat_topdecl.c (flat statics + the ORIGINAL function-top vertex_constants body): model_draw residual
  => the de-aggregation does not close model_draw; the block scope is the load-bearing function fix, and the
  de-aggregation is independently required by the .bss alignment.
- Reflow of the lines wrapped only because of the long struct prefix (cand_final.c): all 57 non-debug
  sections byte/reloc identical to the pre-reflow object.

## FINAL = final/rasterizer_xbox_shadows.c (+ final/symbols.json, final/config.json, final/parked.json)
- lab_gate vs scratch split (config/ symbols edits, csplit: only this object changes of 833), guard
  _point_from_line3d: 18/18 EXACT.
- audit2: PASS - 28/28 January sections (incl .bss 74 align 4, .data 1, 8 .rdata literals), 33/33 symbols.
- pdb_storage_cand: 33 split symbols, 0 disagreements with cachebeta publics.
- surplus_check: 15 candidate-only externals ALL IDENTICAL (_dot_product3d, 3 XDK tables, 6 literals,
  5 __real); provider_link PASS (both orders, 15 rows).
- objdiff 3.3.1: code 5316/5316 (18/18), data 403/403 (production 4633/5316, 17/18) => +683 meaningful
  / +688 padded code bytes.
- /Zs /W3: identical 12 shared-header warnings as production, none new; fake_match_scan: 0 leads.
- Patches (git apply --check clean; post-images byte-identical in a scratch copy): patches/*.patch and the
  combined production.patch (source .c, symbols.json, config.json status Matching, parked.json park removal).
VERDICT: OBJECT_COMPLETE_CANDIDATE.

## Transferable lesson (new VC7 law, measured)
- CALL-RESULT ALIAS SCOPE: a pointer returned by a call may alias only address-taken locals whose
  declaration scope is open at the call. A local declared in a block that opens after the call cannot be
  pointed to, so loads through the pointer may be hoisted above stores into that local. Detector: January
  interleaves `mov reg,[p+x]` / x87 loads from a call-result pointer above stores into a vertex-constant
  array while ours keeps statement order. Same January shape at rasterizer_xbox_models.obj
  `__rasterizer_model_draw` +0x12bd (our reconstruction shows the same early-store defect there).
