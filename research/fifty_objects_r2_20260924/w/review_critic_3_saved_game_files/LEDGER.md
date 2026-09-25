# review_critic_3_saved_game_files LEDGER (review of critic_3 patch 08)

## R0 setup
- HEAD 5d3ca708 (task named 7b9de585; 7b9de585..5d3ca708 = one docs ledger, verified with git diff --stat). Claimed
  source/saved games/saved_game_files as review_critic_3_saved_game_files.
- pr/ = HEAD copies of symbols.json + saved_game_files.c/.h with patch 08 applied by GNU patch -p1 (dry-run clean);
  `git apply --check` of patch 08 at HEAD: rc 0.

## R1 patch content (rename_check inline script)
- .c and .h: pure word-boundary identifier substitution of the 4 names (+4 `_code_<addr>` placeholders in the symbol-listing
  comment) reproduces pr/ exactly (24 hits in .c, 1 in .h). symbols.json: 4 in-place name edits (7350/7363/7364/7374),
  flags/storage/order untouched; line count unchanged.

## R2 January vs Sept raw bytes (sept_cmp.py -> sept_cmp.txt)
- Raw Sept-2001 cachebeta.map: all 4 names are in the STATIC symbols section of saved_game_files.obj, in the same order and
  at identical sizes (0x60, 0x210, 0x200, 0x20). Sept `_enumerate_default_profiles` @ .text 0x1b2240 sits immediately
  before public `_saved_game_files_delete_all_custom_profiles` exactly as January 0x1b4a00 precedes 0x1b4a20.
- Raw bytes (not capstone-masked): ALL non-relocation bytes of the 4 January functions EQUAL the Sept xbe bytes; every
  January relocation slot resolves in Sept to the same symbol (same string literals by mangled name, same callees), except
  the known Sept->Jan mapfile restructure (_append_entry_to_mapfile vs Sept _enumerate_saved_game_file) in the two
  enumerate_default_* functions. Sept `_enumerate_default_profiles` calls Sept `_enumerate_default_playlist_profiles` and
  `_enumerate_default_player_profiles`, January 0x1b4a00 calls 0x1b3520/0x1b3730: the pairing is closed under the call graph.
- cachebeta.pdb publics: no public at 0x1B2CE0/0x1B3520/0x1B3730/0x1B4A00 (0x1B4A20 delete_all_custom_profiles is public):
  all 4 were January file statics; no January-own name survives. Jan atlas already carries the 3 Sept names at 0x1b2ce0,
  0x1b3520, 0x1b3730 (exact tier; critic provenance.txt).
- No other definition/reference of the 4 new names anywhere in source/, libs/, build/split, build/base (grep): no collision.

## R3 emulated split + candidate gate
- csplit (build/tools/csplit.exe -i cachebeta.exe -p <cfg copy> -o <slug dir>): split_ctl == build/split 833/833 byte-identical;
  split_08 (patched symbols.json) differs from split_ctl in saved_game_files.obj ONLY; objcmp.py: every section's bytes,
  relocation (address,type,target) and every symbol (value,section,type,storage) identical after the 4-name map; storage
  unchanged (the three statics stay 3, `_enumerate_default_profiles` stays 2 in the split, as `_saved_game_files_enumerate_default_files` was).
