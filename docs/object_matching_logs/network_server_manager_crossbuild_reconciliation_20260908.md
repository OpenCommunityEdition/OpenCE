# Network server manager cross-build reconciliation — 2026-09-08

## Result

The January-focused baseline was **59 exact / 5 residual / 6 unwritten** out
of 70 listed functions.  The user-supplied Bungie source map at
`C:\Users\isabe\Downloads\haloleak2024\halo1\network_server_manager.c` maps
all six unwritten January sections to a coherent idle call graph.  Rebuilding
those bodies as ordinary typed C produces the final post-owner/config state of
**65 exact / 5 residual / 0 unwritten**.

Six strict gains cover **1,739 meaningful / 1,792 padded target bytes**.

| January symbol | Recovered semantic owner | Meaningful / padded | Result |
| --- | --- | ---: | --- |
| `_code_0011d240` | `network_game_server_handle_public_endpoint` | 146 / 160 | strict exact |
| `_code_0011d3b0` | `network_game_server_idle_postgame_tasks` | 69 / 80 | strict exact |
| `_code_0011d400` | `network_game_server_have_all_machines_have_precached` | 108 / 112 | strict exact |
| `_code_0011dfa0` | `network_game_server_idle_pregame_tasks` | 685 / 688 | strict exact |
| `_network_game_server_idle` | `network_game_server_idle` | 377 / 384 | strict exact after semantic private-name mapping |
| `_code_0011d0d0` | `network_game_server_add_new_client` | 354 / 368 | strict exact |

The source map is reconstruction evidence from a different build.  January's
PC COFF, its relocations, and its disassembly remain the byte authority.

## Recovered source and types

The public idle dispatcher now services the listening connection, accepts or
closes a newly connected client, drains the public datagram endpoint, services
the established client machines, and dispatches pregame or postgame work by
the proper server-state enum.  Its five private helpers remain `static` and
are emitted through that genuine call graph; there is no artificial anchor.

The reconstruction also restores:

- the typed 400-byte aligned datagram buffer and the existing `word` read-size
  contract;
- two-byte countdown, pregame keepalive, and postgame keepalive payloads;
- connected-machine flag bit zero and the 15-second client loading timeout;
- the source-attested `adjusted_time_this_tick` name for the existing
  countdown-state byte at server offset `0x496`;
- an unconditional include of the genuine `cache_files.h` owner, because
  January's active all-precached assertion calls its boolean predicate even in
  this DEBUG compile;
- signed/unsigned timer comparisons that reproduce January without signed
  addition or subtraction overflow.

All 59 inherited exact functions remain exact in the isolated compile.  No
inline assembly, volatile/register forcing, compiler-control pragma, raw-byte
emission, fake dependency, address-derived C name, representation pun, or
nonsensical branch was introduced.

## Exact and fuzzy evidence

The public-endpoint helper has all 65 normalized instructions and eight
relocations in January order.  The standard gate's textual `--alias` also
rewrites its diagnostic string, so that convenience path reports only a
relocation-identity difference; a non-textual semantic symbol comparison
shows no instruction difference and the frozen independent candidate audit
records the body strict exact.

The postgame helper is directly strict exact at 80 padded bytes.  Its unsigned
five-second deadline is intentionally distinct from the pregame keepalive's
signed deadline comparison.

The all-precached helper is directly strict exact at 112 padded bytes after the
real cache predicate prototype is visible.  Keeping the include behind
`#ifndef DEBUG` removes the January assertion path and incorrectly yields a
64-byte body.

The pregame helper is directly strict exact at 688 padded bytes and 33
relocations.  It removes dead clients, advances or cancels the countdown,
sends countdown and keepalive messages, and removes clients that exceed the
15-second loading deadline.

The public idle body has all 155 normalized instructions.  After non-textual
mapping of its four private callees, its only focused comparison distinction is
the selected provider identity for the pooled
`"network connection went down!"` literal; the frozen whole-object audit
authenticates the selected UI provider and records the function exact.

`network_game_server_add_new_client` is strict exact at 368 padded bytes. The
supplied source names the tested union member `ipv4_address`; adding that
genuine alias to its proper `transport.h` owner and using it here restores the
January C2 TU context naturally. The previous `long_words[0]` spelling was a
known departure from the reconstruction map and its 352-byte residual receives
no historical credit. All 13 direct C consumers of the header compile in
isolated blast-radius gates.

## Proper-owner and configuration handoff

The C file uses Bungie's source-attested UI enum member
`_error_network_connection_lost`. Its exact value is 6 in canonical's complete
error-code enum. The whole anonymous enum is moved unchanged from
`source/interface/ui_widget.c` to its real shared owner,
`source/interface/ui_widget.h`; no partial or invented enum type is introduced.
A numeric literal and a server-local enum are used for neither production
source nor claimed credit.

The five private semantic names require non-textual target mappings owned by
configuration:

| Semantic source name | January target name |
| --- | --- |
| `network_game_server_add_new_client` | `_code_0011d0d0` |
| `network_game_server_handle_public_endpoint` | `_code_0011d240` |
| `network_game_server_idle_postgame_tasks` | `_code_0011d3b0` |
| `network_game_server_have_all_machines_have_precached` | `_code_0011d400` |
| `network_game_server_idle_pregame_tasks` | `_code_0011dfa0` |

The obsolete add-client fuzzy park is retired after the authentic header-owned
alias closes it. No other residual is relabelled exact.

## Verification

- Baseline focused gate: `59 exact / 5 residual / 6 unwritten`.
- Final focused gate: `65 exact / 5 residual / 0 unwritten`.
- `_network_game_server_add_new_client`: directly strict exact, 368 padded
  bytes and 21 relocations.
- All 13 direct `transport.h` C consumers compile in isolated blast-radius
  gates after the union alias is added.
- Emitted-symbol guard for `_point_from_line3d`: pass.
- `git diff --check` for the edited C file and this ledger: pass.

The already parked countdown-increment, dispose, client-machine handler,
start-game, and countdown-update residuals were preserved. There is no new
large blocker for a Claude lane.
