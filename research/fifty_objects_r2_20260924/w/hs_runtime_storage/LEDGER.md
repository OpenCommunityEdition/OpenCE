# LEDGER - hs_runtime storage packet (12 converters), round 2

Worker label: r2w1-hs_runtime (storage: 12 converters). Slug dir: scratch/w/hs_runtime_storage
(the orchestrator's slug name contains ':' which is illegal in a Windows path).
Tree: b7c1b7d9 (round-2 lane marker on f6d00a8c), git status clean at start.

## Claims
- `claim.py claim source/hs/hs_runtime r2w1-hs_runtime-storage` -> exit 3: whole unit HELD by sibling
  `r2w1-hs_runtime::_render_debug_trigger_volumes (1088 B)` (claimed 08:07:54, same second as its sub-key claim).
- Literal orchestrator claim `source/hs/hs_runtime (storage: 12 converters)` -> CLAIMED (exit 0).
- Consequence: this packet works only in scratch; the patch touches hunks disjoint from
  _render_debug_trigger_volumes and must be sequenced by the integrator with the sibling's patch.

## Probe 0: baseline (source/hs/hs_runtime.c at f6d00a8c)
- gate --all: exact 64 residual 2 (_hs_evaluate_begin_random sha, _render_debug_trigger_volumes sha) unwritten 0.
- pdb_storage: 129 split symbols, 12 disagreements: all 12 converters split 3 / ours 2 / NOT PDB-public.
- grep across source/ and libs/: the 12 names occur ONLY in source/hs/hs_runtime.c (prototypes :469-491,
  typecasting_procedures table :606-692, definitions :1541-1660). No header declares them.
- object_audit (base.obj == build/base obj, objcmp 0 section / 0 symbol diffs): FAIL (14) = 2 .text residuals
  + 12 symbol storage 3/2 rows. provider_link (base): FAIL (12) = the 12 converters "NO JANUARY PROVIDER"
  (they are external in ours, static in January, so no split object defines them).

## Evidence for file-static storage (first-party)
- cachebeta.pdb publics (scratch/tools/cachebeta_publics.txt): none of the 12 names is public. Code publics
  between RVA 0xB9C00-0xBA600 are only _hs_runtime_dispose 0xB9C20, _hs_can_cast 0xBA410, _hs_cast 0xBA510;
  the converters occupy 0xBA220-0xBA38F (csplit header listing in hs_runtime.c:46-68). Controls:
  typecasting_procedures, hs_cast, hs_can_cast ARE public and are external in source (consistent).
- HCEX.pdb (DIA2Dump -sym, hcex_all.txt): all 12 report `static function: true` and UndecoratedName
  `static long hs_<name>(long)`; control hs_cast has a SymTag 0xA PublicSymbol and no static flag.
- symbols.json:3203-3214 already mark all 12 `"static": true` -> split storage 3 (no csplit regen needed).
- Why they were left external: docs/object_matching_logs/hs_runtime_obj_opus_reconciliation_20260904.md
  "Deliberate linkage exception" - VC7 dead-strips static helpers until the genuine typecasting_procedures
  matrix exists. The matrix is now present (hs_runtime.c:606-692) and its .data section (owner
  _hs_type_inspectors, 9804 B, typecasting_procedures at +200) is object_audit `ok` -> the exception's
  condition is lifted.

## Probe 1: add `static` to the 12 prototypes (:469-491) and 12 definitions (:1541-1647)
- Hypothesis: address-taken via the table only, so no private ABI; storage-only change is byte-inert.
- Change: 24 lines, `static ` prefix only (cand.c; patch hs_runtime_converter_storage.patch).
- Measured: gate --all exact 64 residual 2 unwritten 0; per-function rows IDENTICAL to baseline.
  objcmp base.obj vs cand.obj: 172/172 sections byte+flags+relocation identical, 505/505 symbols,
  only the 12 storage classes change 2 -> 3.
- object_audit(cand): 129 January symbols, 0 differ; FAIL (2) = only the 2 .text residuals
  (_hs_evaluate_begin_random, _render_debug_trigger_volumes). pdb_storage on cand (pdb_storage_cand.py,
  same logic, ours=cand.obj): 0 disagreements (was 12). provider_link(cand): PASS (was FAIL 12);
  remaining surplus (19 literals, 2 __real, 4 COMDATs) PASS both orders; surplus_identity: 4 COMDATs IDENTICAL
  (production object; candidate sections are byte-identical to production).
- Patch: git apply --check clean in the worktree (LF patch, autocrlf=true); GNU patch on the f6d00a8c blob
  reproduces LF-normalised cand.c exactly.
- Composition with sibling (r2w1 render_debug_trigger_volumes c2.c, edits confined to lines 1018-1131):
  patch applies with offset -8; gate of composed file: exact 65 residual 1 (begin_random);
  objcmp vs sibling c2.obj: 0 section diffs, only the 12 storage classes differ.

## Verdict
ADMISSION_FIX_PROPOSED: mechanical, byte-inert storage patch. Object NOT complete: _hs_evaluate_begin_random
(T, parked private-register-convention, retired by triage) and _render_debug_trigger_volumes (sibling's claim)
remain. No symbols.json edit, no status flip, no parks to retire.
Out-of-scope observation: HCEX signatures are `long f(long)`; the 20260904 reconciliation ledger records
that forcing the later signature loses January identity - not re-probed here.

## Close
- Claim released (sub-key) with outcome ADMISSION_FIX_PROPOSED. git status clean (no tracked modifications).
