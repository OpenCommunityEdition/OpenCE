# `libs/libcmt/vswprint.obj` exact-match ledger

This atomic lane started from campaign commit `c1ef3f93`. The object was
`MISSING` at config index 706, unparked, and absent from production source,
logs, refs, and active worktrees. Root announced the collision-free claim
before editing.

## Plain-English behavior

`vswprintf` formats a `va_list` into a wide-character buffer by wrapping that
buffer in a temporary string-backed `FILE`. After formatting, it writes two
zero bytes—the complete 16-bit terminator. `_vscwprintf` runs the wide
formatter with no destination to calculate the required wide-character count.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\vswprint.obj` member has SHA-256
`89e9659e86e0a012546cb0c8fce8b21fa431c4743c7014fafebc5fb6b50864fb`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\vswprint.c`. The
authenticated Microsoft source is pinned by Git blob
`d82fda86775af06c2540773e52d3ec645b7409ed`. January remains authoritative.

The target owns two external pick-no-duplicates code COMDATs and no runtime
data:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_vswprintf` | 110 | 3 | `a5bb1fe4ce5e3cc41e56983f9e5ccf218ac080f58d2100afbfd6ae4a9ccffc0b` |
| `__vscwprintf` | 48 | 1 | `62335b099989ed9762f9d60201922d800de3ce206965f80169363d4bf48904e3` |

The formatter relocates to `__woutput` at `+0x29` and to `__flsbuf` at
`+0x49` and `+0x63`; the counter relocates to `__woutput` at `+0x27`.
Candidate, January, and authentic archive member match every byte, relocation
identity/addend, COMDAT attribute, and public owner.

## Experiment matrix

| ID | Source factor | Total bytes T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic wide formatter topology; two ordinary `putc` calls for the 16-bit terminator | `158/158` | `4/4` | both functions strict exact on first compile | accepted |

No source-shape sweep was needed. The two ordinary terminator writes naturally
emit January's two independent `_flsbuf` fallback paths.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt `/O1 /Gy` flags.
`tools.coff_compare` reports `all_equal=true` for both candidate functions and
independently for the two authentic archive-member functions. Full
`halobetacache_build`, `libcmt_build`, and `progress` targets pass with zero
semantic unit errors. The lane reports 348/833 completed objects and
3,474/11,060 exact functions; libcmt reports 81/212 objects, 126/476
functions, 8,063 exact code bytes, and 1,728 exact data bytes.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained both functions as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The source uses the real `FILE` fields, typed `wchar_t` interfaces, standard
`va_list`, vertical signatures, natural chained assignments, and one return
per function. It contains no assembly, `volatile`, forced inline, undefined
sequencing, opaque layout, raw offsets, byte forcing, flag changes, or
comparator exceptions. Like its narrow sibling, `vswprintf` is historically
unbounded; defensive callers should determine the size first or use a bounded
wide formatter.
