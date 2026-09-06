# Transport initializer: natural source boundary (2026-09-05)

## Result and exact scope

The single natural candidate adds the genuinely unwritten `transport_initialize`
to `source/bungie_net/network/transport_endpoint_set_winsock.c` and includes its
proper existing `cseries_windows.h` and `errors.h` API owners. No shared header,
config, compiler option, alias, pragma, intrinsic, helper, guard or global owner
was changed. Do not replace the complete TU: apply the two minimal edits in
`scratch/transport_init_production_20260905.manifest.json`.

Frozen baseline **16 exact / 6 residual / 4 unwritten** becomes **16 exact /
7 residual / 3 unwritten**. All 22 inherited code owners, 18 inherited data
owners and six existing parks remain unchanged. This is **zero new exact code
credit**, 416 meaningful January bytes reconstructed, and no whole-object claim.

January `_transport_initialize` is at RVA/file offset **0x707A0 (460704)**,
section12, external storage2/function type32, `.text`, selection1, align16.
Target is **416 meaningful / 416 padded bytes, 33 relocations**, normalized SHA
`47bd2187b394eb1f7975b4bc618fc0d041f563687eb841257fcae4296bd71db0`.
Candidate is **398 meaningful / 400 padded bytes, 33 relocations**, normalized
SHA `a125c3a8cdab486c1d6414d8644d27b067a58b2ce26c9d769a3a41ef0562b45e`.
Ordinary noninteractive objdiff report gives **78.47794%**. Strict comparison
correctly classifies it as residual. No semantic/comparator exception requested.

## January API, types and behavior

The complete January body, every relocation and complete caller contexts are
frozen in `scratch/transport_init_triage_20260905.json` and
`scratch/transport_init_owner_audit_20260905.json`. The latter scanned **all833
target units and every relocation-bearing section**, including data sections.
The sole incoming initializer reference is `game_initialize+208`; it passes no
arguments and ignores the return. No function-pointer/data reference was found.

The existing public owner is `transport.h::short transport_initialize(void)`.
The status-family short type was already documented as an inference in
`game_obj_lifecycle_dispatch_wave_20260904.md`, not as a recovered debug typedef.
January sets full EAX to0 or-5; this candidate preserves the existing public
short API and therefore establishes the same AX status, **not full-EAX return
equivalence**. The only actual caller ignores it. No return-width/header change
or unsupported claim of original source type is part of this packet.

The body preserves all observable operations in their original order:

1. Already initialized: return success without making any external call.
2. Zero real stack `WSADATA` (400B) and real packed `XNetStartupParams` (11B).
   Initialize the SDK parameter bytes shown below. No prefix/custom SDK facade.
3. Call `XNetGetEthernetLinkStatus`, then log the five conditional strings with
   `_error_log`. The real named SDK masks select connected/not-connected,
   100Mbps, 10Mbps, full-duplex and half-duplex text.
4. Open the exact23B NUL-terminated `d:\bypass_security.txt` path in mode `r`.
   Actual C text contains two source backslashes, yielding one runtime slash;
   the frozen target and candidate literal bytes and properties are identical.
   If open succeeds, log `_error_silent`, OR the named SDK bypass mask, and close
   that real FILE pointer before startup.
5. `XNetStartup` failure returns-5 with no cleanup. On success, request exactly
   `MAKEWORD(2,0)` from `WSAStartup`, not SDK `WINSOCK_VERSION` (2.2).
6. Store the WSA result in `short`, test the low16 bits and sign-extend that
   short for `winsock_error_to_string`, matching January SI operations. On
   failure, call `XNetCleanup` first, then the error-string function, return-5.
7. Establish unsigned absolute deadline `system_milliseconds()+10000` and poll
   `XNetGetTitleXnAddr(&global_address)`. On every iteration obtain the current
   time before considering status. If current time is strictly greater than
   deadline, or address status is `XNET_GET_XNADDR_NONE` (1), call `WSACleanup`
   and then `XNetCleanup`, returning-5. Pending0 loops; other values succeed.
   Equality to deadline is not timeout. January unsigned-wrap behavior is
   preserved; no rollover repair, new sleep, or actual Ethernet-link poll.
8. Call `XNetRandom(global_nonce, sizeof(global_nonce))` with exactly8 bytes,
   ignore its return as January does, set the existing initialized byte TRUE,
   and return success. Do not substitute a guessed socket-option API.

| Parameter byte | Real SDK field | January value |
| --- | --- | --- |
| 0 | cfgSizeOfStruct | sizeof(XNetStartupParams)=11 |
| 1 | cfgFlags | 0, or named bypass mask1 after file success |
| 2 | cfgPrivatePoolSizeInPages | 24 |
| 3 | cfgEnetReceiveQueueLength | 8 |
| 4 | cfgIpFragMaxSimultaneous | 4 |
| 5 | cfgIpFragMaxPacketDiv256 | 8 |
| 6 | cfgSockMaxSockets | 128 |
| 7 | cfgSockDefaultRecvBufsizeInK | 0 |
| 8 | cfgSockDefaultSendBufsizeInK | 0 |
| 9 | cfgKeyRegMax | 1 |
| 10 | cfgSecRegMax | 32 |

