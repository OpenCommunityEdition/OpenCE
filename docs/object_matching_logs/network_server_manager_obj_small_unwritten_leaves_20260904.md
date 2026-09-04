# `network_server_manager.obj` small unwritten leaves — 2026-09-04

## Result

This isolated packet starts from canonical commit
`fc057d673a3f0e3a8a782bb4ec8ae8ea8824f5ea` and advances
`source/networking/network_server_manager` from **47 exact / 1 residual / 22
unwritten** to **49 exact / 2 honestly parked residuals / 19 unwritten**.

The rename-stable whole-tree sweep proves **2 strict gains / 256 padded target
bytes / 234 meaningful target bytes / zero regressions**. Aggregate progress
moves from 882,677 to **882,911 / 2,198,102 meaningful code bytes** and from
6,069 to **6,071 / 11,060 functions**. Matched data remains **2,018,310
bytes**, and linked objects remain **391 / 833**.

## Evidence and scope control

The complete prior `network_server_manager.obj` campaign ledgers and all ten
networking owner headers were read before editing. January's PC COFF is the
byte authority. The frozen Opus donor at
`C:\halo-worktrees\opus-small-families-30k-20260902` and its preserved
`scratch/w6_network_server_manager.obj` were used only as read-only source-shape
evidence; the Stian PC lift and later HCEA name topology were corroboration.
No donor file or commit was copied wholesale.

The 22 baseline unwritten sections, in increasing padded-size order, were:

| Padded | January symbol |
| ---: | --- |
| 48 | `_code_0011d380` |
| 48 | `_code_0011d540` |
| 80 | `_code_0011d3b0` |
| 112 | `_code_0011d400` |
| 144 | `_countdown_timer_increment` |
| 160 | `_code_0011d240` |
| 160 | `_code_0011d2e0` |
| 160 | `_network_game_generate_join_game_token` |
| 208 | `_code_0011ba10` |
| 208 | `_code_0011d470` |
| 224 | `_network_game_server_start_network_game` |
| 256 | `_code_0011d570` |
| 288 | `_network_game_server_dispose` |
| 304 | `_code_0011d670` |
| 320 | `_network_game_server_remove_client_machine_from_game` |
| 320 | `_network_game_server_remove_machine_from_game` |
| 336 | `_network_game_server_create` |
| 368 | `_code_0011d0d0` |
| 384 | `_network_game_server_idle` |
| 464 | `_code_0011ddd0` |
| 592 | `_network_game_server_reset_to_pregame` |
| 688 | `_code_0011dfa0` |

The already parked `_network_game_server_update_countdown` was not retuned.
The prior signed-overflow `_countdown_timer_increment` exact form and the
nonsensical fixed-literal join-token form remain rejected. The 48-, 80-, and
112-byte private callback/postgame/precache helpers were not made public merely
to emit them: their January register ABIs and owning callers form a later
cluster, and compiling them without those callers would require false linkage
or an emission anchor. This is the previously prohibited artificial-emission
case, so it contributes no claimed progress here.

## Strict-exact leaf reconstructions

| Function | Meaningful | Padded | Relocs | Target normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_network_game_server_send_player_quit_messages_ingame` | 196 | 208 | 12 | `c6953cf85d3a10150619a0908b6242ed05263388c3b0c905c3fde16176374df1` |
| `_network_game_server_get_client_machine_count` | 38 | 48 | 0 | `89862cc646a32a8bde20a61d66da1a706041c0e4d256b1818451365dd4877c3c` |
| **Total** | **234** | **256** |  |  |

At January address `0x0011BA10`, the exact body walks all 16 typed network
players, selects valid players owned by the departing client machine, copies
the player into a typed `message_server_remove_player_ingame`, schedules the
authenticated 33-tick quit delay, and broadcasts message enum
`_message_server_remove_player_ingame`. The semantic private-family name is a
descriptive recovery from that behavior and the unique diagnostic topology,
not a claimed PDB name. Its ordinary C reproduces all 208 padded bytes and all
12 ordered relocations without an address-derived identifier or compiler
control.

At January address `0x0011D540`, the canonical source already contained the
natural static helper used by countdown logic. An in-memory diagnostic alias
proved its 48 bytes exact before changing the atlas. The target and source now
share the semantic name `network_game_server_get_client_machine_count`; the
body counts only slots with both a connection and a non-`NONE` machine index.
It is naturally emitted by its real caller, so this is semantic symbol recovery,
not a keepalive or public-linkage fiction.

`game_time_get` is obtained from its owner `game/game.h`; no caller-local
prototype was introduced. Message types use the existing
`enum network_game_message_type`, the server-start flag is named
`network_game_started`, and the added payload records use project integer and
player types.

## Honest game-start residual

`_network_game_server_start_network_game` is now fully reconstructed as
ordinary typed C. It validates the server, avoids restarting an active game,
copies the typed game-settings payload, broadcasts settings and begin-game
messages in order, sets `network_game_started`, clears the update number on all
exits, and preserves January's intentionally true result. The source shape is
corroborated independently by the frozen Opus donor and Stian PC lift, while the
function name is present in the Sapien symbol atlas.

January is 224 padded bytes with 13 relocations and normalized SHA-256
`b5b6ecc62e0eff77aeab7b5ca8f40326d7cd7486a5d2ca7ca092e9e9d6967f39`.
The retained natural candidate is 240 bytes with the same 13 relocation
identities, SHA-256
`4fdad9f5a67f8ddb8839f712a0173d6df2fc175a957e7b907e3ce620c5816c48`,
and 87.44 percent objdiff similarity. January keeps a shared zero in EBX,
reuses the dead incoming-parameter slot for the four-byte begin payload, and
orders the final adjacent stores differently. Ordinary declaration-scope and
initializer-lifetime variants return to the same fixed point or worsen it.
The coherent candidate is therefore recorded in `config/parked.json`; no
register/volatile forcing, barrier, pragma, assembly, false linkage, fake
dependency, raw offset, undefined behavior, or nonsensical branch is used to
manufacture the residual.

## Verification

- Full `ninja all_source progress semantic_progress`: pass; **473 units**,
  **6,499 functions evaluated**, **6,127 semantic exact**, **169 hidden exact /
  101,147 hidden bytes**, **6,144 accepted exact**, and **zero unit errors**.
- Focused manager gate: **49 exact / 2 residual / 19 unwritten**.
- Rename-stable sweep: 8,245 January function sections; 6,116 exact before,
  6,118 after; **2 gains / 256 padded bytes / zero regressions**.
- Park manifest: **225 active / 0 stale / 0 invalid**.
- Object admission: **0 candidates / 0 contradictions / 0 revocations**; only
  the two inherited explicit `player_rumble` and `models` rejections remain.
- Fake-match scan of the changed C file: zero review leads.
- Tooling tests: **261 passed**.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact at
  1,920 padded bytes.
- Rebuilt manager object: no `point_from_line3d` symbol or COMDAT.
- `git diff --check`: pass.

This packet adds no inline assembly, forced inline/noinline, volatile/register
forcing, optimization barrier, pragma steering, raw-byte emission,
representation pun, fake dependency, undefined behavior, or coincidental
nonsensical source. The January inline schedule remains intact.
