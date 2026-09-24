# REVIEW2 ledger: rasterizer_xbox_shadows OBJECT_COMPLETE_CANDIDATE (adversarial review)
Base tree 931ed8dc. Reviewer works only in scratch/w/review2_rasterizer_xbox_shadows/.

## R0 patch applicability
- `git apply --check scratch/w/rasterizer_xbox_shadows/production.patch` at 931ed8dc: clean.
- production.patch == cat patches/*.patch (byte-identical).
- Applied to a scratch copy (tree/): post-images byte-identical to final/{rasterizer_xbox_shadows.c,symbols.json,config.json,parked.json}; worker config/symbols.json identical too. All three JSON files parse.

## R1 split regeneration
- csplit with patched config -> review split/: 833 objects, ONLY rasterizer_xbox_shadows.obj differs from build/split.
- control csplit with unmodified config -> identical to build/split (0 differ), so build/split is current.
- review split obj == worker split obj (byte-identical).

## R2 gate / audit / surplus (independent re-run)
- lab_gate (copy of worker lab_gate.py = tools/campaign/gate.py + --split-root/--tmpdir) final .c vs review split,
  guard _point_from_line3d: 18/18 EXACT, guard passed.
- final .c vs UNPATCHED build/split: 13/5 (expected: 5 fns reference renamed .bss statics -> reloc names). So the
  symbols.json edit + csplit regen is REQUIRED together with the .c; integrator must regenerate the split.
- production source vs build/split: 17/1 (model_draw [sha]) = baseline confirmed.
- object_audit (review copy with AUDIT_SPLIT_ROOT) final.obj vs review split: PASS, 28/28 sections (.bss 74/74 align 3/3),
  33/33 symbols. Production obj vs build/split: FAIL(11) = model_draw + .bss align 3/4 + 9 wrapper storage 2/3.
- Surplus set of final.obj == surplus set of production obj (16 rows incl .drectve) -> no NEW surplus.
- surplus_check: 15 candidate-only externals ALL IDENTICAL to January definers. provider_link final.obj: PASS (15 rows).
- cachebeta publics: none of the 9 wrappers, none of _local_*/_shadow_setup/_shadow_used/_shadow_restored/
  _rasterizer_environment_shadows_globals is public; no public at all in 0x46628C..0x4662D5 (next is 0x4662F0,
  another module). contribs.json: 0x46628C size 74 flags 0xc0300080 (align 4) module 110 = this unit.
- HCEX compiland (worker dump, re-read): local_shadow_color @0x171C5F0, local_parameters @+0x44, shadow_setup +0x48,
  shadow_used +0x49 as File Statics; gap +0xC..+0x44 unnamed. model_draw local in HCEX: float[0xC] vsh_constants__texscale.
  (HCEX emits no SymTagBlock -> lexical scope not readable there.)
- /Od 0x80eba0 re-read: stores [ebp-0x40]=scale, [ebp-0x3c]=scale*v_scale (movss [0xd8]; mulss [0xec]); RTC
  vsh_constants__texscale 48 bytes; &x[8], &x[4] via mov 4; shl 3/2 -> real[12] in the later build. No scope evidence.

## R3 reviewer scope probes (mechanism check)
- r1 (PRODUCTION source + a WIDE bare block opened immediately after `model = shader_get_and_verify_type(...)`,
  enclosing the asserts/texture state/stores/animation/SetVertexShaderConstant; explicit stores kept; globals struct
  kept) vs build/split: 18/18 EXACT. => block scope opening after the call is sufficient, independent of block extent,
  of the initializer form, and of the .bss de-aggregation.
- r2 (model declared WITH the call as initializer at the top of the if-block, vertex_constants declared right after
  it in the same block): model_draw residual [sha]. => the effect is BLOCK-level, not declarator-level scope.
- Conclusion: worker's call-result alias-scope mechanism reproduced; the block is load-bearing (strip -> residual).

## R4 independent corroboration in a THIRD January site (rasterizer_xbox_models.c, lab only)
- m_block.c = production models.c with `real_vector4d vertex_constants[3]` moved from function top into a bare block
  opened right after `shader_model = shader_get_and_verify_type(shader, _shader_type_model);` (closing at the end of
  that else-branch). __rasterizer_model_draw stays residual overall (unrelated frame/other residuals, 286 hunks both).
- At January +0x12bd (two-sided texture-animation site): production ours = `mov eax,[esi+0xd8]; mov [ebp-0x80],eax;
  fld [0xd8]; fmul [0xec]; ...; lea edx; fstp`. m_block ours = `mov eax,[esi+0xd8]; fld [0xd8]; fmul [0xec];
  mov [ebp-0x80],eax; ...; fstp; lea edx` = January's ORDER exactly (only slot offsets -0x80/-0x84 differ: models'
  separate frame-layout residual). => the same scheduling signature in a third January function is explained by the
  same block-scope mechanism; consistent with the camo precedent (cfa20176, frame-proven by slot sharing).

## R5 remaining checks
- objdiff 3.3.1 (own project od_proj/): prod code 4633/5316 (17/18), data 403/403; final vs review split code
  5316/5316 (18/18), data 403/403 (.bss 74, .data 1, .rdata 328 all 100%) => +683 matched code bytes.
  (r1 wide-block probe vs build/split also 5316/5316: objdiff ignores .bss alignment; the de-aggregation is needed
  only for the strict COFF object audit.)
- /Zs /W3: production and final have the same 12 shared-header warnings; no C4013; none new.
- fake_match_scan on final .c: 0 leads.
- STRIP S1 re-run (worker strip_s1.c, zero initialisers removed): 18/18 functions but .bss 76 bytes, name-hash order
  shadow_used@0, shadow_setup@1, radius@4, color@8, parameters@0x14, matrix@0x18. The HCEX-attested names
  shadow_used/shadow_setup alone land first under uninitialised order, contradicting January (+0x48/+0x49) =>
  initialised statics are layout-attested (production already had `= { 0 }` / `= FALSE`).
- tools.parked_functions --manifest <patched parked.json>: stale [] invalid [].
- git apply --check --whitespace=error-all: clean; all touched files stay CRLF, no trailing whitespace.
- symbols.json: new rows at unique offsets (no alias sharing), ascending order kept, edited in place; static
  wrapper rows have many precedents in symbols.json (e.g. rows 5591-5599, 5830-5840, 5893-5902).
- No header edits; no other config file references the unit or the old globals name except parked/config.
VERDICT: APPROVE.
