# `libs/libcmt/rand.obj` exact-match ledger

This atomic lane starts from campaign commit `0276e8d6`. The object was
`MISSING` at config index 765, absent from `config/parked.json`, every Git ref,
active worktree, and existing object log. January owns two functions and no
runtime data in this object.

## Plain-English behavior

`srand` stores a caller-supplied seed in the current thread's CRT state.
`rand` advances that seed using Microsoft's traditional linear-congruential
formula (`seed * 214013 + 2531011`) and returns bits 16 through 30. The seed is
per-thread rather than global, which is why both functions first call
`_getptd`.

## Provenance and target inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\rand.obj` member has SHA-256
`a2b22cace99476d94c04c5e54e16c8a3f6bfe38fb13bb9768e77451c07a7b9ef`,
compiler identifier `0x001c2426`, ordinary C CodeView/line records, and source
path `d:\xbox-finalfre\private\sdktools\crt\crtw32\misc\rand.c`.
The member is itself strict-exact to January; this is primary Microsoft C
provenance, not an inferred algorithm or vendor assembly.

Strict inventory:

- `_srand`: 13 bytes, one `REL32` at `+1` to `__getptd + 0`, normalized
  SHA-256 `28489ecb8d8373d25b62e9749486e9768f107c1d0b70ce44fe8c7f9d18c8aec4`;
- `_rand`: 34 bytes, one `REL32` at `+1` to `__getptd + 0`, normalized
  SHA-256 `82a11d96726827a0e4767b31c0a21b4c70036f25b7ca010b9e8edcf1f25430fd`;
- both are external, pick-no-duplicates `.text` COMDATs with exact symbol
  storage/type/value; no aliases and no owned `.rdata`, `.data`, or `.bss`.

The readable `thread_data` prefix is reused from the already admitted
`strtok.obj`: named fields through `random_seed` naturally place the seed at
January's proven `+0x14`. No padding blob, raw offset access, or fabricated
layout is needed.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Strict result | Decision |
|---|---|---:|---:|---|---|
| E01 | Typed thread-data prefix; direct seed assignment; named LCG constants and one stored update | `13/13`, `34/34` | `1/1`, `1/1` | both functions and ownership exact | accepted first compile |

Any later experiment must vary one evidence-backed source factor, record its
strict measurements here, and be reverted unless it improves the complete
object result.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the accepted source with the unchanged
libcmt `/O1 /Gy /I"xbox/include"` flags. Hardened comparison reports
`section_infos_equal` for both functions, including size, normalized bytes,
relocation address/type/destination/addend, COMDAT selection, linkage, and
ownership. Candidate `.drectve` and CodeView sections are discardable build
metadata, not January runtime ownership.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 329/833 objects overall, 267/468 Halo objects, and 62/212 libcmt
objects. Libcmt has 84/476 exact functions and 4,217/55,015 exact code bytes.
This object contributes one object, two functions, and 47 bytes. The semantic
audit reports zero unit errors.

The clean-commit fail-closed per-unit snapshot/check was followed by deletion
of the generated object and a forced rebuild. `_srand` and `_rand` both remain
`still_exact`, with no failures, warnings, changed non-exact siblings, newly
exact surprises, or ownership changes. The complete tooling suite passes
179/179 tests.

## House/Berth and original-bug audit

The source uses explicit types, named constants, named parameters, ordinary C
control flow, and the proven thread-state structure. It contains no assembly,
`volatile`, forced inlining, undefined behavior, type-punning, opaque cast,
packing/alignment trick, flag change, byte patch, or comparator waiver. Both
functions have a single natural return. The unsigned wraparound in the seed
update is defined C behavior and is required by the algorithm; no original
bug or defensive fix is implicated.
