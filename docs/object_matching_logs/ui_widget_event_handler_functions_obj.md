# ui_widget_event_handler_functions.obj

January: 100 functions, 29,216 `.text` bytes. Was the largest object in the tree
with no reconstruction at all — the source file was a bare skeleton, symbol-list
comment and empty banner sections, not even an `#include`.

## 0/100 -> 74/100 (13,008/29,216 bytes, 44.5%)

Contributed externally, then verified here independently: their C rebuilt with
this repository's XDK CL and compared with `tools/coff_compare.py` against
`build/split/source/interface/ui_widget_event_handler_functions.obj`. **74/100
functions exact, 13,008/29,216 bytes.** The claimed figure reproduces exactly.

Pure C. Checked for prohibited constructs and it is clean — no `__asm`, no
`volatile`, no `__forceinline`, no `#pragma optimize`, no compiler-switch
changes. The one pragma present is `#pragma pack(push, 2)` around
`event_handler_globals`, which is a struct-layout directive rather than a codegen
one and is the ordinary way to land a trailing `short` at the right offset.

All 74 matched by symbol name, so the `code_000dXXXX` placeholders were left
alone as required.

Their notes record a packaging error in an earlier 72-function candidate, where
experimental bodies had been appended to the shipped source after the object was
compiled, so source and object disagreed. That was corrected here by recompiling
a pre-experiment snapshot, which also recovered `_code_000d9040` and
`_code_000d9210`. Worth repeating as a general caution: ship the source that
produced the object, and re-derive the numbers from it.

## 74/100 -> 93/100 (24,800/29,216 bytes, 84.9%)

Second external checkpoint, verified the same way: their C rebuilt with this
repository's XDK CL, compared with `tools/coff_compare.py`. **93/100 functions
exact, 24,800/29,216 bytes.** Reproduces exactly. The shipped header is byte
identical to the repository's, so nothing outside this object is touched.

Clean against the prohibitions again -- the only pragma is the same
`#pragma pack(push, 2)` around `event_handler_globals`.

## Remaining: 7 functions, 4,416 bytes

Split into two very different piles.

**Four are written and are pure codegen ties** -- exact size *and* exact
relocation count against January, so the shape and the call graph are already
right and only instruction selection differs:

| bytes | relocs | function |
|--:|--:|---|
| 1408 | 106 | `_code_000dba40` |
| 1360 | 85 | `_code_000dd730` |
| 160 | 7 | `_code_000df650` |
| 160 | 7 | `_code_000df6f0` |

The two 160-byte pair differ by nine normalized bytes each and are near
identical to one another, so whatever fixes one very likely fixes both.

**Three are absent** and are ordinary writing work: `_code_000dff10` (672),
`_code_000df9d0` (384), `_code_000da080` (272).

Worth trying on the four ties before anything else: the EAX return-value rule in
`docs/house_rules.md` §6, and sweeping the signature factors *together* rather
than one at a time. That combination is what closed both `matrix3x3_transpose`
and `matrix3x3_from_forward_and_up` after each had been written off as an
unreachable register permutation, and exact-size-with-exact-relocations is
precisely the profile where a missed return type hides.
