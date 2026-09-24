# REVIEW source/interface/player_ui

approve: True

## per_object
[
 {
  "unit": "source/interface/player_ui",
  "approve": true,
  "issues": "Whole-object admission verified: 42/42 EXACT; object_audit PASS (68/68 symbols) against a real csplit regeneration with patch 05; all data 2004/2004; strings and provider link PASS; pdb_storage 0; fake scan 0; strip tests confirm `= { 0 }` on the path static is load-bearing and the only plain spelling that works. Admit only with reviewer patch 07 (and preferably 04-amended). 04 and 05 must land together."
 },
 {
  "unit": "worker packet as submitted (01-06 without 07)",
  "approve": false,
  "issues": "Patch 03 adds 2 new C4133 incompatible-pointer warnings in source/interface/ui_widget_game_data_input_functions.c. The consumer list (45) misses that unit, which includes both edited headers directly; the correct count is 46. Fixed by reviewer patch 07, verified byte-identical."
 },
 {
  "unit": "optB_delta_owner_flag_enum (optional)",
  "approve": true,
  "issues": "Claims verified: 45/47 consumers identical; only the parked main `_main_update_time` moves, so its park needs a re-baseline; units stays 189/189. Not needed for admission; owner-clean follow-on."
 }
]

## checks
All checks were run independently in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\ (LEDGER.md, REVIEW.md).

(1) Patch application. `git apply --check` passes for 01-06 against the current working files. The worktree has no .git, so git apply was used in no-repo mode. optB also checks clean on top of 01-06. Applying 01-06 in sequence to a copy gives all 11 cand_A2 files (EOL-normalised) and the worker's symbols.json exactly. My amended set (01, 02, 03, 04-amended, 05, 06, 07) applies to a fresh copy, keeps CRLF line endings, and passes `--whitespace=error-all`.

(2) Full-board sweep, not just the listed consumers.
- Method (tcomp.py/fullsweep.py): a full copy of source/ with the patches applied. Each unit is compiled in place with the build.ninja cflags and /I pointed at the copy. Each unit is compared with build/base per section index (name, flags, size, bytes, relocations by symbol) plus the whole symbol table. Compiler-local $L and $name$N label ordinals are normalised.
- Result: 453/454 source units are IDENTICAL. player_ui differs only by the new `_player1_profile_path` static and the two functions that now reference it. January exact counts are unchanged in all 454 units.
- Control: production source through the same script is 454/454 IDENTICAL to build/base.

(3) Consumer census with /showIncludes over all 454 units. game_engine.h has 38 consumers, player_ui.h 14 and playlist_profile.h 5, so the two edited headers reach 46 units. The worker's list of 45 misses source/interface/ui_widget_game_data_input_functions, which includes both edited headers directly. It is absent from their includes.json, which lacks 16 build units.

(4) /W3 warning diff on the 47 consumers, patched vs production. Without a fix, that missed unit gains 2 new C4133 warnings. With my patch 07 the net count of new warnings is 0.

(5) Exactness and object audit for player_ui.
- 42/42 January functions EXACT.
- I ran the real csplit (build/tools/csplit.exe -i cachebeta.exe) with a copy of config/. With production symbols.json it reproduces build/split for all 833 objects. With patch 05 only player_ui.obj changes.
- object_audit against that regenerated split: the candidate passes (68 January symbols, 0 differ). Production fails only because `_player1_profile_path` is missing, so 04 and 05 must land together.
- objdiff 3.3.1 mini-project: the candidate against the new split scores the same as production against the old split. That is 41/42 raw functions plus the known semantic entry at 95.89041, and data 2004/2004.

(6) Surplus, storage and scan checks.
- surplus_identity: 0 code COMDATs.
- provider_link: PASS.
- string_audit.py: each of the 3 surplus strings is UNDEFINED in January's player_ui split, equal to the selected copy in action_obey, input_abstraction or game_engine, with COMDAT selection 2 on both sides.
- pdb_storage: 0 disagreements. `_player_ui_globals` is public in cachebeta; the path is absent, so it is static.
- fake_match_scan: 0 leads on the 13 patched files.

(7) Evidence re-checks.
- HCEX types (DIA2Dump -type) confirm game_variant, universal_variant (with explicit pad0..2 and pad4..6), and the ctf, slayer, king, oddball and race variants, with names and order as proposed.
- The slayer labels in ui_widget_event_handler_functions (0x4C 'death bonus', 0x4D 'kill penalty', 0x4E 'kill in order') agree with the proposed names.
- The January split objects contain the race assert literal and the string "failed to retrieve editable game variant".
- The /Od build strings (universal_variant, human_readable_game_description, game_engine_variant, score_to_win, odd_man_out) agree with the names.
- relocs.json: +0x230 is referenced 6 times, only by `_player_ui_remember_player1_profile` (3) and `_player_ui_get_player1_last_used_profile_index` (3). The highest other offset is +0x22C, which fits a natural 0x230 struct.
- HCEX has player1_profile_path as a File Static char[0x100], and HCEX player_ui_globals has no pad members.

