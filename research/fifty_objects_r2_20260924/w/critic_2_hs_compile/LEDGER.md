# critic_2_hs_compile LEDGER (round 2, completeness-critic route)

Base tree: 5d3ca708 (task said 7b9de585; 7b9de585..5d3ca708 touches only a ledger .md).
Claim: source/hs/hs_compile, label critic_2_hs_compile (claimed exit 0).

Route: storage packet — extern whitespace_characters / eol_characters / hs_type_primitive_parsers,
static hs_compile_finish (TU-local prototype), symbols.json static flags, csplit regen,
declaration-count sweep of hs_library_internal_compile.h consumers.

## Premise verification (read-only)
- pdb_storage.py source/hs/hs_compile (production): 202 split symbols, 4 disagreements:
  _eol_characters / _hs_type_primitive_parsers / _whitespace_characters split 3 ours 3 PDB-public True;
  _hs_compile_finish split 2 ours 2 PDB-public False. source/hs/hs: 0 disagreements.
- cachebeta publics: _whitespace_characters [00269F98], _eol_characters [00269F9C],
  _hs_type_primitive_parsers [00269FA0] (section 0013). symbols.json 16255-16257 file_offset
  2531224/2531228/2531232 = 0x269F98/0x269F9C/0x269FA0 (addresses match), all carry "static": true.
  No public at 0xB8790 (_hs_compile_finish, symbols.json:3145, no static flag).
- Internal consistency over every hs_compile function row in symbols.json: every "static": true row is
  absent from publics and every non-static row is public, EXCEPT _hs_compile_finish (non-static, not public).
- HCEX.pdb (DIA2Dump -sym, hcex_syms.txt / hcex_finish_full.txt / hcex_control_dispose.txt):
  hs_compile_finish: SymTag 5, `static function: true`, UndecoratedName `static void hs_compile_finish()`,
  no PublicSymbol. Control hs_compile_dispose: not static, PublicSymbol present.
  whitespace_characters / eol_characters / hs_type_primitive_parsers: SymTag 7 DataKind 0x6 (DataIsGlobal),
  types const char[2] / const char[2] / function* const[0x31], each with a SymTag 0xA PublicSymbol.
  => three independent sources agree: tables external, hs_compile_finish file-static.
- Consumers: only hs_compile.c references the four names; hs_library_internal_compile.h:32 is the only
  other declaration; includers of that header: source/hs/hs.c and source/hs/hs_compile.c only.
- Prior ledger hs_compile_obj_compile_finish_private_emission_20260902.md asserted "The target root is
  externally visible" without evidence; the split (symbols.json no static flag) inherited it.

## Baseline (production source at 5d3ca708)
- gate hs_compile --all: exact 61 residual 3 unwritten 0 (_hs_compile_postprocess size 704!=720 relocs 47!=48;
  _hs_parse_boolean size 288!=304; _hs_parse_set size 576!=560 relocs 54!=51). gate_base_hs_compile.txt.
- gate hs --all: exact 447 residual 1 (_real_random_range_evaluate sha). gate_base_hs.txt.
- gate recompiles == build/base objects (objcmp: 0 section / 0 symbol diffs for both).
- object_audit production: 202 January symbols 0 differ (split inherits the wrong flags) + 3 .text DIFF rows.
- No parks for hs or hs_compile in config/parked.json.

## Probe 1 (the packet) - final/hs/hs_compile.c + final/hs/hs_library_internal_compile.h
- Hypothesis: storage-only; tables are read by index / passed by address inside the TU; hs_compile_finish is
  void(void) with one caller, so no private-ABI cascade; header -2 names / .c +2 names keeps every function's
  preceding declared-name count unchanged in hs_compile.c.
- Change: drop `static` on the 3 tables; `static void hs_compile_finish(` definition; TU-local
  `static void hs_compile_finish(\n\tvoid);` in the prototypes block between hs_get_parameter_indices and
  hs_find_string_constant (definition-order slot 999 -> 1082 -> 1148); header prototype removed.
