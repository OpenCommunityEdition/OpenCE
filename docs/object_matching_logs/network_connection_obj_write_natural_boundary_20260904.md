# `network_connection.obj` write natural reconstruction boundary (2026-09-04)

## Scope and authority

The initial scratch packet reconstructs the final unwritten public owner,
`network_connection_write`, without changing canonical source, headers,
configuration, generated build products, or the active Fable
`network_server_manager` lane.  The January split object is the byte and ABI
authority.  Current source, the prior NetworkConnection ledgers, and the
frozen Pastudan binary reconstruction corroborate names and reachable
behavior; that reconstruction is not treated as original Bungie source.

The baseline contains 11 strict functions, 11 acknowledged residuals and this
one unwritten function.  None of the inherited residuals was reopened or
retuned.  The full source reconstruction lives in
`scratch/network_connection_write_natural_20260904.inc`; production ownership
and API work is described by
`scratch/network_connection_write_production_manifest_20260904.json`.

## Authenticated public ABI and owner APIs

January proves the public five-argument ABI:

```c
boolean network_connection_write(
	struct network_connection *connection,
	void *message,
	word buffer_size,
	struct transport_address *dest_address,
	boolean reliable);
```

The target loads `buffer_size` with `mov di,word ptr [ebp+0x10]` and reads
`reliable` with byte loads from `[ebp+0x18]`.  The packet pointer remains the
generic public `void *`: the body binds exactly one
`message_header *header = message` at the documented header-first packet
boundary, then uses `GET_MESSAGE_SIZE` and `byte_swap_message_header`.  There
is no raw offset access, union/pointer representation pun, or public
message-type invention.

The target pushes byte-order value one.  In the owning
`message_header_byte_order` enum that is `_message_header_byte_order_host`.
The name describes the incoming/source order supplied to the swap routine;
the outgoing header starts in host order and is converted before transmission.
The first scratch draft incorrectly used `_network` (zero).  The first aligned
comparison exposed that direct semantic error, and the final frozen source
contains the one evidence-backed enum correction.  No control-flow or
compiler-shape alternatives were tried.

`write_to_endpoint` is not reconstructed in canonical
`transport_endpoint_winsock.c`; its source inventory label is not a body.
Nevertheless its complete January implementation authenticates the missing
owner declaration in `bungie_net/network/transport_endpoint.h`:

```c
long write_to_endpoint(
	struct transport_endpoint *endpoint,
	void const *buffer,
	long length,
	struct transport_address const *destination_address);
```

The target reads the four cdecl arguments at `[ebp+8]` through `[ebp+0x14]`,
uses a signed `length > 0` precondition, reads the destination into a local
socket address without mutating it, and returns full-EAX transport results.
The `const` qualifier is source-level ownership inference from that behavior
and must remain consistent with the future real definition; it is ABI-neutral.
Adding the declaration does not claim that implementation as reconstructed.

The existing strict 16-byte `network_game_client_write` wrapper is a plain
five-argument tail jump to this owner (9 meaningful bytes, one relocation,
normalized hash `6fde8ac9...`).  Its source and owning header currently call
the last argument `flags` and type it as `long`; the source has no flag-mask
behavior and every reviewed non-reserved caller supplies zero or one.  The
production proposal changes that wrapper parameter to semantic
`boolean reliable` while retaining its exact tail.  The active Fable server
manager was not inspected or modified.

## Natural behavior retained

The body asserts the packet, nonzero size and connection, verifies the encoded
header size, and swaps the header from host order before dispatching three
target-proven modes:

- A server connection requires an unreliable send and explicit destination,
  diagnoses sizes above the 400-byte datagram maximum, writes through
  `write_to_endpoint`, records one datagram-sent event and returns true.
- A reliable client connection requires at most 2,048 bytes and either the
  client-side or server-side client role.  It retries only the exact
  `_transport_result_operation_would_block` result.  Any positive result is
  considered success, optionally writes the five-column traffic-log row, and
  increments `stream_messages_sent`; nonpositive results are diagnosed through
  `transport_error_to_string`.
