# source/bitmaps/bitmap_drawing

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/bitmap/cand_static.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/fix_bitmap.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/bitmap/lab_c10.c"
]

## production_changes
No data change.
Optional zero-credit ownership fix only: `static word const bitmap_bevel_sequence_indices[...]` (scratch/w/data_gap/bitmap/cand_static.c) plus , "static": true on symbols.json line 13458 (_bitmap_bevel_sequence_indices). Verified 7/7 exact, and its storage then agrees with cachebeta. The lab_*.c files are lab-only and must NOT be landed.

## evidence
January's .data is 2644 B and ours is 2628 B. contribs.json (module 392, size 2644, flags c0400040) proves the first 16 bytes belong to bitmap_drawing itself: 0xFF x10 followed by 0x00 x6, before the public _translation_table at +16.

Nothing ever references that prefix:
- all January split objects: zero references (resolved-address scan);
- the /Od build: zero, although the same 16 bytes sit at 0x9f0e00 in front of translation_table at 0x9f0e10, which is referenced;
- HEK sapien.exe: zero, with the same bytes at 0xA8CCB0.

The pattern is unique board-wide, so it is not a header static. It is not a public and must be an initialised file static.

Neither HCEX.pdb nor HCEX_Release.pdb has a bitmap_drawing module, and the atlas has only function names. The lab shows that a 10-byte char[], 5-short or 3-long prefix all give the identical layout, because VC7 16-aligns the 2592-byte table. So the type cannot be determined from bytes.

## blockers
The 2026-09-20 owner ruling holds every unnamed or unconfirmed global. Name and type of the 16-byte static are unattested, and inventing them would be fake extent-filling.

## reopen_criteria
A first-party source for the name and type of the initialised static that precedes translation_table: a PDB, source or HEK/PC symbol record for bitmap_drawing. Then define it in bitmap_drawing.c, re-split symbols.json at file_offset 2995832 with the real static name, and verify .data at 2644 B/c0400040 plus object_audit.

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
