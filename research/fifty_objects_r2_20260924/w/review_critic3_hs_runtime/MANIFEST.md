# MANIFEST - review_critic3_hs_runtime (critic_3 patch 14: hs_runtime_recompile_error -> script_error)

- Base/tip: worktree HEAD 5d3ca708; git status clean (untracked research/ only) at start and end. No tracked edit, no ninja/configure/git mutation.
- Claim: source/hs/hs_runtime claimed and released as review_critic3_hs_runtime.
- Verdict: APPROVE, no amendment. Zero credit (names are byte-inert).

## Name evidence (independent)
- Sept-2001 cachebeta.map: `_script_error` 0001:000b9320 hs_runtime.obj, Static symbols section; neighbours _hs_global_reconcile_write /
  _hs_runtime_get_executing_thread_name = January's neighbours; slot 0x40 = January padded size. Sept cache.map: `@script_error@12` (3 args).
- Aug-2001 cachebeta.map: `_script_error` hs_runtime.obj, static.
- Byte identity (xbecmp.py): Sept xbe 0xca320 and Aug xbe 0xbd5a0 vs January split function: 64 bytes, 6 relocs, 0 non-reloc byte diffs;
  Sept relocation operands resolve via the Sept map to the same 6 targets.
- January: not a cachebeta.pdb public (static); no January literal names either spelling; January atlas has no name at 0x4bace0; HCEX.pdb has
  neither name. No callers in January/Sept/Aug (body inlined into hs_arguments_evaluate in all three). /Od build: real function, same shape.

## Measurements
| check | production | candidate (patched .c vs own csplit split_R) |
|---|---|---|
| csplit | split_ctl == build/split 833/833 | only hs_runtime.obj differs; IDENTICAL under rename map (all sections, relocs, symbols; storage 3) |
| gate | 65 exact / 1 residual (_hs_evaluate_begin_random) | 65 / 1, row set identical modulo rename |
| objcmp | prod.obj == build/base (non-debug) | cand.obj == prod.obj under rename map (non-debug sections, relocs, symbol table) |
| object_audit | FAIL(1) pre-existing residual; 129 syms 0 differ | same |
| pdb_storage | 0 disagreements | 0 disagreements |
| surplus_identity | 5 COMDATs, 0 not identical | same |
| provider_link | PASS | PASS |
| objdiff 3.3.1 | 11374/14604, 61/66, 11940/11940 | identical |
| git apply --check | OK alone; all 21 critic_3 patches compose | composed hs_runtime.c identical to patch-14-only |

## House rules
- Only identifiers + symbol-listing comment change; authentic first-party name replaces an invented descriptive one; static storage kept;
  prototype stays file-local (static). No new constructs -> strip test / /Od declaration order not applicable.
- References: all 4 source occurrences + symbols.json L3220 updated in place (order untouched, "static": true kept). No parked/semantic/
  rejection/ownership/tools/tests reference. Historical prose mention in a dated docs ledger left as history.

## Landing note
- Source and symbols.json must land in one commit + csplit regen (patched source vs un-regenerated split shows the row UNWRITTEN).
