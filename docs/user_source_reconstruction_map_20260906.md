# User-supplied CE source: reconstruction map, 2026-09-06

Read this alongside [common constants](common_constants.md),
[assertion contracts](assertions.md), and the
[matching methodology](matching_methodology.md). This is a map for reviewed
reconstruction, not authorization to replace three complete canonical files.

## Provenance and boundaries

The user described the files as actual Bungie source. The source text and
January assertion/call-site fingerprints provide unusually strong local
evidence, but the original repository, complete build context, defining
headers, and independent provenance were not supplied. In particular, the
user does **not** have Bungie's `cseries.h`. Do not present reconstructed
convenience macros as recovered original definitions.

The input directory is local and read-only for this campaign:

```text
C:/Users/isabe/Downloads/Wormhole yyO4nv/
```

| File | Size | SHA-256 |
| --- | --- | --- |
| `network_server_manager.c` | 74,686 bytes / 2,534 lines | `d84a91f0220e4158fb2de12e387e1323451c72ca56b347258f56d1565dc46969` |
| `random_math.c` | 7,982 bytes | `3d4a67b28efc9f6e9b33ed83efee72d27a90672737fcad4648fecc69a16d5680` |
| `network_client_manager.c` | 63,632 bytes / 2,049 lines | `b2815faf5e0f34a28a10360314bc8eb1c15b17f5f2ada2baeb9fffc4a5344907` |

Line numbers below refer to these exact inputs, not current canonical lines.
The complete attachments are not republished in this documentation batch.
Canonical baseline for the review was
`98f3bb719d7af4ccf572c51d126e5534356d7709`.

## Assertion evidence

RandomMath lines 41/56, server line 1248, and client line 521 use the
two-argument `vassert(expression, message_expression)` API. These are call
sites, not macro definitions. They corroborate the existing VC7-compatible
message-expression convention. Format messages with the ordinary variadic
`csprintf(temporary, format, ...)` function; the pinned compiler does not
support variadic macros.

An assertion expresses a required invariant. Its diagnostic is evaluated only
if the predicate is false. Warnings continue after reporting; halts are
unconditional. The current project's behavior and the new `d*` aliases are
documented and tested in [assertions.md](assertions.md).

## Network server: new evidence reopens a small rejected body

Canonical owners:

- [source/networking/network_server_manager.c](../source/networking/network_server_manager.c)
- [source/networking/network_server_manager.h](../source/networking/network_server_manager.h)
- [source/networking/network_server_manager_internal.h](../source/networking/network_server_manager_internal.h)

At initial source review the census was **49 exact / 2 parked / 19 unwritten**. All 19 missing
bodies are represented in the supplied source. This is not 19 new credited
closures: headers, global backing, helper reachability, code generation, and
full consumer regressions still need review.

**Integration update:** the join token and both machine removals plus both
private dumps are now verified: **54 exact / 2 parked / 14 unwritten**,
**+1,315 meaningful code bytes**, zero regressions. See the
[source-attested integration ledger](object_matching_logs/network_server_manager_obj_source_attested_packet_20260906.md)
for immutable evidence. The subsequent [reset-connected packet](object_matching_logs/network_server_manager_obj_reset_connected_packet_20260906.md)
adds reset and playlist setup exact, plus an honest fuzzy client handler:
**56 exact / 3 parked / 11 unwritten**, **+792 meaningful exact bytes**.
Creation/disposal and the remaining private idle/connection families are next;
the return-type conflict and existing countdown residuals stay parked.

### First packet: join-game token (150 meaningful / 160 padded bytes)

Supplied lines 1744-1784 define `network_game_generate_join_game_token` with
an array-spelled parameter, a 19-byte automatic initializer, and an assertion
at line **1754 (`0x6DA`)**. The parameter decays to a pointer, so the following
`sizeof(join_token)` clear is **four bytes**, not the full token size. A
subsequent bounded copy writes 16 bytes.

January independently corroborates the line number, stack initializer,
four-byte `csmemset` at function offset `+0x78`, and sixteen-byte `csmemcpy`
at `+0x82`. This directly supplies evidence missing from the
[older rejection](object_matching_logs/network_server_manager_obj_opus_dirty_reconciliation_20260904.md).
The redundant clear is now source-attested, not merely an invented sequence
that happens to match. It is not itself undefined behavior.

