# NetworkServerManager reset-connected packet — 2026-09-06

## Result

Baseline: canonical `74f7644d14d4adb7192cd3910b76187d36b7c455`.
The manager advances from **54 exact / 2 parked / 14 unwritten** to
**56 exact / 3 parked / 11 unwritten**. The naturally connected packet is
public reset-to-pregame plus private playlist setup and client handling.
Only the first two receive exact credit: **792 meaningful / 800 padded bytes**.
Client handling is retained at **99.932884% fuzzy**, with zero exact credit.

The full build's adjusted canonical totals are:

- **929,562 / 2,198,102 meaningful exact code bytes**, previously 928,770.
- **6,279 / 11,060 credited functions**, previously 6,277.
- **391 / 833 Matching objects**, unchanged; the manager is still partial.
- **2,061,024 / 4,176,062 matched data bytes**, unchanged.
- Stable whole-population sweep: **6,325 / 8,245 strict**, two gains and zero
  regressions. Both inherited fuzzy manager bodies also remain unchanged.

No work in an active Fable/Opus worktree was changed. This is new source-backed
work, not automatic reconciliation credit against an Opus headline total.

## Source and target evidence

The [source reconstruction map](../user_source_reconstruction_map_20260906.md)
pins the user-supplied server source, SHA-256
`d84a91f0220e4158fb2de12e387e1323451c72ca56b347258f56d1565dc46969`.
Its original defining headers and independent full provenance are unavailable;
the complete attachment is not republished.

| Function | Supplied lines | Meaningful / padded | Relocs | January normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `network_game_server_setup_game_from_playlist` | 2396-2434 | 208 / 208 | 13 | `c8594ec7a859d7a7eb33bb94f2cc0e6696f9522aad1a1c310c6c9e68e5204bcd` |
| `network_game_server_reset_to_pregame` | 795-918 | 584 / 592 | 26 | `0c0c9c29b194f113e494f367fd6db171972ec1225ffd1dde7e517b5b3dfd048f` |
| `network_game_server_handle_client_machines` | 2081-2158 | 451 / 464 | 25 | `0935e7d0a5d8d56a67ff919a2df0a58398375cd09e1ae6d24ceb16121ca489cc` |

Reset has a public cdecl boundary and returns `boolean` in AL. The old public
`void` declaration was incorrect; the genuine public header now owns the
boolean prototype. Redundant reset/pause declarations were removed from the
internal header, which already includes that owner. No consumer-local foreign
prototype was added.

Both new helpers remain static. January receives their server pointer in EBX
and returns AL; the real reset calls naturally produce that same private ABI.
Target metadata changes only authenticated symbol records at file offsets
1168496 and 1170896, renaming them and marking them static. No export, address
anchor, forced emission or artificial caller was introduced.

Setup obtains the current playlist stage, generates/copies the local machine
name, sets map version/player/team limits, opens the game, and returns actual
success. Reset retains all January branch ordering, including team swaps after
failed non-postgame setup and a false result after the playlist-ended alert
path. These unusual results are source- and target-attested, not missing logic
to invent or simplify.

## Owner and defined-behavior adaptations

- The existing map/version, minimum/maximum player, maximum-team and sent-start
  fields now use supplied semantic names. Storage widths, offsets and the
  existing 0x434 game layout are unchanged. Minimum-player signedness is also
  corroborated by sign-extension in inherited exact countdown code.
- The four-byte switch-to-pregame payload is a genuine message record, matching
  January and the existing client handler. The settings payload uses the
  existing complete local `network_game`, as the prior start-game body does;
  no synthetic padding wrapper or new foreign type facade was added.
- The client handler uses a word array occupying exactly 0x800 bytes. This
  supplies real word alignment/type for its word-pointer handler, instead of
  copying the supplied byte-array pointer cast. January's frame is 0x808 and
  its word capacity slot is reset to 0x800 before each successful next read.
- `_connection_dont_timeout = 0` resides in the actual connection API header.
  January pushes literal zero, and the defining idle implementation treats
  zero as no timeout. The unrelated debug boolean and 0xFFF message-header
  capacity are not substituted. Original header placement is not claimed.
- The game counter retains its established signed `long` storage, but adds
  one through `unsigned long`, then converts back. Addition is defined modulo
  2^32; conversion outside the signed range is implementation-defined and the
  pinned VC7 build retains the same bit pattern. This documented target-ABI
  adaptation removes signed-overflow UB without falsifying the shared field
  type. It still emits January's load/INC/store sequence exactly.
- Team swaps use `_team_red` / `_team_blue`; client flags use `SET_FLAG` with
  their actual word field. New bodies use normal project formatting/returns.

Duplicated legacy `network_game` schemas still have semantic-name/signedness
debt outside the accesses proved here. This packet does not claim a complete
shared-schema cleanup, full network runtime, or whole-program link.

## Honest residual and diagnostic trap

