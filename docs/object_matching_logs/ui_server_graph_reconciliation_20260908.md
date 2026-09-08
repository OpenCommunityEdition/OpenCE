# UI server graph reconciliation (2026-09-08)

Canonical base: `f7afb70475c862d1934c26d698b49be76053d418`.
Opus donor: `C:\halo-worktrees\opus-30k-ui-script-profiles-20260906` at
`08d6bea92b1b7da925bf3687b44310b6203953dd`.

This packet reconciles only `server_list_menu_update`,
`network_pregame_status_screen_update`, and
`variant_profile_update_cache_for_nwide_list`, plus their required narrow
layouts, data owners, and the advertised-game validator declaration. It does
not import the donor file wholesale.

## Disposition

| target | padded bytes | in-tree verdict | verdict after required symbol update |
|---|---:|---|---|
| `_server_list_menu_update` | 2,016 | **EXACT** | **EXACT** |
| `_network_pregame_status_screen_update` | 1,984 | residual `[reloc-identity]` only | residual `[sha]` |
| `_variant_profile_update_cache_for_nwide_list` | 272 | **EXACT** | **EXACT** |

The network-pregame body initially closed only when two impossible
`machine &&` checks were retained after assigning `machine` from an array
element address. Those checks are rejected under the no-nonsensical-logic
rule. The coherent body remains complete, and the semantic controller-frame
symbol fixes all relocation identities, but its code hash is honestly
residual. Target and candidate are both 1,984 padded bytes with 81
relocations. It receives zero exact credit and is parked for future research.

The server-list body uses the authentic Xbox platform enum value rather than a
numeric platform literal. It retains the two hoisted nested-call results whose
evaluation schedule is required for January's second `displayed_servers`
relocation. The variant-cache body retains the signed `NUMBEROF` comparisons,
short array initializer, array reloads, explicit-break free-slot search, and
`match_vassert(..., "not enough cache profiles")` shape that closes the 272-byte
owner. The cast at `playlist_profile_get` preserves the current genuine header
prototype and is code-generation-neutral.

All void bodies end with explicit `return;`. Assertions retain their January
semantics. In particular, the remote-machine bound remains `match_assert` with
the byte-proven stringified expression
`j<(MAXIMUM_NETWORK_MACHINE_COUNT-1)`; it is not converted to a formatted
assert.

## Narrow layouts and data ownership

No shared definition of `network_advertised_game`, `network_machine`, or the
needed `network_game` view exists in a genuine header. This TU therefore keeps
narrow private definitions and compile-time checks for the required sizes and
offsets:

- advertised game: size `0xE4`, `game_name` at `0x30`, `engine_type` at
  `0xD4`, and `platform` at `0xDE`;
- network machine: size `0x44`;
- network game: `players` at `0x226`.

The definitions agree with the independently reconstructed networking owner
TUs. They were not promoted into `network_client_manager.h`.

The donor changed `displayed_servers` to nine pointers and thereby emitted a
164-byte combined BSS, four bytes short of January. Canonical already had the
correct ten-pointer owner. This packet keeps ten pointer slots while clearing
the nine advertised entries explicitly. The final candidate and target both
emit one 168-byte BSS with the existing exact owners:

- `_displayed_servers`, file offset `4541536`, `static: true`;
- `_ui_widget_game_data_build_version_wide_string`, file offset `4541576`,
  `static: true`.

The controller-frame table emits one 24-byte `.rdata` section with flags
`0x40300040` and bytes identical to January. Its second 12-byte half is retained
even though the unsafe/unwritten splitscreen callback is outside this packet;
January owns the complete 24-byte table and the compiler emits it as one object.

## Exact gate accounting

`gate.py --all` function-only padded accounting:

| state | exact | residual | unwritten | exact padded bytes |
|---|---:|---:|---:|---:|
| canonical baseline | 35 | 8 | 3 | 7,744 |
| reconciled source, current symbols | 37 | 8 | 1 | 10,032 |
| reconciled source plus required symbol update | 37 | 8 | 1 | 10,032 |