- Gate method: gate.py --edits redirect_edits.json rewrites the #include to the shadow header (absolute path).
  Negative control (redirect to NONEXISTENT.h) -> C1083, so the redirect is live.
- Measured hs_compile: exact 61 residual 3 unwritten 0, rows identical to baseline. objcmp base vs cand:
  222/222 sections 0 diffs; 480/480 symbols; ONLY _whitespace_characters/_eol_characters/
  _hs_type_primitive_parsers 3->2 and _hs_compile_finish 2->3.
- Measured hs.c (header consumer, -2 declared names, no hs.c edit; claimed source/hs/hs as consumer):
  exact 447 residual 1, rows identical; objcmp base vs cand 1410/1410 sections 0 diffs, 3753/3753 symbols 0 diffs.
- Alt placement (prototype at the top of the prototypes block, alt_proto_top.c): object identical to probe 1
  (0 section / 0 symbol diffs) -> placement inert; chosen slot is by definition order, not steering.

## csplit emulation (build/tools/csplit.exe -i cachebeta.exe -p <cfg>/config -o <split>)
- cfg_ctl (verbatim config copy) -> split_ctl vs build/split: 833/833 byte-identical (emulation faithful).
- cfg_S (4 symbols.json rows edited in place, CRLF preserved, no re-serialisation) -> split_S vs split_ctl:
  exactly 1 object differs = source/hs/hs_compile.obj; objcmp: 200/200 sections 0 diffs, only the same 4
  storage classes flip (tables 3->2, finish 2->3). tinfo --data identical.

## Section-4 audit (candidate vs split_S)
1. per-function (per_function_table.txt): 61/64 EXACT, 3 residual unchanged; every function storage t==o
   (22 external, 42 static); every candidate section == production section.
2. object_audit (audit_vs.py = object_audit with explicit split path; validated identical output on production):
   cand vs split_S: 202 January symbols 0 differ; only the 3 .text residuals -> FAIL(3) (was FAIL(3)).
   Controls: prod vs split_S FAIL(7) (4 storage rows), cand vs build/split FAIL(7) -> source and symbols.json
   must land together.
3. pdb_storage (pdb_storage_vs.py, same logic with SPLIT_ROOT/OURS_OBJ): cand vs split_S 0 disagreements
   (production 4). hs.c candidate 0.
4. Surplus: candidate-only surplus = .drectve + 9 pooled string literals, unchanged from production;
   no candidate-only .text COMDAT (surplus_identity n/a). provider_link_vs (SPLIT_ROOT=split_S): PASS, 9 literals
   both orders. Control cand vs build/split: FAIL(3) NO JANUARY PROVIDER for the 3 tables (expected without
   the symbols.json half). No other build/base or build/split object defines or references any of the 4 names.
5. Data: .rdata [65] 204 B (tables) ok; objdiff 3.3.1 mini project (od/): prod_vs_buildsplit ==
   cand_vs_splitS == canonical build/report.json row (matched_code 14906, data 6876/6876, 61 functions), all 64
   function rows identical -> zero scorer gain or loss.
6. Rejections/parks: none for hs_compile or hs.
- CL /Zs /W3: identical warning sets (16 lines hs_compile, 59 hs), only +2 line shift from the prototype.
- fake_match_scan: 0 leads base and candidate.

## Patch
- hs_compile_storage.patch (LF, 3 files: hs_compile.c 3 hunks, hs_library_internal_compile.h 1 hunk,
  symbols.json 2 hunks). `git apply --check -v --whitespace=error` clean at 5d3ca708; GNU patch on the HEAD
  blobs reproduces patch/b exactly.

## Verdict
ADMISSION_FIX_PROPOSED (storage truth, zero credit). Object NOT complete: 3 residuals remain
(_hs_parse_boolean is the held January-bug candidate; _hs_compile_postprocess and _hs_parse_set structural).
- Claims released (source/hs/hs_compile, source/hs/hs).
