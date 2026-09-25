# MANIFEST - review of critic_3 patch 19 (source/rasterizer/xbox/rasterizer_xbox_environment_fog)

- Base/tip: 5d3ca708, `git status` clean apart from the pre-existing untracked research/ dir, both at start and at end.
  I edited no tracked files and ran no ninja, configure or git mutation. Claim label: review_critic3_rasterizer_xbox_environment_fog.
- Patch under review: scratch/w/critic_3/patches/19_rasterizer_xbox_environment_fog.patch. It renames
  `rasterizer_environment_fog_screen_active` to `rasterizer_environment_fog_screen_is_active`: the .c prototype, the
  definition, 4 call sites, the symbol-listing comment (`_code_00155860`) and symbols.json:5786 (name only;
  `"static": true` and line order are unchanged).
- Verdict: **APPROVE unchanged**. The patch passes `git apply --check` (also with -v and --whitespace=error).

## Source evidence (the rename is the only construct)
| evidence | result |
|---|---|
| Sept-2001 cachebeta.map static table | `_rasterizer_environment_fog_screen_is_active` sits at the January slot, between `_local_random_boolean` and `_rasterizer_environment_fog_screen_wind_update`. Size 0x200 = January 0x200 |
| Aug-2001 cachebeta.map / Sept retail cache.map | same name / `@rasterizer_environment_fog_screen_is_active@0` |
| Raw bytes, Sept cachebeta.xbe (read as data only) | 0 unmasked differences over 0x200 bytes. All 42 relocations resolve at the same offsets to the same Sept symbols. The .bss static offsets are consistent (sept_body_is_active.txt) |
| Callers | January: screen_begin +0x78, screen_draw +0x2c, screen_end +0x31, model_begin +0x22. Sept xbe: the same 4 at the same offsets (Aug: the same 4) (callers.txt) |
| January-hash atlas 4cc87b45 | 0x555860 carries this name at the exact tier. The other 7 fog records re-anchor against the current symbols.json |
| January strings / HCEX.pdb | nothing contradicts. HCEX does not contain the xbox fog statics |
| Prior name | self-described as "reconstructed rather than PDB-recovered" (20260909 ledger) |

## Target vs candidate (emulated split: csplit with the patched symbols.json, slug-local)
- split_ctl == build/split 833/833. In split_P only this object differs, and only in symbol #67's name (storage 3 on both sides).
- gate: 26 EXACT / 1 residual in production and in the candidate, with identical rows. The renamed function is EXACT at
  512 bytes on both sides. The residual `__rasterizer_environment_fog_screen_begin` (4000 vs 3904, relocs 232 vs 235) is
  unchanged.
- Object compare, prod vs cand with the rename mapped: all non-.debug$S sections and all symbols are identical. The
  .debug$S difference is only the gate PID in the object path.
- objdiff 3.3.1 (sha1 3130e428) gives the same numbers for production, the candidate, and build/split vs build/base:
  6273/10174 code, 26/27 functions, 2018/2018 data.

## Admission battery (identical for production and candidate, apart from the rename)
- object_audit FAIL(8): 1 residual section plus 7 D3D thunks with storage 2 vs 3. Both were there before the patch.
- pdb_storage: the same 7 disagreements on both sides. The renamed function is not in the cachebeta publics, so static is correct.
- surplus_identity: 13 surplus COMDATs, 1 of them not identical (`_real_local_random`). provider_link FAIL(1) on
  `_real_local_random` against effects. Both were there before the patch.
- Data: section bytes are identical, so there is no data change.

## House rules
This is an identifier rename only. The strip test and the /Od scope and declaration-order checks do not apply. Rule 7 is
satisfied: an authentic first-party name replaces a descriptive one. The unit is not in a held class and has no park or
rejection entry.

## Credit
None: 0 meaningful bytes, 0 padded bytes, 0 data, and no admission change. The gain is name hygiene only.

## Integrator notes
- The gate_*/stable.json snapshots key this function by section index (`::section:21`), so stable_verdicts treats
  it as the same exact section. Name-keyed reports will show the old name changing to the new one.
- Historical docs and the frozen research/fifty_objects_20260925 snapshots keep the old spelling. That is intended, since
  they are records.