`xbox/include/WinSockX.h:1584-1701` defines the actual record;
1722 onward owns the real XNet API, status and Ethernet constants. It is
included through the existing `transport_endpoint_winsock.h` stock `<xtl.h>`
owner, not a direct-header workaround. `cseries_windows.h` owns the unsigned
timer API; `errors.h` owns the variadic log API and priorities. `cseries.h`
already supplies standard C library ownership. Existing `transport_endpoint.h`
owns `winsock_error_to_string`. No caller-local copied API prototype was added.

The SDK constants are **masks**, whereas `cseries.h` TEST_FLAG/SET_FLAG consume
**bit indices**. Natural operations on the stock named masks are retained;
inventing bit-index aliases just to invoke those macros would be misleading.
All new variable names are semantic; zero-initialized declarations and explicit
returns follow the house conventions. No inline assembly/inlining was needed.

Existing `extern XNADDR global_address` and `extern byte global_nonce[8]` remain
references to their real linked storage; this packet does not define or resize
them. Complete incoming census finds address users only in this initializer
and `transport_get_xnaddr`, and nonce users only here and the existing nonce
get/test APIs. The existing initialized byte and server lifecycle state are
untouched. Do not infer new physical data credit for referenced globals.

## Provenance, residual and stop boundary

Relevant existing object ledgers were read before implementation, especially
`transport_endpoint_set_winsock_obj_lifecycle_packet_20260904.md`,
`transport_endpoint_set_winsock_obj_client_stop_bss_lifecycle_leaves_20260904.md`,
`actor_stimulus_transport_selective_batch_20260904.md`, and the earlier
`transport_endpoint_set_winsock_obj_jonas_two_leaf_rejection_20260826.md`.
Initialization itself had not received a natural source-only attempt under the
current real SDK boundary. This is not a retry of exhausted bind/connect,
set-blocking, dispose-header or client-start work.

