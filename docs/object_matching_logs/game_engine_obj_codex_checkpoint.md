# `game_engine.obj` Codex recovery checkpoint

## Scope and provenance

- Fresh lane: `jonas/game-engine-large-20260820`, based on
  `188aca952343415a52c6034ccd148e508c4be15c`.
- Recovered the independently reproducible source sequence through historical
  donor `e311fa706078abeb2ed8cc7b556909cc1de68c51`, then retained the policy-clean
  typed views from `bf1ff5e2a70fd88cac1d53c6f516c407671b7e01` only where they reproduced on
  the fresh base.
- The donor worktrees and the existing `game_engine_obj.md` ledger were treated
  as read-only. No divergent configuration, parks, shared-header, or unrelated
  documentation changes were imported.
- The recovery itself kept shared headers unchanged. The later policy
  correction on `jonas/game-engine-policy-fix-20260820`, based on
  `30dc49d4`, adds same-size named views to `source/game/game_engine.h` and
  `source/game/players.h`; the bounded consumer proof for those two header
  changes is recorded below.
- `source/game/game_engine` remains `NonMatching`.

## Policy correction

The corrective pass removed the remaining incompatible-call and raw-layout
accesses without tuning any residual function:

- Both statistic sort callbacks now use the exact `qsort` comparator type and
  read a typed `postgame_statistic_entry` view.
- The actual `game_engine` definition now owns correctly typed callback members
  at offsets `0x34`, `0x38`, `0x4C`, `0x50`, `0x54`, `0x64`, and `0x70`, while
  preserving the unused slot at `0x60`. Callers access those members directly;
  the unrelated-structure callback view and its cast are gone. Compile-time
  assertions cover those offsets plus `game_variant.unknown16` and
  `player_datum.target_hold_time`.
- `game_engine_update` uses a union of `data_iterator` and `object_iterator`
  and accesses only the active typed member.
- The game-engine global team counter, the game-variant field at offset
  `0x16`, and the player target-hold counter at offset `0x80` have named,
  size-preserving typed members. Their former integer-address/byte-array
  overlays are gone.
- Item-collection permutation access is through a typed permutation pointer,
  and the netgame-flag helpers now accept `real_point3d const *` directly.
- `code_0009bdf0` now uses a naturally scoped `collision_plane` for the point
  test and a separate zero-initialized `screen_flash_definition`. Its manual
  56-byte scratch union and byte-by-byte clear are gone. This safe form is
  nonexact and receives zero credit.

The pass adds no assembly, volatile access, force-inline directive, pragma,
barrier, incompatible function-pointer cast, integer-address dereference, or
byte-forcing construct. An additional audit advisory identified two
adjacent-global pointer bounds in the custom-motion/nav rendering loops. Those
bounds are byte-identical pre-existing debt from the base, not introduced by
this checkpoint, so this bounded follow-up leaves them unchanged and does not
invent aggregate or alias ownership to recover their relocation identity.

## Strict improvement

The same target object, compiler, splitter, and strict COFF comparator were
used on both sides of the checkpoint.

| Measurement | Fresh base | Recovered | Gain | Target |
| --- | ---: | ---: | ---: | ---: |
| Strict-exact functions | 125 | 164 | 39 | 180 |
| Exact meaningful code bytes | 12,879 | 22,996 | 10,117 | 32,397 |
| Exact padded code bytes | 13,904 | 24,240 | 10,336 | 33,760 |
| Exact semantic relocations | 539 | 1,210 | 671 | 1,679 |

All 125 baseline-exact functions remain strict exact. The 39 newly exact
functions are:

`_code_00096850`, `_code_00096890`, `_code_00096b30`, `_code_00096b40`,
`_code_00096b50`, `_code_00096c10`, `_code_00096d00`, `_code_00096e30`,
`_code_00096ed0`, `_code_00097020`, `_code_00098470`, `_code_00098510`,
`_code_00099b90`, `_code_0009a100`, `_code_0009a840`, `_code_0009a940`,
`_code_0009b260`, `_code_0009b4f0`, `_code_0009b6a0`, `_code_0009b770`,
`_code_0009baa0`, `_code_0009cb60`, `_code_0009cbe0`,
`_code_0009cc20`, `_code_0009cc80`, `_code_0009ccf0`, `_code_0009e9c0`,
`_game_engine_get_goal_position`, `_game_engine_player_added`,
`_game_engine_postspawn_player_update`, `_game_engine_remap_equipment`,
`_game_engine_remap_weapon`, `_game_engine_update`,
`_game_show_score_extended`, `_game_show_score_team`,
`_game_show_score_you_ally_enemy`, `_populate_statistic_buffer`,
`_sort_statistic_buffer_ranking`, and `_team_has_players`.

The raw whole-object SHA-256 is intentionally not used as an admission gate:
compiler-generated COFF metadata varies across otherwise identical rebuilds.
The strict gate fingerprints normalized bytes, padded sections, relocation
semantics, symbols, and owned data instead.

## Owned data

- `.data`: strict exact, 32/32 bytes, zero relocations, normalized SHA-256
  `daa87eb1c49a06849cf7cda46c7cbb445e5de697a99dcf5ebd09cea9d06dc707`
  on both sides.
