# NetworkServerManager source-attested packet — 2026-09-06

## Result

From canonical `99a194a4b9de3fecd5e4bccb666995f915528119`, the manager moves
from **49 exact / 2 parked / 19 unwritten** to
**54 exact / 2 parked / 14 unwritten**. Five newly implemented functions add
**1,315 meaningful / 1,360 padded January code bytes**. No inherited exact or
parked function changed.

Canonical accounting after the full build and semantic adjustments:

- **920,864 / 2,198,102 exact meaningful code bytes**, previously 919,549.
- **6,230 / 11,060 credited functions**, previously 6,225.
- **2,061,024 / 4,176,062 matched data bytes**, unchanged.
- **391 / 833 Matching objects**, unchanged: this manager is still partial.
- Strict stable sweep: **6,276 / 8,245 exact**, five gains and zero regressions.

## Authority and admitted functions

The [source reconstruction map](../user_source_reconstruction_map_20260906.md)
records the user-supplied files, hashes, provenance limits, and source ranges.
The server attachment is SHA-256
`d84a91f0220e4158fb2de12e387e1323451c72ca56b347258f56d1565dc46969`.
The defining headers were not supplied; target relocations and current owner
implementations remain necessary evidence. The complete attachment is not
republished in this packet.

| Function | Source lines | Meaningful | Padded | Relocs | January normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| `network_game_generate_join_game_token` | 1744-1784 | 150 | 160 | 6 | `13f2acfff35e86b80795cc14e79579c1f1335feed3725b35f127a889ca3a8d59` |
| `network_game_server_remove_client_machine_from_game` | 552-608 | 310 | 320 | 17 | `a56fb67fc46ed03085014d7d37c4591d5c7d27af93376e0c8f7a2883a2c0c728` |
| `network_game_server_remove_machine_from_game` | 659-726 | 307 | 320 | 24 | `fe2a2445c11d218b3a36a81bf54c30eb81341bdb8d8a79a593af076baea6906a` |
| `dump_network_game_data` | 2453-2488 | 245 | 256 | 22 | `3df56040678116c0f75c2e24f65d8505af96bac6017ea6925bfb199bb7033d7f` |
| `network_game_server_dump` | 2492-2534 | 303 | 304 | 36 | `ac5cb8260fdb7d40453e7bdd9a909454a7b3adb486e56ffada9e54e25d15697e` |

January assertion lines 559, 560, 665, and 1754 agree with the supplied source.
The preserved Opus donor object independently contains exact equivalents of
the four remove/dump functions, but it was not copied wholesale. The complete
January incoming-relocation census and a source/type walkthrough support the
packet; no external linkage was invented to force private helper emission.

### Join-token rejection legitimately reopened

The earlier rejection lacked credible evidence for the automatic initializer
and redundant pointer-sized clear. The new source supplies both, and January
confirms the 19 initialized stack bytes, original assertion line `0x6DA`,
four-byte clear, and subsequent 16-byte copy. The parameter's array spelling
still decays to a pointer: `sizeof(join_token)` is four in this ABI. This is
source-attested legacy behavior, not undefined behavior or a newly invented
sequence chosen solely for byte equality.

The debug token is the first 16 bytes of the initializer. The source's
non-DEBUG branch is retained: it combines the compilation timestamp and tag
checksum into the first four bytes, as dictated by the same pointer-size
expression. No cryptographic-security claim is made for this legacy token.

`NETWORK_JOIN_GAME_TOKEN_SIZE` is now a fixed protocol-size macro in
`source/networking/network_messages.h`. That is the canonical shared protocol
owner; its absent original header placement is not claimed as recovered.
The public generator prototype remains in `network_server_manager.h`; the
duplicate internal declaration was removed and the internal header includes
the public owner. Real callers provide 16-byte buffers in client join setup
and server advertisement/join validation. Their complete function sections
remain unchanged.

`tag_groups_checksum` is declared in its implementation's own
`source/cache/cache_files.h`, beside the existing cache checksum API. The
generator includes that header only in its non-DEBUG dependency branch. This
is a measured dependency-scope adaptation from the supplied unconditional
include list, not a claim about the original header schedule. Both debug and
release candidates compile with their correct dependencies.

### Connected removals and genuinely private dumps

Remove-client has real callers in the existing stalled-client path and the
new remove-machine path. Remove-machine already has its proper declaration
in the narrow internal manager header and a real message-handler caller.
Both supplied definitions have external linkage; neither dump does.

The two dumps keep their authentic names and static definitions. Only target
metadata offsets `1168752` and `1169008` are renamed and marked static. The
data-dump private ABI (prefix in ESI, game pointer on stack) matches naturally;
no helper export, emission anchor, forced inline, or signature fiction was
needed despite additional callers remaining unwritten.

The canonical typed game/machine/player layouts already cover the accessed
fields. No raw offsets, shadow structs, or new shared structure definitions
were added. `cseries/errors.h` supplies the actual `error` prototype and
`_error_silent` enum; the existing quit helper's literal priority `2` was
replaced with that name without changing its bytes. The supplied original
name for that already-exact quit helper remains a separate names-only task.

## Bounded experiments and regression correction

1. Frozen baseline: 49 exact / 2 parked / 19 unwritten.
2. Owner-only focused gates preserved server, server-handler, and client
   counts. The first join-token body reached 50 exact immediately.
3. The first full header-consumer sweep caught **four regressions**: decals
   delete-permanent, two race functions, and one solo-level UI function.
   This intermediate state was frozen but never admitted or pushed.
4. Removing the checksum declaration from the broad tag-groups header and
   placing it in `cache_files.h` restored the first three. Expressing the fixed
   protocol size as a macro instead of adding an anonymous enum definition
   restored the UI function. These preserve genuine ownership and ordinary
   constant semantics; no compiler flag or optimization control was changed.
