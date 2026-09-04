# Devices, game, input and HUD integration — 2026-09-04

Baseline: `96086c9c4c187dd539af13bdaa9252d6e46ad8f6` on
`jonas/ai-debug-integration-20260829`. Publication destinations are that
branch and `jonas/exact-pilots` on bnunu/halo, after final verification.

## Verified retained progress

- Meaningful credited code: **894,038 -> 895,918** (+1,880).
- Credited functions: **6,121 -> 6,129** (+8).
- Strict snapshot: **6,168 -> 6,176** of 8,245 owners, **zero regressions**.
- Configured Matching objects: **392/833**, unchanged. No partial object is
  promoted or counted as fully complete by this batch.
- Credited data: **2,020,250 -> 2,021,802** (+1,552).

Seven newly reconstructed functions contribute 1,232 padded / 1,187
meaningful exact bytes:

| Packet | Exact functions | Padded / meaningful gain |
| --- | ---: | ---: |
| Devices group allocator | 1 | 112 / 103 |
| Xbox input SDK leaves | 3 | 224 / 206 |
| HUD-state migration | 1 | 128 / 118 |
| Game tick and map precache | 2 | 768 / 760 |

The eighth closure is an unchanged UI function that naturally becomes exact
after legitimate owning-header API additions: 704 padded / 693 meaningful
bytes. This is exact credit, not source newly written during this wave.

## Source and ownership review

Devices uses semantic private helper names, genuine scenario/device records,
typed access and project bit macros. Two reconstructed callers remain
unclassified parks at 78.333336% and 97.56626%. Their only gaps are documented
prefix scheduling and equivalent flag-expression lowering; no unusual mask
or source-shape forcing was admitted. No exact sibling was traded away.

Input uses actual XDK declarations, typed HANDLE fields and verified global
offsets. The broad xtl.h first pass emitted 432 bytes of unrelated D3D data;
narrow genuine exception/WinDef/WinBase/Xbox headers remove those owners while
preserving all 16 exact functions. No SDK prototype was copied or guessed.

HUD uses real instead of raw float, owns its true four-byte static
unit_hud_globals pointer, and moves six scripted APIs from the HS consumer
into hud_unit.h. Its new state copy has the exact assertion order and copies
the correct old record into the new one. HS retains all 445 inherited exacts.

Game retains game_tick and game_precache_new_map, 22 exact functions total,
two pre-existing residuals and three unwritten functions. The four difficulty
pointers and ordinary 0x5F8 profile_section naturally form the exact 0x608
DATA section with five string relocations; the static profile owner boundary
at file offset 3,005,888 is recorded without changing any contribution extent.
The real public game_globals pointer restores four-byte BSS ownership. Along
with HUD's four-byte owner, these explain all 1,552 credited data bytes.

The ABI audit corrected update_server_new to boolean(void), placed it in
player_queues_new.h, and removed main.c's wrong local void declaration.
transport_initialize is a status-returning API, not void; short follows the
established transport family. January's EAX writes prove returned 0/-5, not
the original source-width spelling. Other APIs were checked against actual
definitions/January bodies. New callers use their proper owner headers.

## Full game packet preserved, initializer pair withheld

The complete four-function game reconstruction is preserved at local commit
`d98ec50b9847ace3f1dfc26df42d3400b0cacacf`, branch
`codex/game-lifecycle-header-blocked-20260904`, worktree
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\game-lifecycle-header-blocked-20260904`.
All 20 source/header files were compared against the full candidate before
the split. Nothing was discarded; that branch is not canonically admitted.

Independent shadow-header compilation proves either the new Decals initializer
declarations or game_statistics_start's correct game.h declaration changes
two independent loads in the otherwise unchanged, previously exact
_decals_delete_permanent_from_cluster. A genuine redundant-forward-declaration
cleanup did not help. No ordering hunt, local-prototype workaround or test
waiver was used. Therefore game_initialize and game_initialize_for_new_map
(624 padded / 602 meaningful bytes) and those two causal header deltas are
withheld. Their bodies are absent from canonical, not hidden behind conditionals.
The two independent game closures and all unrelated safe work are retained.

## Bitmap park and incidental UI closure

The authentic cache-status enum is promoted from cache_files_windows.c to
cache_files.h and used by the new map-precache code. This changes a bitmap
park's fingerprint. Independent pristine/current-header compiles prove the
only change is swapping independent stack loads inside [0xFF,0x106); all other
bytes and all 59 ordered relocations are identical. Its 992-byte size,
60.99099% fuzzy score, five exact siblings and reopening criteria are unchanged.
The reviewed fingerprint update grants no exact credit. See
`bitmap_group_obj_cache_enum_reaudit_20260904.md`.

The unchanged UI routine _solo_level_select_list_update_displayed_items is
now strict exact: 704 bytes, 45 ordered relocations, normalized SHA-256
`085a6479228260634b66faf32f9a24a26b26970e7d1e186bdeb793fd3f44e828`.
The prior Fable salvage ledger recorded only independent reload-order gaps.
Independent source review confirms authentic typed list traversal, semantic
names, tag-access macros, owner prototypes and no fake source or extra helper.

## Validation

- Full Ninja build, semantic report and progress gate pass after the split.
- 284 tool tests pass; only the existing protected pytest-cache warning remains.
- 245 valid parks, zero stale or invalid records.
- Admission audit: zero candidates, contradicted labels or revocations; three
  existing ownership rejections remain unchanged.
- Complete candidate code audits find no new owners or point_from_line3d.
- Changed implementation/representative owning-header fake-match scan: zero leads.
- All inherited exact owners, including Decals and the sensitive Units sentinel,
  are preserved by the whole-tree stable sweep.
- Bitmap Utilities and Bitmaps header hashes remain
  `ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140` and
  `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`.
- Matrix Math, Vehicles and AI Debug implementation were not edited. Dependent
  rebuilds do not imply implementation changes. Existing blocked drafts remain.

Stable evidence: `scratch/devices-game-input-wave-before-20260904.json` and
`scratch/devices-game-input-wave-final-20260904.json`; final SHA-256
`c05ff98a2a16c1c56eb2b24850006754a4edafb2374a53be2cf648a0ba79bcac`.
The associated park/admission reports use the same wave prefix in scratch/.
