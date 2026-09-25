# LEDGER - adversarial review: source/hs/hs_runtime storage packet (12 converters)

Reviewer slug: scratch/w/review_r2_hs_runtime_storage_12_converters (orchestrator slug contains ':' - illegal on Windows).
Tree: HEAD b7c1b7d9 (round-2 marker on f6d00a8c). Reviewing worker packet scratch/w/hs_runtime_storage/.

## R0 start
- git status clean at start. Read WORKER_BRIEF_R2 sections 0-9, worker MANIFEST, LEDGER, patch (12 hunks, 24 lines, `static ` prefix only).

## R1 patch reproduction + gate
- `git apply --check -v` of worker patch at HEAD b7c1b7d9: clean (rc 0).
- Independent reproduction: `git show HEAD:source/hs/hs_runtime.c` -> head.c; GNU patch -> cand.c; LF-normalised cand.c == worker cand.c (cmp identical).
  Every one of the 24 changed lines is exactly `'static ' + old line` (scripted assertion).
- Repo grep (excluding scratch/): the 12 names occur only in source/hs/hs_runtime.c (62) and config/symbols.json (12, lines 3203-3214, all `"static": true`).
- gate.py --all on head.c and cand.c: rows byte-for-byte identical text; `== exact 64 residual 2 unwritten 0 (of 66)`;
  residuals _hs_evaluate_begin_random [sha], _render_debug_trigger_volumes [sha] (untouched).
- Raw object diff base.obj vs cand.obj (same length 66653): 20 differing bytes = 8 bytes of PID in `scratch/_gate_<pid>` path
  (.debug$S + .file aux) + exactly 12 storage-class bytes (symbols idx 233.., _hs_long_to_boolean..._hs_object_to_object_list, 2->3).
  Symbol table order/names/sections/values/types identical (505/505). Strongest possible byte-inertness proof.
- My cand.obj vs worker cand.obj: only timestamp + PID path bytes differ.

## R2 audits (independent re-runs on my own objects)
- object_audit base.obj: FAIL(14) = 2 .text residuals + 12 symbol storage 3/2 rows. cand.obj: FAIL(2) = only the 2 .text residuals; 129 January symbols 0 differ. All data/.bss/.rdata sections ok in both.
- pdb_storage (prod build) 12 disagreements; worker's pdb_storage_cand.py (read: identical logic, ours=argv[2]) on my cand.obj: 0; on my base.obj: 12.
- cachebeta publics RVA 0xB9800-0xBAA00: only _hs_runtime_initialize, _hs_runtime_dispose_from_old_map, _hs_runtime_dispose, _hs_can_cast 0xBA410, _hs_cast 0xBA510. Converters (symbols.json file_offset 762400..762720 = 0xBA220..0xBA2E0; file_offset==RVA per _hs_can_cast 762896=0xBA410) are absent -> January file-static.
- HCEX DIA2Dump (my own run): hs_object_to_object_list / hs_real_to_long `static function: true`, `static long f(long)`; hs_cast has SymTag 0xA public, no static flag.
- provider_link base.obj: FAIL(12) NO JANUARY PROVIDER (the 12). cand.obj: PASS, 25 surplus rows PASS both orders.
- surplus_identity variant (same logic, my objs): 4 COMDATs IDENTICAL to January providers in base and cand.
- No January split object defines any of the 12 externally; scan of 1454 build/base+build/split objects: zero undefined references to any of the 12 (no link consumer can break).

## R3 scorer, warnings, scanners, composition
- objdiff-cli 3.3.1 (sha1 3130e428, build/tools) `report generate` on a private mini project (od/objdiff.json, build_target false; target = copy of build/split obj):
  prod/base/cand units identical: matched_code 10294, matched_functions 60, matched_data 11940/11940; all 66 per-function rows identical base vs cand
  (12 converters 100.0 in both); prod row equals canonical build/report.json. No scorer gain or loss.
- CL /Zs /W3 (warn.py, unit flags from build.ninja): 138 warning lines base and cand, identical incl. line numbers. Pre-existing C4028/C4133 on
  the matrix initialiser (typedef `long (*)(long)` at :383 vs union-spelled converters) unchanged - out of scope.
- tools/fake_match_scan.py (read first; stdout only): 0 review leads on head.c and cand.c.
- Sibling composition (r2w1 render_debug_trigger_volumes production.patch, LF-normalised): both application orders give the same file;
  sequential `git apply` in an out-of-repo scratchpad copy (sibling then storage; storage hunks offset -8) == compose file (LF-normalised).
  gate 65/1/0 (only _hs_evaluate_begin_random); object_audit FAIL(1) = begin_random only, 129 symbols 0 differ;
  vs sibling applied_crlf.obj: 173/173 sections 0 diffs, 507/507 symbols, only the 12 storage classes 2->3.
- Tooling/config references: no tools/tests/config file (other than symbols.json/parked.json/config.json) names hs_runtime; parks at
  parked.json:801/1339 are the two residuals (bytes unchanged). No link order file. `git apply --check --whitespace=error` clean; patch LF,
  12 hunks, 24+/24-, sha256 7217373f...bf0e.

## R4 house-rule / held-class / blocker review
- Only construct: `static` storage keyword. STRIP TEST: removing it restores base, whose code/data bytes are identical - the keyword buys no
  byte; it is a storage-truth fix attested by cachebeta publics (absent), HCEX (`static function: true`, `static long hs_<name>(long)`),
  and January split storage 3. Not decoration, not fake matching.
- No new scope/local/declaration (declaration count unchanged; /Od order N/A). Names unchanged and equal to HCEX names.
- Prototypes stay in the owning .c beside the other file-static prototypes (hs_inspect_*, hs_object_type_can_cast) - correct for statics.
- Not a held class (sec. 8), not a January-bug candidate, no header/COMDAT/P1 change, no admission blocker (sec. 9) introduced.
- Consumer check: zero external consumers (source grep + 1454-object undefined-reference scan).
- The 20260904 'Deliberate linkage exception' precondition (matrix absent) is lifted: matrix present since 4f4c8502 and its .data owner audits ok.
  Advisory: integrator should add a superseding note to that ledger when landing.

## Verdict
APPROVE. Mechanical, fully evidenced, byte-inert storage fix; removes 12 object_audit symbol rows, 12 pdb_storage disagreements and
12 provider_link failures. No credit claimed (correct). Sequencing with the sibling verified in both orders. git status clean.
