# REVIEW - source/hs/hs_runtime storage packet (12 converters)

Reviewer slug: `scratch/w/review_r2_hs_runtime_storage_12_converters/`. The orchestrator's slug contains ':', which Windows paths cannot hold.
Tree: HEAD b7c1b7d9, the round-2 marker on f6d00a8c. `git status` was clean at the start and the end of the review.
Patch under review: `scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch` (sha256 7217373f...bf0e, LF, 12 hunks, 24+/24-).

## Verdict: APPROVE (no amendment needed)

The patch adds a `static` storage keyword to the 12 prototypes and 12 definitions of the hs_runtime converter functions. It changes nothing else. Every claim the worker made was reproduced independently.

## Independent checks
| check | result |
|---|---|
| `git apply --check -v` (and `--whitespace=error`) at HEAD | clean |
| Reproduction | GNU patch on the HEAD blob gives exactly the worker's cand.c. Each of the 24 changed lines equals `'static ' + old line` (scripted assertion). |
| gate.py --all, base and cand | identical rows: exact 64 / residual 2 / unwritten 0. All 12 converters are EXACT. The residuals are `_hs_evaluate_begin_random` and `_render_debug_trigger_volumes`, both untouched. |
| raw object diff base.obj vs cand.obj | Both are 66653 B. There are 20 differing bytes: 8 are the PID in the `scratch/_gate_<pid>` path (.debug$S and .file aux) and 12 are the storage-class bytes (2 to 3). The symbol table order, names, sections and values are identical (505/505). |
| object_audit | base FAIL(14) = 2 .text residuals + 12 storage rows. cand FAIL(2) = only the 2 .text residuals; 129 January symbols, 0 differ. All data sections ok. |
| PDB storage | production: 12 disagreements. cand (same logic): 0. |
| cachebeta publics | Publics in RVA 0xB9800-0xBAA00 are only `_hs_runtime_initialize`, `_hs_runtime_dispose_from_old_map`, `_hs_runtime_dispose`, `_hs_can_cast` and `_hs_cast`. The converters (0xBA220-0xBA38F) are absent, so they were file-static in January. |
| HCEX.pdb (own DIA2Dump run) | `hs_object_to_object_list` and `hs_real_to_long` are `static function: true`, `static long f(long)`. The control `hs_cast` has a SymTag 0xA public and no static flag. |
| provider_link | base FAIL(12) NO JANUARY PROVIDER. cand PASS: all 25 surplus rows pass in both orders. |
| surplus_identity (on my objects) | the 4 COMDATs are IDENTICAL to January's providers in both base and cand |
| consumers | Outside scratch, the names occur only in hs_runtime.c and symbols.json:3203-3214 (already `"static": true`). A scan of 1454 build/base and build/split objects finds zero undefined references, and no January object defines any of the names as external. |
| objdiff 3.3.1 (private mini project) | The prod, base and cand units are identical (matched_code 10294, matched_functions 60, data 100%), and all 66 per-function rows are equal. No scorer gain or loss. |
| CL /Zs /W3 | 138 warnings in base and cand, identical. The C4028/C4133 warnings on the matrix initialiser already exist in base. |
| fake_match_scan | 0 leads in base and cand |
| composition with sibling render_debug_trigger_volumes patch | Both orders produce an identical file. Sequential `git apply` works, with the storage hunks at offset -8. gate 65/1/0; audit FAIL(1) = begin_random only. Against the sibling's applied object: 0 section diffs, and only the 12 storage classes differ. |

## House rules / held classes / blockers
- STRIP TEST: dropping `static` gives back base, and base has identical code and data bytes. The keyword buys no byte match. It is a storage-truth fix backed by three independent sources: January split storage 3, the absence of the names from cachebeta publics, and the HCEX static flag.
- No new local, scope or declaration, so /Od ordering does not apply and the declaration count is unchanged. The names are unchanged and match HCEX.
- The static prototypes stay in the owning .c, next to the existing static prototypes.
- The change is not in a held class, not a January-bug candidate, and not a header, COMDAT or P1 change. It introduces no admission blocker.
- The 20260904 "Deliberate linkage exception" assumed the matrix was absent. That precondition no longer holds: the matrix has been present since 4f4c8502, and its `.data` owner audits ok.

## Integrator notes (advisory, not blocking)
1. The whole-unit claim is held by the sibling (render_debug_trigger_volumes). Either application order works.
2. Add a note to `docs/object_matching_logs/hs_runtime_obj_opus_reconciliation_20260904.md` that supersedes its "Deliberate linkage exception" paragraph.
3. Credit is 0 B, which is correct. The object stays NonMatching (`_hs_evaluate_begin_random` T tie; the trigger_volumes result depends on the sibling).
4. Out of scope and already present: the `hs_typecasting_procedure` typedef (`long (*)(long)`, matching HCEX) does not match the union-spelled converters (C4133). This patch leaves it unchanged.
