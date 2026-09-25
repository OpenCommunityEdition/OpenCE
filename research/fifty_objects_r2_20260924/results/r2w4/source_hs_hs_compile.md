# source/hs/hs_compile

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
One combined LF patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_2_hs_compile\hs_compile_storage.patch (sha256 c247e7ed93c267b9..., 3 files, 6 hunks). (1) source/hs/hs_compile.c: drop `static` on whitespace_characters, eol_characters and hs_type_primitive_parsers (old lines 634-636); define `static void hs_compile_finish(`; add a TU-local `static void hs_compile_finish(\n\tvoid);` in the static-prototype block between hs_get_parameter_indices and hs_find_string_constant. (2) source/hs/hs_library_internal_compile.h: remove the hs_compile_finish prototype (old line 32). (3) config/symbols.json: line 3145 _hs_compile_finish gets "static": true; lines 16255-16257 lose "static": true. The rows are edited in place with no re-serialisation. All three files must land together, then a csplit regen (ninja) must run. `git apply --check -v --whitespace=error` is clean at 5d3ca708, and GNU patch on the HEAD blobs reproduces the intended files exactly. Integrator should also add a note superseding the 'target root is externally visible' claim in docs/object_matching_logs/hs_compile_obj_compile_finish_private_emission_20260902.md.

## evidence
Premise check: pdb_storage on production shows 4 disagreements (the 3 tables are split 3 / ours 3 / PDB-public True; _hs_compile_finish is split 2 / ours 2 / PDB-public False). cachebeta publics list _whitespace_characters [00269F98], _eol_characters [00269F9C] and _hs_type_primitive_parsers [00269FA0], which match the symbols.json file_offsets, and have no public at 0xB8790. Every other hs_compile function row follows the rule static <=> non-public. HCEX.pdb (DIA2Dump) independently shows hs_compile_finish as `static function: true` / `static void hs_compile_finish()` with no PublicSymbol (the control hs_compile_dispose is public), and shows the 3 tables as DataIsGlobal with PublicSymbols. No other build/base or build/split object defines or references any of the four names; the header's only includers are hs.c and hs_compile.c.

Measurements: gate.py with an in-memory include redirect to a shadow header (a negative control proved the redirect is live) gives hs_compile 61/3/0, identical rows. objcmp against production: 222/222 sections 0 diffs; only the 4 storage classes flip. hs.c (-2 declared names) gives 447/1 identical rows, 1410/1410 sections and 3753/3753 symbols with 0 diffs. A different prototype placement is object-identical, so placement is inert.

csplit emulation: the verbatim config reproduces build/split 833/833. The patched symbols.json changes only source/hs/hs_compile.obj (sections identical, 4 storage flips).

Section-4 audit (candidate vs emulated split): object_audit shows 202 January symbols, 0 differ, FAIL(3) for the 3 .text residuals only. Controls: each half alone gives FAIL(7). pdb_storage 4 -> 0. provider_link passes in both orders (9 pooled literals); the control against the old split gives FAIL(3) NO JANUARY PROVIDER. Surplus has no .text COMDAT. Data .rdata[65] is ok and tinfo is identical. objdiff 3.3.1 on a mini project matches the canonical row (matched_code 14906, data 6876/6876, 61 fns), with all 64 rows identical, so there is zero scorer effect. CL /W3 warning sets are identical (+2 line shift only). fake_match_scan: 0 leads. No parks or rejections apply. Full record: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_2_hs_compile\LEDGER.md and MANIFEST.md.

## blockers
The object is not complete; the packet is storage truth only (0 B code, 0 B data, 0 functions credited). Three residuals remain and are untouched. _hs_parse_boolean (304 vs 288) is a held January-bug candidate. _hs_compile_postprocess (720 vs 704, relocs 48/47) and _hs_parse_set (560 vs 576, relocs 51/54) are structural. Landing requires the source and symbols.json halves together plus a csplit regen; either half alone breaks object_audit and provider_link.

## reopen_criteria
Storage packet: none needed, since it is complete as proposed. For whole-object completion: _hs_parse_boolean only with owner approval of the held January-bug reproduction. _hs_compile_postprocess and _hs_parse_set only with new structural evidence (/Od statement shape or frame/relocation-multiset readout) beyond the negatives in the existing hs_compile ledgers.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_2_hs_compile\MANIFEST.md