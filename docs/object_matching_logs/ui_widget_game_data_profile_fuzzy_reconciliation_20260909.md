# UI widget game-data profile fuzzy reconciliation (2026-09-09)

Canonical base: `105d9acaacda589e21a4389d7e6bd69db0b42aba`.
Opus donor: `C:\halo-worktrees\opus-30k-ui-script-profiles-20260906` at
`08d6bea92b1b7da925bf3687b44310b6203953dd`.

This pass reviewed the remaining donor UI-widget packet without importing the
translation unit wholesale. It retains four semantically complete or improved
fuzzy bodies for future work, but admits **zero new exact functions and zero
exact bytes**. Every one of the 37 inherited exact owners remains exact.

## Exact-candidate disposition

| donor candidate | padded bytes | disposition |
|---|---:|---|
| `_widget_instance_reload_recursive` | 16 | rejected: the donor makes a January-private owner public solely to force emission; `config/symbols.json` records `static: true` and January has no incoming relocation |
| `_ui_widget_reload_by_tag` | 16 | rejected for the same linkage/emission violation |
| `_game_options_menu_update_text_desc` | 304 | rejected: the donor exact body reads an uninitialized `description_index` |
| `_game_options_menu_update_pic_desc` | 304 | rejected: the donor exact body reads an uninitialized `description_index` |
| `_network_pregame_status_screen_update` | 1,984 | rejected: exactness requires retaining impossible non-null tests immediately after taking array-element addresses; canonical keeps the coherent zero-credit body |
| `_solo_level_select_list_update_displayed_items` | 704 | no transferable source change: donor and canonical function text agree; donor exactness is a translation-unit scheduling cliff |

The two public-wrapper changes are not harmless prototypes: they change
storage class and introduce code owners that January did not emit from this
translation unit. The two game-options bodies are explicit undefined behavior.
The pregame null tests are semantically impossible. None are acceptable under
the no-fake-match and no-nonsensical-logic rules, so none receive credit.

## Retained fuzzy frontier

All measurements below are target/candidate function-section measurements.
Trailing `0x90`/`0xCC` bytes are excluded from meaningful size. Exact credit is
zero for every row.

| function | target meaningful / padded | base candidate | reconciled candidate | relocations | result |
|---|---:|---:|---:|---:|---|
| `_splitscreen_pregame_status_screen_update` | 1,208 / 1,216 | unwritten | 1,208 / 1,216 | 48 / 48 | complete typed reconstruction; residual `[sha]` |
| `_player_profile_1wide_list_update` | 1,142 / 1,152 | 1,022 / 1,024 | 1,137 / 1,152 | 70 / 70 | exact envelope and relocation count; remaining schedule/hash residual |
| `_player_profile_3wide_list_update` | 1,222 / 1,232 | 1,217 / 1,232 | 1,222 / 1,232 | 71 / 71 | exact envelope and relocation count; one independent load transposition plus relocation-label artifacts |
| `_multiplayer_game_directions` | 315 / 320 | 307 / 320 | 309 / 320 | 11 / 11 | array-indexed player traversal fixes the target's induction shape; residual register/block schedule |

The retained frontier covers 3,887 meaningful target bytes / 3,920 padded
target bytes. This is useful reconstruction scope, **not matched-byte
progress**. `_solo_level_select_list_update_displayed_items` remains the
existing 693 / 704-byte fuzzy body unchanged because no authentic source edit
distinguishes the donor result.

`_player_profile_1wide_list_update` restores the source-shaped repeated tag
lookups, direct selected-index access, signed `NUMBEROF` loops, `PIN` bounds,
the default-profile flag enum, canonical `player_name` field, and January
diagnostics. `_player_profile_3wide_list_update` uses the conditional unknown
name fallback, `PIN`, and an explicit `highest_level` update. The splitscreen
body uses typed widget-definition access and team enum constants; the donor's
impossible array-address null check is omitted.

An additional donor reconstruction of
`_player_profile_update_cache_for_nwide_list` reached a relocation-identity-only
boundary, but it changed translation-unit scheduling and reopened the inherited
exact 1,424-byte
`_mutliplayer_settings_select_list_update_displayed_items`. The probe was
fully reverted. Preserving an inherited exact owner takes precedence over a
zero-credit fuzzy schedule improvement.

## Focused gate and audit

```text
python -B tools/campaign/gate.py source/interface/ui_widget_game_data_input_functions --all --out scratch/ui_game_final.obj
== exact 37  residual 9  unwritten 0  (of 46 listed)

python -B tools/fake_match_scan.py source/interface/ui_widget_game_data_input_functions.c --fail-on-findings
Scanned 1 file(s); 0 review lead(s); skipped 0 vendored path(s).

git diff --check
pass (Git reports only the existing LF-to-CRLF checkout warning)
```

The final gate preserves `_mutliplayer_settings_select_list_update_displayed_items`
at 1,424 bytes exact and preserves all other inherited exact rows. No header,
symbol, configuration, compiler-flag, inline schedule, or object-access owner
is changed. Parameters retain the project formatting, every modified void body
ends with an explicit `return;`, project scalar/enum/macro types are used, and
there is no inline assembly, volatile/register steering, representation
punning, fake dependency, or raw-address access.

No configure, Ninja build, push, or exact-progress/treemap update was
performed. This document intentionally records the best credible fuzzy source
without awarding matching credit.