- `.bss`: strict exact, 1,148/1,148 bytes, zero relocations, normalized SHA-256
  `509607d81ec2cc01c8776f0ef9a4adf126f8a4c2f021838d350f48d6e5a8d19d`
  on both sides.
- Public ownership offsets reproduce exactly: `_debug_player_color` at `.data`
  offset 0; `_global_goal` at `.bss` offset 0; `_global_variant` at 1,024;
  `_game_engine` at 1,128; `_global_autogenerate_list` at 1,140; and
  `_global_autogenerate_count` at 1,144. The private teleporter state occupies
  the target-backed intervening offsets 1,132 and 1,136.

## Fail-closed residual emission

Six nonexact standalone definitions are omitted, so objdiff reports no base
emission for them:

- `_game_engine_should_spawn_player`
- `_game_engine_get_place`
- `_code_0009a680`
- `_code_0009b3a0`
- `_game_engine_get_state_message`
- `_game_engine_player_killed`

Ten nonexact bodies remain emitted with zero credit because removing them also
removes independently exact private callees or callers:

- `_code_0009b6e0` is the natural private callee required by exact
  `_game_engine_player_added`, the three exact score-display functions,
  `_code_0009b770`, `_code_0009b6a0`, and `_code_0009a940`.
- `_game_engine_get_starting_location_rating` and `_code_0009c460` are the
  natural emission chain for exact `_code_00097570` and `_code_0009c340`.
- `_code_0009e670`, `_code_0009d140`, `_code_000994f0`, and `_code_0009a490`
  are the natural static cluster required to retain exact `_code_00096b50`.
- `_game_engine_post_rasterize_post_game` and `_code_00096ba0` are the natural
  emission chain required to retain exact `_code_0009cb60`.
- `_code_0009bdf0` is the naturally typed teleport/update body called by exact
  `_game_engine_update`; its former scratch overlay was exact but unsafe.

At the recovery checkpoint, omitting the original 15 residual bodies reduced
the exact set to 154 and the retained nine-body minimum restored 165. The
policy correction converts `_code_0009bdf0` into one additional emitted
zero-credit residual. Candidate emission remains 174/180 target functions:
164 strict exact, ten emitted residuals, and six omitted residuals.

## Validation

- Forced `game_engine.c` XDK 3911 compile: warning-free.
- Full Halo and libcmt dependency graphs: green; the final aggregate build had
  no remaining work after the forced translation-unit rebuild.
- The final policy-corrected translation unit is 164/180 strict exact, with
  22,996 meaningful and 24,240 padded exact code bytes. Direct callback-member
  access preserves every affected exact function; the sole loss is the honest
  safe-form withdrawal of `_code_0009bdf0`.
- Strict semantic report: 470 units, 3,865 functions evaluated, 3,724 semantic
  exact, 99 hidden exact, 61,015 hidden code bytes, 39 ordinary-only findings
  (38 structural and one rejected), zero unit errors, and 3,788 accepted exact.
- Progress after the clean rebuild: 375/833 complete objects, 3,777/11,060
  exact functions, and 460,004/2,198,102 code bytes overall; Halo is 273/468
  objects and 3,610/7,574 functions; libcmt is 102/212 objects and 167/476
  functions. The second global count change is rejection of the pre-existing
  stale `source/shell/shell_xbox:_main` ordinary false positive.
- Admission audit: zero candidates, zero revocations, and only the pre-existing
  `source/shell/shell_xbox` contradiction.
- Parked-function audit: three active, zero stale, zero invalid.
- Tool tests: 179/179 pass.
- A clean 569-object corrected-header rebuild was byte-for-byte reproducible
  after timestamp normalization. A bounded original-header/corrected-header
  A/B found raw deltas only in `game_engine.obj` and five consumers whose local
  compiler-label spellings moved with header line numbers. All sections,
  relocations, and meaningful symbol ownership in those five consumers were
  identical. In particular, frozen `ai_debug.obj` and `units.obj` are
  unchanged by the type correction; frozen `vehicles.obj`, `ai_script.obj`,
  and `actions.obj` were normalized raw-identical across the A/B.
- A second bounded A/B for the direct callback members rebuilt all 568
  non-`game_engine` consumers under the old and corrected declarations. 565
  were timestamp-normalized raw-identical. The only raw deltas were
  `players.obj`, `objects.obj`, and frozen `units.obj`; every section,
  relocation, and meaningful symbol was identical in all three. The other
  four frozen objects were normalized raw-identical. Before the assertions, a
  final corrected rebuild reproduced all 569 corrected snapshot objects exactly
  after timestamp normalization. Adding the zero-emission offset assertions
  produced only compiler-local-label raw deltas in six objects
  (`ai_debug`, `game_engine`, `players`, `network_game_globals`, `objects`, and
  `units`); every section, relocation, and meaningful symbol remained
  identical in all six.
- Policy, house-style, scope, frozen-object, and Markdown audits pass. No
  frozen source or configuration file is touched, and this Codex checkpoint is
  the only Markdown file modified.

## Disposition

This is a positive but incomplete recovery. Keep `game_engine.obj` active and
`NonMatching`; the 16 residuals receive zero credit until independently proven
strict exact.