Frozen older semantic donor is repository
`../../research-cache/stian-halo-full-history-20260828`, commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`, file
`src/halo/bungie_net/network/transport_endpoint_set_winsock.c`, SHA
`5b29f184398c7228fdd95bcc073c30abd27b6e5dfd87dd95aa3a241e905ecbac`.
Only the broad sequence corroborates January. Its void signature, wrong raw
API/cleanup names, mislabeled parameter bytes and WSA2.2 commentary were not
copied. January and the real SDK declarations govern this reconstruction.
The active Fable tree was not inspected or used.

The natural output differs in zero-register allocation, removal of redundant
parameter byte stores, branch ordering and return-tail layout. January writes
size11/flags0 again after the link log; no pointer to the parameter record has
escaped at that point. The candidate uses natural complete initialization and
does not add those repeated inert stores solely for byte shape. Both records
passed to the SDK have the same values. Its inherited short public return
produces AX instructions instead of January EAX instructions, as disclosed.
The error cleanup tail is laid out after success, not before it. The aligned
instruction diff shows the real additions/deletions; a zipped gate view is not
used to claim alignment or closure.

Park as `unclassified` after this one natural body. No source spelling search,
barrier, volatile, forced-inline, optional layout assertion, compiler switch,
padding, helper anchor or alias was tried. Reopen only with genuine original
type/source/caller evidence. Do not enlarge into the withheld dispose/header
packet, active Fable client work or the unproven empty16B helper.

## Full runtime and data owner audit

The audit records symbol offset/storage/type and section logical size, flags,
alignment, COMDAT selection, raw presence, normalized bytes and ordered
relocations for **every real code/data owner**, plus local code label owner
properties. Frozen original base and untouched PID baseline agree for all
runtime owners. Candidate preserves every inherited owner and local label.

- Target26code/23data; baseline22code/18data; candidate23code/29data.
- Sole new code owner: `_transport_initialize`; no candidate-only code owner.
- Eleven new literals total204B. Nine are original target-owned literals,
  **201B**, strictly equal in bytes/relocations and owner properties. All are
  offset0, storage2/type0, `.rdata`, align4, selection2, flags0x40301040.
- Two new pooled copies total3B: `r` (2B) matches target owner
  `source/bitmaps/tiff_file`, and empty string (1B) matches target owner
  `source/ai/action_obey`. Both match payload and full properties, including
  selection2; empty is align1/flags0x40101040. No duplicate/global data credit.
  These target objects were read for owner metadata only, not source work.
- All four inherited candidate-only data owners remain unchanged: one assert
  string and three SDK D3D constants. The two pooled literals bring this count
  to six; neither is an unsupported new SDK/helper owner.
- The existing tentative1B `_server_transport_globals` COMMON remains exactly
  unchanged. There is **no new COMMON**, not an assertion that none exists.
- No new global extent, private helper, callback, function pointer, COMDAT
  math helper or `point_from_line3d`. Public/private linkage is unchanged.
- The source credibility scan found zero issues in one file. This audit is
  function-level admission evidence, not a whole-object/link completion claim.

## Reproduction and frozen artifacts

Exactly one untouched PID baseline and one natural code-producing candidate
gate were run. The manifest contains exact commands for both, the full census,
aligned diff, ordinary isolated report and source credibility scan. The report
uses `scratch/transport-init-isolated-report-20260905/objdiff.json`, two frozen
object paths and no custom build/config flags. It does not open an interactive
diff or run a compiler.

All paths below are relative to canonical
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

| Artifact | SHA-256 |
| --- | --- |
| scratch/transport-init-target-20260905.obj | c97fa1ec9b2bc577555a5e384e266c47953a57c6f1c055ce7c492e3de576034c |
| scratch/transport-init-base-20260905.obj | 5611785b7326118c5f16efa6c8d0e878ba00813263c3a3cf83e01462917fc1da |
| scratch/transport-init-baseline-20260905.obj | 0db3a2b07db3b747c7cd7693aec684a8152a073418d3be791845c4b406bd68c3 |
| scratch/transport-init-first-natural-20260905.obj | 6d9fa427d523f97cfcf677adf141b3f9bf597f65f94970d04307f4ed4084c658 |
| scratch/transport_init_first_natural_20260905.c | 5d0dabd93f5eb4d9a45f5dc2ec5e5361772264b0d04d7ff8eec98634db49e25c |
| scratch/transport_init_first_natural_20260905.edits.json | b69ff354978b9e2760d192bfedee7f6e6071bea3332b209b8726a58e1d279119 |
| scratch/transport_init_triage_20260905.json | e962a42aa1bebd9a75ff10ad94b872e763aee49850e83ce4410a085c89b610b2 |
| scratch/transport_init_owner_audit_20260905.json | 9988a6306269830980791febc92e3bb134b9f7d03349e344b7fbc8d5bf19e142 |

Independent review is separate and must be included by root before admission.
Root alone applies the minimal production manifest, installs the park using
`docs/object_matching_logs/transport_endpoint_set_winsock_obj_initialize_natural_boundary_20260905.md`
as its durable evidence path, runs full build and no-regression checks, and
decides publication. Worker performed no production edits, shared-header edits,
Ninja/configure, commit or push.

## Canonical integration verification

Root applied the two minimal source edits and the measured 78.47794% park.
The actual full-build object SHA-256 is
`c6838eaa84ee3ed97d469691a0d3281bf1ddbefe19b34305f041b0add08a5ae5`.
Its initializer retains the reviewed 400-byte, 33-relocation candidate
fingerprint. No return-type, body, flag, or string correction was made.

Root's complete actual-before/after runtime audit finds precisely 12 new
records (the function and eleven literals), zero lost or changed inherited
records, and unchanged one-byte server COMMON. It does not need even a
compiler-local-label normalization or undefined-to-defined caller exception.
The JSON is `scratch/transport_init_canonical_runtime_audit_20260905.json`,
SHA-256
`23d7dd16ce26d6fe50e18a765df860ead788873e5d7c0d0de6df8b49999b33fb`.

The unchanged independent behavior harness passes all seven cases against
January and the actual canonical object. The result is
`scratch/transport_init_canonical_behavior_20260905.json`, SHA-256
`634fe9efb96744ad832a786a5710efbb66e87a368c9dc10906a5da83d63375dd`.
These are bounded function checks, not a full game/network execution test.

The final independent review added a negative-low-word WSA failure case.
Root reran that eight-case harness against January and the same actual
canonical object; all sixteen runs pass, including sign extension into the
error-string API. The final result is
`scratch/transport_init_canonical_behavior_final_20260905.json`, SHA-256
`8532b7c55338eafff8474f4cba25ba3a63ef2c29934fd511c62f2f64f29baab1`.
The separate independent review ledger approves honest fuzzy retention with
the disclosed short-result ABI and no duplicate-data/whole-object waiver.

Full Ninja passes. The stable snapshot
`scratch/transport-init-canonical-first-20260905.json` preserves all
6,217/8,245 strict owners, with zero gains and zero regressions versus the
published Main/Texture baseline. Units remains 189/189 exact, both protected
Bitmap hashes are unchanged, and the source credibility scan has zero leads.
All 281 parks validate; all 288 tests plus 26 subtests pass. Fresh admission
reports zero candidates/contradictions/revocations and five recorded rejections.

Production accounting is 906,391 meaningful matched code bytes, 6,171 credited
functions, 2,031,312 matched data bytes, and 391/833 Matching objects.
This packet adds 416 meaningful fuzzy source bytes, zero exact code bytes,
and 444 reported data bytes. The 444-byte delta is the report's newly completed
aggregate .rdata contribution, not 444 newly emitted literal bytes: nine
new target-owned literals total 201 bytes and the other two pooled copies
total three bytes. The existing eight-byte .bss was already credited.
Inherited/pooled surplus data and three unwritten functions still prevent
any whole-object completion claim.
