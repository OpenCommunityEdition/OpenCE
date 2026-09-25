# MANIFEST - source/hs/hs_compile storage packet (critic_2_hs_compile, round 2)

- Base/tip: 5d3ca708 (claude/fifty-objects-r2-20260924). The task named 7b9de585, and 7b9de585..5d3ca708 changes
  only one ledger .md. `git status` was clean (only the pre-existing untracked research/ dir) at the start and the end.
- Claims: source/hs/hs_compile (critic_2_hs_compile) and source/hs/hs as the header consumer. Both released at the end.
- Patch: `hs_compile_storage.patch` (LF, sha256 c247e7ed93c267b9..., 3 files, 6 hunks). `git apply --check -v
  --whitespace=error` is clean at 5d3ca708. GNU patch on the HEAD blobs reproduces `patch/b/*` exactly.
- **The source half and the symbols.json half must land together, followed by csplit regen (ninja).** Either half
  alone fails object_audit and provider_link (see the controls below).

## Changed constructs and evidence

| construct | change | January evidence |
|---|---|---|
| `whitespace_characters`, `eol_characters`, `hs_type_primitive_parsers` (hs_compile.c:636-638 after patch) | drop `static` | cachebeta publics `_whitespace_characters [00269F98]`, `_eol_characters [00269F9C]`, `_hs_type_primitive_parsers [00269FA0]`. These equal the symbols.json file_offsets 2531224/2531228/2531232. HCEX.pdb: SymTag 7, DataKind 0x6 (DataIsGlobal), each with a SymTag 0xA PublicSymbol. The types (const char[2], const char[2], function* const[0x31]) match our declarations. |
| `hs_compile_finish` | `static` definition plus a TU-local `static` prototype in the hs_compile.c prototypes block. Its prototype is removed from hs_library_internal_compile.h:32 | Not public in cachebeta (no public at 0xB8790). The unit's own publics: every other symbols.json hs_compile function row follows the rule static <=> absent from publics. HCEX.pdb: `static function: true`, `static void hs_compile_finish()`, no PublicSymbol (the control hs_compile_dispose is public and not static). The only caller is hs_compile_dispose (same TU). No other object references it. |
| symbols.json:3145 | add `"static": true` to `_hs_compile_finish` | as above |
| symbols.json:16255-16257 | remove `"static": true` from the 3 tables | as above |

The earlier ledger claim "The target root is externally visible" (docs/object_matching_logs/
hs_compile_obj_compile_finish_private_emission_20260902.md) gave no evidence and is contradicted by both PDBs. The
integrator should add a superseding note there.

## Per-function (candidate vs emulated split_S; full table in per_function_table.txt)

- 64 January functions: 61 EXACT, 3 residual. These rows are identical to production and to the canonical
  build/report.json. Every candidate .text section is section_infos_equal to the production section (cand==prod True x64).
- Rows touched by the packet (padded size / relocations / normalized sha256 prefix, target == candidate):
  `_hs_compile_finish` 256/19/48d82dd1a23bb519 EXACT (storage 3/3); `_hs_compile_dispose` 208/24/e771dfd286020f83
  EXACT; `_hs_parse_primitive` 288/24/3f4332b05bdc607f EXACT; `_hs_tokenize_primitive` 240/13/a3e5afe1e2977c4f
  EXACT; `_skip_whitespace` 240/9/030c590d7fb40908 EXACT.
- Residuals (unchanged, not in scope): `_hs_compile_postprocess` 720 vs 704, relocs 48/47;
  `_hs_parse_boolean` 304 vs 288 (held January-bug candidate); `_hs_parse_set` 560 vs 576, relocs 51/54.

## Objects

- hs_compile.obj, base vs candidate (objcmp.py): 222/222 sections with 0 diffs. 480/480 symbols: only the 4
  storage classes change (tables 3->2, finish 2->3).
- hs.obj (header consumer, -2 declared names, no hs.c edit): 1410/1410 sections with 0 diffs and 3753/3753 symbols with
  0 diffs. gate 447/1 identical rows.
- Prototype placement probe (top of the prototypes block): object identical to the chosen slot, so placement is inert.
- csplit emulation: split_ctl (verbatim config) == build/split 833/833. split_S (patched symbols.json) differs
  from split_ctl in exactly 1 object (source/hs/hs_compile.obj): 200/200 sections identical, and only the 4 storage classes differ.

## Section-4 admission audit

| check | production | candidate + split_S |
|---|---|---|
| gate --all | 61 / 3 / 0 | 61 / 3 / 0 (identical rows) |
| object_audit (audit_vs.py) | FAIL(3), 202 symbols 0 differ (split inherits the wrong flags) | FAIL(3), 202 symbols 0 differ; the 3 .text residuals only |
| object_audit controls | prod vs split_S FAIL(7) | cand vs build/split FAIL(7) |
| pdb_storage | 4 disagreements | 0 disagreements (hs.c candidate: 0) |
| surplus | .drectve + 9 pooled literals; no .text COMDAT | unchanged |
| provider_link (both orders) | PASS | PASS (SPLIT_ROOT=split_S). Control vs build/split: FAIL(3) NO JANUARY PROVIDER for the tables |
| data (tinfo --data / audit) | .rdata[65] 204 B ok | identical, ok |
| objdiff 3.3.1 mini project (od/) | matched_code 14906, data 6876/6876, 61 fns | identical. All 64 rows equal, so zero scorer effect |
| CL /Zs /W3 | 16 (hs_compile) / 59 (hs) | identical sets, +2 line shift only |
| fake_match_scan | 0 leads | 0 leads |
| parks / admission rejections | none for hs_compile or hs | none |

## House-rule review

- STRIP TEST: removing the storage keywords gives back production, which has identical code and data bytes. The
  keywords buy no byte match. They are storage truth, attested by cachebeta publics and HCEX.pdb.
- The static prototype is in the owning .c, in the existing static-prototype block. A function-local static
  keeps no prototype in a consumer header (rule 9). Parameter formatting follows rules 1 and 2.
- The globals are external without a header extern, like the precedent `typecasting_procedures` in hs_runtime.c. No
  consumer needs an extern, so no header declaration is added, which would also add a declaration-count effect.
- The net declared-name count before every hs_compile.c function is unchanged. hs.c loses 2 names and was measured
  byte-identical. The packet adds no new local, scope, cast, pragma or COMDAT.

## Credit

- 0 bytes of code, 0 bytes of data, 0 functions. This is an admission/storage-truth fix only. The object stays
  NonMatching (3 residuals).
- It removes the only 4 pdb_storage disagreements in hs_compile. When the residuals close, whole-object admission
  will no longer be blocked by storage that currently passes object_audit only because the split inherited it.

## Held / reopen

- `_hs_parse_boolean` is a held January-bug candidate (brief section 8). Not touched.
- `_hs_compile_postprocess` and `_hs_parse_set` are structural size and relocation residuals. Not in this route. Reopen them with
  new structural evidence (/Od statement shape) per their existing ledgers.
- The C4013 `hs_node_gc` implicit declaration inside the exact `_hs_compile_finish` predates this packet and is not touched
  (recorded E18-class observation in the opus5 ledgers).
