# REVIEW source/saved games/player_profile

approve: True

## per_object
[
 {
  "unit": "source/saved games/player_profile :: packet A production.patch (final_min.c + symbols.json line 22790)",
  "approve": true,
  "issues": "Admissible at zero code credit. The HCEX File Static `profile_color_table` becomes static with symbols.json `\"static\": true`; cachebeta has no public at 0x316880; only this object references it. The invented `byte pad[3]` is removed (HCEX has no pad member; byte-inert). The `if (failed)` placement is attested by /Od 0x8597f2 and by January bytes (lab: EXACT vs a one-byte +0x10b diff) and is semantically equivalent. Against the simulated split the result is 19/3 with the new and create_default shas unchanged. Requires the csplit regen and a write-thread park re-baseline to 384/27/bbb652e0.../88.451324."
 },
 {
  "unit": "source/saved games/player_profile :: production_defined_full.patch (final_def.c)",
  "approve": true,
  "issues": "An optional alternative to A. It is defined code: the HCEX union and internal struct plus the /Od and HCEX local names profile_file and full_path. Against the simulated split it is 19/3. objdiff: new 81.26, write_thread 87.56637, create_default 82.65909 (down from 83.80), object 95.71. It re-baselines all 3 parks with the verified values. read and get_from_path still use byte arrays (follow-up)."
 },
 {
  "unit": "source/saved games/player_profile :: packet B production_owner_gated.patch / owner_gated_over_production.patch (final_og.c)",
  "approve": false,
  "issues": "Owner-gated; do not land. It needs R1 (uninitialised 512-byte block serialising 444 stack bytes), R2 (a failed seek skips file_close) and R3 (close(&file, NONE) after delete). The mechanics are verified: 22/22 EXACT against the simulated split, object_audit PASS, provider link PASS, data 1644/1644, and /Od plus January-byte attestation reproduced. If granted, retire the 3 parks and promote config.json line 400 after the full build and sweep. Advisory: adopt the HCEX globals type names first (byte-inert) and extend the R3 BUG comment to cover the delete-while-open."
 }
]

## checks
Everything below was re-run in scratch/w/review3_player_profile/ (LEDGER.md, REVIEW.md).

1. Production gate at 954eebd2: 19 exact / 3 residual, the same tags as the worker's.
2. Patches:
   - `git apply --check` passes on the current tree for production.patch, production_defined_full.patch and production_owner_gated.patch.
   - Each patch applied to the HEAD blobs reproduces final_min.c, final_def.c or final_og.c byte-for-byte (modulo EOL).
   - owner_gated_over_production.patch and owner_gated_over_defined_full.patch each stack to final_og.c.
   - The symbols.json copy differs from config/symbols.json by one in-place line (22790).
3. Own csplit simulation (splitcfg = config jsons + the edited symbols.json):
   - A control regen with the unedited symbols.json equals build/split byte-for-byte.
   - A full regen with the edit (833 objects) changes only player_profile.obj: the symbol `_player_profile_primary_colors` (storage 2) becomes `_profile_color_table` (storage 3), and get_rgb_color's relocation name changes.
4. Gate against the current split: final_min 18/4, final_def 18/4, final_og 21/1. The extra residual is get_rgb_color [reloc-identity] because the split is not regenerated.
5. Own simgate (same logic as gate.py) against the simulated split:
   - final_min 19/3. create_default 336/14 sha 3ce0cc6f and new 464/18 sha 4df7452a are unchanged from base. write_thread is 384/27 sha bbb652e083aadc92e51e4445f93d49d41941e7c53f50cc1883658c355aa579cb.
   - final_def 19/3: create_default 320/14 c4f4ba33, new 464/18 4f42ddba, write_thread 384/27 2d428375.
   - final_og 22/22 EXACT.
6. object_audit (target overridden to the simulated split):
   - final_og: PASS, 51 January symbols, 0 differ, every section ok.
   - final_min and final_def: FAIL(3), and only on the three residual .text sections. 0 symbol differences.
   - Surplus is identical in base, min, def and og: .drectve, 4 pooled literals, 3 D3D tables and `__real@3b808081`. The packets add no new surplus.
7. provider_link: SELECTED-PROVIDER LINK PASS for final_og, final_min and final_def. surplus_identity: 0 code COMDATs.
8. pdb_storage on production: 1 disagreement (the color table is external but absent from cachebeta publics). Packet A fixes it.
9. objdiff 3.3.1 against the simulated split, all matching the worker's figures:
   - base 95.341644
   - final_min 95.34539; write_thread 88.451324; new and create_default unchanged at 76.566666 and 83.795456
   - final_def 95.71412 (write_thread 87.56637, new 81.26, create_default 82.65909)
   - final_og 100 (code 4119/4119, data 1644/1644)
