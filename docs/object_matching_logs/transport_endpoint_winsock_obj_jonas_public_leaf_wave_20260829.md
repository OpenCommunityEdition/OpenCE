# `transport_endpoint_winsock.obj`: public endpoint leaf wave (2026-08-29)

## Result

This bounded source-only wave starts from canonical commit
`7bc38a253f5a4ff3f41213d95e2691d54c027df3`. Six ordinary typed-C functions
are strict exact. `transport_endpoint_winsock.obj` advances from 0/30 to 6/30
strict functions, covering 396 meaningful and 448 padded code bytes with 31
relocations. It remains `NonMatching`; this wave claims no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_get_endpoint_type` | 91 | 96 | 9 | `d16ca01c8165bf76d4e4aa26cc28552c307caa8a614febe2ae1a40e017fe0761` |
| `_endpoint_connected` | 54 | 64 | 4 | `5a4d1a1b22907170e81491858f910db989169d20cb13f27b36d244aef66d83cd` |
| `_endpoint_blocking` | 59 | 64 | 4 | `163dbd67ddb1e46a77b310b051b16c93135924e27a049e7087e66aa1e48599e5` |
| `_get_endpoint_error` | 50 | 64 | 4 | `8e0dbb2decc820575dd499dfa4fb8ada34065fea076f647f597977cb2ca5170f` |
| `_endpoint_equivalent` | 109 | 112 | 8 | `a890a2892c8440016beff60b363fc45e373a3835e87d7da6d8a590ea3af1f352` |
| `_reject_endpoint` | 33 | 48 | 2 | `faa17835f043a7ef98f253639000b0c07f610e48b7866d65c46789dd1c5d51ef` |
| **Wave gain** | **396** | **448** | **31** | |

The hardened comparator reports `all_equal: true` for all six retained
functions. It verifies complete padded sections and every relocation address,
type, target, and addend. The target-authoritative whole-object gate reports
six exact functions, zero residuals, and 24 unwritten functions.

## Scope and provenance

The target is the January `cachebeta.exe` build 2342 split object. Its
`transport_endpoint_winsock.obj` SHA-256 is
`3a8ab424186b274d8d729c67f6b9096f8ddaf5a9dc1e8c280a2d3e7a0c57510f`.
Compilation uses the pinned XDK 3911 VC7 compiler and the translation unit's
unchanged repository flags.

The user-supplied Xbox reconstruction at `pastudan/halo` commit
`918af885935ec470a31256ecce9a977b12b01f80` independently supplies the
endpoint lifecycle semantics and proves the first eight bytes of the endpoint
record: a 32-bit socket, a flag byte, a signed endpoint-type byte, and a
16-bit transport error. January PC COFF remains authoritative for names,
types, assertions, exact line numbers, expression topology, and codegen.

January's owned strings and relocations prove the source path, the `ep`, `a`,
and `b` assertion spellings, the `transport_initialized` guard, and source
lines `0x12C`, `0x12D`, `0x426`, `0x436`, `0x43E`, `0x447`, and `0x448`.
The target instructions independently prove the signed type return, flag-bit
operations, 16-bit error return, valid-socket equivalence test, and the
accept/delete/reject sequence.

## Accepted controls

- Naming the assertion parameter `ep` is semantically natural and required
  for the target-owned assertion-string relocation identity. The initial
  descriptive `endpoint` spelling had exact instructions but correctly
  failed the hardened relocation-identity gate.
- `endpoint_blocking` preserves the target's stored nonblocking flag meaning:
  shift bit four, invert, and mask to one.
- `endpoint_equivalent` rejects the invalid `-1` socket before comparing the
  second endpoint. This branch topology matched January on first emission.
- `reject_endpoint` uses the target-proven 16-bit result, accepts exactly one
  pending endpoint, deletes it when non-null, and returns zero. It matched on
  first emission.

## Experiment matrix

| ID | Function / source shape | Result | Decision |
| --- | --- | --- | --- |
| E01 | Six retained functions with descriptive `endpoint` assertion names | All instruction hashes except `_endpoint_listening` match; five assertion-bearing leaves fail only because the candidate owns `"endpoint"` while January owns `"ep"`; `_endpoint_equivalent` and `_reject_endpoint` are strict exact | Rename only the proven assertion parameter; retain exact bodies |
| E02 | Rename the five assertion parameters to `ep` | `_get_endpoint_type`, `_endpoint_connected`, `_endpoint_blocking`, and `_get_endpoint_error` become strict exact; exact siblings stay exact | Retained |
| E03 | `_endpoint_listening`: direct `(flags & 2) >> 1` with an unsigned byte field | Equal size and relocations; candidate emits `shr eax,1; and eax,1`, while January emits `and eax,2; shr eax,1` | Rejected |
| E04 | `_endpoint_listening`: signed flag byte | Listening still nonexact and exact `_endpoint_blocking` regresses | Rejected |
| E05 | `_endpoint_listening`: typed one-byte bitfield accessor | Same nonexact shift/mask order as E03 | Rejected |
| E06 | `_endpoint_listening`: masked division by two | Same nonexact shift/mask order as E03 | Rejected |
| E07 | `_endpoint_listening`: equality against the masked bit | Different boolean materialization sequence; nonexact | Rejected |
| E08 | `_endpoint_listening`: explicit typed local, mask assignment, then shift | Optimizer emits the same nonexact order as E03 | Rejected and body removed |

The listening residual is a two-instruction scheduler/algebraic-order tie.
Per the repository's five-shape stop rule, it remains unwritten rather than
leaving a residual or applying a synthetic compiler control. Reopen it only
with authenticated original-source expression provenance or a demonstrated
natural typed-C control that preserves every exact sibling.

## Source policy and ownership

The retained source uses a local named endpoint record whose four fields and
widths are supported by both Xbox reconstruction and January instructions.
All accesses use those fields. Every function has one parameter per line and
an explicit terminal return.

The wave contains no assembly, intrinsic, barrier, pragma, `volatile`,
`register`, attribute, raw address or byte-offset access, pointer/integer
reconstruction, aliasing pun, undefined behavior, synthetic anchor,
compiler-flag change, object patch, comparator exception, semantic allowlist,
or park entry. Only
`source/bungie_net/network/transport_endpoint_winsock.c` and this additive
ledger change. No protected source, shared type owner, configuration file, or
pre-existing ledger is modified.

## Validation

- Selected hardened comparison: all six functions strict exact for 448 padded
  bytes and all 31 relocations.
- Whole-object campaign gate: six exact, zero residual, 24 unwritten.
- Full Halo and libcmt builds: pass.
- Progress: 576,085/2,198,102 meaningful code bytes and 4,599/11,060
  functions overall; Halo is 563,171/1,770,166 bytes and 4,432/7,574
  functions. This wave contributes exactly six functions and 396 meaningful
  bytes.
- Semantic audit: 470 units, 4,748 functions evaluated, 4,626 semantic exact,
  4,636 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, deleted-path review, protected-source boundary, and
  retained-path review: pass.
