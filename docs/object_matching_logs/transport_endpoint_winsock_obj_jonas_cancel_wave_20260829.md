# `transport_endpoint_winsock.obj`: connect-cancellation wave (2026-08-29)

## Result

Starting from canonical commit `43e3affd8370bc1a9b379441bdc0369da7f212f2`,
this source-only wave adds one strict function. The object advances from 15/30
to 16/30 strict functions with zero residuals and 14 unwritten functions.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_cancel_connect_process` | 151 | 160 | 12 | `05658b3db7140d64cb98f117f820d1a3ca7af03f037372e9ccfbea7733bbd09b` |

The hardened comparator reports equality for every padded byte and every
relocation address, type, target, and addend. Combined with the preceding
transport waves, the object now owns 2,060 meaningful and 2,208 padded strict
code bytes with 139 relocations. It remains `NonMatching`, and this wave
claims no data credit.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
file `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`.

The donor and January agree on the cancellation lifecycle: validate the
input, clean completed endpoint threads, acquire the connection mutex for one
second, disconnect the endpoint, clear its error, set the cancellation flag,
and release the mutex. January's field accesses prove the endpoint, thread,
mutex, and cancellation members and their layout.

## Accepted controls

- `connect_process_input` is a typed record with known pointer and boolean
  members plus one explicitly unknown six-long region. No raw address or
  byte-offset access is used.
- The first assertion uses the target-owned expression
  `input && input->ep && input->thread` at line `0x298`.
- The mutex-failure assertion is represented with `match_vassert` so its
  target-owned descriptive text is preserved while the executable condition
  remains the proven constant false condition.
- The function has one terminal explicit return; VC7 naturally duplicates
  the success and assertion epilogues exactly.

## Experiment matrix

| ID | Source shape | Result | Decision |
| --- | --- | --- | --- |
| E43 | Typed connection record and donor lifecycle with January assertions | 160/160, 12 relocations, strict exact on first complete emission | Retained |

## Source policy

The retained implementation is ordinary readable typed C with one parameter
per line and an explicit terminal return. It contains no assembly, intrinsic,
pragma, `volatile`, `register`, attribute, raw address or byte-offset access,
pointer/integer reconstruction, aliasing pun, undefined behavior, synthetic
scheduling anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or park entry. No protected source, shared type owner,
configuration file, earlier ledger, or matrix-math source is modified.

## Validation

- Selected hardened comparator: `_cancel_connect_process` is strict exact for
  160 padded bytes and all 12 relocations.
- Whole-object campaign gate: 16 exact, zero residual, 14 unwritten.
- Strict campaign board: 632,007/1,922,669 padded code bytes and 4,617/8,246
  functions.
- Full Halo and libcmt builds: pass.
- Progress: 577,514/2,198,102 meaningful code bytes and 4,608/11,060
  functions overall; Halo is 564,600/1,770,166 bytes and 4,441/7,574
  functions. This wave contributes exactly one link-live function and 151
  meaningful bytes.
- Semantic audit: 470 units, 4,758 functions evaluated, 4,636 semantic exact,
  4,646 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, protected
  source boundary, and matrix-math boundary: pass.
