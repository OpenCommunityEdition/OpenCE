# NetworkServerManager creation, rejection and safe disposal — 2026-09-06

## Result

Baseline: canonical 8549cb79af14b917bc7a7a84ad3ad5298a3a7b30.
The manager advances from **56 exact / 3 parked / 11 unwritten** to
**59 exact / 4 parked / 7 unwritten**. Creation/rejection adds **512 meaningful /
544 padded exact code bytes** and three credited functions. Safe disposal is
retained at **97.77778% fuzzy**, with zero exact credit.

Verified adjusted canonical totals:

- **930,074 / 2,198,102 meaningful exact code bytes**.
- **6,282 / 11,060 credited functions**.
- **391 / 833 Matching objects**, unchanged; this object is still partial.
- **2,061,024 / 4,176,062 matched data bytes**, unchanged.
- Stable sweep: **6,328 / 8,245 strict functions**, three gains, zero regressions.

The existing add-player function receives a genuine storage-owner correction,
not another function or byte credit. All three inherited fuzzy bodies survive.
Active Fable/Opus lanes remain untouched. This independent source-backed work
is not automatic credit against a donor lane's headline total.

## Source and January evidence

The [supplied-source map](../user_source_reconstruction_map_20260906.md) pins
the user-provided server source at SHA-256
d84a91f0220e4158fb2de12e387e1323451c72ca56b347258f56d1565dc46969.
The complete attachment and proprietary input binaries are not republished.
Missing original headers and independent full provenance are not claimed.

| Function | Supplied lines | Meaningful / padded target bytes | Relocs | January normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| network_game_server_create | 219–284 | 332 / 336 | 40 | e657627b10c96be0fec0c7aae04c92800778325a729c272ddd43c3756b5690ec |
| network_game_server_send_rejection_message | 2161–2188 | 147 / 160 | 12 | a946f2a05e51728349e1ec2962fb0091ba1899b3af49c04831578a6005f4804f |
| network_game_server_reject_connection_game_is_full | 2190–2197 | 33 / 48 | 3 | 1a6ffe5110f6cabf44dd23d50678c9ebfdf84ab5727eb134a12728d0d5b8be7b |
| network_game_server_dispose | 285–376 | 281 / 288 | 27 | f85cfb83330267866b08d5d10437b8f9b0879ae564ba22026d33a16cfb33ff89 |

Create/dispose are genuine public cdecl functions. Create takes the real
address of its static rejection callback, which naturally supplies the
private sender's EBX endpoint and AX reason. No artificial caller, export,
forced inline, private ABI wrapper or emission anchor is used. Only target
metadata records at file offsets 1168096 and 1168256 are renamed and marked
static; all target section bytes and relocation tables survive.

Create retains the supplied singleton allocation's constant non-null check.
That legacy topology is directly present in the attachment, not an invented
code-generation guard; VC7 removes it naturally. Create clears the 0x4BC
server, creates its connection on port 5150, initializes all four clients and
real state fields, installs the callback, invokes reset and disposes on error.

The rejection payload is a genuine two-byte word reason record, corroborated
by January schemas/consumers. The sender obtains header size before swapping
and writing. It preserves the full failure diagnostic, including the 105-byte
literal referenced at +0x82. The final build/gate uses no textual aliases.

## Correct storage ownership, not a fake extern

Supplied source lines 214–215 define the backing singleton tentatively and
initialize its separate one-byte in-use guard to FALSE.

- network_game_server_memory_do_not_use_directly is a natural VC7 COMMON of
  **0x4BC / 1,212 bytes**. January's manager has an undefined external; its
  selected zero-filled owner is linker_common.obj, section 119, offset zero,
  size 0x4BC, flags 0xC0600080. No invented initializer, extern-only declaration,
  layout facade, custom section or linker script is introduced.
- The explicitly initialized guard is an external manager .bss definition at
  offset zero. January's manager .bss is **8 bytes**, flags 0xC0300080.
