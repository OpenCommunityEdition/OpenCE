# review_critic3_decals LEDGER (adversarial review of critic_3 patch 16_decals: decal_verify_neighbors -> decal_check)

Worktree HEAD 5d3ca708. Reviewer slug scratch/w/review_critic3_decals/. No tracked edits.

## R0 start
- Read WORKER_BRIEF_R2 0-9, critic_3 MANIFEST/LEDGER, patches/16_decals.patch (symbols.json line 1840 + decals.c header comment, prototype, 2 call sites, definition).

## R1 name evidence (independent)
- Sept-2001 cachebeta.map (2001-09-25, Static symbols): `_decal_check` 0001:00086520 f decals.obj, next static `_decal_set_first_decal_index` 0x86650 -> size 0x130 (January 0x130). Order static decal_check, set_first_decal_index (0x80), sprite_get_bounds (0x180) = January's first three functions, same sizes.
- Aug-2001 cachebeta.map: `_decal_check` 0001:0007d7f0, also first decals.obj static.
- January atlas 4cc87b45 (2002-01-14 cachebeta.exe): {"off":"0x486fe0","name":"_decal_check","lib":"decals.obj","tier":"exact"}; 0x486fe0 = symbols.json file_offset 552928 (0x86fe0) + 0x400000.
- bodyid.py (own tool): January split `_decal_verify_neighbors` (0x130, 26 relocs, storage 3) vs Sept xbe VA 0x97520 (= .text 0x11000 + 0x86520): 0 non-relocation byte diffs; Aug xbe 0x8e7f0: 4 diffs at +0x51/+0x7e/+0xd4/+0x101 (assert line immediates only); January cachebeta.exe @0x486fe0: 0 diffs.
- Callers: January split references at decals_reconnect_to_structure_bsp +0x10c and +0x1ba; Sept xbe E8 calls to 0x97520 at map rva 0x8782b/0x878d9 = inside _decals_reconnect_to_structure_bsp (0x87720) at +0x10b/+0x1b9 (same relative positions). Only caller in both.
- January cachebeta.exe strings: `decal_check`, `decal_verify`, `verify_neighbors`, `neighbors` 0 hits; `layer_check` 1 hit = assert text "!layer_check || layer==decal->layer" (matches the function's own layer_check parameter; consistent vocabulary). No contradiction.
- Old name provenance: `_code_00086fe0` -> `_decal_verify_neighbors` in 4277f892 (2026-08-31), no first-party source cited (descriptive, invented). Prior lanes had already flagged decal_check (docs decals_obj_opus5_100k_20260914.md 8.C; opus5 handoff "names-only config batch") - never ruled on or rejected (object_admission_rejections.json: no naming entry).
- /Od build halo_cache_symbols.exe has no symbols -> no name evidence either way.

## R2 mechanical checks
- `git apply --check` (worktree and --cached) of critic_3/patches/16_decals.patch at 5d3ca708: PASS (no mutation).
- Applied the patch to slug copies (pr/: config/ copy + decals.c; CRLF restored after GNU patch): decals.c differs in exactly 5 lines (header comment, prototype, 2 calls, definition); symbols.json only line 1840.
- csplit (build/tools/csplit.exe, same command as build.ninja rule) control split with production config == build/split 833/833; candidate split vs build/split: ONLY source/effects/decals.obj differs; objcmp.py (own): sections bytes + relocations + symbol table (name/value/section/type/storage/aux) IDENTICAL modulo `_decal_verify_neighbors=_decal_check`; storage stays 3 (static).
- gate production (vs build/split) 31 exact / 2 residual; candidate (with_split -> split_cand, --source pr/decals.c) 31 / 2; sorted rows identical modulo rename (`EXACT 304 _decal_check`). Residuals unchanged: _decal_clip_to_surface [sha], _decal_new_from_collision [sha] (lane-closed, not probed).
- objcmp prod.obj vs cand.obj: identical except the `.file` aux record (PID-named gate source path) - rename is byte-inert.
- Sept map alignment (order_align.txt, own parse): Sept decals.obj 33 fns / January 33; 32 name anchors with IDENTICAL sizes in IDENTICAL order; the only unnamed slot is #0 Sept `_decal_check` 0x130 = January `_decal_verify_neighbors` 0x130.
- HCEX.pdb: no decal_check / decal_verify_neighbors / decal_update / decal_set_first_decal_index (decals_initialize and create_weapon present) -> HCEX lacks the decals statics altogether; no signal, no contradiction.

## R3 admission battery (candidate via with_split -> split_cand; BASE_OVERRIDE for build/base readers; redirect verified by redirect_probe.py)
- object_audit prod FAIL(4) == cand FAIL(4) modulo rename: clip_to_surface/new_from_collision .text DIFF + `_decal_globals`/`_decal_points2d_temp` .bss offsets (pre-existing). `_decal_check` row ok 304/304, storage 3/3.
- pdb_storage prod 0 / cand 0 disagreements (neither name is a cachebeta public -> static is correct).
- surplus_identity: 21 candidate-only COMDATs, 0 not identical, prod == cand.
- provider_link: FAIL(1) `_cross_product2d` vs actor_combat (pre-existing NODUP provider class, RESERVED actor_combat repair) identical prod vs cand; rename is static so it never enters the surplus set.
- data: tinfo --data prod vs cand differ only in .debug$S (object path); split vs split_cand data identical.
- objdiff 3.3.1 (sha1 3130e428) objdiff/: prod 7365/15295 code, 31/33 fns, 33755/33755 data, 96.9359 == cand; `_decal_check` 100% (291). Zero credit.
- References: git grep old name after patch -> only docs/object_matching_logs/decals_obj_opus5_100k_20260914.md:118 (historical ledger that PROPOSED this very rename; leave). No parked.json / semantic_*.json / rejections / tools / tests / header references. No other TU defines `decal_check`. No other patch in scratch/w touches decals.c or symbols.json lines 1837-1843.
- Strip test / /Od declaration order: N/A (no construct or scope added; identifier-only). House rules: rule 7 (authentic name over invented descriptive one), prototype/definition layout unchanged.
- Closure note: claude_decals_physics_20260924.md closes the THREE residual functions (new_from_collision, clip_to_surface, physics_update_old); this exact, non-residual rename does not touch them.

## R4 verdict
APPROVE patch 16 as-is (no amendment needed). Zero credit, byte-inert, name first-party attested (Sept + Aug linker maps, January atlas exact, instruction-identical body, same sole caller at the same call offsets).
