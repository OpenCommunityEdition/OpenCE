# `ui_widget_game_data_input_functions.obj` Fable salvage (2026-09-02)

## Decision

Admit this isolated source reconstruction as a zero-regression candidate based on canonical commit `88d0bd31eb25f74c9f7064c4e5fcb941ee8f52c1`.  The wave replaces address-derived function and data labels with independently attested semantic names, reconstructs all of the small callbacks that closed naturally, retains coherent ordinary-C implementations for the honest residuals, and leaves the three large network callbacks unwritten.  It does not claim the residual or merely semantic data bytes as exact.

The isolated address-normalized gate moved the translation unit from **1 exact / 0 residual / 45 unwritten** to **21 exact / 22 residual / 3 unwritten**.  Canonical target-metadata regeneration made `set_textbox_to_build_number` exact, and restoring five January-authenticated assertion diagnostics closed the other relocation-only callbacks.  The admitted canonical result is therefore **27 exact / 16 residual / 3 unwritten**.  The inherited 16-byte null callback remains exact.  The strict new code gain is **26 functions / 5,120 padded bytes**, with no inherited exact-address regression.

## Provenance and naming

The January split object, its assertions and string references, PDB-derived symbol evidence, and the later HaloCEA source were compared for each callback.  Public donor trees and the local campaign ledgers were used as supporting evidence only; the January object remained authoritative wherever the later donor simplified behavior or compiled to a different envelope.  Private linkage is recorded for `game_options_menu_update_text_desc`, `game_options_menu_update_pic_desc`, `spinner_list_3wide_determine_displayed_item_indices`, `list_indices_sort_proc`, and `filter_invalid_list_indices`.  The attested `mutliplayer_settings_select_list_update_displayed_items` misspelling is intentionally preserved.

All labels in this object's configured address range now have semantic names.  The 41-entry callback array is `game_data_input_function_list`; the BSS ownership is split into `displayed_servers` and `ui_widget_game_data_build_version_wide_string`.  No live definition uses a `code_<address>`, `data_<address>`, or `bss_<address>` name.

## Strict exact additions

| Function | Padded bytes |
|---|---:|
| `playlist_settings_menu_update_extended_description` | 176 |
| `netgame_prejoin_players` | 288 |
| `player_profile_edit_select_menu_update_extended_description` | 176 |
| `solo_game_objective_text` | 144 |
| `get_active_player_profile_display_name` | 176 |
| `get_editable_player_profile_display_name` | 176 |
| `get_editable_playlist_profile_display_name` | 192 |
| `multiplayer_game_set_text_box_for_map_name` | 448 |
| `multiplayer_game_set_text_box_for_game_ruleset` | 272 |
| `multiplayer_game_set_text_box_for_score_limit_type` | 176 |
| `multiplayer_game_set_bitmap_for_map` | 448 |
| `multiplayer_game_set_bitmap_for_ruleset` | 176 |
| `multiplayer_edit_profile_set_ruleset_textbox_string_index` | 176 |
| `system_link_status_check` | 48 |
| `warn_if_difficulty_will_nuke_saved_game` | 176 |
| `spinner_list_3wide_determine_displayed_item_indices` | 176 |
| `list_indices_sort_proc` | 48 |
| `player_profile_color_picker_update` | 400 |
| `mp_level_select_list_update_displayed_items` | 432 |
| `ui_widget_game_data_function_invoke` | 96 |
| `set_textbox_to_build_number` | 144 |
| `multiplayer_game_set_text_box_for_teams_noteams` | 112 |
| `multiplayer_game_set_text_box_for_score_limit` | 144 |
| `multiplayer_game_set_text_box_for_number_of_players` | 160 |
| `teams_no_teams_mp_game_bitmap_update` | 80 |
| `dim_if_no_system_link_cable` | 80 |
| **Total** | **5,120** |

## Data reconstruction

The callback table compiles to 164 initialized bytes with 41 relocations.  After address/name aliasing, its bytes, relocation identities, and normalized digest are identical to January (`4b22ee6807a2e4ddfd1bf16851609a7a009f4c6bd5aeb78162f121cb89492f02`).  This is **164 address-normalized exact data bytes** in addition to the strict code total.

The build-version buffer restores another 128 bytes of coherent semantic BSS.  The 40-byte `displayed_servers` declaration is correctly owned here but is not emitted by C2 until `server_list_menu_update` is written.  Neither BSS region is counted as strict exact data in this ledger because the old target exposes one aggregate 168-byte address label and the candidate target metadata has not yet been regenerated with the semantic split.

## Honest residual frontier

Six callbacks that were instruction-identical during isolated work are exact in
canonical. Regenerating the semantic target metadata closed
`set_textbox_to_build_number`; the other five referenced shorter donor
diagnostics rather than January's assertion text. Restoring the strings exposed
by the target object closed all five without changing executable behavior or
introducing compiler steering.

The strongest nontrivial candidates are:

