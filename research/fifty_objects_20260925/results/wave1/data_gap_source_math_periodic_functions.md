# source/math/periodic_functions

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/periodic/cand_c.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/patches/periodic_functions.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/fix_periodic2.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/periodic/cand_d.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/semantic_data_matches_additions.json"
]

## production_changes
(A) Case A credit, +96 B. Append the _global_periodic_functions_enum_strings entry from scratch/w/data_gap/semantic_data_matches_additions.json to config/semantic_data_matches.json:
  unit source/math/periodic_functions
  symbol _global_periodic_functions_enum_strings
  allow_incomplete_unit true
  measurements size 96, relocation_count 20, normalized_sha256 8fdd4bf9e8be002fe8938ee0849dfec244c27138f7b77eff28ec8ba7b4538614
(B) Zero-credit authenticity/ownership fix, verified:
  - git apply scratch/w/data_gap/patches/periodic_functions.patch. It:
    - deletes the invented struct periodic_functions_globals (with byte pad[3]) and the `#define function_tables_initialized periodic_functions_globals.function_tables_initialized` alias;
    - adds `static boolean function_tables_initialized = FALSE; static byte *transition_function_tables[NUMBER_OF_TRANSITION_FUNCTIONS] = { 0 }; static byte *periodic_function_tables[NUMBER_OF_PERIODIC_FUNCTIONS] = { 0 };` in that order;
    - makes periodic_function_build_table static and adds its static prototype.
  - symbols.json edits:
    - line 23079 becomes { "file_offset": 4547004, "flags": 0, "name": "_function_tables_initialized", "static": true },
    - insert after it { "file_offset": 4547008, "flags": 0, "name": "_transition_function_tables", "static": true },
    - then { "file_offset": 4547032, "flags": 0, "name": "_periodic_function_tables", "static": true },
    - add , "static": true to _periodic_function_build_table.
    - All of these are in patches/symbols_json_all.patch.
(C) The status must stay NonMatching because of the park.

## evidence
The .data section (96 B, 20 relocations) resolves identically, with flags c0300040 on both sides. The 'noise' and 'linear' literals are UNDEF in January (folded to other objects), and every relocation targets a '?$AA' literal, so objdiff 3.3.1 scores the section 70.83%.

The unit's six unparked functions are strict exact.

Whole-object audit finding: our .bss is 8-aligned and January's is 4-aligned. VC7 8-aligns a section only when it holds an object of 64 B or more, and our invented 76-byte struct triggers that.

What January actually had:
- The assert text is the bare '!function_tables_initialized'.
- HCEX.pdb lists File Statics periodic_function_tables (unsigned char *[12]) and transition_function_tables (unsigned char *[6]).
- HCEX also lists `static void periodic_function_build_table(short, unsigned char *)`, and cachebeta does not list it as a public.

Measured results:
- cand_c against a scratch csplit (only this object changes): 6/6 exact, .bss flags equal (align 4/4), 46/46 January symbols, 0 PDB storage disagreements.
- The only remaining difference is the parked function, whose bytes are unchanged (af8e65f5, the parked.json base).
- Control cand_d uses uninitialised statics and gets name-hash order with a 73-byte section, which is wrong. That proves January's `= 0` initialisers (declaration-order layout).
- _fast_ftol surplus is IDENTICAL to January's selected copy and provider_link PASSes.
- objdiff rows are unchanged.

## blockers
@periodic_function_build_variable_period_x_table@4 is parked (a two-instruction EDX/ECX scheduling tie), so the object can never be Matching until that park closes.

## reopen_criteria
Flip to Matching and drop allow_incomplete_unit only if the parked fastcall builder becomes strict exact under that park's own reopen criteria.

## task notes
Expected credit: +1,973 data bytes.
- +1,773 comes from the three Case A single-section entries (periodic 96, king 136, object_lights 1,541); scratch/tools/progress_calc.py takes data from 2,579,142 to 2,580,915, with 38 credits and 0 revoked.
- +200 is path_structure_bsp's ordinary objdiff credit after the Case B reconstruction.
- Code is unchanged and there are no function regressions.

The Case A entries and the ownership/source patches are independent. The entries verify against today's production objects, and I re-checked their snapshots under the proposed symbols/storage changes; they are unchanged.

Integrator order: apply the patches, then csplit plus ninja, then rerun the verifier and progress. The combined symbols.json edits (34 ops, scratch/w/data_gap/fix_all.txt) have been tested with a scratch csplit (cfg_all_split). They change exactly 4 split objects out of 833.

Tool caveat for other workers: tools/campaign/gate.py copies --source to scratch/_gate_PID.c, so a shadow header placed beside the candidate is silently ignored. scratch/w/data_gap/sgate.py compiles in place and honours shadow headers; I confirmed this with a #error test.

Code under-credit is already handled: objdiff scores periodic_function_build_table, transition_function_build_table, bitmap_copy and bitmap_draw_line below 100%, but all four are strict exact and already credited through build/semantic_report.json hidden_exact.

Systemic blocker found: 18 base objects emit a header-inline _distance3d whose bytes differ from January's selected action_vehicle copy. It also fails pair-link against action_vehicle's NODUP hand copy, and none of those objects is Matching. This alone stops king and object_lights from becoming complete objects.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/LEDGER.md
