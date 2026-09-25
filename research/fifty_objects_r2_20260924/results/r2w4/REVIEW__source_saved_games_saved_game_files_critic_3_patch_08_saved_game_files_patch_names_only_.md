# REVIEW source/saved games/saved_game_files (critic_3 patch 08_saved_game_files.patch, names only)

approve: True

amended_patch: None

## checks
All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_saved_game_files\ (LEDGER.md R0-R8, MANIFEST.md). HEAD is 5d3ca708; 7b9de585..5d3ca708 changes only one docs ledger. No tracked edits and no git/ninja mutation. The unit and its 7 consumers were claimed and then released.
1. Patch 08 passes git apply --check at HEAD (rc 0). The .c and .h changes are a pure word-boundary rename of the 4 identifiers plus 4 `_code_<addr>` updates in the symbol-listing comment. symbols.json gets 4 in-place name edits (lines 7350/7363/7364/7374) with flags, storage and order unchanged.
2. Names checked against raw bytes, not a masked stream (sept_cmp.py). In the raw 2001-09-25 cachebeta.map all 4 names are in the STATIC section of saved_game_files.obj, in the same order and at the same sizes (0x60/0x210/0x200/0x20). Sept `_enumerate_default_profiles` sits directly before public delete_all_custom_profiles, as January's 0x1b4a00 sits before 0x1b4a20. Every non-relocation byte of all 4 January functions equals the Sept xbe. Every relocation resolves to the same Sept symbol, except the known mapfile restructure callee: January append_entry_to_mapfile vs Sept enumerate_saved_game_file. The call graph closes: Sept enumerate_default_profiles calls the two Sept helpers, and January 0x1b4a00 calls 0x1b3520/0x1b3730. cachebeta.pdb has no public at any of the 4 addresses, so all 4 were January statics. The new names collide with nothing in source/, libs/, build/split or build/base. The January error literals ("failed to enumerate default playlist files ...") are identical in Sept and contradict nothing.
3. csplit emulation, run in my slug: the control split equals build/split for all 833 objects. With the patched symbols.json only saved_game_files.obj changes, and it is identical after the 4-name map (sections, relocations, symbol value/section/type/storage).
4. Owner compile: the header shadow was proven with an #error probe. Gate against the emulated split gives 45/1 with rows identical to production after the rename map (the only residual is the held enumerate_available park, [sha]). objcmp prod vs cand is IDENTICAL, and the production compile equals build/base.
5. All 7 includers of saved_game_files.h compile to IDENTICAL objects with unchanged gate rows. The #error probe fired in every one, so the shadow header was really used.
6. Battery, prod vs cand, identical on both sides:
   - object_audit: FAIL(1), the pre-existing residual; 152 symbols, 0 differ.
   - pdb_storage: 4 pre-existing disagreements; the renamed public row carries over.
   - surplus_identity: 0/0.
   - provider_link: PASS in both orders.
   - Data: every section identical, 6156/6156.
   - objdiff 3.3.1 (sha1 3130e428): 11709/12068 code, 45/46 fns, 6156/6156 data on both sides.
7. Composition: all 21 critic_3 patches apply in sequence on HEAD copies. saved_game_files.c/.h end identical to applying 08 alone (ignoring line endings). No config/ or tools/ file names the old symbols. stable_verdicts keys by section index, so the rename does not show up as a lost exact.
8. House rules and held classes: the patch adds no construct, scope, cast or declaration, so the strip test and the /Od declaration-order check do not apply. Authentic first-party names replace invented ones (rule 7). The enumerate_available bug hold, the only held item in this unit, is untouched.

## issues
No blocking issue; approve patch 08 as is. It earns zero credit, is byte-inert, and completes no object.
1. Defect in the critic's evidence (fixed during this review). critic_3's header-consumer check set only CL=/I<shadow>/source. marketing_and_strategic_business_development.c uses a bare `#include "saved_game_files.h"`, so the real header was compiled and that unit's "identical" result proved nothing. An #error probe shows this. I reran with /I<root> and /I<root>/saved games: the probe fired in all 7 includers and all 7 compile identical. The same gap may affect bare-spelled includers of progress_bar.h in patch 06; that is not my unit, so I did not check it.
2. Integration conflict. The held owner-queue storage packet (research/fifty_objects_20260925/w/owner_queue/ub_dead_camera_saved_game/patches/{,index_lf/}02_saved_game_files_storage.patch, which makes enumerate_memory_units static) applies at HEAD but not after 08. The only conflict is symbols.json line 7374, which 02 uses as context. The two changes commute: applying 02 after 08 gives the same result as 02 with the rename applied, checked on all 3 files. A rebased LF form is at scratch/w/review_critic_3_saved_game_files/oq02_rebased_on_08.patch (apply --check clean on top of 08). Held patch 03 still applies after 08 (offset -2).
3. Old name still used elsewhere. The LAB_ONLY helper-storage diff and owner-queue item 10 prose refer to `_saved_game_files_enumerate_default_files`. After landing, read that as `_enumerate_default_profiles`.
4. Pre-existing storage divergence, not introduced by this patch. `enumerate_default_profiles` is public and declared in saved_game_files.h but never called; its body is hand-copied at 2 sites. January had it static: it is not a PDB public, and split sections 42/43 call the two helpers directly, so its uses were inlined. The rename carries the authentic static name into the owner header. Fixing that needs the separately reviewed static + call-site packet, not a names packet; making the function static alone would drop it.
5. After landing, the integrator must regenerate csplit, which changes only saved_game_files.obj, and run the full ninja/sweep as usual.
