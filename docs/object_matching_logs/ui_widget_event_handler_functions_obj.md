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

## Remaining: 26 functions, 16,208 bytes

All 26 are absent rather than mismatched — not yet written. The work is
concentrated in a few large game-settings handlers:

| bytes | function |
|--:|---|
| 1408 | `_code_000dba40` |
| 1408 | `_code_000db1f0` |
| 1360 | `_code_000dd730` |
| 1296 | `_code_000dcf40` |
| 912 | `_code_000dc630` |
| 848 | `_code_000dcbf0` |
| 784 | `_code_000ddc80` |
| 736 | `_code_000de890` |

These are the option-spinner handlers with the heaviest assert traffic. Their
mangled string symbols decode to the assert text verbatim (`?5` space, `?0`
comma, `?4` period, `?8` apostrophe), so the semantics are largely readable off
the symbol list before touching the disassembly.
