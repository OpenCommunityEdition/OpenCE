# `libs/libcmt/vsnwprnt.obj` exact-match ledger

This atomic lane started from campaign commit `a955302f`. The object was
`MISSING` at config index 707, unparked, and absent from production source,
logs, refs, and worktrees. Root announced the collision-free claim before
editing.

## Plain-English behavior

`_vsnwprintf` wraps a caller's wide-character buffer in a temporary
string-backed `FILE`, limits the stream to twice the caller's wide-character
count, passes the supplied `va_list` to the wide formatter, and writes the two
zero bytes of a 16-bit terminator. It returns the formatter's count.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\vsnwprnt.obj` member has SHA-256
`94c3284aba5b03c68773995b0383c3d601335229ea38b3066480a06dab694c29`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and the
authenticated Microsoft `crtw32\stdio\vswprint.c` source topology. January
remains authoritative.

The object owns one external pick-no-duplicates code COMDAT and no runtime
data:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__vsnwprintf` | 111 | 3 | `078b3172e2744dac0e2ca139a5b058636c1f95b106c0369622dc339ba3f7f443` |

Relocations are `+0x2A -> __woutput+0`, `+0x4A -> __flsbuf+0`, and
`+0x64 -> __flsbuf+0`. Candidate, January, and authentic archive member match
all bytes, relocation identities/addends, COMDAT attributes, and ownership.

## Experiment matrix

| ID | Source factor | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic bounded-wide topology; count scaled by `sizeof(wchar_t)`; two ordinary terminator writes | `111/111` | `3/3` | strict exact on first compile | accepted |

No code-shape sweep was needed. Together with `vswprint.obj`, this proves both
bounded and unbounded wide `va_list` formatters from the same readable source
family.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt `/O1 /Gy` flags.
`tools.coff_compare` reports `all_equal=true` for the reconstructed candidate
and independently for the authentic archive member. Full
`halobetacache_build`, `libcmt_build`, and `progress` targets pass with zero
semantic unit errors. The lane reports 350/833 completed objects and
3,476/11,060 exact functions; libcmt reports 83/212 objects, 128/476
functions, 8,558 exact code bytes, and 1,728 exact data bytes.

The clean-commit regression snapshot/check forced a fresh XDK rebuild and
retained the function as `still_exact`, with no failures or warnings. The
complete repository tool suite passes 179/179 tests.

## House/Berth and behavior audit

The source uses the real `FILE` structure, typed wide interfaces, standard
`va_list`, vertical signatures, named fields, and one return. It contains no
assembly, `volatile`, forced inline, undefined sequencing, opaque layout, raw
offsets, byte forcing, flag changes, or comparator exceptions. As with the
historical narrow bounded formatter, the terminator is not guaranteed when
the supplied count is exhausted; callers requiring guaranteed termination
must reserve space or terminate explicitly.