Reopen this one coherent candidate. Restore `NETWORK_JOIN_GAME_TOKEN_SIZE`
through its genuine shared header owner and audit the real manager/handler/
client callers. The defining header was not attached; value **16** is target
corroborated, not a recovered original declaration. Preserve the debug/release
conditional source distinction and January's actual build configuration.
Do not generalize this evidence into permission for unrelated redundant or
nonsensical code. The subsequent integration is recorded in the linked ledger.

### Private names supplied directly

The abbreviated offset suffixes below identify January target symbols within
this object. Resolve the complete `config/symbols.json` record before renaming;
do not guess a full address from the suffix. These helpers remain TU-private.

| January suffix | Supplied source name | Input lines |
| --- | --- | --- |
| `D0D0` | `network_game_server_add_new_client` | 1997-2053 |
| `D240` | `network_game_server_handle_public_endpoint` | 2055-2077 |
| `D2E0` | `network_game_server_send_rejection_message` | 2161-2188 |
| `D380` | `network_game_server_reject_connection_game_is_full` | 2190-2197 |
| `D3B0` | `network_game_server_idle_postgame_tasks` | 2199-2222 |
| `D400` | `network_game_server_have_all_machines_have_precached` | 2225-2252 |
| `D470` | `network_game_server_setup_game_from_playlist` | 2396-2434 |
| `D570` | `dump_network_game_data` | 2453-2488 |
| `D670` | `network_game_server_dump` | 2492-2534 |
| `DDD0` | `network_game_server_handle_client_machines` | 2081-2158 |
| `DFA0` | `network_game_server_idle_pregame_tasks` | 2254-2394 |

The dump name is literally `dump_network_game_data`, replacing the earlier
inferred name `network_game_dump` as the stronger naming evidence. Source and
target topology agree on the pregame-to-precache helper and the
client-machine-to-server-dump-to-data-dump chain.

### Following connected packets

1. **Remove-client, remove-machine, and two dumps:** approximately 1,200
   padded target bytes, with live callers. Input lines 552-608 and 659-726
   connect naturally to dumps at 2453-2534. Keep both dumps private; the supplied
   remove-client and remove-machine definitions have external linkage. The intervening
   `network_game_server_remove_players_from_machine_ingame` (611-657) maps to
   an already-exact helper currently named `send_player_quit_messages_ingame`.
   Audit that rename separately from body gains.
2. **Reset-to-pregame plus playlist setup:** 592 + 208 padded target bytes;
   input 795-918 and 2396-2434. Authenticate map version, minimum/maximum
   players, and team fields in shared owners before removing synthetic local
   representations.

The countdown enum (31-48) gives original spellings and millisecond
expressions recorded in [common_constants.md](common_constants.md). Assertion
lines 95, 126, 137, 1248, and 1646 also align with January. However, the
increment function's signed-overflow probe remains a policy concern: original
source evidence does **not** waive the no-undefined-behavior rule. Nor does
this attachment automatically solve the two parked code-generation residuals.

Existing continuation context:

- [Small unwritten server leaves](object_matching_logs/network_server_manager_obj_small_unwritten_leaves_20260904.md)
- [Server idle natural boundary](object_matching_logs/network_connection_obj_server_idle_natural_boundary_20260904.md)

## RandomMath: source-quality reconciliation, not new-byte credit

[source/math/random_math.c](../source/math/random_math.c) is already
**17/17 exact, 1,328 padded bytes**. Do not count a cleanup as new exact code.
The supplied file is particularly useful for authentic local names and
constant expressions:

- Line 14 names the geosphere argument
  `RANDOM_DIRECTION_TABLE_GEOSPHERE_SEGMENT_COUNT = 16`.
- Lines 92-93 and 150-153 supply the LCG coefficient/range macros. Preserve
  unsigned 32-bit seed arithmetic and the different real/integer endpoints.
- The private helper at declaration 22 / definition 244 is
  `direction3d_from_table(real_vector3d *direction, short index)`; canonical
  currently uses the address-derived `code_000fab20` with reversed formal
  order. Rename only with a complete caller/ABI check, not a blind signature
  text replacement. The source's `inlined` spelling is not permission to
  change January's compiler-established inline schedule.
- The source uses separate seed globals and the real geometry owner.
  Canonical currently has an aggregate with forced BSS placement and a local
  geosphere facade. Reconcile these through genuine ownership/layout evidence,
  not a second TU-local shadow type. Do not import representation-punning
  casts merely because they occur in the supplied source.
