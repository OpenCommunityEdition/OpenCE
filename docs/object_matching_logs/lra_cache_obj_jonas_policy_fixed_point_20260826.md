# `lra_cache.obj` policy fixed point (2026-08-26)

This is a current-campaign audit of `source/memory/lra_cache.obj`. No source,
header, configuration, semantic exception, parked-function entry, object
artifact, or completion label is changed.

## Current strict state

- 13/14 functions are strict-exact.
- 1,004/1,392 meaningful code bytes are accepted.
- All 160 target-owned padded data bytes are exact.
- Sole residual: `_lra_allocate`.

| Measurement | January target | Current rebuild |
| --- | ---: | ---: |
| Meaningful bytes | 388 | 365 |
| Padded bytes | 400 | 368 |
| Relocations | 4 | 4 |
| Normalized SHA-256 | `93beee39f5b398662960640adbdef20b5569e5407bff1526a6021970f3001e2f` | `bd671ca359d22c92b9d24e5428f9c62f6fc4397f1643ed41d5a7df44ccdaa9ef` |

Target relocation addresses are `0x13`, `0x6B`, `0x89`, and `0x8E`.
Current addresses are `0x13`, `0x5F`, `0x83`, and `0x88`; destinations,
types, and addends otherwise agree.

Current reproducibility hashes:

- source:
  `89d3db50efc65d9599bf05391281bbebb263a20cfe4a4e8c19a185e69e4efd33`;
- rebuilt object:
  `f5763f1ae76b9a795bcd91443ee8bc81f149691ab85f3338aaf8be60605736dc`;
- target object:
  `6ecd5583976033242fdd8a89d7f946d591bc19cc296091b2cb9d3707dbd85b7e`.

## Claude provenance reviewed

The complete 202-line historical ledger was read from
`b298f939:docs/object_matching_logs/lra_cache_obj.md`, together with the
source and commit history at `52448097`, `4c769166`, `b298f939`, and
`71cc36e5`. That corpus records 50 measured experiment families plus a
compiler discriminator and donor-motif scan.

The branch name `codex/lra-cache-full-admission-20260729` is misleading:
its own strict ledger still reports 13/14 functions. Its best 400-byte
candidate has four relocation destinations but remains nonexact, with the
last two relocation addresses seven bytes late and 258 differing normalized
positions.

The important recovered facts are:

- the single-return valid-size wrapper creates the target's initial
  `ESI=0` lifetime, exact result/pass stack homes, and 400-byte padded size;
- the branch-local `(short)(number_of_passes++)` guard is authoritative;
- the target carries the zero register through the loop head and uses it for
  the null `write_offset` store and shared bottom comparison;
- VC7 otherwise emits an immediate zero store plus reload, shifting the last
  two relocations by seven bytes.

## Why no candidate was compiled

The historical matrix already closes predicate spellings, declaration
permutations, loop/switch/goto boundaries, aggregate and separate locals,
zero aliases, default assignments, explicit per-iteration resets,
`register`, both locally compatible XDK compiler builds, and the documented
barrier placements.

The only control that preserves the desired shared bottom test is
`_ReadWriteBarrier()` in the loop condition. That is a synthetic compiler
scheduling device and is forbidden by the current campaign rules, alongside
`volatile`, assembly, intrinsics, pragmas, undefined behavior, and byte
forcing. Restoring the historical form would therefore be policy-invalid
even if it improved size or relocation layout.

The old ledger's own reopen conditions require one of:

1. authoritative January source/local records;
2. a real donor carrying this exact zero-register loop dependency;
3. a campaign-compatible original C2/QFE compiler; or
4. a new defined-C control that directly explains the zero lifetime.

None is currently present. A normal candidate compile would only repeat a
closed family, so the one-compile evidence rule correctly forbids it.

## Disposition

`lra_cache.obj` remains `NonMatching` at 13/14. The current legal source is
preserved. This is a policy/provenance fixed point, not a compiler-tie park:
size and relocation addresses are still unequal. Reopen only with genuinely
new evidence matching one of the conditions above.
