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

## 93/100 -> 94/100 (26,160/29,216 bytes, 89.5%)

Third external checkpoint, verified the same way. **94/100 functions exact,
26,160/29,216 bytes.** `_code_000dd730` (1,360 bytes) closed -- it was one of the
four exact-size, exact-relocation ties.

## Remaining: 6 functions, 3,056 bytes

**Three written, pure codegen ties** -- exact size and exact relocation count, so
shape and call graph are right and only instruction selection differs:

| bytes | relocs | function |
|--:|--:|---|
| 1408 | 106 | `_code_000dba40` |
| 160 | 7 | `_code_000df650` |
| 160 | 7 | `_code_000df6f0` |

The 160-byte pair are near identical to each other, so one insight should close
both.

**Three absent**, ordinary writing work: `_code_000dff10` (672),
`_code_000df9d0` (384), `_code_000da080` (272).

Still worth trying on the three ties: the EAX return-value rule in
`docs/house_rules.md` §6, sweeping signature factors *together* rather than one
at a time. That is what closed `matrix3x3_transpose` and
`matrix3x3_from_forward_and_up` after both had been written off, and
exact-size-with-exact-relocations is the profile where a missed return type
hides.

## 94/100 -> 100/100 (29,216/29,216 bytes)

The final checkpoint closes all six remaining functions. The three absent
functions (`_code_000da080`, `_code_000df9d0`, and `_code_000dff10`) now have
readable C bodies. `_code_000dba40` uses one function-wide result variable,
which gives VC7 the same tail merge as January. `_code_000df650` and
`_code_000df6f0` close from one shared type correction:
`network_game_client_get_local_machine_index` returns `short`, and the locals
which receive it are also `short`.

The persistent-game data block is represented as one named 0x106-byte record
with a two-byte difficulty subobject. Keeping that source-level object boundary
produces January's 16-bit load and preserves the final write without changing
the single csplit-owned data object.

Independent admission audit on 2026-08-07 rebuilt the object with XDK 3911 and
reproduced the checkpoint:

- 100/100 functions pass `section_infos_equal`;
- all 449 January-owned sections pass strict size, relocation-count,
  normalized-byte, and relocation-identity comparison;
- zero target-owned sections are unequal or missing from the candidate;
- no prohibited assembly, `volatile`, `__forceinline`, optimization pragma, or
  compiler-flag override is present.

The candidate object contains additional compiler/debug sections, but they do
not replace or alter any January-owned section. The object is therefore safe to
mark `Matching`.
