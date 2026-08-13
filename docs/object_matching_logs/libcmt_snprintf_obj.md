# `libs/libcmt/snprintf.obj` exact-match ledger

This atomic lane started from campaign commit `78472a43`. The object was
`MISSING` at config index 717, absent from `config/parked.json`, production
source, object logs, all refs, and active worktrees. Root announced the claim
before editing and excluded the three concurrent CRT objects.

## Plain-English behavior

`_snprintf` makes a temporary `FILE` structure point at the caller's character
buffer, gives that stream the caller's byte limit, and asks the shared CRT
formatter to write into it. If a destination buffer exists, it then attempts
to append a terminating zero through the normal stream macro. The function
returns the formatter's character count.

## Primary provenance and inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\snprintf.obj` member has SHA-256
`a7324cfbdeafd7f4907cc4e618a62d0347772fc18fd7891ade2475a9acb02fd2`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\sprintf.c`. The
authenticated historical Microsoft source is pinned by Git blob
`5e10325e60d3840292ae97fc5a7a84732a224803`. January remains authoritative;
the archive and source are independent provenance.

The target owns one external pick-no-duplicates code COMDAT and no runtime
`.rdata`, `.data`, or BSS:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__snprintf` | 87 | 2 | `9f16eddec2b1afb29d2d482925604b5d87b03794f93f36b9224f115e8bbd711c` |

The REL32 records are exactly `+0x2A -> __output+0` and
`+0x4B -> __flsbuf+0`. Target, reconstructed candidate, and authentic archive
member match in size, bytes, relocation address/type/destination/addend, COMDAT
selection, alignment, and public symbol ownership.

## Experiment matrix

| ID | Source factor | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authenticated Microsoft topology; typed local `FILE`; standard `putc` macro for the terminator | `87/87` | `2/2` | strict exact on first compile | accepted |

No source-shaping sweep was needed. In particular, the `putc` expression is
not a hand-written byte trick: the campaign XDK header's ordinary stream macro
naturally emits the exact decrement/store-or-`_flsbuf` sequence.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with the unchanged libcmt `/O1 /Gy`
configuration. `tools.coff_compare` reports `all_equal=true` for the candidate
and independently for the authentic archive member.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
This lane reports 343/833 completed objects, 3,464/11,060 exact functions, and
413,420 exact code bytes overall; libcmt reports 76/212 objects, 116/476
functions, and 7,414 exact code bytes. Semantic progress reports zero unit
errors.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained `__snprintf` as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The implementation uses the actual `FILE` type and named fields, a typed
formatter declaration, standard `va_list`, vertical signatures, one source
return, and natural control flow. It contains no assembly, `volatile`, forced
inline, undefined sequencing, opaque layout, raw offset, byte forcing, flag
change, or comparator exception.

The historical `_snprintf` contract does not guarantee a trailing zero when
the count is exhausted: the final `putc` can call `_flsbuf`, which refuses to
extend this string-backed stream. That is intentional legacy CRT behavior, not
a newly introduced reconstruction bug; callers needing guaranteed termination
must reserve space or terminate explicitly.
