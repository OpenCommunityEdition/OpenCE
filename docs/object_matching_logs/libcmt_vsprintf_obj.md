# `libs/libcmt/vsprintf.obj` exact-match ledger

This atomic lane started from campaign commit `96b66de9`. The object was
`MISSING` at config index 709, absent from `config/parked.json`, production
source, object logs, all refs, and active worktrees. Root announced the claim
before editing and excluded the concurrent CRT lanes.

## Plain-English behavior

`vsprintf` creates a temporary string-backed `FILE`, points it at the caller's
buffer, gives it an effectively unlimited count, and passes the caller's
`va_list` to the shared formatter. It then appends a zero terminator and
returns the formatter's count. `_vscprintf` uses the same formatter with a
null destination so callers can learn how much space formatted output needs.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\vsprintf.obj` member has SHA-256
`2e6e6ad629cd4b7378887ab5af1efe91a40d425a644f99cb7fc31851f91d7c5f`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\vsprintf.c`. The
authenticated Microsoft source is pinned by Git blob
`3b824fa99ffb05adf539ee2437726959f44cecbf`. January remains authoritative.

The target owns two external pick-no-duplicates code COMDATs and no runtime
`.rdata`, `.data`, or BSS:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_vsprintf` | 87 | 2 | `89c0f1413a39fcdb249b07c38a8beae895f1322c743d58562cecfe9e3b642683` |
| `__vscprintf` | 48 | 1 | `62335b099989ed9762f9d60201922d800de3ce206965f80169363d4bf48904e3` |

`_vsprintf` relocates at `+0x2A` to `__output+0` and `+0x4B` to
`__flsbuf+0`; `__vscprintf` relocates at `+0x27` to `__output+0`. Target,
candidate, and authentic archive member match in bytes, relocation identity,
COMDAT selection/alignment, and public symbol ownership.

## Experiment matrix

| ID | Source factor | Total bytes T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authenticated Microsoft topology, chained `_ptr = _base = ...`, standard `putc` macro | `135/135` | `3/3` | both functions strict exact on first compile | accepted |

No code-shape sweep was needed. Together with the already admitted
`snprintf.obj` and `vsnprint.obj`, this independently validates one readable
`FILE`-backed formatter family across variadic, `va_list`, bounded, unbounded,
and count-only entry points.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt `/O1 /Gy` flags.
`tools.coff_compare` reports `all_equal=true` for both candidate functions and
independently for both authentic archive-member functions.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
The lane reports 346/833 completed objects, 3,470/11,060 exact functions, and
413,774 exact code bytes overall; libcmt reports 79/212 objects, 122/476
functions, 7,768 exact code bytes, and 1,728 exact data bytes. Semantic
progress reports zero unit errors.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained both functions as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The implementation uses the real `FILE` layout and named fields, a typed
formatter declaration, standard `va_list`, vertical signatures, chained
assignments from the authenticated source, and one return per function. It
contains no assembly, `volatile`, forced inline, undefined sequencing, opaque
layout, raw offset, byte forcing, flag change, or comparator exception.

`vsprintf` is historically unbounded: it assumes the caller supplied enough
storage for the formatted result and terminator. That unsafe legacy contract
is preserved rather than disguised; defensive code should use the admitted
bounded formatter or call `_vscprintf` first.
