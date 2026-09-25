# MANIFEST - review of critic_3 patch 08 (source/saved games/saved_game_files, names only)

- Base/tip: worktree HEAD 5d3ca708 (task named 7b9de585; the difference is one docs ledger). No tracked edits, no
  ninja/configure/git mutation. `git status` clean except the pre-existing untracked research/ dir.
- Claim: `source/saved games/saved_game_files` by review_critic_3_saved_game_files. The 7 header consumers were claimed
  verification-only and released.
- Reviewed packet: scratch/w/critic_3/patches/08_saved_game_files.patch. It touches symbols.json (4 in-place renames) and
  saved_game_files.c/.h. `git apply --check` at HEAD returns rc 0.

## Evidence for the 4 names (sept_cmp.txt, LEDGER R2)
| January addr / size | current | proposed (Sept-2001 map, STATIC section) | raw non-reloc bytes vs Sept | relocations |
|---|---|---|---|---|
| 0x1b2ce0 / 0x60 | _find_or_create_directory | _find_and_create_directory_if_necessary | equal | 3/3 same callees |
| 0x1b3520 / 0x210 | _enumerate_default_playlist_profile_files | _enumerate_default_playlist_profiles | equal | 27/27 same except Jan append_entry_to_mapfile vs Sept enumerate_saved_game_file (known mapfile restructure) |
| 0x1b3730 / 0x200 | _enumerate_default_player_profile_files | _enumerate_default_player_profiles | equal | same, with the same single restructure callee |
| 0x1b4a00 / 0x20 | _saved_game_files_enumerate_default_files (public in our source) | _enumerate_default_profiles | equal | calls the two above, in both builds (call-graph closure) |

cachebeta.pdb has no public at any of the 4 addresses, so all 4 were January statics with no surviving January name.
The January atlas already carries the first 3 names at these addresses. No other definition or reference of the new names
exists in source/, libs/, build/split or build/base. The January error strings that inspired the invented names are
identical literals in Sept, so they do not contradict the Sept names.

## Measurements
- The patch is a pure word-boundary rename. It adds 4 symbol-listing-comment updates and changes no symbols.json flags or order.
- csplit emulation: split_ctl == build/split (833/833). split_08 differs only in saved_game_files.obj, and objcmp finds that
  object identical once the 4 names are mapped (storage unchanged).
- Gate: the candidate (header shadow proven by an #error probe) against split_08 scores 45/1. Rows equal production after the
  rename map. objcmp prod vs cand is IDENTICAL, and prod equals build/base.
- Consumers: all 7 includers compile to IDENTICAL objects, and the #error probe shows the shadow header was used in every one.
  The critic's check missed the bare-spelled include in marketing_and_strategic_business_development.
- Battery (prod == cand): object_audit FAIL(1) (pre-existing enumerate_available residual); pdb_storage 4 (pre-existing;
  the renamed public row carries over); surplus 0/0; provider_link PASS. Data 6156/6156; objdiff 3.3.1 identical.
- Composition: all 21 critic_3 patches apply in sequence, and this file ends identical to patch 08 applied alone.
- Credit: ZERO (names only). No object completes.

## Integration notes
- Owner-queue held packet ub_dead_camera_saved_game 02 (enumerate_memory_units static) has a context-only conflict after
  08 in symbols.json. The two changes commute. A rebased LF form is at oq02_rebased_on_08.patch.
- After csplit regen, the LAB_ONLY helper-storage diff and owner-queue item 10 prose use the old public name.
- A storage follow-up is still open, and this patch does not introduce it: `enumerate_default_profiles` is public/header
  declared while January had it static and inlined at the 2 hand-expanded sites.