Client handling has the same 464-byte size, 162 instructions, and 25 ordered
relocations as January. Its only non-relocation byte difference is at +0xED:
TEST EAX versus TEST AL after `network_connection_connected`. The actual
provider is declared and defined as returning `long`; the supplied caller
does not contain that owner's missing original header. Keep the truthful
owner and park the natural condition. A matching-only cast or contradictory
local boolean declaration is not accepted evidence.

An initial diagnostic alias pass textually replaced a helper name inside an
event string and therefore changed its relocation identity. That scratch
result is not admission evidence. The integrated build uses real target names
without aliases and retains the full supplied/January failure diagnostic.
The existing start-network-game and update-countdown parks are unchanged.

## Verification

- Full Ninja build, including the shared-header transitive consumers: pass.
- Full stable sweep: two gains, zero regressions across all 8,245 entries.
- Frozen comparison of all **572 rebuilt and 833 target objects**: pass.
  All **171 inherited server runtime sections** retain bytes, relocations,
  owners, offsets, linkage and selection. Exactly three new code owners are
  added, with no new COMMON. Twelve other candidates change compiler metadata
  only; their complete runtime-section, symbol and undefined-owner inventories
  remain equal. The server target's 222 section headers, payloads and raw
  relocation tables are unchanged; exactly the two authenticated symbol
  names/storage classes change. Other targets are timestamp-normalized equal.
- Focused final gate without aliases: 56 exact / 3 residual / 11 unwritten;
  explicit `_point_from_line3d` emitted-symbol guard passes.
- Tools suite: **987 passed, 2 skipped, 26 subtests passed** with the local
  compiler, target objects and emulator available. No new skip was accepted.
- Park validation: **295 active / 0 stale / 0 invalid**. Object admission:
  zero candidates, contradictions or revocations; five existing rejections.
- Lexical fake-match scan of all four edited C/header owners: zero leads;
source/ABI review remains the substantive credibility check.

The new 20-byte UTF-16 `L"<unknown>"` literal is naturally pooled SELECT_ANY
data, not a server-target-owned gain. January's selected copy is in
`hud_messaging.obj`; the existing rebuilt copy is in
`ui_widget_game_data_input_functions.obj`. Both match the new literal's full
payload, flags, symbol offset/storage and selection. This accounts for the
extra ordinary compiler owner without suppressing it or granting data/whole-
object credit. It is not a claim that a complete ordinary final link was run.

`tools/test_network_server_reset_contract.py` verifies that a caller can use
the actual public boolean result. Its temporary void-header negative control
requires the specific VC7 error at the result assignment, not an arbitrary
compile failure.

`tools/test_network_server_reset_counter.py` pins the actual target function
and freshly compiled complete manager function before executing their
call-free +0x4A..+0x77 counter/store segment. Its 30 cases cover seven counter
values under two memory fills for both owners, adjacent-store bounds, and an
executed INC-to-DEC negative control. This verifies the compiler-specific
counter behavior, not the whole state machine or a portable signed conversion.
Unicorn's internally handled Windows mapping exceptions produced noisy
faulthandler messages on the first passing run; the same scoped diagnostic
suppression as the existing emulator tests yields a clean final passing run.

## Immutable local evidence

- Before capture: `scratch/server-reset-connected-before-20260906.json`,
  SHA-256 `3da6e1f1ed0e4738523e8af431e10f6e68a4d0ef5ef851854218f42fca542769`.
- Final source/config/build capture:
  `scratch/server-reset-connected-final-20260906.json`, SHA-256
  `e48b7789b8773da30dcc2139b50c3658db28efec0e8471ea394d322d9c402ed5`.
- Stable snapshots: `scratch/server-reset-connected-before-stable-20260906.json`
  and `scratch/server-reset-connected-final-stable-20260906.json`.
- Independent ABI/layout review:
  `scratch/network_server_manager_reset_setup_handler_abi_review_rasterizer_scout_20260906.md`,
  SHA-256 `43ce04f2edeaaf3113a3afc872577956f0ead2ca225cc4f0162773f379e9829c`.
- Root full frozen-object audit:
  `scratch/server_reset_root_whole_audit_20260906.py` and
  `scratch/server-reset-root-whole-audit-20260906.json`, report SHA-256
  `684e93b131098673213c9c9bbd035b3cc8e9a622ffd84522cb0a79f22dea7a40`.

The capture includes source/config/compiler inputs and every built object,
not documentation or Python tests. Scratch files and proprietary inputs stay
local. IDA remains unavailable; this packet does not falsely claim a new IDA
analysis. The cross-build Ghidra intake remains separate corroborating work.

## Next connected work

Eleven target functions remain unwritten. Continue through creation/disposal
and the public-endpoint/rejection or pregame-idle helper chains using the
supplied source and January ABI. Work from real callers; do not export dead
leaves. The countdown overflow rejection and three fuzzy residuals require
new evidence rather than renewed declaration or code-generation sweeps.
