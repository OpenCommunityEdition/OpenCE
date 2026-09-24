# source/hs/hs (data gap 54,780 B)

## verdict
BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
None. I propose no semantic_data_matches entry, because no valid single-section entry exists for hs.

## evidence
In report.json (objdiff 3.3.1): .bss is 18/18. .data is 3,496 B at 96.83% and .rdata is 51,284 B at 99.64%, so matched data is only 18 bytes.

coff_compare (datasec.py) shows January's 1 .data, 909 .rdata and 1 .bss sections all resolved-equal to ours. data_census.py lists no absent and no different data symbol for hs, so there are no unwritten data symbols. Ours also emits 20 surplus literal COMDATs (78 B) that January's splitter gave to other objects; all of them pass provider_link.

I built a mini objdiff project in scratch/w/hs/od. Its control run reproduces report.json exactly. Scoring the same production objects with objdiff 3.6.0 (claude-aim-grenade's binary, run read-only) gives .data 100 / .rdata 100 and data 54,798/54,798.

So the entire 54,780 B gap is the 3.3.1 '$'-literal relocation-name defect. There is zero content difference.

The verifier (tools/semantic_progress.py) requires a single-section entry to cover all of the unit's unmatched data. hs has two merged unmatched sections, so no single-section entry can pass. A grouped entry would need 910 members, and its padded extent does not equal objdiff's merged 51,284 (51,328 alignment-padded, 49,626 raw). It also needs the unit to be marked complete.

## blockers
The lane scorer is frozen at objdiff 3.3.1. Crediting this data needs either the owner's scorer upgrade (3.6.0 recovers all 54,780 B at zero source cost) or a grouped-entry verifier ruling. Separately, the unit is not complete.

## reopen_criteria
Reopen when the owner moves the scorer to 3.6.0, or explicitly admits a grouped hs data entry once the object is complete.

## task notes
The integrator can land scratch/w/hs/production.patch plus the edited scratch/w/hs/config/symbols.json (838 `static` flags; csplit-only regen) as a verified, score-neutral ownership correction. It includes the byte-identical real_random_range cleanup.

After it lands, hs.obj has exactly one blocker: the dependency-free `mov ecx,edx` order in `_real_random_range_evaluate`. Every other January section and symbol already passes the audit, the surplus literals pass provider_link, and the data is 100% under objdiff 3.6.0.

Tooling notes:
- I ran csplit.exe (the project's own tool; it reads cachebeta.exe as data) with a scratch config into a scratch output directory to simulate the regen. Only split_sim_keep/hs.obj is kept.
- I ran objdiff-cli 3.3.1, and 3.6.0 from another worktree, read-only against a scratch mini project. Nothing under build/, config/, source/ or tools/ was modified.
- The integrator should re-run pdb_storage after the regen. The one deliberate disagreement left is `_hs_enumerate_scenario_data` (unreferenced, so it must stay external).
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/hs/LEDGER.md