- Supplied add-player source line 1129 has a function-local static round-robin
  counter (flipflop). Its topology is restored while retaining the established
  descriptive canonical counter name and 32-bit long type. VC7 now emits the
  natural local-static decorated symbol at .bss+4, matching January's logical
  counter offset/storage rather than the former file-static .bss+0.

Only add-player references +0x8B and +0xA4 change data-owner identity to the
recovered .bss+4 record. Complete normalized instructions, other relocations
and function ownership remain unchanged. The strict matcher already credited
this function by symbolic counter identity; this is **not new exact code**.

The target census includes all 833 objects. External create/dispose callers
are in NetworkGameGlobals; no extra cross-TU backing-singleton consumer was
found. This is ownership evidence, not a whole-program link or live network test.

## Safe disposal is intentionally fuzzy

The pregame and postgame graceful-exit packet schemas in
[network_messages.c](../../source/networking/network_messages.c) serialize
one long, despite its name unused. The supplied source leaves these fields
uninitialized; January reuses the incoming argument home slot as the payload.

The reconstruction explicitly initializes each payload to zero and retains
the genuine struct network_message broadcast API and enum states/message kinds.
The safe candidate is **304 bytes / 27 relocations**, normalized SHA-256
ee98d32cde746e108cf3edd6b4174f6c73a8fb5851d7edc740da092f042e47ef.

A complete aligned instruction/relocation comparison proves the difference:
two zero stores to [ebp+8] at candidate +0x41 and +0x63, consequent internal
branch displacements, and trailing NOP padding. The comparison removes only
those two pinned stores, maps every jump to its instruction destination, and
compares all remaining instruction bytes and relocation identities. It is
not the gate's index-zipped display or a full network equivalence proof.

Disposal otherwise preserves state-specific notification, client processing,
connection deletion, Xbox sleep/transport termination, full server clear,
in-use assertion/reset and diagnostics. Reproducing the source's stack-data
disclosure for a byte match is not permitted. This residual is measured in
config/parked.json; removing initialization is not an acceptable reopening.

## Header and constant ownership

Create, dispose and the source-authenticated public boolean idle declaration
now reside in network_server_manager.h. NetworkGameGlobals consumes the
associated internal header, which includes that public owner and already
owns get-game; four redundant foreign prototypes were removed from the C file.

Connection-creation bits move from the provider C file to network_connection.h.
Actual protocol consumers share ports/rejection reasons through the narrow
network_game_protocol.h; duplicated constants are removed.

An initial broad-header placement regressed the existing UI function
_solo_level_select_list_update_displayed_items. That intermediate was frozen
and rejected. The final protocol header is included only by client manager,
server manager and server message handler. UI source is untouched and its
runtime sections/strict result are restored. The broad game-manager header
differs from the initial capture only by EOLs.

The former inferred byte-order labels were reversed. Source-attested
_byte_order_host=0 and _byte_order_network=1 now name the actual directions:
inbound reads still pass 0; outbound connection/key/rejection writes still
pass 1. Provider branch identifiers are synchronized. Every inherited runtime
byte remains unchanged. Later HCEA spelling is corroboration, not the sole
January authority. The [constant lookup](../common_constants.md) also fixes
the actual header name MAXIMUM_MESSAGE_SIZE=0xFFF, distinct from manager 0x800
receive capacity.

tools/test_network_protocol_contracts.py checks nine emitted values from real
headers and three independently compiled bad direction/port/rejection controls.
Compilation failure cannot satisfy those controls. This verifies constants
and declaration ownership, not network runtime behavior.

tools/test_network_server_dispose_payloads.py freshly compiles the complete
canonical TU using the gate's configured compiler flags. It proves both
message-creation calls receive their actual four-byte stack payload initialized
to zero. Two separate successful mutant compiles remove only the pregame or
postgame initialization: the affected path is rejected while the other remains
safe. The checker allows only the actual modeled argument-block instructions;
unknown instructions, indirect/partial stores and intervening calls fail the
analysis. Compile/analysis failures cannot satisfy the expected missing-payload
exception. This is a bounded call-site proof, not an emulator or full lifecycle
runtime test.