- A normal unreliable connection returns true when no endpoint exists.  It
  requires at most 400 bytes, writes to a connected peer when no explicit
  destination is present, otherwise performs an addressed write, and records
  the datagram event for actual send attempts.  Its transport result is not
  used to make the public return false.

January's reliable path intentionally treats any positive stream write as
success, even when it is smaller than `buffer_size`; it does not loop to send
the remainder.  The source preserves this observable original behavior and
does not describe it as guaranteed all-bytes delivery.  Similarly, the server
and ordinary unreliable paths preserve January's true return independent of
the addressed-write result.

The only direct bitwise spelling is inside the role `match_assert`, where the
January assertion literal itself authenticates
`connection->flags&FLAG(...)` for both alternatives.  Runtime flag decisions
use `TEST_FLAG`.  The function uses project types and enums, one parameter per
line, declaration-time initialization where natural, and explicit returns.
It contains no forced inline/noinline, volatile/register steering, pragma,
assembly, raw address, fake dependency, unreachable duplicate, helper anchor,
or nonsensical matching-only logic.

## Frozen comparison

Baseline gate:

```text
python tools/campaign/gate.py source/networking/network_connection \
  --all --forbid-emitted-symbol _point_from_line3d \
  --out scratch/network-connection-write-baseline-20260904.obj
```

Final corrected gate:

```text
python tools/campaign/gate.py source/networking/network_connection \
  --edits scratch/network_connection_write_natural_20260904.edits.json \
  --all --forbid-emitted-symbol _point_from_line3d \
  --out scratch/network-connection-write-natural-corrected-20260904.obj
```

| State | Baseline | Corrected candidate |
|---|---:|---:|
| strict exact | 11 | 11 |
| credible residual | 11 | 12 |
| unwritten | 1 | 0 |

| Owner | Target | Candidate | Result |
|---|---|---|---|
| `_network_connection_write` | 802 meaningful / 816 padded, 60 relocs, `3ab29726...` | 772 meaningful / 784 padded, 59 relocs, `9a5c37ee...` | natural fuzzy, objdiff 74.26% |

The fail-closed classifier reports `UNKNOWN/unclassified`; it grants no exact
credit.  All 22 inherited function sections are byte- and
relocation-identical between baseline and candidate, so all 11 inherited
strict owners and all 11 existing residual measurements are preserved.

Aligned comparison shows broad agreement through the complete three-mode call
and assertion topology.  The retained natural differences are:

- January returns immediately from the server path after a reliable-byte test,
  while the candidate shares the final boolean return;
- January lays out the reliable hard-error arm before the positive-success
  arm, while natural C emits the equivalent success-first branch;
- January tests AL from `endpoint_connected`, while the current genuine owner
  declaration returns `long` and natural C tests EAX; no narrowing cast or
  unsupported public ABI change is used;
- January keeps separate notify-and-return tails for the connected and
  addressed unreliable paths, while VC7 coalesces the candidate tails.

That final merge explains the complete relocation-count difference: the only
missing relocation identity is one call to
`network_connection_notify_traffic_event`.  The candidate has no relocation
identity absent from January.  These are honest source/compiler-shape
boundaries; no branch inversion, duplicate call, cast, declaration-order, or
register-lifetime search was performed.

## Complete owner census

Target/baseline/candidate code-owner counts are 23/22/23.  The candidate adds
exactly `_network_connection_write`; there is no candidate-only or missing
target code owner.  The hard `_point_from_line3d` emitted-symbol guard passes.

Target/baseline/candidate non-code-owner counts are 76/71/79.  The eight
candidate additions over baseline are all January-owned literals: `message`,
`!reliable`, `dest_address`, the buffer-size diagnostic and assertion, the
shared maximum-message diagnostic, the write-endpoint error, and the bad
message/buffer-size diagnostic.  No new candidate-only non-code owner is
introduced.  Candidate-only non-code symbols relative to January are inherited
unchanged from the baseline header/object context and receive no new credit.

All three objects have zero COMMON owners.  The only new undefined references
are genuine January callees `_write_endpoint` and `_write_to_endpoint`; every
other referenced owner already existed in the baseline. The candidate object
contains no extra helper, fake callback, standalone anchor, new BSS or
operational-state owner; its added literals are accounted for above.

