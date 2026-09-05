# Endpoint-set operations cluster (2026-09-04)

## Disposition

Starting from published canonical `416c57f21e19d46c740b7e198bc4c43260bc014e`,
the first source-faithful reconstruction fills the five-owner endpoint-set
operation cluster. It preserves all 12 inherited strict functions and the
existing nonce residual. The normal source-order scratch gate is **13 exact / 5
residual / 8 unwritten**, compared with production's **12 exact / 1 residual /
13 unwritten**.

One newly reconstructed private owner is strict exact:
`transport_endpoint_set_get_next_index` -> January `_code_00070430`, **56
meaningful / 64 padded bytes**, four relocations, normalized SHA-256
`66796631e48f5e44c8f43a0b9c2e11fe91cca128cde82e4068a8bcaa5433b810`.
It is an ordinary static function called by `add_endpoint_to_set`; C2 naturally
inlines the call while retaining the standalone target owner. There is no
`inline` keyword, optimizer control, retention reference, or invented caller.

The other four credible bodies remain honest residuals and should be parked,
not credited as exact:

| Source owner | Target meaningful / padded | Relocs | Target SHA-256 | Candidate padded / SHA-256 |
| --- | ---: | ---: | --- | --- |
| `transport_endpoint_set_compare_entries` -> `_code_00070b10` | 41 / 48 | 0 | `10d0cbdd04c0ad15a3cd14bffdde707c085ce85c4b9fed4c036fb4c103ca9c55` | 48 / `6621f23910b74ac690b7593956ec7d8b7feb44be8401480ceaff51642dcd6c1f` |
| `poll_endpoint_set` | 551 / 560 | 16 | `4a0375522c8a3ff103c5dad92342325f3a41658e90dead5cdcd60d92a8f79f4b` | 512 / `c605635cb9ce7070a69e80e0b8943c998279fb64fab139b41bdeb8cecb2572a2` |
| `add_endpoint_to_set` | 336 / 336 | 13 | `4e3ad84da528b844823d85558c2b994dd8680fbc92b1a456f1f1147a44d91807` | 240 / `d7e631f5948455759368b6908fb1ddf9f24fe91fc234521cb6f62a1686a2e091` |
| `remove_endpoint_from_set` | 238 / 240 | 9 | `6b4b631ac61f5eee45694fbee875f88fceafc045056afac5289baa7de53f5d34` | 240 / `be7a5e04decfb01718f78a0b2ff47e856c0c7cd8a90091637ec5b44db1e6acb2` |

This is a function-progress proposal, not whole-object admission. Root still
owns production replay, private target-name metadata, parked records, full
build/regression testing, commit, and publication.

## Authentic types and API ownership

The packet compiles against the new narrow owning
`source/bungie_net/network/transport_endpoint.h`, not a caller-local view. That
header contains the one complete eight-byte `transport_endpoint` definition
(`socket`, flag byte, type byte, error short), the January-observed flag enum,
and the non-XDK endpoint API. `transport_endpoint_winsock.h` includes that
owner and retains the XDK-only API. The endpoint implementation no longer
duplicates the layout or the four common declarations. Root gated all seven
direct consumers and reported every inherited emitted owner byte- and
relocation-identical before this packet.

January loads the poll timeout with `movzx` from a word argument; the owning
prototype therefore uses `word`. The implementation multiplies it by the real
`MILLISECONDS_PER_SECOND` constant into `timeval.tv_usec`. The three operation
return declarations remain the established `short` API. Full-EAX materializing
instructions in some January return paths do not independently prove a C
return type. The January PDB module is stripped to OBJNAME record `0x0009` and
COMPILE record `0x1013`, so it contains no procedure-type record that could
settle the spelling. Pastudan independently types poll/add as `int` but remove
as `short`; that inconsistent reverse-engineered donor is corroboration only.
No ABI was changed to chase instruction width.

The two private semantic names are inferred because January exposes only the
stripped `_code_` labels:

- address `0x00070430`: `transport_endpoint_set_get_next_index`, static;
- address `0x00070B10`: `transport_endpoint_set_compare_entries`, static.

The first name follows its exact assertion/body and sole conceptual caller.
The second describes the qsort callback, which orders non-NULL typed endpoint
entries before NULL entries. The callback's 48-byte residual is only a register
allocation difference: January holds entry A in EAX and B in ECX, while C2's
natural candidate swaps the live registers. Both load both typed entries before
branching and return the same `-1/0/1` relation.

## Recovered behavior and explicit defect boundaries

`poll_endpoint_set` optionally sorts holes to the end, contracts the inclusive
last index, rebuilds the stock XDK `fd_set`, clears every endpoint's readable
flag, copies that set for `select`, and marks endpoints returned by
`FD_ISSET`. Invalid sockets return `_transport_error_bad_endpoint`; a negative
select result is logged through the real `winsock_error_to_string` owner and
returns `_transport_error_poll_error`; zero returns
`_transport_result_poll_timeout`. January redundantly compares the already
nonnegative path with `SOCKET_ERROR` after its negative branch. The candidate
uses the natural and semantically faithful `select_result < 0` condition rather
than encoding an unreachable duplicate test.

