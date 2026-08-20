# `player_ui.obj` Codex strict checkpoint

This Codex-owned ledger records a bounded ordinary-C recovery of
`source/interface/player_ui.obj`. The object remains `NonMatching`: eleven
functions and eleven target-owned read-only sections are still absent. No
pre-existing or Claude-authored Markdown was edited or deleted.

## Scope and donor provenance

- Integration base: `4e5f17ee63c358b1f82dab8cc95fa87770cf4486`.
- The retained source comes from the cumulative historical donor chain
  `cd7b0109` through `f11db1b7929dfcc654ef2195045c76fede89792d`.
- The final independently rebuilt donor blobs are
  `85a072aa9a64a914959f21108c229239f474e6d8` for `player_ui.c` and
  `4aeb2317d615eef4bd1ad9c81588026655a528e4` for `player_ui.h`.
- Only `player_ui.c`, its self-consumed header, and this new ledger are in
  scope. No configuration, shared unrelated header, parked entry, semantic
  exception, or frozen-object source is changed.

## Strict code result

The untouched base had 12/42 strict-exact functions, 299 meaningful bytes,
384 padded bytes, and 28 relocations. The final forced XDK 3911 build has
31/42 strict-exact functions, 1,899 meaningful bytes, 2,144 padded bytes, and
164 relocations. The strict gain is therefore 19 functions, 1,600 meaningful
bytes, 1,760 padded bytes, and 136 relocations. All twelve baseline functions
remain exact.