10. /W3 /Zs: the same 12 header-only warnings in base, min, def and og; 0 TU-local warnings.
11. fake_match_scan: 0 leads on all three candidates.
12. HCEX, my own DIA2Dump queries:
   - `profile_color_table` is DataKind 5 (File Static), `long[0x12]`. There is no `player_profile_primary_colors` symbol.
   - `union player_profile_block` = `{ unsigned char data[0x200]; struct player_profile_internal profile_block; }`.
   - `player_profile_new` has locals `profile_file` and `block` (the union).
   - `_player_profile_globals` has no pad member.
   - `player_profile_write_internal_input` = `{ long index; struct player_profile profile; }`.
13. cachebeta publics: there is no public between 0x31620C and 0x316AA8, so 0x316880 is file-static. `_player_profile_globals` is public.
14. Symbol atlas: no data name for the table.
15. /Od: `odbuild fn` for 0x8596d0, 0x85d0c0 and 0x85acf0 matches the worker's od_*.txt. In 0x8596d0, `if (failed) delete` is at 0x8597f2, after the open if/else. R1-R3 are visible in the /Od listings as the worker described.
16. January-byte lab for the `if (failed)` placement:
   - Uninitialised block with production's in-arm placement: [sha], a single raw byte at +0x10b (target 09, ours 21). This is the 20260909 donor byte.
   - Uninitialised block with the /Od placement: EXACT 352.
17. January failure path in `_player_profile_new`: error, delete(index), `or esi,-1`, close(&file, esi), return esi. R3 is confirmed in January's bytes.
18. Section-8 blocker scan of final_og: no consumer-local prototypes, no float, no hand-expanded helpers, no new block scopes.
19. Lab min_hcexnames (final_min with HCEX type and member names): 19/3 with identical shas; the whole object is identical except .debug$S.

## issues
No blocking issues in packet A. Every number the worker reported was reproduced independently.

**Integrator requirements for A** (production.patch; full file scratch/w/player_profile/final_min.c):
1. Apply production.patch: the .c changes plus config/symbols.json line 22790 edited in place.
2. Regenerate the split with csplit. This is mandatory: without it `_player_profile_get_rgb_color` drops to [reloc-identity]. A full regen changes only player_profile.obj.
3. Run a full ninja build and the stable sweep.
4. Re-baseline only the `_player_profile_write_thread_proc@4` park: base 384 / 27 relocs / sha bbb652e083aadc92e51e4445f93d49d41941e7c53f50cc1883658c355aa579cb / objdiff 88.451324.

The new and create_default parks stay valid (shas unchanged). Code credit is 0 and data stays at 1644/1644. There are no header changes, and nothing else in source/, config/ or build/split references the renamed table.

**production_defined_full.patch** (final_def.c) is an acceptable optional alternative. It is defined code with first-party HCEX types and /Od names. Its fuzzy effect is mixed (create_default drops 83.80 to 82.66), and it re-baselines all 3 parks with the values the worker gave (verified).

**Packet B** (production_owner_gated.patch / owner_gated_over_production.patch) must NOT be landed. It needs owner rulings R1-R3: this is the authentic-bug class, and it reverses the 20260909 rejection. Its evidence is real, verified, and correctly marked with BUG comments.

**Advisories (non-blocking):**
- **Globals type names.** The type tag and members are descriptive where HCEX has names: HCEX spells them `struct _player_profile_globals`, `struct player_profile_write_internal_input { long index; ... }` and the member `thread_input`.
  - Adopting them is byte-inert (verified).
  - Optional patch: scratch/w/review3_player_profile/hcex_names_over_production.patch. It applies over production.patch and gives 19/3 with identical shas.
  - If it lands, owner_gated_over_production.patch must be regenerated, because its write-thread context lines change.
  - Recommended before any whole-object admission.
- **Partial union adoption.** In final_def, player_profile_read and get_from_path still use `byte block[512]`, although HCEX types read's block as the union. This is a follow-up.
- **B's R3 BUG comment is incomplete.** It should also say that the file is deleted while still open.
- **R1's /Od attestation covers only the missing initialiser.** In the later 0x2000-byte format the block is fully overwritten. January's 444 serialized stack bytes come from its 0x200 format, which the worker disclosed.
