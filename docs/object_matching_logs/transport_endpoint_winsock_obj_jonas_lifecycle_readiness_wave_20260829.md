# `transport_endpoint_winsock.obj`: lifecycle and write-readiness wave (2026-08-29)

## Result

Starting from canonical commit `15b172a82e599bfd7b9a0775bb085a727e1d1fa9`,
this source-only wave adds five strict functions. The object advances from
6/30 to 11/30 strict functions with zero residuals. This wave contributes 674
meaningful and 720 padded code bytes with 51 relocations; combined with the
preceding public-leaf wave, the object now has 1,070 meaningful and 1,168
padded strict code bytes with 82 relocations. It remains `NonMatching`, and
this wave claims no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_create_transport_endpoint` | 118 | 128 | 8 | `3900dc5f80a6cc5e4202935e3e7c51e96fab85661d03dbd5ce4d3af59890ab0c` |
| `_disconnect_endpoint` | 128 | 128 | 12 | `2eb8ac7a230e73e757e6460b0dab3701800418f4a8396a698a421ded2be50419` |
| `_delete_transport_endpoint` | 116 | 128 | 13 | `29b417dbba561903507676b054e285b1d4e056c1157a7a5649cee049084ef080` |
| `_listen_endpoint` | 161 | 176 | 12 | `032ba8f80298e816eb6346563ce239bb34ced565a95bc5762658d7af95ecca5b` |
| `_endpoint_writeable` | 151 | 160 | 6 | `d942d596e0730d16501d34011fd4ae1f4de620c9fd7b4da825a0b148aab92875` |
| **Wave gain** | **674** | **720** | **51** | |

The hardened comparator reports equality for every padded byte and every
relocation address, type, target, and addend in all five retained functions.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and the unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`;
January PC COFF remains authoritative for types, line numbers, assertion
spelling, control flow, instruction scheduling, relocations, and acceptance.

The target proves the endpoint's typed socket/flags/type/error layout, the UDP
and TCP transport-type values, the invalid socket value, the pending-listen
backlog, and the WinSock ABI decorations. The local `winsock_fd_set` and
`winsock_timeval` records model only their public typed fields; no byte offsets
or pointer reconstruction are used.

## Accepted controls

- `create_transport_endpoint`, `disconnect_endpoint`, and
  `delete_transport_endpoint` matched on their first complete typed-C
  emissions. Their allocation, close/error logging, state reset, free, and
  endpoint-count maintenance follow the donor semantics and January line and
  relocation evidence.
- `listen_endpoint` keeps its error lifetime as a 32-bit `long` and writes it
  once to the endpoint's 16-bit error field. Expressing the valid-socket path
  as the positive branch reproduces January's fall-through and failure tail.
- `endpoint_writeable` initializes the timeout before the socket set and
  writes the socket element before its count. That natural structure ownership
  gives January's load/register assignment and final two-store order exactly.

## Experiment matrix

| ID | Function / source shape | Result | Decision |
| --- | --- | --- | --- |
| E09 | Create, disconnect, and delete lifecycle bodies from authenticated semantics plus January assertions | All three exact on first complete emission | Retained |
| E10 | `listen_endpoint` with 16-bit/enum error and early returns | Correct behavior but wrong 32-bit register lifetime and 160/176-byte variants | Rejected |
| E11 | `listen_endpoint` with `long error`, one final field store, and invalid-socket positive branch | Correct lifetime and size; branch order still differed | Rejected |
| E12 | Invert to the target's positive valid-socket branch | 176/176, 12 relocations, strict exact | Retained |
| E13 | `endpoint_writeable` with count written before socket element | 160/160 and all relocations equal; only the two independent structure stores were reversed | Rejected |
| E14 | Write socket element before count | Strict exact without affecting any sibling | Retained |
| E15 | `endpoint_readable` direct-return and natural field-order forms | 176/192; January keeps a zero value in EBX for null arguments and the invalid return | Rejected |
| E16 | Result local used only by the invalid return | Optimized away; same 176/192 residual | Rejected |
| E17 | Result local carried through the final return | Recovers EBX and the prologue, but rotates the invalid branch and mutates the success/failure return topology | Rejected |
| E18 | Natural zero-seconds local with direct returns | Local folds away and returns to the 176/192 residual | Rejected; body removed |

`endpoint_readable` is now a bounded zero-register/lifetime fixed point. A
further exact shape would require coupling an unrelated boolean to null pointer
arguments or timeout storage, which would be a synthetic code-generation
control. The function remains unwritten.

## Source policy

The retained source is ordinary typed C with one parameter per line and an
explicit terminal return in every function. It contains no assembly,
intrinsic, pragma, `volatile`, `register`, attribute, raw address or byte
offset, pointer/integer reconstruction, aliasing pun, undefined behavior,
synthetic anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or park entry. No protected source, shared type owner,
configuration file, or earlier ledger is modified.

## Validation

- Selected hardened comparison: all five functions strict exact for 720
  padded bytes and all 51 relocations.
- Whole-object campaign gate: 11 exact, zero residual, 19 unwritten.
- Full Halo and libcmt builds: pass.
- Progress: 576,759/2,198,102 meaningful code bytes and 4,604/11,060
  functions overall; Halo is 563,845/1,770,166 bytes and 4,437/7,574
  functions. This wave contributes exactly five functions and 674 meaningful
  bytes.
- Semantic audit: 470 units, 4,753 functions evaluated, 4,631 semantic exact,
  4,641 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, and protected
  source boundary: pass.