(8) Strip tests.
- Path uninitialised: the path lands at .bss 0 and exactness drops to 40/42.
- Globals and path both uninitialised: the globals become COMMON, which fails.
- So `= { 0 }` is the only plain spelling that works. The same file already uses `player_ui_globals = { 0 }`, and 86 file-scope `= {0}` statics already exist in source/. Race match_vassert -> match_assert is the plainer spelling and is identical. The pads and ball_spawn_delay are HCEX/January-attested. No new block scopes were added.

(9) Oracles, not landing choices.
- Adding HCEX's terminator_variant costs units `_unit_preprocess_node_orientations`, which supports the worker's omission.
- optB: 45/47 identical; only the parked main `_main_update_time` moves and units stays 189/189. The worker's claim holds.

(10) Other.
- The edited admission JSON loads through object_admission_policy.rejection_index.
- No tools, tests or docs reference the removed identifiers. The two tests that mention the rejection build their own fixtures.

## issues
APPROVED ONLY WITH the reviewer's mandatory amendment. As submitted, the packet would be rejected for one real defect.

DEFECT (fixed): patch 03 changes the return type of player_ui_get_edit_playlist_profile() to struct game_variant *. This adds 2 new C4133 warnings ("'=' : incompatible types - from 'game_variant *' to 'playlist_profile *'") at lines 2119 and 2757 of source/interface/ui_widget_game_data_input_functions.c. That unit keeps its own local `struct playlist_profile` view and was missing from the worker's census, so the 45-unit consumer list is wrong. The object bytes are identical, but the packet puts a type-incompatible pointer assignment into production source.

Fix: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\patches\07_ui_widget_game_data_input_functions_game_variant.patch. In get_editable_playlist_profile_display_name and multiplayer_edit_profile_set_ruleset_textbox_string_index, the local becomes `struct game_variant *profile` and uses human_readable_game_description and game_engine_index. It needs 01 and 03. Verified: all 181 sections IDENTICAL to production, the file stays at 44/46, and the 2 warnings are gone.

APPLY ORDER for the integrator:
1. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\01_game_engine_h_game_variant_layout.patch (SHARED header).
2. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\02_game_variant_member_renames.patch.
3. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\03_player_ui_h_game_variant.patch (SHARED header).
4. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\patches\04_player_ui_c_amended.patch. This is the worker's 04 plus 5 comment-only renames: the stale `_code_000d0530/05b0/05f0/0800/0810` names in the file's symbol comment become the symbols.json names (_hud_message_to_all, _generate_default_player_profile, _set_local_player_controls_from_player_profile, _clear_profile_edit_data, _reset_local_player_profile). The object is identical. The worker's 04 is also acceptable.
5. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\05_symbols_json_player1_profile_path.patch. The edited full copy is C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\config\symbols.json. It must land with 04, and build/split player_ui.obj must be regenerated.
6. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_ui\patches\06_admission_config.patch.
7. C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\patches\07_ui_widget_game_data_input_functions_game_variant.patch.

Full candidate files are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\cand\.

CONSUMER SWEEP LIST (corrected): the two edited headers reach 46 units.
- game_engine.h (38): the worker's 37 plus interface/ui_widget_game_data_input_functions.
- player_ui.h (14): the worker's 13 plus interface/ui_widget_game_data_input_functions.
- Lists: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_player_ui\consumers_header_union.txt, consumers_game_engine_h.txt and consumers_player_ui_h.txt.
- My full-board sweep already shows 0 moved sections outside player_ui, but the integrator should still run the full ninja.

ADVISORY (not blocking):
- The oddball and race members are typed `long`; HCEX types them as enums, which are still local to their files.
- universal_variant.flags keeps production's `unsigned long` (HCEX has long). The worker's `shr` argument is weak, since masking with 1 permits shr for signed values too, but keeping the existing type is the conservative choice.
- The system-default flag constant is duplicated locally in player_ui.c and playlist_profile.c. optB unifies it, but needs a re-baseline of the `_main_update_time` park; it is optional and verified.
- The reason text of the semantic_matches entry for `_set_local_player_controls_from_player_profile` still cites the playlist boundary and is now stale.
- Removing the #define aliases leaves a double blank line under the macros comment in race, oddball and slayer. Cosmetic only.
- Out of scope for this object: ui_widget_event_handler_functions still has its raw (byte *) and prefix views and 2 pre-existing wchar_t* C4133 warnings; ui_widget_game_data_input_functions still has its local playlist_profile and cached_variant_profile_entry views.