## Integration and admission boundary

The production manifest is
`scratch/network_connection_write_production_manifest_20260904.json` and
references the body, connection-owner header patch, transport-owner header
patch and strict client-wrapper API patch.  The two overlay headers are
diagnostic only and must never become production caller-local declarations.
The wrapper edits should be applied structurally because the real files use
CRLF; wholesale newline normalization is explicitly out of scope.

Root must apply the real owner declarations and body, gate
`network_connection`, re-gate the exact wrapper and every direct/transitive
consumer of both changed headers, then run full Ninja, a stable-owner sweep,
the Units sentinel, park validation, tool tests and a fresh whole-owner census.
The proposed park is
`scratch/network_connection_write_park_proposal_20260904.json`.

This packet is ready for that canonical validation as a credible fuzzy source
reconstruction.  It does not authorize whole-object `Matching`: after
integration the object has no unwritten functions but still has 12 explicit
residuals, including this one.

## Canonical integration recheck

Root applied the body and the three genuine owner-header/wrapper corrections,
then completed the full Ninja build before this independent final census.  The
saved isolated production-context object is
`scratch/network-connection-write-canonical-first-20260904.obj`; the current
full-build object is `build/base/source/networking/network_connection.obj`.
Their raw object hashes differ because they are separate compiler invocations,
but all 23 emitted code sections compare identically.  Their code-owner,
non-code-owner, COMMON and undefined-symbol name sets are also identical.

