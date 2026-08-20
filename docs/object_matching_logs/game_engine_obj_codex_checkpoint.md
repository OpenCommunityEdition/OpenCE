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
- `source/game/game_engine.h` is content-identical to the fresh base. Callback
  typing, offset assertions, tag wrappers, and collision scratch typing are
  localized to `source/game/game_engine.c`.
- `source/game/game_engine` remains `NonMatching`.

## Strict improvement

The same target object, compiler, splitter, and strict COFF comparator were
used on both sides of the checkpoint.

| Measurement | Fresh base | Recovered | Gain | Target |
| --- | ---: | ---: | ---: | ---: |
| Strict-exact functions | 125 | 165 | 40 | 180 |
| Exact meaningful code bytes | 12,879 | 23,974 | 11,095 | 32,397 |
| Exact padded code bytes | 13,904 | 25,232 | 11,328 | 33,760 |
| Exact semantic relocations | 539 | 1,256 | 717 | 1,679 |

All 125 baseline-exact functions remain strict exact. The 40 newly exact
functions are:

`_code_00096850`, `_code_00096890`, `_code_00096b30`, `_code_00096b40`,
`_code_00096b50`, `_code_00096c10`, `_code_00096d00`, `_code_00096e30`,
`_code_00096ed0`, `_code_00097020`, `_code_00098470`, `_code_00098510`,
`_code_00099b90`, `_code_0009a100`, `_code_0009a840`, `_code_0009a940`,
`_code_0009b260`, `_code_0009b4f0`, `_code_0009b6a0`, `_code_0009b770`,
`_code_0009baa0`, `_code_0009bdf0`, `_code_0009cb60`, `_code_0009cbe0`,
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

Nine nonexact bodies remain emitted with zero credit because removing them also
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

Omitting all 15 residual bodies reduced the exact set to 154. The retained
nine-body minimum restores all 165 proven functions without granting any
residual exact or structural credit. Candidate emission is 174/180 target
functions: 165 strict exact, nine emitted residuals, and six omitted residuals.

## Validation

- Forced `game_engine.c` XDK 3911 compile: warning-free.
- Full Halo and libcmt dependency graphs: green; the final aggregate build had
  no remaining work after the forced translation-unit rebuild.
- Strict semantic report: 470 units, 3,773 functions evaluated, 3,631 semantic
  exact, 97 hidden exact, 59,719 hidden code bytes, 39 ordinary-only findings
  (38 structural and one rejected), zero unit errors, and 3,695 accepted exact.
- Progress: 375/833 complete objects, 3,685/11,060 exact functions, and
  448,699/2,198,102 code bytes overall; Halo is 273/468 objects and
  3,518/7,574 functions; libcmt is 102/212 objects and 167/476 functions.
- Admission audit: zero candidates, zero revocations, and only the pre-existing
  `source/shell/shell_xbox` contradiction.
- Parked-function audit: three active, zero stale, zero invalid.
- Tool tests: 179/179 pass.
- Policy, house-style, scope, frozen-object, and Markdown audits pass.
  `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units` sources/config are
  untouched; no existing Markdown file is modified or deleted.

## Disposition

This is a positive but incomplete recovery. Keep `game_engine.obj` active and
`NonMatching`; the 15 residuals receive zero credit until independently proven
strict exact.
