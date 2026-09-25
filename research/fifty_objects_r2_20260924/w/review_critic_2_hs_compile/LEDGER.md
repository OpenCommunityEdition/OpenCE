# review_critic_2_hs_compile LEDGER (adversarial review of critic_2 storage packet)

Tree: 5d3ca708. Claimed source/hs/hs_compile + source/hs/hs (label review_critic_2_hs_compile).

## Independent verification (reviewer)
- Premise: pdb_storage (production) hs_compile 4 disagreements (3 tables split/ours 3 PDB-public True;
  _hs_compile_finish split/ours 2 PDB-public False); hs 0. cachebeta publics: _whitespace_characters [00269F98],
  _eol_characters [00269F9C], _hs_type_primitive_parsers [00269FA0]; no public at 0xB8790 (grep b879 / compile_finish: none).
  Control _hs_compile_dispose [000B8890] public.
- HCEX DIA2Dump (hcex_syms.txt): hs_compile_finish SymTag 5 `static function: true`, `static void hs_compile_finish()`;
  hs_compile_dispose not static; the 3 tables DataKind 0x6 (DataIsGlobal) + SymTag 0xA PublicSymbol.
- References: git grep (all tracked non-doc files): the four names only in hs_compile.c, header line 32, symbols.json;
  header includers only hs.c and hs_compile.c (config.json lists it only in the IDE header list).
- Patch: git apply --check -v --whitespace=error clean at 5d3ca708 (worktree CRLF / index LF, autocrlf=true);
  GNU patch -p1 on HEAD blobs in tree/ applies clean; diff -r base_tree tree = exactly the 6 intended hunks.
- gate hs_compile (patched .c + include redirect to patched header): 61/3/0, rows identical to base recompile.
  Negative control (redirect to NONEXISTENT) C1083 -> redirect live.
- objcmp2 (positional sections incl. relocation records by symbol name, positional symbol table):
  prod vs base recompile 0/0; base vs cand 222/222 sections 0 diffs, 480 symbols, only 4 storage flips at the SAME
  table positions (5,6,7 tables 3->2; 463 finish 2->3).
- hs.c with patched header: gate 447/1; objcmp2 vs build/base hs.obj 1410 sections 0 diffs, 3753 symbols 0 positional diffs.
  Negative control for hs.c redirect: C1083 -> live.
- Control: patched .c with the OLD header (extern prototype + later static) still compiles silently -> the header half
  is required for well-defined linkage (C99 6.2.2p7); the packet carries it.
- csplit emulation (own run, cfg_S = CRLF working-tree symbols.json with the 4 rows edited in place; equals the
  GNU-patched LF copy after EOL normalisation): split_ctl == build/split 833/833; split_S vs split_ctl differs ONLY in
  source/hs/hs_compile.obj: 200 sections 0 diffs, 456 symbols, 4 storage flips. Raw symbol records incl. aux
  (COMDAT selection) identical apart from the 4 storage bytes (base-vs-cand also shows .file/.debug$S aux = gate path only).
- refscan: in build/split, build/base and split_S only hs_compile.obj names any of the 4 symbols (defined or undefined).
- object_audit (wrapper = original with SPLIT_ROOT; verified byte-identical output on production):
  cand vs split_S FAIL(3) = the 3 pre-existing .text residuals only, 202 January symbols 0 differ.
  Controls: prod vs split_S FAIL(7); cand vs build/split FAIL(7) -> both halves must land together.
- pdb_storage logic: cand vs split_S 0 disagreements (prod 4); hs.c candidate 0.
- surplus_identity (cand, split_S): 0 candidate-only code COMDATs. provider_link (both orders, split_S): PASS,
  9 pooled literals; control vs build/split FAIL(3) NO JANUARY PROVIDER (tables).
- objdiff 3.3.1 (sha1 3130e428) mini project: all 4 pairings 14906/16469 code, 6876/6876 data, 61/64 fns,
  64 function rows identical to prod_vs_buildsplit and to canonical build/report.json -> zero scorer effect.
- CL /Zs /W3: hs_compile 16 warnings identical modulo +2 line shift; hs.c 59 identical. No C4211/linkage warning.
- fake_match_scan: 0 leads. No parks / admission rejections / semantic pins for hs_compile or the names.
- Worker final/ files and cfg_S symbols.json byte-equal my independent rebuild; worker objects equal mine.
- House rules: static prototype in owning .c static block (rule 9); void on own line (rule 1); external const tables
  without header extern have many precedents (typecasting_procedures, actions.c, actor_moving.c ...). STRIP TEST:
  removing the keywords returns production with identical bytes -> no byte lever, storage truth only.
  No new scopes/locals, so /Od declaration order is N/A. Held classes untouched (_hs_parse_boolean remains held).

## Verdict: APPROVE as submitted (zero credit). Integrator: apply all 3 files together + csplit regen; add the
superseding note to hs_compile_obj_compile_finish_private_emission_20260902.md.
