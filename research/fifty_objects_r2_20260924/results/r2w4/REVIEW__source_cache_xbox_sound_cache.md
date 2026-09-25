# REVIEW source/cache/xbox_sound_cache

approve: True

amended_patch: None

## checks
All checks were run at worktree HEAD 5d3ca708; the task text named 7b9de585. Slug: scratch/w/review_critic_3_xbox_sound_cache/. The orchestrator's slug string was garbled, so I shortened it. I claimed the unit and released it at the end. The tree is clean: no tracked edits, and no ninja, configure or git mutation.

1. `git apply --check` of patch 10 at HEAD passes. No other critic_3 patch has a symbols.json hunk near L7224. I also applied it with GNU patch to HEAD copies (patchroot/). It changes exactly 3 .c lines (listing comment, definition, lruv_debug_to_file argument) plus symbols.json L7224. `git grep` finds the old and new names only in historical docs and research. There is no header prototype and no use in tools, tests or config (parked.json, semantic*, ownership).
2. Name provenance, checked independently:
   - The raw Sept-2001 cachebeta.map, the Aug-2001 cachebeta.map and the Sept-2001 retail cache.map all list `_cache_block_get_sound_permutation_name` (@4 in retail) under Static symbols. It is 0x40 bytes and sits between delete_block_proc and start_loading_sound. The other 17 names in the object match ours.
   - The January atlas has the same name at 0x5add60, tier exact.
   - cachebeta.pdb publics do not list it, so it was a January static. HCEX.pdb has neither name.
   - No cachebeta.exe string contradicts the name ('block_name', 'permutation_name' and 'sound_cache_debug' are absent).
3. Byte identity, read from the XBEs as data with septcmp.py:
   - January's 64 B body against Sept cachebeta.xbe VA 0x1bc660: the only differing bytes fall inside the 7 January relocation fields; none outside.
   - Against Aug 1749betaP.xbe VA 0x1a4350: also nothing outside the relocation fields.
   - Sept start_loading_sound pushes this function's address at body offset +249. January's DIR32 relocation is at +250, the imm32 of that same push.
4. Emulated split: csplit on a config copy gives split_ctl, which equals build/split for all 833 objects. The patched config gives split_P, which differs only in xbox_sound_cache.obj. There it is identical modulo the rename across all sections including .debug$S, relocations by name, and the full symbol table.
5. Gate:
   - Production against build/split: 17 EXACT / 1 residual (_sound_cache_debug_render [sha]).
   - Candidate against split_P: 17/1 with the same rows; the renamed function is EXACT at 64 B.
   - Candidate against the un-regenerated split: 15/2/1. So the symbols.json edit and a csplit regen are required; both are in the same patch.
   - objcmp of prod.obj against cand.obj: identical modulo the rename (non-debug sections, relocations, symbol table). Data sections are unchanged.
6. Section-4 battery (with_split redirect, tools unmodified), production vs candidate:
   - object_audit: FAIL(3) on both, same rows.
   - pdb_storage: 6 disagreements on both, same rows modulo the rename.
   - surplus_identity: 2 COMDATs, 0 not identical, on both.
   - provider_link: PASS on both.
7. objdiff 3.3.1 (sha1 3130e428, verified), mini project: prod, prod(gate obj) and candidate all measure 2208/2576 code, 17/18 functions, 1224/1224 data, 98.03722 fuzzy.
8. symscan over build/split, build/base and split_P: no other object defines or references either name, so there is no collision and no consumer.
9. House rules and held classes: this is a pure identifier rename, so the strip test and the /Od declaration-order checks do not apply. Rule 7 improves, since an invented descriptive name becomes an authentic first-party name. Precedent: 7979cf8f admitted the xbox_texture_cache sibling callback under its Sept name `_texture_cache_name_block_proc`. The unit is not held, not reserved, has no COMDAT and touches no header.
10. Interaction check against the pending round-1 storage packet A (research/fifty_objects_20260925/w/xbox_sound_cache/storage.patch), dry-run both orders:
   - Each order fails on the other's hunks.
   - I built a rebased A on top of 10 and measured it with csplit, gate, objcmp, battery and objdiff (details in issues).
11. Not run: full ninja, pytest, board-wide stable-verdicts sweep (integrator).

## issues
No defect in patch 10. I approve it unchanged. It is a byte-inert rename that earns zero credit and completes no object. It swaps an invented descriptive name for an authentic first-party one. Three 2001 maps attest it, relocation-masked body identity holds against both the Sept and Aug XBEs, and the Sept caller pushes the function's address at the same site as January's relocation.

Integration notes:

(1) The symbols.json L7224 edit and the .c edit must land in one commit, followed by a csplit regen (ninja). Against the old split the candidate gates 15/2/1.

(2) Textual conflict with the pending round-1 storage packet A (research/fifty_objects_20260925/w/xbox_sound_cache/storage.patch). The R2 triage lists it as "still-pending zero-credit hygiene". Both packets edit .c L471 and symbols.json L7224, and each fails to apply after the other. A's symbols.json side is also a stale full-file copy: _xbox_sound_cache_globals is now at L23298. Never apply the round-1 A files as they are.
- If A is wanted, use the optional companion scratch/w/review_critic_3_xbox_sound_cache/storageA_rebased_after_10.patch. It applies after 10; its index hashes 30961b42/642529f7 are patch 10's post-images.
- I measured A+10 as byte-inert. Relative to 10 alone, only storage flips 2->3 (6 split rows, 4 candidate symbols). Gate rows are the same. object_audit goes from FAIL(3) to FAIL(1), leaving only the parked debug_render residual. pdb_storage goes from 6 to 0. provider_link passes and objdiff is identical.
- That companion is evidence only. It is not part of this verdict and needs its own review or ruling.

(3) Pre-existing and not introduced by this patch: the renamed function stays external in both source and split, while January had it static.

(4) The integrator's stable-verdicts diff should treat this as a rename at the same address, as in the 7979cf8f precedent.

Evidence: scratch/w/review_critic_3_xbox_sound_cache/ (LEDGER.md R1-R9, MANIFEST.md, gate_*.txt, battery_*.txt, objdiff/summary.txt, septcmp.txt, symscan.txt, splitdiff_ctl_vs_P.txt).