## Verification

- Full Ninja build passes; adjusted normal-report totals are above.
- Stable sweep: three gains, zero regressions across all 8,245 entries.
- Root frozen audit: **572 rebuilt and 833 split objects** pass.
  Of **186 inherited server runtime sections**, 184 are identical; the other
  two are the precise BSS/add-player corrections described above. Four new
  code owners and fourteen genuine target literal owners (**819 bytes**) are
  accounted for. Literals match full payload, flags, selection, offset, type
  and linkage; no automatic data-match credit is added.
- Twelve other rebuilt objects change compiler metadata only; full runtime,
  defined-symbol and section-zero/COMMON inventories are equal. Target manager
  section headers, payloads and raw relocations remain unchanged; only the two
  authenticated private name/storage records change.
- Final gate: **59 exact / 4 residual / 7 unwritten**, no aliases;
  _point_from_line3d emitted-symbol guard passes.
- Final suite: **994 passed, 2 skipped, 26 subtests passed**.
  Parks: **296 active / 0 stale / 0 invalid**. Admission: zero candidates,
  contradictions and revocations; five inherited rejections.
- Lexical fake-match scan: eleven edited C/header owners, zero review leads.
  Source/control-flow/ownership review remains the substantive credibility check.

## Immutable local evidence

| Artifact | SHA-256 |
| --- | --- |
| scratch/server-create-connected-before-20260906.json | 2b9b4d5cfcb90968e3abef0ceac54b689b0f70ed018e5adb44a42b49a7b41a52 |
| scratch/server-create-connected-final-20260906.json | 1824052931a76710e8fa0aa65e84224e81816683d3b2b3fe06e8c27c3f624452 |
| scratch/server_create_root_whole_audit_20260906.py | c344edccf8634e81836fa3a86cf4c8f569100e4f96a0bb4b8089cb5c1ccf868a |
| scratch/server-create-root-whole-audit-20260906.json | 0cd0139c3013795041d6a854a3e26b4d979842065e3f5e51f48e2d3533e0eb9c |
| Independent all-object audit report: scratch/network-server-create-connected-independent-full-audit-20260906.json | ad5dac843c94e414b0548043e8e49ebda47bed305451a99dfa44a21e353e5052 |
| Independent source/ABI ledger: scratch/network_server_create_connected_independent_audit_20260906.md | ae043e36ad1b7726ec18c94dee15ab8e0fdf11fa5b4e841c4ded2bbc748536bc |
| tools/test_network_protocol_contracts.py | 116521b467db5a888a9e7ceec59f3bc9670378aaffb02851dcda0ff60f4fb53b |
| tools/test_network_server_dispose_payloads.py | 018876b0890711cc02d117b308b0b29e3232f5b69ceefdb2de0759aeafa8b57d |
| Rejected broad-header capture: scratch/server-create-owner-regression-20260906.json | debfdce4fc82bbd7971bbfa13356d10d0661d6ff3a2c0a79eb71e27055afdc92 |
| Corrected owner-only capture: scratch/server-create-owner-corrected-20260906.json | acf5b537b52db1d6f5fb013efa8709b360bfb6b6856e5494a3e4cb474234d369 |

Stable snapshots are scratch/server-create-connected-before-stable-20260906.json
and scratch/server-create-connected-after-stable-20260906.json. Captures include
source/config/compiler/build objects, not documentation or Python tests;
the commit identifies those final artifacts. Scratch/reference binaries remain
local. IDA is unavailable and no IDA use is claimed; existing cross-build
Ghidra intake remains separate evidence.

## Next connected work

Seven targets remain unwritten: public idle, five connected private helpers,
and countdown increment. Continue the real idle chain from supplied source
and January callers. Do not restart the rejected signed-overflow countdown or
source-shape residual searches without new evidence. This packet does not
claim full-object completion.
