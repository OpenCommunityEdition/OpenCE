# source/hs/hs (ownership/storage packet)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/storage.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/static_symbols.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/pdb_storage.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/split_sim_keep/hs.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/gate_cand.txt"
]

## production_changes
Apply scratch/w/hs/production.patch to source/hs/hs.c (git apply --check clean against 954eebd2). It is storage.patch plus cleanup_only.patch.

Storage part:
- Add `static` to 465 prototype and definition lines: the 394 *_evaluate functions and 23 helpers (alphabetize, alphabetize_file_references, hs_allocate, hs_compile_source, hs_rebuild_source and the hs_enumerate_* functions). _hs_enumerate_scenario_data is excluded.
- Add `static` to the 28 `void evaluator( \` HS_EVALUATE macro heads.
- Remove `static` from the 39 `_hs_type_*_default` constants and from `hs_function_table_count`.

Then replace config/symbols.json with the edited copy scratch/w/hs/config/symbols.json. It has 838 lines edited in place (`, "static": true` appended), CRLF and order preserved, and no re-serialisation. The symbols are listed in scratch/w/hs/static_symbols.txt: 417 functions and 421 `_hs_*_definition` structs. Derive line ops with a diff, then run a csplit-only regen.

There is no shared-header change, so no consumer sweep is needed. No other TU references any of the 417 functions (grep over source/**/*.c and *.h).

## evidence
Storage truth comes from cachebeta publics (pdb_storage.py):
- 394 evaluators and 24 helpers are absent from the publics, so they were static in January. HCEX agrees for real_random_range_evaluate, hs_compile_source, alphabetize, hs_allocate and hs_enumerate_type_names.
- The 421 definitions are static in ours but external in the split.
- The 39 type defaults and hs_function_table_count are public in cachebeta. There is also link-level proof: January's hs_runtime.obj imports all 39 `__hs_type_*_default` symbols as undefined externals, and hs_runtime.c:696 already declares them `extern`.

One anomaly: _hs_enumerate_scenario_data is absent from the publics but has zero references anywhere in January's hs.obj. Making it static makes VC7 drop it (it becomes UNWRITTEN), so it stays external. This is disclosed in the ledger.

Verification:
- The applied patch re-gates at 447/1/0, identical to production. Every section is byte-identical to build/base; only storage differs.
- The /W3 warning census is unchanged at 58. fake_match_scan reports 0 leads.
- I ran csplit with the edited symbols.json into scratch. 832 of 833 split objects came out byte-identical; hs.obj changed only in the storage of 838 symbols (2 to 3), with 0 section-info changes.
- object_audit (simulated split vs cand.obj): 1830/1830 symbols match. The only failure is the `_real_random_range_evaluate` .text section.
- surplus_identity: 0 code COMDATs. provider_link: all 20 surplus literal COMDATs PASS.
- objdiff 3.3.1 mini-report is unchanged (25,248 code bytes, 447 functions, 18 data bytes).

## blockers
The object stays incomplete only because of `_real_random_range_evaluate` (see the unit above). With this packet the whole-object audit reduces to that single .text row.

## reopen_criteria
This packet is ready to land now; it earns zero credit and is score-neutral. After landing, the hs object can be admitted as soon as `_real_random_range_evaluate` becomes exact.

## task notes
The integrator can land scratch/w/hs/production.patch plus the edited scratch/w/hs/config/symbols.json (838 `static` flags; csplit-only regen) as a verified, score-neutral ownership correction. It includes the byte-identical real_random_range cleanup.

After it lands, hs.obj has exactly one blocker: the dependency-free `mov ecx,edx` order in `_real_random_range_evaluate`. Every other January section and symbol already passes the audit, the surplus literals pass provider_link, and the data is 100% under objdiff 3.6.0.

Tooling notes:
- I ran csplit.exe (the project's own tool; it reads cachebeta.exe as data) with a scratch config into a scratch output directory to simulate the regen. Only split_sim_keep/hs.obj is kept.
- I ran objdiff-cli 3.3.1, and 3.6.0 from another worktree, read-only against a scratch mini project. Nothing under build/, config/, source/ or tools/ was modified.
- The integrator should re-run pdb_storage after the regen. The one deliberate disagreement left is `_hs_enumerate_scenario_data` (unreferenced, so it must stay external).
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/LEDGER.md
