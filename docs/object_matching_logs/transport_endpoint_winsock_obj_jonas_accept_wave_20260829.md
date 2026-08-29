# `transport_endpoint_winsock.obj`: accept-endpoint wave (2026-08-29)

## Result

Starting from canonical commit `87f3ddd8bf9b6f4952f60932a98b60470c4b50f9`,
this source-only wave adds one strict function. The object advances from 12/30
to 13/30 strict functions with zero residuals and 17 unwritten functions.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_accept_endpoint` | 198 | 208 | 13 | `7570680ecb141cce986dfdb5c99705e4df2f491ae87b23777d365c0a394bb836` |

The hardened comparator reports equality for every padded byte and every
relocation address, type, target, and addend. Combined with the preceding
transport waves, the object now owns 1,319 meaningful and 1,440 padded strict
code bytes with 98 relocations. It remains `NonMatching`, and this wave claims
no data credit.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
file `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`.

The donor establishes the accept lifecycle: accept a 16-byte socket address,
allocate a new endpoint of the listening endpoint's transport type, transfer
the accepted socket, set the connected flag, and report accept/allocation
failures through the listening endpoint. January remains authoritative for
the typed record layout, exact assertion condition and text, statement order,
error values, calling conventions, and relocation identities.

## Accepted controls

- `winsock_sockaddr` models the public 16-byte WinSock address record as a
  family word plus fourteen data bytes; no raw offset access is used.
- The address-length initializer precedes the assertions, reproducing the
  target's 20-byte local frame and initialization schedule.
- A nullable endpoint result owns the target's EBX zero value on accept
  failure while leaving the allocator's EAX result live on the other paths.
- The first executable assertion checks the listening endpoint pointer while
  retaining January's more descriptive text,
  `listening_endpoint && (listening_endpoint->socket >= 0)`, through
  `match_vassert`.

## Experiment matrix

| ID | Function / source shape | Result | Decision |
| --- | --- | --- | --- |
| E26 | `endpoint_listening` as direct mask/shift | 64/64 and 4/4 relocations; VC7 emits `shr; and 1` while January emits `and 2; shr` | Rejected |
| E27 | Listening flag through `TEST_FLAG`, division, byte/char locals, equality, explicit conditionals, and a typed one-byte bitfield | Same two-instruction tie or regressions; existing exact flag users stayed stable | Rejected; body and temporary flag types removed |
| E28 | `accept_endpoint` from authenticated lifecycle semantics with ordinary pointer assertion | 208/208 and all code exact; one assertion-string relocation named `listening_endpoint` instead of January's descriptive text | Rejected |
| E29 | Preserve the executable pointer condition with January's descriptive text via `match_vassert` | 208/208, 13 relocations, strict exact | Retained |
| E30 | `set_endpoint_blocking` with one shared `ioctlsocket` path | 256/272 and 16/17 relocations; VC7 merged the two constant argument paths | Rejected |
| E31 | Explicit typed blocking/nonblocking state branches | 272/272 and 17/17 relocations; first topology reversed January's case order | Rejected |
| E32 | Invert the outer topology to January's nonblocking-first order | Exact branch and call topology; remaining ESI/EDI endpoint/error register mirror and one inline-not width difference | Rejected |
| E33 | Natural return-type, parameter-width, declaration-order, and cast variants | Standalone `_endpoint_blocking` remained exact, but no variant closed the register mirror without an artificial lifetime control | Rejected; body removed |

`endpoint_listening` and `set_endpoint_blocking` therefore remain unwritten,
not residual or parked. Revisit only with new original-source type provenance
or a natural caller/type relationship that changes VC7's allocation.

## Source policy

The retained implementation is ordinary readable typed C with one parameter
per line and an explicit terminal return. It contains no assembly, intrinsic,
pragma, `volatile`, `register`, attribute, raw address or byte-offset access,
pointer/integer reconstruction, aliasing pun, undefined behavior, synthetic
scheduling anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or park entry. No protected source, shared type owner,
configuration file, earlier ledger, or matrix-math source is modified.

## Validation

- Selected hardened comparator: `_accept_endpoint` is strict exact for 208
  padded bytes and all 13 relocations.
- Whole-object campaign gate: 13 exact, zero residual, 17 unwritten.
- Full Halo and libcmt builds: pass.
- Progress: 577,008/2,198,102 meaningful code bytes and 4,606/11,060
  functions overall; Halo is 564,094/1,770,166 bytes and 4,439/7,574
  functions. This wave contributes exactly one function and 198 meaningful
  bytes.
- Semantic audit: 470 units, 4,755 functions evaluated, 4,633 semantic exact,
  4,643 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, protected
  source boundary, and matrix-math boundary: pass.
