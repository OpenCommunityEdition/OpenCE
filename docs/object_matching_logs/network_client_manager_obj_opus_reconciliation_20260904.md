# `network_client_manager.obj`: selective Opus reconciliation (2026-09-04)

## Outcome

This packet selectively reconciles the frozen Opus reconstruction of
`source/networking/network_client_manager.c` onto canonical commit
`0a211a992`.  It does not cherry-pick the donor's broad multi-object commit.

The object advances from 25 exact / 0 residual / 27 unwritten functions to
47 exact / 5 residual / 0 unwritten functions:

- 22 newly strict-exact functions;
- 5,975 newly exact meaningful code bytes;
- 6,096 newly exact padded code bytes;
- no data-byte change (the inherited two-byte BSS remains exact); and
- zero rename-stable regressions across all 8,245 tracked functions.

The retained object has 7,776 meaningful / 8,128 padded exact function bytes
out of 10,082 meaningful / 10,480 padded target bytes.  It remains honestly
`NonMatching`; the five credible residual bodies are recorded in
`config/parked.json` rather than forced.

## Frozen provenance and selection

The read-only donor is
`C:\halo-worktrees\opus-small-families-30k-20260902`, committed reconstruction
`608c66380dca2d841b1e95150d1f88abe41a06d8`, and frozen tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`.  The donor commit reconstructed
351 functions across 30 translation units, so only this translation unit and
the declarations required by its real call graph were reviewed here.

The following evidence was checked before admission:

- January's split `network_client_manager.obj`, including every function
  extent, normalized instruction stream, ordered relocation, assertion string,
  jump table, and the two-byte BSS;
- the donor source and its stable snapshot
  `scratch/opus-30k-after-hdr2.json`;
- all preceding `network_client_manager.obj` ledgers in
  `docs/object_matching_logs/`;
- `docs/opus_unfinished_session_reconciliation_backlog_20260903.md` and the
  prior Opus canonical-reconciliation ledger;
- the authenticated `network_connection_connected` January boundary; and
- the HCEX PDB symbol census at
  `research/pdb200/pdb_symbols.json`.

January remains the byte authority.  Opus supplies source candidates, semantic
names, and type/control-flow evidence only.

## Newly exact functions

| Function | Meaningful | Padded |
| --- | ---: | ---: |
| `network_game_client_address_matches_server` | 202 | 208 |
| `network_game_client_ponged` | 173 | 176 |
| `network_game_client_accepted_into_game` | 272 | 272 |
| `network_game_client_game_settings_updated` | 304 | 304 |
| `network_game_client_game_has_started` | 411 | 416 |
| `network_game_client_handle_game_update` | 395 | 400 |
| `network_game_client_add_player_to_game` | 244 | 256 |
| `network_game_client_add_player` | 489 | 496 |
| `network_game_client_update_local_player_data` | 256 | 256 |
| `network_game_client_request_start_time_change` | 197 | 208 |
| `network_game_client_set_error` | 69 | 80 |
| `network_game_client_precache_map` | 185 | 192 |
| `network_game_client_process_incoming_messages` | 114 | 128 |
| `network_game_client_request_remove_player` | 493 | 496 |
| `network_game_client_new_advertised_game` | 67 | 80 |
| `network_game_client_reset` | 221 | 224 |
| `network_game_client_idle_searching` | 704 | 704 |
| `network_game_client_idle_joining` | 380 | 384 |
| `network_game_client_idle_pregame` | 207 | 208 |
| `network_game_client_idle_postgame` | 150 | 160 |
| `network_game_client_create` | 141 | 144 |
| `network_game_client_idle` | 301 | 304 |
| **Total** | **5,975** | **6,096** |

Seven January address placeholders are replaced with semantic private names,
and three already-semantic private candidates are explicitly marked static in
both source and split configuration.  The HCEX PDB contains the surrounding
public network-client API, including `unstrip_player_index`, but no public
labels at these ten private boundaries.  Their source behavior, diagnostic
strings, and placement identify the conventional names; no `code_...` name is
retained and no private function is exposed merely to satisfy the linker.

## Type, owner, and header reconciliation

The retained code uses named network-game, message, address, player, and
connection records with zero-emission size/offset assertions.  Message size
comes through `GET_MESSAGE_SIZE`, datum indices through
`DATUM_INDEX_TO_ABSOLUTE_INDEX`, and advertisement/client flags through
`TEST_FLAG`/`SET_FLAG`.  Switches use the network client state, rejection,
platform, and message enum constants rather than numeric cases.

Public declarations are moved to their owners: transport operations,
connection write/idle, cache precaching, the local-player setter, the server
join-token generator, and the complete network-client API.  The three
translation units which consume update-queue APIs use the narrow owning
`game/player_queues_new.h`; placing those declarations in broad `players.h`
was proven to change C2 allocation in
`units::_unit_preprocess_node_orientations`.  The narrow owner header restores
that sentinel while keeping every queue/client/global function stable.

One explicitly measured declaration boundary remains: `get_random_seed` is
defined in `math/random_math.c`, whose existing closest associated owner is
the very broad `math/real_math.h`.  Adding the missing declaration there
regresses the unrelated exact
`effects/decals::_decals_delete_permanent_from_cluster` through C2
definition-position sensitivity.  A false TU-local declaration would violate
ownership, so neither form is retained; the existing C89 implicit declaration
is documented for a future narrow random-math header/whole-tree migration.

The donor declared and defined `network_connection_connected` as `boolean`.
January's owner body instead materializes its result in full EAX, and canonical
correctly declares it as `long`.  This packet preserves that truthful owner
contract and uses ordinary `(boolean)` narrowing casts at client predicates.
That decision deliberately turns two donor-exact assertion owners into honest
relocation-only residuals instead of preserving a contradictory local
prototype.

## Parked residuals

| Function | Padded / relocs | Objdiff | Boundary |
| --- | ---: | ---: | --- |
| `network_game_client_add_advertised_game` | 720 / 22 | 82.70755% | spill, closed-slot branch, aggregate-copy, and platform-selection schedule |
| `network_game_client_idle_ingame` | 400 / 27 | 76.03846% | success/silent value lifetimes and failure-block placement |
| `network_game_client_remove_player` | 368 / 18 | 98.06201% | two narrow byte-register/load-order choices |
| `network_game_client_initiate_join_game` | 256 / 15 | 100.0% | assertion string only: truthful narrowing cast versus false boolean prototype |
| `network_game_client_leave_game` | 608 / 42 | 96.15385% | assertion string only; all code and jump-table destinations agree |

The first three fail-closed classifier runs return `UNKNOWN`; their park class
is therefore `unclassified`, not an invented recipe.  The final two have
identical normalized code hashes on both sides.  Hard-coding assertion text,
restoring the false boolean prototype, or relying on upper-byte assumptions
would be fake matching and is rejected.

## Canonical replay

The isolated packet was replayed after the admitted first-person-weapon packet
at canonical commit `6d351b357`.  All 22 network-client gains survived, but
the newer whole-tree context exposed two shared-header effects which were not
admissible as first written.

Adding `local_player_set_player_index` to the broad `game/players.h` reopened
the already-finished 1,920-byte
`units::_unit_preprocess_node_orientations`.  Moving that declaration to the
narrow domain owner `game/local_players.h`, included by the reconstructed
client, restores `units.obj` to 189/189 without a caller-local prototype.
Likewise, adding `cache_files_give_time_to_precache` to broad
`cache/cache_files.h` changed the measured compiler frontier of the unrelated
parked `bitmap_group_add_bitmap`.  The narrow
`cache/cache_file_precaching.h` owner preserves the bitmap park and keeps the
client exact.  Neither correction uses conditional declarations, compiler
steering, or a declaration in the consuming `.c` file.

The final current-tree stable sweep reports **22 gains / 6,096 padded bytes /
zero regressions** across all 8,245 target function sections.  Aggregate
semantic progress moves from 855,687 to **860,625 / 2,198,102 meaningful code
bytes** and from 5,955 to **5,975 / 11,060 functions**.  That net
**+4,938-byte / +20-function** movement is smaller than the strict gain because
three newly named strict functions were already credited through the
name-independent semantic COFF overlay (`add_player`, 489 bytes;
`request_remove_player`, 493; and `idle`, 301), while the instruction-exact
`initiate_join_game` assertion-relocation boundary adds 246 bytes and one
ordinary structural credit.  In other words, the accounting difference is
deduplication, not lost code.

Matched data remains **2,007,670 bytes** and linked objects remain **391/833**.
The combined semantic report contains 6,047 accepted exact functions and zero
unit errors.  The additive park manifest validates **205 active / 0 stale / 0
invalid**, including the five honest client residuals.

## Validation

- focused gate: 47 exact, 5 residual, 0 unwritten;
- `ninja all_source`: pass for all 572 translation units;
- rename-stable comparison against
  `scratch/after-game-safety-pair-20260904.json`: +22 exact / +6,096 padded
  bytes / zero regressions;
- semantic audit: 473 units, 6,296 functions evaluated, 5,956 semantic exact,
  5,974 accepted exact, 165 hidden exact, zero unit errors;
- all five new parked records validate active; the manifest has zero invalid
  entries and one unrelated pre-existing stale bitmap-group record;
- object admission: zero candidates, contradictions, or revocations;
- fake-match scan over all 14 changed source/header files: zero findings;
- tooling tests: 261 passed;
- no `point_from_line3d` symbol or COMDAT is emitted by the rebuilt object;
- protected bitmap files are unchanged; and
- `units::_unit_preprocess_node_orientations`, `player_queues_new.obj`, and
  `network_game_globals.obj` remain stable at 1/1, 9/23, and 26/26 exact
  focused gates respectively.

The retained source contains no inline assembly, `volatile`, `register`,
compiler barrier, pragma steering, forced inline/noinline, raw-address field
access, representation pun, fake dependency, or nonsensical coincidental
branch.
