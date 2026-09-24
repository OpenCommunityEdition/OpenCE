# review source/rasterizer/xbox/rasterizer_xbox_shadows
approve=True

## per_object
null

## checks
All checks ran at 931ed8dc (tree clean). Everything is under C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review2_rasterizer_xbox_shadows/: LEDGER.md, REVIEW.md, tree/, split/, final.obj and the probe files.
(1) Patch applicability:
- `git apply --check` and `--whitespace=error-all` of production.patch are clean.
- production.patch equals the concatenation of patches/*.patch.
- Applied to a scratch copy, the post-images are byte-identical to final/{.c, symbols.json, config.json, parked.json} and to the worker's config/symbols.json.
- All touched files stay CRLF with no trailing whitespace, and all three JSON files parse.
(2) Split regeneration:
- csplit with the patched config gives 833 objects; only rasterizer_xbox_shadows.obj differs from build/split.
- A control csplit with the unmodified config is identical to build/split.
(3) Gate: the worker's lab_gate.py is tools/campaign/gate.py plus --split-root/--tmpdir.
- Final .c against the review split, guarding _point_from_line3d: 18/18 EXACT, guard passed.
- Production source against build/split: 17/1, with model_draw residual on sha (the baseline).
- Final .c against the unpatched build/split: 13/5, because 5 functions reference the renamed .bss statics.
(4) object_audit (copy honouring AUDIT_SPLIT_ROOT):
- Final against the review split: PASS, 28/28 sections including .bss 74/74 at align 4, and 33/33 symbols.
- Production against build/split: FAIL(11), made up of model_draw, .bss align 4 vs 8, and 9 wrapper rows with storage 2 vs 3.
(5) Surplus:
- The final object's surplus set equals production's (16 rows), so nothing new is added.
- surplus_check: 15 candidate-only externals, all identical to January's definers.
- provider_link on final.obj: PASS for all 15 rows.
(6) Storage: cachebeta_publics.txt has no public for the 9 wrappers, the 6 statics or the old globals name, and none in 0x46628C..0x4662D5. contribs.json has 0x46628C, size 74, flags 0xc0300080 (align 4), module 110.
(7) HCEX dump re-read:
- File statics: local_shadow_color at +0, local_parameters at +0x44, shadow_setup at +0x48, shadow_used at +0x49.
- The model_draw local is float[0xC] vsh_constants__texscale.
- HCEX emits no block records, so lexical scope cannot be read from it.
(8) /Od 0x80eba0 re-read: [0] is scale, [1] is scale*v_scale (movss 0xd8; mulss 0xec), and the constant rows are {1,0,0,0} and {0,1,0,0}.
(9) Reviewer mechanism probes:
- r1 (production + a wide bare block opened right after the call, globals struct kept): 18/18 EXACT against build/split.
- r2 (model declared with the call initializer and vertex_constants declared after it in the same block): residual.
- So the effect comes from a block that opens after the call, not from where the declaration sits within a block.
(10) Third-site corroboration, lab only, in rasterizer_xbox_models.c: moving vertex_constants into a block opened after shader_get_and_verify_type reproduces January's +0x12bd order. The eax load and fld/fmul now precede the store, and fstp precedes lea. Only the slot offsets from that function's separate frame residual still differ.
(11) objdiff 3.3.1 (own project): production code 4633/5316, final 5316/5316 (+683 matched), data 403/403.
(12) /Zs /W3: the same 12 shared-header warnings as production, and no C4013.
(13) fake_match_scan: 0 leads.
(14) Strip test S1 re-run (zero initialisers removed): .bss grows to 76 bytes in name-hash order, with shadow_used at 0 and shadow_setup at 1. That contradicts January, so the initialised statics are layout-attested.
(15) tools.parked_functions with the patched manifest: stale 0, invalid 0.
(16) symbols.json: the new rows sit at unique offsets with ascending order kept, edited in place. Static XDK wrapper rows already have precedents elsewhere in the file.
(17) No header edits, and no other config file or tool references the unit or the old globals name.

## issues
No blocking issues.
- The model_draw bare block is load-bearing: removing it gives production, which is residual. It is still admissible because it is attested independently of the match:
  - The landed exact camo precedent (cfa20176) uses the same idiom. There, frame-slot sharing proves the scope.
  - The same January scheduling signature at rasterizer_xbox_models +0x12bd is reproduced by the same block scoping.
  - The mechanism is verified: a block opening after the call is what matters, not the declarator position.
  - C89 needs the block for an initializer that reads `model`.
- Non-blocking notes:
  - HCEX and the /Od RTC name the array vsh_constants__texscale, typed real[12]. The pre-existing descriptive name vertex_constants is acceptable, and renaming is optional.
  - The gap statics local_object_bounding_radius and local_shadow_matrix have invented but descriptive names. Their split into two statics (versus one 56-byte aggregate) cannot be decided from bytes. Loose statics are the simplest reading consistent with the align-4 .bss and HCEX's loose statics.
  - The integrator must regenerate the split (csplit) together with the symbols.json edit. Without it, 5 functions that reference .bss fail on relocation names (13/5 against the old split).
- Evidence is in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review2_rasterizer_xbox_shadows/REVIEW.md and LEDGER.md.