- `solo_level_select_list_update_displayed_items`: 704/704 bytes and matching relocations; the aligned streams differ at two independent loop-continuation reloads.
- `player_profile_3wide_list_update`: 1,232/1,232 bytes; behavior is complete, with remaining C2 register/scheduling differences.
- `mutliplayer_settings_select_list_update_displayed_items`: coherent 1,184-byte candidate versus 1,424-byte January function.
- `player_profile_1wide_list_update`: coherent 1,040-byte candidate versus 1,152-byte January function.
- `game_options_menu_update_text_desc` and `game_options_menu_update_pic_desc`: safe initialized description indices are retained instead of reproducing target-visible undefined behavior solely to influence code generation.

No volatile or register forcing, fake dependency, optimizer pragma, manual byte emission, inline assembly, forced inline, representation pun, or nonsensical branch was admitted.  The remaining residuals should be revisited only with authoritative source/compiler evidence or a natural source shape; this wave does not add speculative park entries.

The only unwritten callbacks are the large network/UI routines:

| Function | Padded bytes |
|---|---:|
| `server_list_menu_update` | 2,016 |
| `network_pregame_status_screen_update` | 1,984 |
| `splitscreen_pregame_status_screen_update` | 1,216 |

## Header ownership and blast-radius audit

The public callback type, callback prototypes, dispatcher prototype, and table declaration are owned by `ui_widget_game_data_input_functions.h`.  Networking declarations used here are in `network_client_manager.h`; profile retrieval declarations are in their saved-game owner headers.  Translation-unit layout structures remain private to the implementation, avoiding the known C2 definition-position hazard from broad structure declarations.

Every non-owning translation unit that includes a changed broad header was gated both at the frozen base and with the candidate headers.  Exact/residual/unwritten sets were unchanged:

| Unit | Base | Candidate |
|---|---:|---:|
| `source/saved games/player_profile` | 9 / 0 / 13 | 9 / 0 / 13 |
| `source/interface/ui_widget` | 53 / 3 / 46 | 53 / 3 / 46 |
| `source/main/main` | 73 / 2 / 20 | 73 / 2 / 20 |
| `source/game/game_engine` | 169 / 11 / 0 | 169 / 11 / 0 |

`playlist_profile.h` is otherwise consumed by `ui_widget`, which is included in that sweep.  `network_client_manager.h` and this object's owner header have no other C consumers at this revision.  A canonical full build is still required after integration because this isolated lane was explicitly restricted from running shared `ninja`.

## Verification

- Final canonical `gate.py`: **27 exact / 16 residual / 3 unwritten**.
- Frozen-base exact set: the sole inherited exact callback is retained; strict delta **+26 functions / +5,120 padded code bytes**.
- Callback-table COFF comparison: 164 bytes, 41 relocations, identical normalized bytes and identities under semantic aliases.
- Header-blast gate sweep: no exact-set change in the four affected non-owning units listed above.
- `python -m pytest -q -p no:cacheprovider`: **258 passed**.
- `tools/fake_match_scan.py --fail-on-findings` over all changed C/header files: **0 review leads**.
- Park manifest audit against the available canonical report: **116 active / 0 stale / 0 invalid**.
- Object-admission audit against the available canonical report: **0 candidates / 0 contradicted / 2 pre-existing rejected / 0 revoked**.  This candidate is intentionally absent from that report because no shared full build was run.
- `git diff --check`: clean.

Final pre-commit source/object digests:

```text
B55B9C7F7A09BE5E9B5FE39F23D3DBD1A5339EC8C4CCD973D204DB0CE97502A6  config/symbols.json
C79BD3CB229DDD60E6ACFE28C8E656B2A54B56F8A3698DA9EA76E1535115F790  source/interface/ui_widget_game_data_input_functions.c
8B20BC74A12690357C0B7CC899798ABA45DFBAFA143E9ABE8B7D1F4D23FED30C  source/interface/ui_widget_game_data_input_functions.h
F89D2F255E052D611A5FAF8CCD59CD9AB3613B6FABC87623AF0C85C69C865651  source/networking/network_client_manager.h
F4565330AEE69A14AF39C5A8729C89C390B8B780A5AA3BE89E5556B3ECEC7005  source/saved games/player_profile.h
528BBF7F845E9F045F222B81A082FC4F32473E5BAC1D3F08F30950330DD99DBA  source/saved games/playlist_profile.h
95C2B5D0E5924171988716DAF81140B87D18F72CB8F04093BE550939442C8BDD  scratch/final_source_verified.obj
```

## Integration caveats

1. The semantic target metadata has been regenerated. The six formerly
   relocation-only callbacks are strict exact; the split BSS remains semantic
   reconstruction and is not credited as strict data.
2. Run the canonical full build and per-function exact-set sweep after cherry-pick; the isolated header blast sweep is strong but does not replace link-wide validation.
3. Keep the three large network callbacks as the next frontier.  Completing `server_list_menu_update` should naturally cause the 40-byte `displayed_servers` array to be emitted.
4. `player_profile_number_of_available_primary_colors` currently has an owner definition returning `short`, while an older interface-local declaration elsewhere says `long`.  This wave preserves the actual owner signature and does not expand scope to repair that pre-existing declaration mismatch.