5. The four connected bodies initially failed compilation because the error
   enum was not included. Adding its real owner header made the first complete
   trial exact for all four, preserving every existing function. The two dump
   names were first tested with diagnostic-only aliases, then integrated into
   source/target metadata and rebuilt without aliases.
6. Final full build/sweep: five gains, zero regressions. The complete remaining
   source-side metadata differences in seven other objects are compiler-local
   label numbering, not changed code/data sections or relocation destinations.

The two existing parked functions, start-network-game and update-countdown,
remain byte-for-byte at their prior candidate verdicts. The signed-overflow
countdown increment remains rejected; source provenance does not waive that
rule. No broad residual-shape search was restarted.

## Verification

- Pinned XDK 3911 VC7 13.00.9254.1 and repository flags, unchanged.
  Local `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Full Ninja build after the final source/config changes: pass.
- Full stable per-function sweep: **five gains / zero regressions**.
- All **572 candidate and 833 target objects** compared against the immutable
  baseline. Other target objects are unchanged after timestamp normalization.
  The manager target's **222 section headers, payloads, and raw relocation
  tables** are unchanged; only the two authenticated symbol names/storage
  classes change.
- All **52 inherited emitted manager function sections**, including fuzzy and
  candidate-only functions, are strictly unchanged. All sections of seven
  other metadata-changed candidates retain size, flags, normalized payloads,
  and ordered resolved relocations. Their symbol records agree after narrowly
  normalizing defined compiler `$L`/`$cleanup$` label numbers; offsets, section
  identity, symbol type/storage, and multiplicity are preserved. No comparator
  policy or semantic allowlist was modified.
- Manager emits no `point_from_line3d` code symbol or COMDAT. Units remains
  **189/189 exact**, including the 1,920-byte node-orientation sentinel.
- Park manifest: **297 active / 0 stale / 0 invalid**.
- Fake-match lexical scan of the five edited source/header owners: zero leads;
  independent source/ABI review found no production blocker.
- Full tools suite: **953 passed, 2 skipped, 26 subtests passed**. The two
  skips deliberately exclude release from January-offset mutation tests; the
  required local compiler, target, and emulator were present.

### Runtime oracle scope

[tools/test_network_join_token_runtime.py](../../tools/test_network_join_token_runtime.py)
executes the actual January function and actual VC7-compiled debug/release
translation units. It pins January's size, normalized code hash, and six
ordered relocation identities, and explicitly selects the local VC7 compiler.
The **115 passing cases** cover aligned/unaligned and page-crossing output,
four initial fills, three checksums, output/caller-stack guards, return address,
callee-saved registers/ESP, seeded volatile registers/flags, the complete
initializer, exact clear/copy arguments, null assertion message/location/fatal
dispatch, and release transformation. The stack guard conservatively permits
512 bytes below entry ESP for locals and outgoing arguments.
The dead incoming pointer slot is not a caller canary: VC7 demonstrably reuses
it in the release build. An overstrict pair-immutability probe failed there
and was corrected to protect the return address and true caller-owned region,
without changing production code or weakening the output/ABI checks.

Negative controls change clear/copy sizes only in memory. Their setup is
validated outside the expected-exception scope, and a dedicated exception must
come from an **executed** incorrect memory-API call; unrelated setup failures
cannot satisfy the control. This fixes a weakness caught by independent review.

The external diagnostic/memory/checksum APIs are bounded doubles, not whole
subsystem tests. Fatal dispatch is tested, not process termination itself.
Release expectations use the emitted timestamp string; original `__DATE__`
and `__TIME__` provenance is established by source review, not independently
proved by that runtime check. Missing proprietary prerequisites cause portable
pytest skips; this publication relies on the concrete dependency-present run.

## Immutable local evidence

| Artifact | SHA-256 / role |
| --- | --- |
| `scratch/server-source-packet-before-20260906.json` | `073cb2f04c83f774e05c9cdf0d92ebca9f1867da0facde51cfbbbd14c9e44c27`; original source/config, reports, and all built objects |
| `scratch/server-source-packet-token-20260906.json` | `139338dbbd0073290391cc9ed8bb5828248f1c90599af8c71e2a0ca72f7bee39`; rejected four-regression intermediate |
| `scratch/server-source-packet-corrected-20260906.json` | `55226942f52fde641128cf199f4cb8426de2eed1a2b358a60019385d9f1a9950`; token-only, regression-free owner correction |
| `scratch/server-source-packet-final-20260906.json` | `55251aed6eb642051bac29a1aa4420c253c710a5b791937cf4af96a9f8e8f3d9`; five-function final sources/objects |
| `scratch/server-source-packet-final-whole-objects-20260906.json` | `8d6052549fc71843a33488985040cb216623832b2f631dc6e608b1cd214b6a78` |
| `scratch/server-source-packet-before-stable-20260906.json` / `scratch/server-source-packet-final-stable-20260906.json` | 6,271 to 6,276 exact, no regressions |
| `scratch/network_server_remove_machine_dump_packet_handoff_20260906.md` | `a7a307bc3bbf9d099dabd38f46b47bdefee4c7aab5394389a6fc2c5c45084e9c`; source/target/layout and live-caller census |

Scratch objects, source attachments, and proprietary binaries remain local.
No active Opus worktree was modified.

## Next work

There are 14 unwritten functions left. Reconstruct the source-backed
reset-to-pregame/playlist-setup pair (592 + 208 padded bytes) after resolving
its real field names; then the private client-handling/pregame-idle and
creation/rejection families as connected callers and helpers. Do not export
small callbacks merely to make the compiler emit them. Keep the two parked
residuals and undefined-overflow rejection out of the next easy-gain queue.