The admitted gain is two functions and 2,288 padded bytes. The required symbol
update supplies the correct semantic owner and relocation identities but does
not turn the coherent pregame body into an exact code match. Every one of the
35 inherited exact rows remains exact.

Commands and results:

```text
python -B tools/campaign/gate.py source/interface/ui_widget_game_data_input_functions --all
== exact 37  residual 8  unwritten 1  (of 46 listed)

python -B tools/fake_match_scan.py source/interface/ui_widget_game_data_input_functions.c --fail-on-findings
Scanned 1 file(s); 0 review lead(s); skipped 0 vendored path(s).
```

A `/W3 /WX` probe is not a usable admission gate in this canonical tree: it
fails on existing warnings in `cseries.h` and `real_math.h`, plus the TU's
pre-existing implicit `error` declaration. The proper `cseries_windows.h`
owner is included for `system_milliseconds`; the final isolated gate preserves
the inherited 1,424-byte
`_mutliplayer_settings_select_list_update_displayed_items` row exactly.

## Header owner and blast radius

`network_game_client_advertised_game_is_valid` is defined publicly in
`network_client_manager.c`, so its prototype now lives in
`network_client_manager.h` and accepts the existing incomplete
`struct network_advertised_game *`. This fixes the new UI caller's return type
without exposing the private record layout.

There are five direct C includers of the header:

- `source/interface/ui_widget_game_data_input_functions.c`;
- `source/interface/ui_widget.c`;
- `source/networking/network_client_manager.c`;
- `source/networking/network_client_message_handler.c`;
- `source/networking/network_server_manager.c`.

Focused gates after the header edit report UI game data 37/46 in-tree,
`network_client_manager` 47/52, `network_client_message_handler` 17/17, and
`ui_widget` 85/102. The manager's own 48-byte validator remains exact.
`network_server_manager` cannot currently be gated because its existing source
probe fails on undeclared `_ui_error_network_connection_lost` before producing
an object; the new prototype is unused in that TU. The blast-radius risk is
therefore low and declaration-only, but that one pre-existing compile failure
prevents claiming a complete five-TU row comparison.

## Required configuration update

Root must make exactly this one `config/symbols.json` change:

```diff
-{ "file_offset": 2584076, "flags": 0, "name": "_rdata_00276e0c" },
+{ "file_offset": 2584076, "flags": 0, "name": "_local_player_controller_bitmap_frames", "static": true },
```

No function symbol changes are required: all three target function names are
already correct. No BSS/data symbol or static changes are required: the two BSS
owners listed above already have their exact semantic names and `static: true`.
No other `.rdata` rename is required. Do not change this object's
`config/config.json` status from `NonMatching`; unrelated residual and unwritten
owners remain.

The networking-lost constant is not TU-private. Bungie's supplied networking
source calls `_error_network_connection_lost`, and canonical already contained
the complete ordered error-code enum in `ui_widget.c`. Admission moves that
enum verbatim into `ui_widget.h`, removes this TU's temporary one-member enum,
and uses the source-attested shared member without changing its value (6).

## Explicit non-credit

The network-pregame callback and the two game-options callbacks remain safe,
complete non-exact implementations:

- `_network_pregame_status_screen_update`: residual `[sha]`, 1,984 padded
  bytes; target/candidate relocation counts and identities agree after the
  semantic data-owner rename. The two redundant non-null tests required by the
  prior exact candidate are deliberately omitted.

- `_game_options_menu_update_text_desc`: residual `[sha]`, 304 padded bytes;
- `_game_options_menu_update_pic_desc`: residual `[sha]`, 304 padded bytes.

Both retain `description_index = 0`. The donor's byte-matching uninitialized
local reads were not imported and receive zero exact or fuzzy byte credit in
this packet. `splitscreen_pregame_status_screen_update` also remains unwritten;
no broad screen-profile import was performed.

No configure, full Ninja build, staging, commit, push, or configuration-file
edit was performed.
