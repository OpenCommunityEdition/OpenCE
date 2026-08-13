# `libs/libcmt/calloc.obj` exact-match ledger

This atomic lane starts from campaign commit `0261a4b3`. The object was
`MISSING` at config index 807, absent from `config/parked.json`, and had no
source, object log, topic branch, or active worktree. January owns one
77-byte function and no runtime data.

## Plain-English behavior

`calloc` multiplies the requested element count by the element size and turns
a zero-byte request into one byte. It rejects requests above the CRT heap
limit, rounds requests to 16 bytes for the version-6 heap, and asks the Xbox
process heap for zero-filled memory. If allocation fails and new-handler mode
is enabled, it calls the installed new handler and retries when that handler
succeeds.

## Primary provenance and target inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\calloc.obj` member has SHA-256
`39a75d7e9e931fb84ab780fc519ce10853711e50c274a1e80e8a88c80cb4420a`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\heap\calloc.c`. The public
historical Microsoft Xbox CRT source is pinned by Git blob
`721a4cc4bee5856c8bb13110235a0daf140a947e` and supplies the original control
flow. January bytes remain authoritative.

Strict January inventory:

- `_calloc`: 77 bytes, five relocations, normalized SHA-256
  `254dda758aa80b29770d89a8c79dda44e9a0fe3b533ef2c160d3078bd07cd968`;
- DIR32 relocations at `+0x18` to `___active_heap` and `+0x39` to
  `__newmode`;
- REL32 relocations at `+0x29` to `_GetProcessHeap@0`, `+0x2f` to
  `_RtlAllocateHeap@12`, and `+0x42` to `__callnewh`;
- all relocation addends are zero; `_calloc` is an external function at
  section offset zero; no `.rdata`, `.data`, or BSS is owned.

The extracted XDK member, January target, and rebuilt candidate all pass
`section_infos_equal` for `_calloc`, including all relocation identities.

## Experiment matrix

| ID | Source factor | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic Microsoft retry loop, zero-size normalization, V6-heap paragraph rounding, and new-handler tests with narrow typed Xbox interfaces | `77/77` | `5/5` | strict exact on the first compile; XDK archive member independently has the same runtime section | accepted |

No compiler-shaping sweep was needed. The source uses the original semantic
structure instead of transcribing the disassembly as byte-oriented C.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports exact size,
normalized bytes, relocation count/address/type/destination/addend, external
symbol type/storage/value, and sole-COMDAT ownership. No fuzzy match,
semantic exception, flag change, or comparator adjudication is involved.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
The lane reports 335/833 completed objects overall and 68/212 libcmt objects.
Libcmt has 94/476 exact functions, 5,521/55,015 exact code bytes, and
1,368/8,637 exact data bytes. The semantic scan covers 437 units and 3,531
functions, accepts 3,447 exact functions, and reports zero unit errors. The
admission audit reports no candidates or revocations; its only contradiction
is the pre-existing unrelated `shell_xbox` accounting item.

A clean-commit fail-closed regression snapshot/check followed by a forced
object rebuild retained `_calloc` as `still_exact`, with no failures or
warnings. The complete tool suite passes 179/179 tests.

## House/Berth and original-bug audit

The production source uses named constants, `size_t`, typed API declarations,
readable control flow, and the original Microsoft retry topology. It contains
no assembly, `volatile`, forced inlining, undefined sequencing, type-punning,
opaque padding, alignment trick, magic address, build-flag change, byte patch,
or comparator waiver. The two early returns are the authentic CRT structure;
the compiler cross-jumps them to the single machine epilogue visible in
January.

The original implementation multiplies `number * size` without checking for
unsigned overflow. A sufficiently large request can wrap to a smaller size
and allocate less memory than the caller intended. This shipped behavior is
preserved for exact matching. A defensive implementation would reject
`number != 0 && size > HEAP_MAXIMUM_REQUEST / number` before multiplying, but
that intentionally changes code and runtime behavior.