- `seed_random_orientation` (267 onward) expresses yaw/pitch/roll directly and
  calls `roll_vectors`; canonical uses range helpers and a related
  `yaw_vectors` formulation. Compare the actual math owner and generated code
  before replacing an already-exact implementation.
- `get_number_suitable_for_initializing_random_seed` returns `long` in the
  supplied file; canonical uses `unsigned long`. Audit its public owner and
  callers even though both occupy four bytes in the January ABI.

Use [source/math/random_math.h](../source/math/random_math.h),
[source/math/real_math.h](../source/math/real_math.h), and the genuine geometry
and system headers. Do not duplicate foreign prototypes in RandomMath merely
to keep the current translation-unit layout.

## Network client: source map, not a wholesale replacement

Compare against
[source/networking/network_client_manager.c](../source/networking/network_client_manager.c)
and its associated public/internal headers. Current status is **47 exact /
5 parked / 0 unwritten**, as recorded in the
[Opus reconciliation ledger](object_matching_logs/network_client_manager_obj_opus_reconciliation_20260904.md).
The supplied 2,049-line file
provides connected caller logic as well as the two-argument assertion at 521.
Its constants block (34-51) distinguishes debug/release connection timeouts,
search cadence, advertised-game expiry, ping cadence, and initial packet
sequence number. These are recorded in [common_constants.md](common_constants.md)
as supplied-source names rather than assumed canonical header declarations.
Treat differences in machine/player structures, transport ownership, and
source-private helper linkage as review work, not as automatically compatible
declarations. Preserve existing exact implementations while taking one
caller/helper packet at a time.

Three private names have stronger direct-source evidence now:

| January offset | Current canonical name | Supplied private name / lines |
| --- | --- | --- |
| `0x001141C0` | `network_game_client_network_available` | `check_networking_and_generate_error`, 120-136 |
| `0x00115530` | `network_game_client_add_advertised_game` | `add_advertised_game`, 1510-1628 |
| `0x00115850` | `network_game_client_precache_map` | `network_game_client_update_precache_status`, 1835-1870 |

Keep all three static and update true callers plus target-symbol metadata in
one names-only packet, with a rename-stable object comparison. The other seven
private names already agree with the supplied file. Do not credit renames as
new bytes.

The supplied `network_game_client_reset` parameter is `teardown_connection`
(1258-1261); TRUE disconnects at 1281-1292. Canonical calls the same-behaving
parameter `leave_connection_open`, a misleading name that should be corrected
in the definition and owner prototype without inverting its behavior.

The supplied `network_game_client_get_state` returns `word` and takes
`word *progress` (245-261), while the current owner uses `short`/`short *`
based on later type evidence. Width agreement is not signedness agreement;
resolve the owner evidence before changing it. Likewise, the attachment lacks
the defining `network_connection_connected` declaration and does not settle
the long-versus-boolean owner conflict behind two assertion residuals.

Supplied remove-player (618-679), add-advertised-game (1510-1628), and
idle-ingame (1925-1989) add direct source-line provenance but do not establish
a new natural code-generation family beyond the recorded Opus experiments.
Do not restart those parked searches merely because the files are new.
The concrete low-risk client continuation is the name/parameter cleanup;
new body selection remains pending genuinely new target/compiler evidence.

## Admission procedure for every follow-up

1. Freeze current source and all affected candidate/target objects; record
   meaningful exact bytes separately from padded function sizes.
2. Read this source evidence together with January disassembly, relocations,
   and existing rejection/park ledgers. A source resemblance is not a match.
3. Restore names, enums, real owner types, and prototypes in their proper
   places. A shared-header edit can alter unrelated VC7 allocation; enumerate
   consumers and run the full build and stable per-function sweep.
4. Preserve the January inline schedule; do not add `point_from_line3d`
   COMDATs, fabricated dependencies, forced codegen, or undefined behavior.
5. Run focused strict gates, runtime/ABI checks where appropriate, full tools
   tests, and a rename-stable regression comparison. Park a credible fuzzy
   implementation if easy exact matching fails.
6. Commit and push only reviewed source/tooling gains. Keep attachments,
   original binaries, scratch objects, and other agents' worktrees out of the
   publication payload. Do not claim already-exact bodies as new progress.
