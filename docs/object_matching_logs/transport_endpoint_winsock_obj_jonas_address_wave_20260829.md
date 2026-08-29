# `transport_endpoint_winsock.obj`: endpoint-address wave (2026-08-29)

## Result

Starting from canonical commit `bd520100c0296b3353e13d9ae5c6b0c4db5e93a4`,
this source-only wave adds one strict function. The object advances from 13/30
to 14/30 strict functions with zero residuals and 16 unwritten functions.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_get_endpoint_address` | 355 | 368 | 13 | `921fb72020bc88206fd243e65714a28067c79d7bb884e07ae263e0cc5718de40` |

The hardened comparator reports equality for every padded byte and every
relocation address, type, target, and addend. Combined with the preceding
transport waves, the object now owns 1,674 meaningful and 1,808 padded strict
code bytes with 111 relocations. It remains `NonMatching`, and this wave
claims no data credit.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
file `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`.

The donor establishes the 16-byte IPv4 socket-address record, endpoint and
transport-address layouts, address conversion, error behavior, and two-name
fallback. January remains authoritative where the donor differs: January
calls `getpeername` first and falls back to `getsockname`, as proven by the
two call relocations and branch schedule.

## Accepted controls

- `winsock_sockaddr_in` models the family, network-order port, IPv4 address,
  and eight-byte trailing field as typed members; no raw offsets are used.
- The address-length initializer precedes the assertions, reproducing the
  target's 20-byte local frame and initialization schedule.
- A 32-bit transport-error lifetime is narrowed only at the endpoint field
  and function return. Keeping one terminal store/return lets VC7 naturally
  duplicate the success tails and retain January's EBX/ESI/EDI allocation.
- The IPv4 and port conversion expressions reproduce January's ordinary
  inline shifts and masks without intrinsics or helper relocations.

## Experiment matrix

| ID | Source shape | Result | Decision |
| --- | --- | --- | --- |
| E34 | Donor-documented `getsockname` first with early success returns | 352/368; correct semantics but wrong call-relocation order and endpoint/error register lifetime | Rejected |
| E35 | January-proven `getpeername` first, then `getsockname`, with early returns | Call identities corrected; still 352/368 because the zero/error lifetime folds into immediates | Rejected |
| E36 | One 32-bit result lifetime, duplicated typed conversion blocks, and one terminal field store/return | 368/368, 13 relocations, strict exact | Retained |

## Source policy

The retained implementation is ordinary readable typed C with one parameter
per line and an explicit terminal return. It contains no assembly, intrinsic,
pragma, `volatile`, `register`, attribute, raw address or byte-offset access,
pointer/integer reconstruction, aliasing pun, undefined behavior, synthetic
scheduling anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or park entry. No protected source, shared type owner,
configuration file, earlier ledger, or matrix-math source is modified.

## Validation

- Selected hardened comparator: `_get_endpoint_address` is strict exact for
  368 padded bytes and all 13 relocations.
- Whole-object campaign gate: 14 exact, zero residual, 16 unwritten.
- Full Halo and libcmt builds: pass.
- Progress: 577,363/2,198,102 meaningful code bytes and 4,607/11,060
  functions overall; Halo is 564,449/1,770,166 bytes and 4,440/7,574
  functions. This wave contributes exactly one function and 355 meaningful
  bytes.
- Semantic audit: 470 units, 4,756 functions evaluated, 4,634 semantic exact,
  4,644 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, protected
  source boundary, and matrix-math boundary: pass.
