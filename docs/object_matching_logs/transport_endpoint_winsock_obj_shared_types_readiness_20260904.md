# Transport endpoint shared type and credible readiness leaves, 2026-09-04

## Result

Starting at canonical `56635021ee5f664a56680f7c805cafd8e76d5e71`, move the
existing complete eight-byte endpoint record into `transport_endpoint.h`
and restore two natural readiness functions that the old August29 exact-only
policy had removed. Current policy calls for credible fuzzy implementations to
be retained and parked. Endpoint Winsock becomes **16 exact / 2 parked /
12 unwritten**; no new exact code or whole-object completion is claimed.

## Shared ownership and ABI

The common endpoint owner contains exactly the existing four fields: long socket,
byte flags, signed char type, short error. The owner C consumes this header;
`transport_endpoint_winsock.h` consumes the same definition, so the set
implementation needs no duplicate or incomplete endpoint view. No packing,
anonymous carrier, opaque prefix, flag control, or field offset changes occur.

The endpoint C now consumes its actual shared endpoint API. Non-XDK-dependent
endpoint declarations are owned by `transport_endpoint.h`, while the existing
Winsock extension owns XNADDR/key interfaces and includes the common API. The
public `winsock_error_to_string` declaration is shared there too; redundant
accept/delete/disconnect/error-string declarations are removed from the C file.
The endpoint C still retains legacy hand-declared Winsock SDK API types; their
larger cleanup is separate from this concrete type/API migration. No SDK macro
override or alternate fake SDK header is introduced.

The five flag bits have inferred semantic spellings, not claimed original
names: connected0, listening1, readable2, in-set3, nonblocking4. January's
read/write/disconnect/accept, listen, poll, add/remove, and blocking functions
prove their meanings. Existing field widths were independently grounded by
January and the prior public-leaf/lifecycle ledgers.

`poll_endpoint_set` and `endpoint_readable` timeout declarations change from
long to word, following each target's `movzx` word argument load. Poll's sole
January network-connection caller pushes immediate zero and tests the returned
short. The new listening prototype returns long and takes a const endpoint,
consistent with the full EAX 0/1 target return. Public declarations stay in the
shared endpoint owner header, re-exported by the Winsock extension; no
consumer-local declarations are added.

## Natural readiness source

- `endpoint_listening` preserves the original `ep` assertion (line0x42E) and
  uses `TEST_FLAG` on the listening bit. The generated64-byte body and all four
  relocations have the expected topology; January masks then shifts, while
  VC7 emits the algebraically equivalent shift then mask. It is not exact.
- `endpoint_readable` asserts `ep` at0x3F3, rejects INVALID_SOCKET, and uses the
  cached readable flag when the endpoint belongs to a set. Otherwise it builds
  the existing typed socket set and timeout, calls select, and checks the
  returned membership. It returns FALSE for invalid sockets, timeout, or error.
  The word timeout is converted to microseconds by the existing named
  MILLISECONDS_PER_SECOND constant, matching January's multiply by1000.
  Candidate176/target192 bytes, six relocations each; the natural candidate
  does not retain January's separate EBX zero lifetime and invalid return.

These are one-shot natural implementations. The earlier exhaustive residual
searches in `transport_endpoint_winsock_obj_jonas_public_leaf_wave_20260829.md`
and `transport_endpoint_winsock_obj_jonas_lifecycle_readiness_wave_20260829.md`
were read, not repeated. No register coupling, qualifier change, bitfield,
assembly, alternate condition lottery, or function inlining is attempted.
Both are parked unclassified with measured target/candidate hashes in
`config/parked.json` rather than assigning an unsupported compiler cause.

## Focused validation

```powershell
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_winsock --all --forbid-emitted-symbol _point_from_line3d --out scratch/endpoint-readiness-natural-20260904.obj
python tools/campaign/alndiff.py source/bungie_net/network/transport_endpoint_winsock _endpoint_listening --ours-object scratch/endpoint-readiness-natural-20260904.obj
python tools/campaign/alndiff.py source/bungie_net/network/transport_endpoint_winsock _endpoint_readable --ours-object scratch/endpoint-readiness-natural-20260904.obj
```

All16 inherited strict code owners remain byte/relocation-identical. There are
no candidate-only code owners, and the hard no-point_from_line3d guard passes.
The type-only migration was separately gated before adding the two functions:
all existing emitted code owners (including residuals) stayed identical in
all seven consumers: Endpoint Winsock, Endpoint Set Winsock, Key Agreement,
Network Connection, Network Client Manager, Network Client Message Handler,
and Network Server Message Handler. Full post-API-change validation follows in
the accompanying canonical batch checkpoint.

The new active Fable worktree and its assigned implementation files are not
inspected or changed by this packet.
