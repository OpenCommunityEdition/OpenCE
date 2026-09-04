# `network_server_manager.obj` Opus dirty reconciliation — 2026-09-04

## Result

The unfinished Opus translation-unit reconstruction was audited as a read-only
donor and selectively rebuilt on canonical commit
`1c834b12f250db60ee841c151e201ffc2d706c49` in the isolated worktree
`network-server-manager-dirty-reconcile-20260904`.  This packet advances
`source/networking/network_server_manager` from **26 exact / 0 residual / 44
unwritten** to **47 exact / 1 honestly parked residual / 22 unwritten**.

The rename-stable whole-tree sweep proves **21 strict gains / 4,288 padded
target bytes / 4,121 meaningful target bytes / zero regressions**.  Aggregate
progress moves from 871,741 to **875,862 / 2,198,102 meaningful code bytes**
and from 6,021 to **6,042 / 11,060 functions**.  Matched data remains
**2,015,906 bytes**, and linked objects remain **391 / 833**.

## Frozen provenance and selection

The donor remained read-only throughout reconciliation:

- worktree: `C:\halo-worktrees\opus-small-families-30k-20260902`;
- frozen committed tip: `eeedd72e5ba0fc1761519f68cad241fe606320e7`;
- live dirty `source/networking/network_server_manager.c` SHA-256:
  `6B91A6622C1E2DE8E71364379B8FA798374CA4A0D6D62B35E67D4B9D573E251C`;
- preserved donor object: `scratch/w6_network_server_manager.obj`, SHA-256
  `7920E226899900301F10D1CC40FB0091DF2A637DB6DB71550D5FCBF7A975F1BE`.

The donor object measured 46 exact functions against the then-current target,
versus 26 at the canonical baseline.  It was not copied or cherry-picked.
Every retained body was transferred independently, compiled through the
PID-local gate, compared with January's function section and ordered
relocations, and then replayed through the full generated build.  The dirty
source and donor scratch objects are evidence only; January remains byte
authority.

The following project documentation was read before admission:

- `docs/campaign_throughput_lessons.md`;
- `docs/opus_unfinished_session_reconciliation_backlog_20260903.md`;
- all existing `network_server_manager.obj` ledgers under
  `docs/object_matching_logs/`; and
- the adjacent admitted Opus network-client and server-message-handler
  reconciliation ledgers, including their header-position regression notes.

## Newly strict-exact functions

