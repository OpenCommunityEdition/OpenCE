# `network_server_message_handler.obj` Opus reconciliation

## Result

The frozen Opus session at commit `608c66380` contained a substantial,
authentic reconstruction of the server message handlers.  This packet audits
that work against canonical commit `43bc9e074`, imports it selectively rather
than cherry-picking the donor, replaces its temporary local declarations and
opaque stand-ins with owned declarations and typed structures, and retains the
result only after a clean whole-tree comparison.

The focused strict gate advances from **4 exact / 0 residual / 18 unwritten**
to **19 exact / 2 residual / 1 unwritten**.  The fifteen newly exact functions
add **4,704 meaningful bytes** and **4,784 padded bytes**.  A stable-section
snapshot over all 8,245 January functions advances from 5,977 to 5,992 exact
with **zero regressions**.

| Newly exact function | Meaningful / padded | Relocations | Target normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_server_handle_client_message` | 1024 / 1024 | 73 | `707032f0246c13ca68ef56a8c98ac7b5dd43a6b1720493dd9d0fec0fc6c8f1c6` |
| `_network_game_server_handle_message_client_add_player_request_ingame` | 135 / 144 | 7 | `b2fedcae41528f08d0734e741717095e5d016d41a390c03c640075cadcf520e7` |
| `_network_game_server_handle_message_client_add_player_request_pregame` | 191 / 192 | 12 | `547171a012b4031b7656c29a2b3f5d461dc736862a672aebe7848f6ff7dbe9f4` |
| `_network_game_server_handle_message_client_broadcast_game_search` | 575 / 576 | 21 | `cbcdc3e703e3896f6ebf23355f8497a4bc79f44414557280aa90691d80fbb811` |
| `_network_game_server_handle_message_client_game_start_request` | 134 / 144 | 7 | `b32b05b7b56cbd45d56544d5a209c12faab4b1702218d3222f936aa960c75413` |
| `_network_game_server_handle_message_client_graceful_game_exit_pregame` | 196 / 208 | 13 | `e6d61ea184d7687d178f428b0c2e479b7c7661dc48a662de0919047c16c1897f` |
| `_network_game_server_handle_message_client_join_game_request` | 997 / 1008 | 61 | `9760c97d13144c85ad04ac8ab51af3ddb926930f118cbbeee48bd6f34471d3ec` |
| `_network_game_server_handle_message_client_loaded` | 139 / 144 | 7 | `16822ef17156b50be03bbc25c44754a09df83c37a90ca8cd057b1c2cfe2661ff` |
| `_network_game_server_handle_message_client_map_is_precached_pregame` | 147 / 160 | 7 | `99c2da9ec182256eac4ec31ee43e38a89e5dcd0eccd99c4ed91c354d7d5ec9a0` |
| `_network_game_server_handle_message_client_player_settings_request` | 206 / 208 | 15 | `045b2c32458f7727f1022356d6b5e684276fbe8e8fe924f88fbbe1eba20ab4b0` |
| `_network_game_server_handle_message_client_remove_player_request_ingame` | 232 / 240 | 13 | `aaa3314a69ae5853bd9835a0b7a608e5ef9b987cf1d06d5e7e35c5c744ae37e5` |
| `_network_game_server_handle_message_client_remove_player_request_postgame` | 156 / 160 | 9 | `fc20206dc1b1917e32d01fd73f3f6369991574e62d17aedbef4244da8f5e17cf` |
| `_network_game_server_handle_message_client_remove_player_request_pregame` | 191 / 192 | 12 | `176102679c1560d5beca3a0acc9f4c0e2ae32e75ffc1d260cc4cab98e6bbfe7b` |
| `_network_game_server_handle_message_client_settings_request` | 222 / 224 | 15 | `682dbdb6fc28f700da5a981e4e0dc0c32956372b5dcef65c05e3bfdee14a0a61` |
| `_network_game_server_handle_message_client_switch_to_pregame` | 159 / 160 | 9 | `abb44becd6e8e194158f5a8e76b669464090049edb0ebe37f61ce2323307039f` |

The January split object SHA-256 is
`955db39e021e87b4f70dce940ef1fcf896907aaca8d2f681f1b89f6edcf4a739`.
The audited rebuilt object SHA-256 is
`b27b82eda251834b02e0d3ea9b1388d4ad417a26a90e8197af8f15b893e4cd8b`.

## Source and ownership audit

The private handlers are named from their jump-table cases, packet constants,
assertion/log strings, and callees; all sixteen recovered target symbols are
recorded as static in `config/symbols.json`.  No retained private name is an
address-derived `code_*` placeholder.  The implementation uses the established
network-message enum constants and `GET_MESSAGE_SIZE` / `GET_MESSAGE_TYPE` /
`GET_MESSAGE_FLAGS` macros rather than open-coded header bit manipulation.

Temporary donor prototypes were removed from the translation unit.  Public
handler declarations live in `network_server_message_handler.h`; the manager
and handler share server-state and machine operations through the narrow owner
`network_server_manager_internal.h`; the public manager facade retains only
the declarations used by unrelated clients.  The connection writer lives in
`network_connection.h`; and the XDK-backed nonce/address/key
queries live in the closest associated transport owner header,
`transport_endpoint_winsock.h`.  The latter exposes the real `XNADDR`, `XNKID`,
and `XNKEY` types instead of local opaque byte wrappers.  Three pre-existing
server-manager definitions whose declarations described the wrong machine
kind were corrected to `network_game_server_client_machine`; the semantic game
machine is consistently named `network_machine`.  Those type-only corrections
leave the manager's focused result unchanged at 26 exact / 0 residual / 44
unwritten.

The retained TU-private network and message structures use semantic field
names, real `boolean`, `byte`, and `word` types, and typed nested payloads.
No raw address arithmetic or offset-named field was introduced.  Declarations
and assignments are combined where the target-compatible source naturally
allows it, every parameter is on its own line, and every reconstructed function
has an explicit return.

## January behavior preserved

Several apparently odd paths are not matching inventions; they are directly
proven by January's bytes, relocation graph, and owned strings:

- The game-start request decoder passes packet type 16
  (`_message_client_player_settings_request`) rather than 17.
- The in-game remove-player decoder passes packet type 26
  (`_message_client_add_player_request_ingame`) rather than 27.
- The switch-to-pregame decode failure uses the preceding remove-player
  failure string.
- Client-message cases 18 and 34 share the graceful pregame handler, and the
  map-is-precached failure path shares the graceful-exit caller string.
- The accepted-join path calls `network_game_server_get_game(server)` and
  discards the result immediately before the line-718 assertion.  The call is
  visible in January and is retained directly; no fake local or emission
  anchor was added.

These behaviors are documented so a later cleanup does not silently replace
target-authentic bugs with nicer but non-January logic.

## Honest residuals and rejection

`_network_game_server_handle_message_client_ping` is parked as
`register-allocation`.  Target and candidate are both 192 padded bytes with 11
relocations.  January gives the private helper two register arguments (server
in ECX, client message in EAX) while the current VC7 lowering gives the natural
static source one register argument.  Ten ordinary parameter-order,
declaration-lifetime, and expression-order families did not change that ABI.
The target/candidate normalized hashes are respectively
`96c721475f9cc12f4132e7b05d91aa3d576f804e99d6ffc134e39df030820831`
and
`4ac6454d882e733bbbb663871ea567e3dc945ffa42ec5a3b98ddc64b9e17d8f5`;
objdiff reports 79.17333 percent.

`_network_game_server_handle_datagram` is parked with the same blocker.  Both
sides are 784 padded bytes with all 47 relocations, and the remaining
difference is the ping call-site cleanup caused by that private ABI.  Its
target/candidate normalized hashes are respectively
`f32e88d94387bb9f05b146c7f2410d457f7c64e104167a6c4c42c665bebaae54`
and
`88dde40002615b6a6602eb63b3aa0a7d21f18eab3ecd19137faa658446dbbad8`;
objdiff reports 98.851616 percent.

`_network_game_server_write` remains honestly unwritten.  January contains an
unreferenced 32-byte private five-argument passthrough with one relocation.
Normal static source either inlines it or omits the out-of-line body.  Routing
calls through it regresses already exact senders, while a dormant reference or
emission anchor would be fake source.  It is therefore rejected rather than
manufactured and is not falsely parked without a candidate section.

## Validation

- Focused strict gate: 19 exact, 2 residual, 1 unwritten.
- Full generated Ninja graph: pass, including all sources, fresh target split,
  objdiff report, semantic report, and progress calculation.
- Stable-section sweep: 8,245 functions; 5,977 exact before, 5,992 after;
  15 gains / 4,784 padded bytes; zero regressions.
- Semantic audit: 473 units, 6,346 functions evaluated, 6,018 accepted exact,
  163 hidden exact / 98,729 hidden bytes, zero unit errors.
- Park manifest: 198 active, zero stale, zero invalid.
- Object admission: zero candidates, contradictions, or revocations; the two
  reported rejections are pre-existing `player_rumble` and `models` decisions.
- Focused fake-match scan: seven changed C/header files, zero review leads.
- Tooling tests: 261/261 pass.
- The final handler candidate contains no `point_from_line3d` symbol or COMDAT.
- Protected bitmap sources are unchanged from the audited base.  Their local
  SHA-256 values are `dcc516b95c8c51ed52bc245170244be2fdc01135138d82cf1a774a0bfd09fe64`
  (`bitmap_utilities.c`) and
  `7011541e1ab8f5e0f732d7fccc379d381f65da6a27c2c06661aa4f3037598e8d`
  (`bitmaps.h`).

The retained packet introduces no inline assembly, volatile/register forcing,
barrier, pragma, raw-byte emission, codegen directive, fake dependency,
representation pun, undefined behavior, forced inline, or nonsensical branch.
The duplicate `network_game_generate_join_game_token` owner-header declaration
is also part of the concurrent network-client packet; it should be merged once
when both isolated commits are replayed.

## Canonical replay

The isolated packet was replayed after the admitted network-client packet at
canonical commit `99a8e4735`.  Its first full-tree build exposed one C2
definition-position regression outside the packet:
`game/game_engine::_populate_statistic_buffer` fell from exact to residual even
though it names none of the newly declared server-manager types or functions.
`game_engine.c` includes the public `network_server_manager.h`, and the isolated
packet had expanded that widely included facade from three declarations to the
entire private manager/handler interface.

The final replay restores the public header to its pre-packet three-declaration
surface and places the complete private interface in the closest associated
owner, `network_server_manager_internal.h`.  Only
`network_server_manager.c` and `network_server_message_handler.c` include the
internal header, at the same declaration position and with the same declaration
order as the isolated build.  This is an ordinary ownership split, not a
caller-local prototype or conditional compiler steering.  Focused gates prove
that the handler remains 19 exact / 2 residual / 1 unwritten, the manager
remains 26 exact / 0 residual / 44 unwritten, and
`_populate_statistic_buffer` returns to exact.  The protected
`units::_unit_preprocess_node_orientations` sentinel remains exact as well.

The rename-stable whole-tree sweep against
`scratch/after-network-client-replay-final-20260904.json` reports **15 strict
gains / 4,784 padded bytes / zero regressions** across all 8,245 January
function sections.  Those gains contain **4,704 meaningful bytes**.  Aggregate
semantic progress moves from 860,625 to **864,305 / 2,198,102 meaningful code
bytes** and from 5,975 to **5,989 / 11,060 functions**.  The net semantic
movement is **+3,680 bytes / +14 functions** because the newly strict
1,024-byte `_network_game_server_handle_client_message` was already credited by
the semantic COFF proof channel before its strict closure.  The reconstructed
translation unit also makes its complete **7,688-byte `.rdata`** section exact,
moving matched data from 2,007,670 to **2,015,358 bytes**.  Linked objects remain
**391 / 833**.

Final current-tree validation passes: full generated Ninja build; semantic
audit with 473 units, 6,398 functions evaluated, 6,062 accepted exact, and zero
unit errors; 207 active parks with zero stale or invalid entries; object
admission with zero candidates, contradictions, or revocations and only the two
inherited rejections; focused fake-match scan over all seven packet/internal
source and header files with zero findings; 261 tooling tests; clean diff check;
and no `point_from_line3d` symbol in the rebuilt handler object.  The protected
bitmap work remains byte-for-byte at its audited local hashes
`AB6002BF2BAECDBC18C7B308BB7846C58FBCF1C7C72F586621D2B1F271D49140`
and `B435F38E406C7DCDB60B4DF12E78BCF98D1E4B0E57FE2AB2240BC78F799EBA93`.
