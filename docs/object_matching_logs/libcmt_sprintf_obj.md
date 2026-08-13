# `libs/libcmt/sprintf.obj` exact-match ledger

This atomic lane started from campaign commit `1626bac6`. The object was
`MISSING` at config index 715, absent from `config/parked.json`, production
source, object logs, all refs, and active worktrees. Root announced the claim
before editing and excluded all concurrent CRT lanes.

## Plain-English behavior

`sprintf` wraps a caller's buffer in a temporary string-backed `FILE`, feeds
its variadic arguments to the shared CRT formatter, appends a zero, and returns
the formatter's character count. `_scprintf` runs that formatter with a null
destination so callers can calculate the required buffer length first.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\sprintf.obj` member has SHA-256
`7694ab8f6c559138f2a9a00bf2c512635a27a9e639421114247c34155ceefde8`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\sprintf.c`. The
authenticated historical Microsoft source is pinned by Git blob
`5e10325e60d3840292ae97fc5a7a84732a224803`. January remains authoritative.

The target owns two external pick-no-duplicates code COMDATs and no runtime
`.rdata`, `.data`, or BSS:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_sprintf` | 88 | 2 | `b2976083077c6db20e77e970849cbd86de322c00e74f035376383ae99f7f3689` |
| `__scprintf` | 49 | 1 | `a4f0ea5af60331328fc864f07a787c3a5fac1a288265b317d02aa1b65ef0dae3` |

`_sprintf` relocates at `+0x2B` to `__output+0` and `+0x4C` to
`__flsbuf+0`; `__scprintf` relocates at `+0x28` to `__output+0`. Candidate,
January, and authentic archive member agree on bytes, relocation identity,
COMDAT selection/alignment, and symbol ownership.

## Experiment matrix

| ID | Source factor | Total bytes T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authenticated Microsoft variadic topology, chained stream assignments, standard `putc` terminator | `137/137` | `3/3` | both functions strict exact on first compile | accepted |

No source-shape sweep was needed. This closes the narrow four-object formatter
cluster alongside `snprintf`, `vsnprint`, and `vsprintf`, and validates the
same readable stream construction across eight entry points.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt `/O1 /Gy` flags.
`tools.coff_compare` reports `all_equal=true` for both reconstructed functions
and independently for both functions extracted from the authentic archive.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
The lane reports 347/833 completed objects, 3,472/11,060 exact functions, and
413,911 exact code bytes overall; libcmt reports 80/212 objects, 124/476
functions, 7,905 exact code bytes, and 1,728 exact data bytes. Semantic
progress reports zero unit errors.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained both functions as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The implementation uses the actual `FILE` structure and named fields, typed
prototypes, standard variadic handling, vertical signatures, authenticated
chained assignments, and one return per function. It contains no assembly,
`volatile`, forced inline, undefined sequencing, opaque layout, raw offset,
byte forcing, build-flag change, or comparator exception.

`sprintf` is historically unbounded and assumes the destination is large
enough. That legacy contract is preserved and documented; defensive callers
should use the admitted bounded formatter or call `_scprintf` first.