| Newly exact function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_player_ui_set_single_player_local_player_controller` | 123 / 128 | 10 | `9ca46a9e2951819446d50a99db86ae41d8661068ccbbb046d305398f75abec4d` |
| `_player_ui_get_single_player_local_player_controller` | 79 / 80 | 6 | `065d6e3d23cd09e8702b2612568cc810063e52ef2a2f4475b252f54141a55c97` |
| `_player_ui_local_player_joined_multiplayer_game` | 76 / 80 | 6 | `8ee5f75b03146d45db5b6958ddc8787a4eb106a4dee51cf237eb94bcb121d91c` |
| `_player_ui_local_player_wants_to_play_multiplayer` | 66 / 80 | 5 | `becc7bacee552caed973847c3ba260c16048e328f0c572662b56f346b607c320` |
| `_player_ui_clear_multiplayer_autojoin_for_local_player` | 67 / 80 | 5 | `40057b463d23cc9636804d1f89c0dda3312dce875d3133ef2b19103a41c78b95` |
| `_player_ui_get_active_player_profile` | 86 / 96 | 6 | `99d8923857725a6c73f48c7bfbf5731dacc6e33fdee3140ba0536887807d4b61` |
| `_player_ui_get_last_single_player_level_played` | 67 / 80 | 5 | `caaf0e9a10a947409e419c0ad5eef9d8f76255b7fd86231d1ee8bff104d32865` |
| `_player_ui_set_game_variant` | 69 / 80 | 7 | `ba3554ae1baf5b98ceeb9ed42c236c264825246dd77ce891e8d46bcb83dfac5d` |
| `_player_ui_game_variant_specified` | 76 / 80 | 8 | `e37f2f861030e07378eee6bb9d1a82dc58f145c7e4492bf85d2507cab8a2962c` |
| `_player_ui_rumble_disabled` | 77 / 80 | 5 | `b10b52c1ec47eba6be7f5563ce29908fa2135a7a3e6fb744b104d8bcd71111ff` |
| `_player_ui_autolevel_enabled` | 159 / 160 | 11 | `8f2584b31a595d91a274a947a79063dc2033afc02a1c7d0bf193d09a032c7b95` |
| `_player_ui_get_path_to_local_player_profile_directory` | 49 / 64 | 2 | `22fcdbe42c905f13f4a2fe324db55d0f4cb984860e5ac3c13e86b650524b97e4` |
| `_player_ui_remember_player1_profile` | 95 / 96 | 11 | `07e82665406ed912831cbe280c390e09bcdff45465afbdf002ffa937d452aeaf` |
| `_player_ui_get_player1_last_used_profile_index` | 53 / 64 | 7 | `9cee8394afc3d5855a4e20a11cfea7c1a7702f5edce2d460b9712e796be61bdd` |
| `_player_ui_edit_profile_is_default_profile` | 65 / 80 | 5 | `34e097edccb3554d94927d9029cf3612830513f8b740972860c452bba6605f0a` |
| `_player_ui_edit_profile_name_is_dirty` | 99 / 112 | 9 | `4db8fa67795a9f3c202b8924203a9534f5da5a58115b56060852bd058dacc203` |
| `_player_ui_prompt_user_to_rename_edit_profile` | 90 / 96 | 8 | `5f73ad56907c2f54057f362def2c74c7eda1704f31e2352d4dd58e58b6e9f9ce` |
| `_player_ui_activate_all_solo_levels` | 41 / 48 | 4 | `5a2a73d3387df0f6c414b02e7c9b0012ac465d5ca5e7fef2ec6a3d120d7bc39f` |
| `_player_ui_begin_editing_profile` | 163 / 176 | 16 | `63ace4ff1927dd8dfc78dbf90bb263b59508dbc739acfa21663ac8854ac948c0` |

Every accepted function has the target's external storage class 2 and type.
Every relocation address, type, destination identity, and addend agrees with
January. The candidate object SHA-256 is
`861b63ca4329aacb84c69d2f1d266ea34836ccbed3c56d71ad0025ee2078cc5c`;
the January object SHA-256 is
`a99457d852cd4701dc43c9fc1229e782d4639d43d305a7027b9331380f354649`.

## Fail-closed residual boundary

The donor emits no nonexact body. All eleven residual target functions are
absent and receive no credit:

| Absent function | Meaningful / padded | Relocations |
| --- | ---: | ---: |
| `_player_ui_fast_setup_network_server` | 174 / 176 | 24 |
| `_player_ui_edit_profile_is_dirty` | 228 / 240 | 22 |
| `_code_000d0530` | 77 / 80 | 5 |
| `_code_000d05b0` | 64 / 64 | 5 |
| `_code_000d05f0` | 528 / 528 | 14 |
| `_code_000d0810` | 97 / 112 | 8 |
| `_player_ui_initialize` | 152 / 160 | 12 |
| `_player_ui_clear_multiplayer_joins` | 149 / 160 | 11 |
| `_player_ui_set_active_player_profile` | 100 / 112 | 8 |
| `_player_ui_save_profile` | 420 / 432 | 40 |
| `_player0_look_invert_pitch` | 101 / 112 | 11 |

Together they account for the remaining 2,090 meaningful bytes, 2,176 padded
bytes, and 160 relocations.

## Data and ownership

The ordinary progress report increases matched data from 816 to 820 bytes:
the unchanged external 816-byte `_player_ui_globals` BSS owner remains exact,
and the new external four-byte `_data_002fd5a4` owner is strict exact. The
initialized `NONE` datum has zero relocations and normalized SHA-256
`ad95131bc0b799c0b1af477fb14fcf26a6a9f76079e48bf090acb7e8367bfd0e`.

The hardened logical-owner audit additionally proves twelve naturally emitted
target string COMDATs exact. They total 546 bytes, have zero relocations, and
cover the controller/local-player assertions, source path, variant name,
player-one error, edit-state errors, and profile retrieval/index errors. Thus
14/25 target runtime non-code sections and 1,366/2,004 target bytes are
strict-exact; the unmatched eleven RDATA sections total 638 bytes and remain
absent.

The candidate emits one additional 61-byte controller-index assertion COMDAT.
The January object references that same symbol as undefined, while VC7 emits a
local copy. Its selection is `IMAGE_COMDAT_SELECT_ANY`, so the ordinary linker
coalesces it with the existing definition. It receives no target-owned data
credit. Target/candidate non-aux symbol inventories are 180/130, with the
missing target inventory confined to the fail-closed functions and RDATA.

## Source quality and verification

The retained implementation is readable typed C. It uses no assembly,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, undefined type-punning, raw byte-offset access, or byte forcing.
Parameters follow the vertical repository format, no-argument functions use
the explicit `void` form, and void functions end with explicit `return;`.

The unit remains `NonMatching`, with no park or semantic-exception entry. Full
Halo and libcmt builds pass. The semantic audit scans 470 units and 3,829
functions, finds 3,688 semantic-exact and 3,752 accepted-exact functions, and
reports zero unit errors. Campaign progress is 375/833 complete objects,
3,741/11,060 exact functions, 457,605/2,198,102 exact code bytes, and
1,803,160/4,176,062 matched data bytes.

Admission reports zero candidates and zero revocations, with only the inherited
`source/shell/shell_xbox` contradiction. Park validation reports three active,
zero stale, and zero invalid entries. The complete tooling suite passes
179/179 tests. A clean-commit forced-rebuild regression replay is required for
the final handoff.

## Typed edit-profile follow-up

A later policy audit found that the two edit-profile getters cast the same
generic buffer to unrelated incomplete structure types. The corrected model
recovers the two layouts already established by the surrounding APIs: a
48-byte player profile and a 104-byte playlist profile, each beginning with
the twelve-character name. The current and original edit slots are now unions
of those two complete types. Each saved-game-type branch writes, copies,
compares, edits, and returns only its corresponding member; compile-time size
checks preserve the 212-byte edit block and the 816-byte global layout. No
cast, inactive-member read, raw offset, or writable type-punning escape remains.

The forced XDK rebuild preserves all 31/42 strict-exact functions. The five
directly affected accepted functions retain their normalized hashes, padded
sizes, and relocation semantics, including both getters and
`_player_ui_begin_editing_profile`. The pre-correction regression manifest
reports all 31 accepted functions `still_exact`, no changed nonexact function,
no newly exact function, and no runtime data or symbol-ownership change. The
only object delta is compiler type information in `.debug$S`, expected from
replacing the generic buffer description with the recovered union; it receives
no runtime matching credit. The 1,366/2,004 strict target-owned data-byte result
is unchanged.
