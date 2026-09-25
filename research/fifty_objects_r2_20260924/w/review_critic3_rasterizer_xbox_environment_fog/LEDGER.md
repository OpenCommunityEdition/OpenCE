# LEDGER - adversarial review of critic_3 patch 19 (rasterizer_xbox_environment_fog)
Slug: scratch/w/review_critic3_rasterizer_xbox_environment_fog. Base 5d3ca708. Claimed unit (review label).
Patch: rasterizer_environment_fog_screen_active -> rasterizer_environment_fog_screen_is_active (.c + symbols.json 5786).

## R1 apply + reference census
- `git apply --check` (and -v, --whitespace=error) of critic_3/patches/19 at 5d3ca708: clean. GNU-patch copy in
  patchroot/ (LF); CR-insensitive diff vs production = exactly 7 source lines (symbol-listing comment `_code_00155860`,
  prototype, definition, 4 call sites) + symbols.json line 5786 (name only, `"static": true` kept, order untouched).
- `git grep` of every tracked file for old/new name + `code_00155860`: outside docs/ and frozen research/ round-1
  snapshots, the only references are symbols.json:5786 and the .c (all updated). No header, parked.json, semantic
  entry, rejection entry, splits/ownership/relocs, tools or tests reference it. `fog_screen_active` (bss array,
  symbols.json 23177) is a different identifier, untouched (word-bounded).
- Integrator note: scratch/campaign gate_*/stable.json key it by section index (`::section:21`), so stable_verdicts
  sees the rename as the same exact section; name-keyed reports (report.json) will show old->new name.

## R2 emulated split (csplit, slug-local)
- build/tools/csplit.exe -i cachebeta.exe -p cfg_ctl -> split_ctl == build/split 833/833 (splitcmp.py).
- cfg_P (patched symbols.json) -> split_P: ONLY rasterizer_xbox_environment_fog.obj differs; rawcmp.py (name-resolved,
  own parser): 58 sections, all bytes/relocs identical; the sole symbol difference is #67 name (value 0, sect 21,
  type 0x20, storage 3 on both sides) -> IDENTICAL modulo the one rename.

## R3 gate / objcmp / objdiff
- gate prod (vs build/split) and cand (patchroot .c vs split_P via with_split.py): 26 EXACT / 1 residual both;
  identical rows (residual `__rasterizer_environment_fog_screen_begin` [size 4000!=3904, relocs 232!=235, sha]
  unchanged); renamed row EXACT 512 on both sides. gate_prod.txt / gate_cand.txt.
- rawcmp prod.obj vs cand.obj (rename map): all 116 sections identical except .debug$S, whose 5 differing bytes are the
  gate PID in the object path (146 B both; no function names in .debug$S). build/base vs prod.obj: identical except
  .debug$S (source path) -> gate compile == production.
- objdiff-cli 3.3.1 (sha1 3130e428) mini project objdiff/: prod, cand, and build/split-vs-build/base all
  6273/10174 code, 26/27 fns, 2018/2018 data, 91.724396%. Zero credit change (as claimed).

## R4 section-4 battery (battery_prod.txt / battery_cand.txt)
- object_audit FAIL(8) both (1 residual section + 7 D3D thunk storage 2/3) -> diff = rename only.
- pdb_storage: 7 disagreements both (same D3D thunks); renamed fn not in cachebeta publics -> static is right.
- surplus_identity 13 COMDATs / 1 not identical (_real_local_random) both; provider_link FAIL(1) both
  (_real_local_random vs effects, pre-existing). No change attributable to the patch.

## R5 first-party name evidence (independent)
- Sept-2001 cachebeta.map (static table): `_rasterizer_environment_fog_screen_is_active` 0001:00153510, the slot between
  `_local_random_boolean` and `_rasterizer_environment_fog_screen_wind_update` (January names both identically),
  size-to-next 0x200 == January 0x200. Aug-2001 map: same name (0x539010). Sept retail cache.map:
  `@rasterizer_environment_fog_screen_is_active@0`.
- Raw bytes (sept_body.py, Sept cachebeta.xbe read as data, VA = xbe section VA + map offset): 0 unmasked byte
  differences over 0x200 bytes; all 42 January relocations resolve at the same offsets to the same Sept names
  (global_window_parameters+N, rasterizer_globals+N, rasterizer_debug_options+N, __real@..., same assert
  literals, display_assert/system_exit); the .bss statics resolve with the same relative layout
  (600/608/612/276 vs +0x7458/+0x7460/+0x7464/+0x7314: deltas equal).
- Callers (callers.py): January relocs in screen_begin+0x79, screen_draw+0x2d, screen_end+0x32, model_begin+0x23 ->
  E8 at +0x78/+0x2c/+0x31/+0x22; Sept xbe E8 scan: exactly the same 4 callers at the same offsets (Aug: same 4).
- January-hash atlas (4cc87b45 = cachebeta.exe, 2002-01-14): 0x555860 = `_rasterizer_environment_fog_screen_is_active`
  (tier exact); re-anchored 7/7 other fog records at off-0x400000 to the current symbols.json names.
- January strings: only `fog_screen`, `rasterizer_environment_fog_screen`, `..._begin` - nothing contradicts.
  HCEX.pdb: neither name present (only the rasterizer.c public stubs + initialize/model_begin/model_end); no contradiction.
- Prior tree name was self-described "reconstructed rather than PDB-recovered" (docs ..._screen_end_reconciliation_20260909);
  astra wave17 (20260920) already recorded the Sept/Aug spelling as authentic; the remaining caveat there
  ("not identical January source revision") is closed for this body by the raw-byte identity above.

## R6 house rules / strip / held
- Pure identifier rename; no casts, parens, declarations, scopes, or statements change -> strip test and /Od
  declaration order not applicable. Rule 7: authentic first-party name replaces a descriptive one. Not a held class.
- No other R2 claim on the unit; triage_rasterizer_b/lab/lab_fog_rlr.c (lab only) still spells the old name.

## Verdict: APPROVE unchanged (no amendment needed). Zero credit.
