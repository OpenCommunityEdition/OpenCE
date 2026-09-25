# source/saved games/saved_game_files

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/08: only the 4 renames whose bodies are identical to Sept. find_or_create_directory->find_and_create_directory_if_necessary, enumerate_default_playlist_profile_files->enumerate_default_playlist_profiles, enumerate_default_player_profile_files->enumerate_default_player_profiles, and the public saved_game_files_enumerate_default_files->enumerate_default_profiles, which also changes saved_game_files.h. Zero credit.

## evidence
All 4 are identical to Sept. Gate 45/1 with SAME rows; objcmp identical; the 7 other includers of the header (plus progress_bar.h consumers) compile to identical objects; objdiff unchanged. The mapfile functions were restructured between Sept and January (0.67-0.97 similarity, reordered), so they were NOT renamed.

## blockers
Held class: January-bug candidate. Residual _saved_game_files_enumerate_available_to_local_player_index. object_audit FAIL(1) and pdb 4 are unchanged.

## reopen_criteria
A first-party name source for the restructured mapfile functions.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md