| Function | Meaningful | Padded | Relocs | Target normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_countdown_timer_decrement` | 141 | 144 | 9 | `7c5b237e22f7fac9000c1c72c715fafabfb3e33ea0dd8ba9f6c477b429c5902f` |
| `_network_game_server_open_game` | 73 | 80 | 7 | `fb21f9919e068cf10dbfb75d0e5de94037fc3b89eb70cabfb20af18a2f1cdd77` |
| `_network_game_server_close_game` | 73 | 80 | 7 | `9e6ef8b1600ec0ff818c05466b7dbedf1c44e90bbd4c0ba5e6ca71b58a366fef` |
| `_network_game_server_switch_to_postgame` | 145 | 160 | 12 | `300be470adefb4f097a0947baac835d016b62785231f98190ab47880b8a0e865` |
| `_network_game_server_graceful_shutdown` | 231 | 240 | 19 | `eb2474eda7861c02d0d09489aba623a7b43b106c2ff8a908a398d3740f999a87` |
| `_network_game_server_accept_client_machine_into_game` | 293 | 304 | 18 | `11e4cfc65f4395b23011ca8dba3a7a24c2d8f7300be9c160707b3ba2934bae5a` |
| `_player_name_is_unique` | 62 | 64 | 2 | `006f7dfc0a47a77fffbfbf307b9a3d06bdc9b3c95bfb75243f99fb48df88452f` |
| `_get_unique_random_name` | 119 | 128 | 4 | `6a307f4a5a90b9cd02b9741aa5bc107785d1ca8390a738dded2dd076cff040ea` |
| `_get_unique_random_color` | 130 | 144 | 3 | `d66f8cbf80065e9f6f5ff90ae4684d3ea4493b3ebe67aa2a90b732da0ece7e49` |
| `_network_game_server_add_player_to_game` | 310 | 320 | 25 | `ab78f4f7bfe73a59d467a9d2b79653dcfc71ce198c98c58b10a7d610af71049e` |
| `_network_game_server_remove_player_from_game` | 224 | 224 | 19 | `f02066bea5de65581ee27d5616a603e6805c4da86adca7207d62c1a0928edf8d` |
| `_network_game_server_adjust_machine_settings` | 160 | 160 | 11 | `52991315a94eee0bcacfa0cf6f423439703cde96f24227d2cd4fa5b3cc46fd88` |
| `_network_game_server_all_machines_have_loaded` | 115 | 128 | 9 | `aa0fc0af1b74957c8b528345e76343b544d07cafe01426982e160ed67aa01224` |
| `_network_game_server_client_machine_game_loading_complete` | 189 | 192 | 12 | `eba2eb1a50a77b320686327c0c37cf8ba0e50afbc0b78aa76f4cf32e87d0a1d8` |
| `_network_game_server_handle_client_update_packet` | 322 | 336 | 21 | `668c52d989c67fb85336111035f90533a25b9004acfddbac05cc662f97e8f015` |
| `_network_game_server_switch_machine_from_postgame_to_pregame` | 77 | 80 | 6 | `a62d4adcd3dd5c00385e5e40513535e252e05b2154c22ca43119f5b28f7b2a25` |
| `_network_game_server_update_ticks` | 420 | 432 | 23 | `fb57ca21671bcf33a2744b6b0bc4bedf7f81db172ce878b369741a03b13b9a12` |
| `_network_game_server_get_client_machine_at_address` | 200 | 208 | 11 | `86051bc8e8934556a616375c220b0f645dbebfe1c4f8273631766b6e4f2418f2` |
| `_network_game_server_change_map_name` | 249 | 256 | 12 | `69eb0a6c2443a7d8fd9fe0f9105595c99f46987102bcd6d09730c23cadaf5df6` |
| `_network_game_server_change_game_variant` | 135 | 144 | 12 | `be0ea7ed75cc53c19e49f1fa9ae7c5d71e79b232e17e89eeb476c9abc05a7256` |
| `_network_game_server_stalled_on_client` | 453 | 464 | 19 | `76c7519e9af523af7d26699aab9e3606295a859a798034601c1a0a015d95770a` |
| **Total** | **4,121** | **4,288** |  |  |

## Semantic names and storage ownership

The target placeholder at January address `0x0011BEE0` is now named
`player_name_is_unique` and marked static.  The name is descriptive inference,
not claimed PDB authentication: the exact body walks valid network players and
returns false on a matching Unicode name, and its sole recovered caller uses
it precisely for duplicate-name normalization.  The source symbol inventory
and `config/symbols.json` now agree; no `code_<address>` identifier remains at
that implemented boundary.

The alternating-team counter is named
`network_game_server_next_team_index`.  January's two player-admission
relocations resolve to `0x00456CF8`, four bytes after the named
`network_game_server_memory_do_not_use_directly_in_use` flag and before the
next BSS owner.  The recovered code reads, increments modulo the two-team
domain, and writes that exact four-byte counter address.  Naming the target address
with the same static long already used by the semantic source turns
`network_game_server_add_player_to_game` from a relocation-only semantic exact
into a strict 320-byte exact match.  This is target-address symbol recovery,
not a fabricated alias or aggregate facade.

`get_unique_random_name` and `get_unique_random_color` retain external
emission because January's symbol map and calling convention prove those
out-of-line owners.  No fake keepalive or false `static` qualifier is used to
control their emission.

## Type and owner reconciliation

The retained implementation replaces opaque server/game fields with the
smallest semantic records required by the proven accesses and guards them with
zero-emission size/offset assertions.  Server state and flags are `word`,
client-machine flags are `word`, player/action/update structures use cseries
integer types, and switch cases use named server/countdown/message enum
constants.  Flag operations go through `TEST_FLAG` and `SET_FLAG`; no raw
bit-twiddling substitute is introduced.

Public declarations live in `network_server_manager.h`; the manager/handler
shared interface stays in the narrow
`network_server_manager_internal.h`.  Queue APIs are declared in
`game/player_queues_new.h`, `game_engine_update` in the narrow
`game/game_engine_runtime.h`, and `get_random_seed` in the new dedicated
`math/random_math.h`, which is also included by its defining translation
unit.  The message handler now passes the semantic `network_machine` record
to the manager rather than a packet-wrapper type with the same bytes.

Two broad-header experiments were explicitly rejected after measurement.
Putting `get_random_seed` in `math/real_math.h` reopened exact
`effects/decals::_decals_delete_permanent_from_cluster`.  Putting
`game_engine_update` in broad `game/game_engine.h` reopened the protected
1,920-byte `units::_unit_preprocess_node_orientations`.  Moving each
declaration to its narrow owner restores both functions without a consumer-C
prototype, conditional declaration, or compiler directive.

`network_game_server_update_ticks` is target-proven with a `short` tick
parameter and is declared that way in its owner header.  The pre-existing
`game_time.c` local `long` declaration remains outside this packet because
changing its already-exact caller or changing the manager definition to long
reopens target code.  This historical cross-TU declaration debt is documented
rather than hidden with another local prototype.

## Primary 480-byte leaf and honest residual

The requested `_network_game_server_update_countdown` leaf is fully
reconstructed as a typed ordinary-C countdown state machine.  It covers the
player-left, player-joined, stop, and immediate-start events; existing active
and complete states; server readiness; remote-connection and split-screen
rules; timer adjustment and minimum bounds; and the message-time reset.

January is 480 padded bytes with 21 relocations and normalized SHA-256
`4e64f4f9ca4b1c3e6971dca2af566fbe87b5abd43272ed45535a12592f23ac51`.
The closest natural candidate is 496 bytes with 20 relocations and hash
`19afd9e555269508c01d42c537ee32f4df08d3088646b9b1db15fe1e094ff997`;
objdiff reports 72.8 percent.  January retains one additional
`system_milliseconds` relocation and chooses a different saved-register/event
schedule.  Natural short/long event types and helper definition-placement
families were measured; they either return to this fixed point or create a
worse out-of-line-helper shape.  The fail-closed classifier reports
`UNKNOWN / unclassified / confidence NONE`.

The source therefore stays as the coherent 496-byte fuzzy candidate and the
function is recorded in `config/parked.json`.  No forced inline/noinline,
false prototype or linkage, volatile/register forcing, barrier, pragma,
assembly, raw offset, fake dependency, undefined behavior, or nonsensical
branch is used to manufacture the remaining bytes.  Reopen only for
authoritative January source/local records or a natural same-compiler TU
context explaining the extra timing call and register schedule.

## Explicit rejection

The donor's `network_game_generate_join_game_token` body is not admitted.  It
declares a pointer parameter, clears `sizeof(join_token)` bytes (therefore only
the pointer width), and then overwrites the token with a fixed stack literal.
The clear is semantically redundant, the literal is unsupported by adjacent
source evidence, and the resulting match is exactly the kind of nonsensical
coincidence prohibited by the anti-fake rule.  The function remains unwritten
rather than claiming progress through implausible original source.

## Verification

- Full generated Ninja build: pass after all source, owner-header, and symbol
  changes; semantic report has **473 units**, **6,457 functions evaluated**,
  **6,097 semantic exact**, **169 hidden exact / 101,147 hidden bytes**,
  **6,115 accepted exact**, and **zero unit errors**.
- Focused manager gate: **47 exact / 1 residual / 22 unwritten**.
- Rename-stable snapshot: 8,245 January function sections; 6,067 exact before,
  6,088 after; **21 gains / 4,288 padded bytes / zero regressions**.
- Park manifest: **213 active / 0 stale / 0 invalid**.
- Object admission: **0 candidates / 0 contradictions / 0 revocations**; only
  the two inherited explicit `player_rumble` and `models` rejections remain.
- Semantic audit: zero unit errors.
- Fake-match scan across all eight changed C/header files: zero findings.
- Tooling tests: **261 passed**.
- Protected `source/units/units`: **189 exact / 0 residual / 0 unwritten**.
- Rebuilt manager object: no `point_from_line3d` symbol or COMDAT.
- Protected bitmap diff against the base: empty.  Base-preserved SHA-256 values
  are `DCC516B95C8C51ED52BC245170244BE2FDC01135138D82CF1A774A0BFD09FE64`
  for `source/bitmaps/bitmap_utilities.c` and
  `7011541E1AB8F5E0F732D7FCCC379D381F65DA6A27C2C06661AA4F3037598E8D`
  for `source/bitmaps/bitmaps.h`.
- `git diff --check`: pass.

The final packet contains no inline assembly, forced inline/noinline,
volatile/register forcing, optimization barrier, pragma steering, raw-byte
emission, representation pun, fabricated dependency, undefined behavior, or
nonsensical coincidental branch.  The January inline schedule is preserved,
and this object emits no `point_from_line3d` owner.
