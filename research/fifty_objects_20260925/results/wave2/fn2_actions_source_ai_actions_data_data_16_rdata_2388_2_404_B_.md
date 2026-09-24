# source/ai/actions :: data (.data 16 + .rdata 2388 = 2,404 B)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\verifier_combined_extent.diff",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\semantic_data_entry.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\data_sim.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\od331\\mk.py"
]

## production_changes
PROPOSED (tools change, needs integrator/owner acceptance): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\verifier_combined_extent.diff. It passes git apply --check against tools/semantic_progress.py. For padded grouped entries, apply_semantic_data_matches now computes each section's extent the way objdiff's do_combine_sections does. The members must be every target section of that name, taken in section-table order, with the running offset aligned to max(align,4) after each section. The verifier also skips the coverage comparison when the unit has no unmatched data, so grouped entries stay safe under 3.6.0. Then add the grouped entry in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\semantic_data_entry.json to config/semantic_data_matches.json. It has 46 members, which is every January .data/.rdata section. Add `"allow_incomplete_unit": true` to credit the data before the function ruling; that path is also verified to give +2404.

## evidence
Every January data section is identical under coff_compare. _global_actor_mode_names is 16 B / 3 relocs. The 888-byte table (4 owners) has 96 relocs. The 37 string COMDATs and 7 float literals all match (object_audit OK). A one-unit objdiff 3.3.1 report (od331/mk.py) reproduces build/report.json: .data 57.14286, .rdata 98.38118. The cause is the '$'-literal relocation-name defect: 3 + 14 relocations point to ??_C@..?$AA@ strings. The brief's single-section entry cannot credit this unit, because the verifier requires the credited size to equal the whole-unit unmatched 2404 across two report sections. A grouped entry fails the current verifier: objdiff's combined .rdata extent is 2388 (running-offset alignment), while the member padded sizes sum to 2396 (raw 2322). Measured with the real function, it raises 'does not cover the reported unmatched sections'. On the patched copy: tools/test_semantic_progress.py passes 28; all 36 existing credited entries give identical results (shell_xbox grouped still +32); the actions entry credits exactly +2404 under 3.3.1 and credits nothing, without error, on a 3.6.0-like report. A full admission-audit simulation (patched verifier + entry + cand_exact.obj, unit marked complete) puts source/ai/actions at function_gap 0 / data_gap 0 and 'audit-coff-ownership-before-admission', with 0 contradicted. The three gate-exact switch functions that objdiff scores under 100% (set_default_state, combat_status, try_to_dive) are already credited through the semantic accepted ledger (hidden_exact).

## blockers
The brief allows only single-section data entries, and this unit mathematically cannot use one. Crediting needs either this verifier fix (tools change) or the pinned objdiff 3.6.0 scorer, which the lane froze at 3.3.1. Whole-object completion additionally needs the vehicle_entry macro ruling above.

## reopen_criteria
The integrator accepts the verifier fix and the grouped entry, or the lane moves to objdiff 3.6.0. Under 3.6.0 the data sections score 100% and no entry is needed.

## task notes
Summary of state:
- Landable now: production_fuzzy.patch. vehicle_entry is still not exact, but it is down to one 5-byte FP/int transposition, with an updated park measurement. It adds zero credit and changes no other section.
- Held: owner_gated_exact.patch makes the whole object's code 63/63 strict exact, but only with a TU-private ((x) * (x)) squaring macro. The owner must extend the 2026-09-20 actor_perception macro ruling to actions.c. If the owner approves, apply it INSTEAD of production_fuzzy.patch; it also retires the vehicle_entry park.

Data: object completion also needs verifier_combined_extent.diff plus semantic_data_entry.json, because a single-section '$'-defect entry cannot cover this unit under objdiff 3.3.1. With both in place, the admission audit reports the object as an admission candidate (function_gap 0, data_gap 0).

All work stayed under scratch/w/actions. No tracked file was edited, and no ninja or git-mutating command was run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\LEDGER.md