- Owner compile with CL=/I"pr/source" (shadow header proven by an #error probe copy): gate vs split_08 45/1, rows identical to
  production after the rename map (only residual `_saved_game_files_enumerate_available_to_local_player_index` [sha], the held
  January-bug park); objcmp prod.obj vs cand.obj IDENTICAL (names mapped); prod.obj == build/base object.

## R4 header consumers (consumers.py -> consumers.txt)
- 7 includers of saved_game_files.h. FINDING: critic_3's consumer check (CL=/I<shadow>/source only) did NOT exercise
  marketing_and_strategic_business_development, which spells `#include "saved_game_files.h"` (bare) - my #error probe shows the
  real header wins in that configuration (probe A compiles fine). Rerun with CL=/I<root> /I<root>/saved games: #error probe
  fires for all 7, and all 7 consumers compile to IDENTICAL objects (sections, relocations, symbols; gate rows unchanged).
  Harmless for this patch (the consumer names none of the 4 symbols), but the critic's 8/8 claim was partly vacuous;
  the same shadowing hole may apply to bare-spelled includers of progress_bar.h (patch 06) - not my unit.

## R5 section-4 battery (battery.sh -> battery.txt, battery/*.txt; candidate via with_split.py split_08 + obj/cand.obj)
- object_audit: prod FAIL(1) == cand FAIL(1) (the pre-existing enumerate_available [sha] residual; 152 January symbols, 0 differ).
- pdb_storage: 4 disagreements both sides (enumerate_memory_units, take/release_mapfile_mutex, and the renamed row:
  `_saved_game_files_enumerate_default_files` -> `_enumerate_default_profiles`, split 2 ours 2, PDB static). Unchanged count.
- surplus_identity 0/0 both; provider_link PASS both (both link orders built in). Data: every section byte/reloc identical
  (objcmp), objdiff 3.3.1 (build/tools/objdiff-cli.exe sha1 3130e428) prod vs cand identical: 11709/12068 code, 45/46 fns,
  6156/6156 data, only enumerate_available <100 (93.27) on both sides.

## R6 composition
- All 21 critic_3 patches apply sequentially (git apply outside the repo via GIT_CEILING_DIRECTORIES) on HEAD copies of the 24
  touched files; saved_game_files.c/.h after the full sequence == 08 alone (EOL-insensitive).
- INTEGRATION NOTE: the held owner-queue storage packet research/fifty_objects_20260925/w/owner_queue/ub_dead_camera_saved_game/
  patches/{,index_lf/}02_saved_game_files_storage.patch (enumerate_memory_units static; applies at HEAD) has line 7374 as
  symbols.json CONTEXT, so it no longer applies after 08 (context conflict only). It commutes semantically (08∘02 ==
  rename(02∘HEAD), checked on all 3 files); rebased LF form: oq02_rebased_on_08.patch (git apply --check clean on pr/).
  03 (the held bug patch) still applies after 08 (offset -2). The LAB_ONLY helper-storage diff and owner-queue item 10 text
  name `_saved_game_files_enumerate_default_files`: read as `_enumerate_default_profiles` after landing.
- scratch stable baselines (scratch/campaign/gate_*/stable.json) key by section index (tools/campaign/stable_verdicts.py), so
  the rename does not look like a lost exact there; no config/ or tools/ file names the old symbols.

## R7 house rules / held classes
- Names-only: no new construct, scope, cast, declaration or filler; strip test and /Od declaration order not applicable.
- Authentic first-party Sept names replace invented ones (rule 7). January strings "failed to enumerate default playlist files ..."
  / "... player profile files ..." (the origin of the invented names) are identical literals in Sept, where the functions are
  named enumerate_default_playlist_profiles / _player_profiles: no contradiction.
- Held class: saved_game_files is a canonical January-bug hold (enumerate_available park). Patch 08 does not touch that
  function or its park key; it is a separate zero-credit names packet.
- Pre-existing, NOT introduced: `enumerate_default_profiles` is public in our source + saved_game_files.h (never called; its body
  is hand-expanded at 2 sites) while January had it static (not PDB public; split sections 42/43 call the two helpers directly
  = inlined uses). The rename carries the authentic static name into the owner header; fixing storage needs the reviewed
  static+call-site packet (LAB_ONLY diff), not a names packet.

## R8 verdict
- APPROVE patch 08 as is (no amendment). Zero credit, byte-inert, names proven by raw bytes + map order/size + call-graph
  closure + PDB storage. Critic defect found: consumer check did not exercise the bare `#include "saved_game_files.h"` in
  marketing_and_strategic_business_development (vacuous for that unit); closed here, identical.