The final baseline-to-production result remains exactly 11 strict / 12
residual / 0 unwritten.  Every one of the 22 inherited code sections is
identical to the pre-packet baseline, including all inherited residual hashes;
the proposed write park still measures 784/59/`9a5c37ee...`.  The integrated
`network_game_client_write` wrapper remains strict at 16/1 with normalized
hash `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.

A scan of every January split object found 23 code relocations to
`network_connection_write`, all in `network_client_manager.obj` or
`network_server_message_handler.obj`.  It found exactly two relocations to
`write_to_endpoint`, both from the target `network_connection_write`, and 14
relocations to the ordinary `write_endpoint` owner family.  The scan completed
without a COFF parse error.  The target `write_to_endpoint` body is 384 padded
bytes / 21 relocations / normalized hash `d14d251c...`; current canonical
`transport_endpoint_winsock.obj` still emits no such body, confirming that the
new header declaration is an API restoration only.

For completeness, the 12 candidate-only non-code names relative to January
were already present before this packet and are not hidden or credited here:

- inherited literals `"w"`, `"buffer"`, `"!TEST_FLAG(header, 5)"`, the
  well-known-port assertion, the local-connection assertion, two unusually
  large message/datagram diagnostics, and the server-role flag assertion;
- inherited XDK header tables `_D3DPRIMITIVETOVERTEXCOUNT`,
  `_D3DSIMPLERENDERSTATEENCODE` and `_D3DTEXTUREDIRECTENCODE`;
- inherited select-any constant owner `__real@41f0000000000000`.

The machine-readable rechecks are:

- `scratch/network_connection_write_canonical_owner_census_20260904.json`;
- `scratch/network_connection_write_reference_census_20260904.json`;
- `scratch/network_connection_write_owner_census_20260904.py`;
- `scratch/network_connection_write_reference_census_20260904.py`.

This independent recheck approves the function source and packet ownership as
a credible fuzzy admission.  It still does not approve the whole object as
strict `Matching` or credit the absent transport callee body.

## Frozen hashes

- January target object:
  `ABCF8D1833AF14FA3BBE9F73B65942C2ED321C74FB48290810ED55B9CA5C5836`;
- baseline object:
  `5713FE944A543020D4FF31722AB2DAB9BF78CB5100A6B794F33A171651F38684`;
- corrected candidate object:
  `6BB335AFA80480EB04E4CEFF4E6F8228DFBBB5285403BEE8CA86F9C23C94E7F2`;
- saved production-context object:
  `305E1AC44702103FD7BE7A56E767E078C2516FC16E11469848BA6FEA45B0B209`;
- full-build production object:
  `CE0BF234010CCEA24C55AC9167D431932FD60B5D45F3042F1E23F2F60BB8C441`;
- canonical owner-census JSON:
  `919CE080668B1C49405807BA1629D70F4438DC63EF25690FB1E494AE76F1FA1F`;
- reference-census JSON:
  `991DAB1AD19D684E01A1E8BF5428268163BC5D340CBFD8BB3BE6E5ECE052E39E`;
- frozen body include:
  `CFF0D1AE1333C7FE94B25EBDD58494929991EFEA1C60B40B503BF978400C18FF`;
- source-edit manifest:
  `99DD307898F70D4A89DA2C4DEF5789145544E3A1234ECFFA721C5A523882A496`;
- fail-closed classifier:
  `968A2EE7F342B25B79FBAF055E8E1BFBB23492126B3BD3D3274030C86F59EDB`.

The baseline canonical `network_connection.c` SHA-256 was
`89176FD2C92EA8AAF2D2F661D7ED9AB0A6E40162C23E7C908BB608C5A66B7F96`.

## Canonical integration and full verification

The production packet is integrated on published baseline
`93a4e0fefa1443d6ecf2b4d6c3fa50ce598b10f2`. The body is placed in the
public-code section immediately after network_server_allow_client_connections.
All declarations are in the genuine owner headers, with `long length` used
for write_to_endpoint. The client wrapper and its header use boolean reliable.
No diagnostic overlay, caller-local prototype, or generated-source substitute
is part of production. An initial patch accidentally targeted the private
prototype instead of its later definition; that placement error was corrected
before the successful production compile, with no body-shape change.

Before changing the three headers, direct source/header includers were
inventoried and all workers' target/base access was frozen. Full Ninja rebuilt
the actual dependency graph and passed. Stable snapshots
`scratch/profile-frame-end-narrow-owner-20260904.json` and
`scratch/network-write-canonical-first-20260904.json` prove all **6,213 strict
owners retained, zero gains and zero regressions**. Canonical reserved Fable
consumers were only rebuilt for verification; their implementations and the
active Fable tree were not edited.

The production gate runs without overlays or aliases and reproduces
**11E/12R/0U**, with the no-point guard passing. The client wrapper remains
16 padded / 9 meaningful bytes, one relocation, normalized SHA-256
`6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.
Network write's target/candidate normalized hashes are respectively
`3ab29726ced2524439c488e5d3b8442130baeec0ce4a86deae89ab50559822f6`
and `9a5c37ee266f90e94139446b81189e71eed5b386616ab2d00fc7637eee6770c6`.
The actual report's full-precision fuzzy measurement is **74.261536%**;
the displayed 74.26% is rounded. Only the precise value is used in the new
park record. No inherited park fingerprint was refreshed or waived.

Final isolated canonical object:
`scratch/network-connection-write-canonical-first-20260904.obj`, SHA-256
`305e1ac44702103fd7be7a56e767e078c2516fc16e11469848ba6fea45b0b209`.
The source before Git newline normalization has SHA-256
`82c10b0a4c02810c3c5fe143fe9d3fd08378927f9222000e30752010dfeaf259`.

All **288 tests plus 26 subtests** pass. **271 parks** validate with zero
stale/invalid records; admission remains **0 candidates / 0 contradictions /
5 rejections / 0 revocations**. The optional pytest-cache permission warning
does not affect test results. The source-credibility scan reports zero review
leads in the five changed source/header files. `git diff --check` and the
protected Bitmap hashes pass unchanged. Evidence is in
`scratch/network-write-final-parks-20260904.json`,
`scratch/network-write-final-admission-20260904.json`, and
`scratch/pytest-network-write-final-20260904`.

Canonical credit remains **904,719 meaningful exact code bytes**, **6,167
credited functions**, **2,028,748 data bytes**, and **391/833 Matching
objects**. This closes one unwritten-source gap without claiming an exact
function or complete object. Existing twelve residuals and inherited data/
extra-owner debt remain explicit follow-up work.
