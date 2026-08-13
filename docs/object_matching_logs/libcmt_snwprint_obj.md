# `libs/libcmt/snwprint.obj` exact-match ledger

This atomic lane started from campaign commit `2cb1bb27`. The object was
`MISSING` at config index 716, unparked, and absent from production source,
logs, refs, and worktrees. Root announced the collision-free claim first.

## Plain-English behavior

`_snwprintf` is the variadic bounded-wide formatter. It wraps the caller's
wide buffer in a temporary string-backed `FILE`, scales the wide-character
limit into bytes, calls the shared wide formatter, writes both bytes of the
wide zero terminator, and returns the formatter's count.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\snwprint.obj` member has SHA-256
`129fd544c8958147d7b3213d45f0df3c3989106c63246a932a4edbcf2227e386`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\swprintf.c`. The
authenticated Microsoft source is pinned by Git blob
`e25af72f1c9e35f981c8b6b8c772713c596ec87f`. January remains authoritative.

The object owns one external code COMDAT and no runtime data:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__snwprintf` | 112 | 3 | `286d50e03941a8c7119c339150f6913c7e133e1d9c0c05e2ab1972ad65928447` |

Relocations are `+0x2B -> __woutput+0`, `+0x4B -> __flsbuf+0`, and
`+0x65 -> __flsbuf+0`. Candidate, January, and authentic archive member agree
on every byte, relocation identity/addend, COMDAT attribute, and public owner.

## Experiment matrix

| ID | Source factor | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic variadic bounded-wide topology; byte-scaled count; two ordinary terminator writes | `112/112` | `3/3` | strict exact on first compile | accepted |

No source-shape sweep was needed. This completes the readable narrow/wide,
variadic/`va_list`, bounded/unbounded formatter wrapper family.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt `/O1 /Gy` flags.
`tools.coff_compare` reports `all_equal=true` for both the reconstruction and
the independently extracted authentic archive member. Full
`halobetacache_build`, `libcmt_build`, and `progress` targets pass with zero
semantic unit errors. The lane reports 351/833 completed objects and
3,477/11,060 exact functions; libcmt reports 84/212 objects, 129/476
functions, 8,670 exact code bytes, and 1,728 exact data bytes.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained the function as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The source uses typed wide interfaces, the real `FILE` fields, standard
variadic handling, vertical signatures, named values, and one return. It has
no assembly, `volatile`, forced inline, undefined sequencing, opaque layouts,
raw offsets, byte forcing, flag changes, or comparator exceptions. Historical
bounded formatting may omit the terminator when the count is exhausted; that
legacy contract is preserved and documented rather than silently changed.