`add_endpoint_to_set` gets the next array index through the real helper, stores
the typed endpoint, inserts its socket with the stock XDK `FD_SET`, advances the
inclusive last index, and sets `_transport_endpoint_in_set_bit` through
`SET_FLAG`. January tests `_transport_endpoint_listening_bit` and emits two
separate but instruction-identical `FD_SET(ep->socket, &set->sockets)` blocks.
Each branch reads/writes exactly the same count and socket array and has no
branch-local call or other side effect. The candidate deliberately uses one
natural `FD_SET`; manufacturing the semantically inert duplicated conditional
would violate the source-authenticity rule. This accounts for the 240/336 size
gap.

`remove_endpoint_from_set` finds the exact typed pointer, uses the stock
`FD_CLR`, clears `_transport_endpoint_in_set_bit`, leaves a NULL hole and marks
the set for compaction. A missing pointer returns
`_transport_error_endpoint_not_in_set`. Its size and all nine relocation
identities match; remaining differences are prologue/register scheduling,
full-EAX versus AX materialization on direct error returns, and padding. No
declaration/order search is justified without better source or type evidence.

Two unsafe edge behaviors are target-established and are not concealed as
safe invariants:

1. The exact next-index helper rejects only when
   `last_endpoint_index > max_endpoints - 1`; when they are equal it returns
   `max_endpoints`, although `create_endpoint_set` allocated exactly
   `max_endpoints * sizeof(pointer)` and no sentinel. This is a real off-by-one
   boundary in January, not a match trick or a safety fix.
2. Poll compaction decrements `last_endpoint_index` while the tail entry is
   NULL without testing a lower bound. A set containing no non-NULL entry can
   read before `ep_array`. This is also the literal January control flow.

The observed server caller constructs capacity
`MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1`, adds a listening endpoint first, checks
the endpoint count before accepting clients, removes client endpoints, and
deletes the set rather than polling it after server teardown. That call graph
explains why January normally avoids both edges, but it is not promoted to a
general API contract or proof that the functions are intrinsically safe.

## House rules and complete owner census

The proposal uses named fields, the owning endpoint type, XDK
`FD_ZERO`/`FD_SET`/`FD_CLR`/`FD_ISSET`, and cseries `TEST_FLAG`/`SET_FLAG`
conventions. It has no raw offsets, partial struct facade, type pun, manual flag
mask, inline assembly, forced inline, optimizer directive, fake branch, or
caller-local prototype. Every parameter is on its own line. The January-backed
assert spelling is `ep && set`, and the real source-line values are retained.

The normal-layout candidate has 18 code owners against January's 26: all 18
are target owners, with no candidate-only code definition. The two new private
owners have COFF storage class 3; the three public operations have storage
class 2. The eight remaining target-only functions are `_code_00070420`,
`transport_client_start`, `transport_client_stop`, `transport_dispose`,
`transport_initialize`, `transport_network_available`,
`transport_server_initialize`, and `transport_server_terminate`.

There is no COMMON, `.data`, or `.bss` definition. Relative to the production
baseline, the sole new non-code owner is the authentic January pooled literal
`ep && set` (10 bytes). The three stock XDK D3D encode tables and the pooled
`transport_initialized` assertion literal are inherited candidate-only
ownership debts; this packet neither introduces nor waives them. The full
emitted-symbol gate reports no `_point_from_line3d` definition.

## Reproduction and frozen artifacts

The normal source-order gate uses two scratch include fragments only to apply
the bodies at their proposed production positions without touching the shared
source. The accompanying apply-patch manifest contains the formatted actual C
proposal; production must use those real bodies, not retain scratch includes.

```text
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_set_winsock --edits scratch/transport-endpoint-set-normal-layout-edits-20260904.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-endpoint-set-normal-layout-final-20260904.obj

python ..\..\research\pdb200\inspect_module.py tools/pdb200_extract.py "C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb" transport_endpoint_set_winsock
```

- formatted production proposal:
  `scratch/transport-endpoint-set-operations-natural-20260904.patch`, SHA-256
  `b087c078bd925f3cd2f615716b9e4bc53891f2af0b63d69e7d48f5a637775133`;
- normal-layout edit manifest:
  `scratch/transport-endpoint-set-normal-layout-edits-20260904.json`, SHA-256
  `e5dac48e5c87c900405011e084240e8520b50c4022871169ffb3c3bec0edc564`;
- helper fragment:
  `scratch/transport-endpoint-set-helper-natural-20260904.inc`, SHA-256
  `86daa27f81aac545cdb0281d89669f36004dafd0d78243460188fe5cfdf17a1b`;
- operation fragment:
  `scratch/transport-endpoint-set-public-operations-natural-20260904.inc`,
  SHA-256
  `11b6dbd3e89e9906783a5a29281fa2abb28cf6479f58c5bb87cb43416bb26b58`;
- normal-layout artifact:
  `scratch/transport-endpoint-set-normal-layout-final-20260904.obj`, SHA-256
  `64b9f09368fdb922b91223e653c0df87cd227040adf50d14a7d39cac4aac35d2`.

The wrapper experiment and the normal-layout gate produce strictly identical
normalized code/relocation packets for all five reconstructed owners. No
source, header, config, build file, commit, Git ref, or Fable-lane file was
changed by this reconstruction worker.
