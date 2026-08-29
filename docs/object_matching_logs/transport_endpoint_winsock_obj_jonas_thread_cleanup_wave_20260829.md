# `transport_endpoint_winsock.obj`: endpoint-thread cleanup wave (2026-08-29)

## Result

Starting from canonical commit `ad7b4a05c59003602a91b41a186d5ddc05b78b8c`,
this source-only wave adds one strict function and typed ownership of the
endpoint-thread global. The object advances from 11/30 to 12/30 strict
functions with zero residuals and 18 unwritten functions.

| Retained symbol | Meaningful | Padded / owned | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_000713a0` | 51 | 64 | 3 | `be152f22a680e4035a31109cae277098c7221fb9a860f174fa55853a4bf40bb2` |
| `_bss_0031ce38` | 524 | 524 | 0 | `c1dcdd7d8c2a5d273f73cca0b55d798344bf94f5834c1a6b852cb0ee709278d7` |

The hardened comparator reports equality for every padded function byte and
all three relocation records. The BSS symbol has the same 524-byte logical
size, storage ownership, and normalized zero-byte hash on both sides. Because
the translation unit remains incomplete, this wave does not add a semantic
data allowlist or claim the 524 BSS bytes in ordinary progress.

Combined with the two preceding waves, the object now owns 1,121 meaningful
and 1,232 padded strict code bytes with 85 relocations.

## Scope and provenance

The authority is January `cachebeta.exe` build 2342 split with the pinned
`csplit` tooling. Compilation uses the repository-pinned XDK 3911 VC7 compiler
and the unchanged translation-unit flags. The authenticated semantic donor is
`pastudan/halo` commit `918af885935ec470a31256ecce9a977b12b01f80`,
file `src/halo/bungie_net/network/transport_endpoint_set_winsock.c`.

The donor establishes a 64-entry endpoint-thread pool and cleanup semantics:
dispose completed thread references, clear their handles, and clear their
dispose flags. January remains authoritative for the private helper's loop
shape, signed comparison, relocation identities, record size, and global
ownership. The target's relocations prove the pool begins at BSS offset 8 and
ends at offset 520; the resulting typed owner is 524 bytes.

## Accepted controls

- A forward-declared `thread_reference` and an eight-byte
  `endpoint_thread_reference` preserve typed pointer and boolean ownership.
- A 64-element array inside `transport_endpoint_winsock_globals` gives the
  target's +8 pool base and +520 loop-end address without raw offset access.
- A signed `long` index with typed element selection strength-reduces to the
  target's pointer walk and signed `jl` back edge.
- The helper ends with an explicit return and follows the campaign's
  one-parameter-per-line / no-argument-`void` formatting rules.

## Experiment matrix

| ID | Function / source shape | Result | Decision |
| --- | --- | --- | --- |
| E19 | Cleanup with a direct pointer `do/while` | 64/64 with matching body and relocations, but emitted unsigned `jb` instead of target `jl` | Rejected |
| E20 | Cleanup with signed `long` index and typed array selection | 64/64, three relocations, strict exact | Retained |
| E21 | `_code_00071300` registration helper as `for` plus short-circuit | VC7 unrolled eight-way; 208/96 | Rejected |
| E22 | Registration helper as bounded `while` with `break` | 80/96 with wrong increment/load schedule | Rejected |
| E23 | Registration helper with a pointer alias | 176/96 and unrolled | Rejected |
| E24 | Registration helper as nested-break `while` | 208/96 and unrolled | Rejected; body removed |
| E25 | `_code_00071360` registration subhelper | Not attempted: target uses a private ESI argument convention and its caller remains unwritten | Left unwritten |

The rejected registration experiments leave no source body and therefore no
residual candidate. Revisit the pair only with caller topology sufficient to
recover the private convention naturally.

## Source policy

The retained implementation is ordinary readable typed C. It contains no
assembly, intrinsic, pragma, `volatile`, `register`, attribute, raw address or
byte-offset access, pointer/integer reconstruction, aliasing pun, undefined
behavior, synthetic scheduling anchor, compiler-flag change, object patch,
comparator exception, semantic allowlist, or park entry. No protected source,
shared type owner, configuration file, or earlier ledger is modified.

## Validation

- Hardened comparator: `_code_000713a0` and `_bss_0031ce38` both report
  `equal: true`; combined result is `all_equal: true`.
- Whole-object campaign gate: 12 exact, zero residual, 18 unwritten.
- Full Halo and libcmt builds: pass.
- Progress: 576,810/2,198,102 meaningful code bytes and 4,605/11,060
  functions overall; Halo is 563,896/1,770,166 bytes and 4,438/7,574
  functions. This wave contributes exactly one function and 51 meaningful
  bytes.
- Semantic audit: 470 units, 4,754 functions evaluated, 4,632 semantic exact,
  4,642 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, prohibited-source scan, exact-path review, protected
  source boundary, and matrix-math boundary: pass.
