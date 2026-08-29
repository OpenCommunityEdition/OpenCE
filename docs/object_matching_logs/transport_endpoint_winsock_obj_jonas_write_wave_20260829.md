# `transport_endpoint_winsock.obj`: stream-write wave (2026-08-29)

## Result

Starting from canonical commit `11af2375c3da04dfc0edf5f4e8b4493e71dd8a10`,
this source-only wave adds one strict function. The object advances from 14/30
to 15/30 strict functions with zero residuals and 15 unwritten functions.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_write_endpoint` | 235 | 240 | 16 | `e29993809324a6d4e225053392e61c7d42a078eefe51e974cd72b436ccb575d6` |

The hardened comparator reports equality for every padded byte and every
relocation address, type, target, and addend, including the switch's internal
tables and targets. Combined with the preceding transport waves, the object
now owns 1,909 meaningful and 2,048 padded strict code bytes with 127
relocations. It remains `NonMatching`, and this wave claims no data credit.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
file `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`.

The donor and January agree on the stream I/O calls, validation, WinSock error
classification, endpoint error values, and connection-flag changes. January's
COFF remains authoritative for switch lowering, internal tables, store order,
assertion strings, source lines, and all relocation identities.

## Accepted controls

- The public `send` declaration uses the proven four-argument WinSock stdcall
  ABI and a const typed buffer.
- `write_endpoint` keeps the send result in a 32-bit local, returns successful
  byte counts directly, and classifies only `INVALID_SOCKET` through January's
  sparse WinSock switch.
- Each switch arm updates the typed endpoint fields before returning its
  transport result. VC7 naturally emits January's three-arm tail duplication
  and identical internal redirect/jump tables.

## Experiment matrix

| ID | Function / source shape | Result | Decision |
| --- | --- | --- | --- |
| E37 | `write_endpoint` from donor semantics and January assertions | 240/240, 16 relocations, strict exact on first complete emission | Retained |
| E38 | `read_endpoint` with direct typed switch arms | 256/256 with matching calls and switch topology; default arm uses an in-memory `and` where January splits flag load/mask/store | Rejected |
| E39 | Non-compound flag assignment | Same default-arm fold | Rejected |
| E40 | Reverse typed endpoint-field update order | Recovers January's split load/mask but reverses the final flag/error stores | Rejected |
| E41 | Shared result lifetime and terminal endpoint-error store | 240/256; VC7 merges the switch-arm epilogues, unlike January | Rejected |
| E42 | Explicit typed flag local with target field order | Optimizer folds the local back to the E38 in-memory update | Rejected; body and unused `recv` declaration removed |

`read_endpoint` therefore remains unwritten rather than residual or parked.
Revisit only with original-source macro/type provenance or a natural caller
relationship that changes VC7's default-arm scheduling.

## Source policy

The retained implementation is ordinary readable typed C with one parameter
per line and an explicit terminal return. It contains no assembly, intrinsic,
pragma, `volatile`, `register`, attribute, raw address or byte-offset access,
pointer/integer reconstruction, aliasing pun, undefined behavior, synthetic
scheduling anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or park entry. No protected source, shared type owner,
configuration file, earlier ledger, or matrix-math source is modified.

## Validation

- Selected hardened comparator: `_write_endpoint` is strict exact for 240
  padded bytes and all 16 relocations.
- Whole-object campaign gate: 15 exact, zero residual, 15 unwritten.
- Strict campaign board: 631,847/1,922,669 padded code bytes and 4,616/8,246
  functions.
- Full Halo and libcmt builds: pass.
- Semantic audit: 470 units, 4,757 functions evaluated, 4,635 semantic exact,
  4,645 accepted exact, 128 hidden exact functions covering 75,275 meaningful
  bytes, and zero unit errors or ordinary rejections. This unreferenced public
  function is presently classified as hidden exact, so the linked progress
  report remains 577,363/2,198,102 code bytes and 4,607/11,060 functions until
  a reconstructed caller makes it link-live.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, protected
  source boundary, and matrix-math boundary: pass.
