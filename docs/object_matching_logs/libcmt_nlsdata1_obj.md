# `libcmt/nlsdata1.obj` exact-match ledger

## Plain-English behavior

This data-only CRT object describes the default C locale: multibyte characters
are one byte wide, the decimal separator is `.`, and that separator has length
one. It contains no executable code.

## January and XDK provenance

January owns one 12-byte writable `.data` section with three external symbols:
`___mb_cur_max` at offset 0, `___decimal_point` at offset 4, and
`___decimal_point_length` at offset 8. The bytes are
`01 00 00 00 2E 00 00 00 01 00 00 00`, with no relocations.

XDK 3911 `libcmt.lib` contains `obj\\i386\\nlsdata1.obj`. Its embedded
records identify `..\\nlsdata1.c` and Microsoft VC7, and its 12-byte runtime
section, symbol offsets, flags, and bytes are identical to January. Therefore
the three natural C declarations are original-source reconstruction, not a
layout guess.

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| E01 | Two initialized integers around `char __decimal_point[4] = "."` | Strict exact on first build |

The fresh XDK 3911 build proves 12/12 bytes, 0/0 relocations, and identical
normalized SHA-256
`45fb2b0c15987b86b53ead635e76e203f8cfad29956dffb04a0b25afed5baefb`.
Both `.data` sections have flags `0xC0300040`. All three symbols are external,
type zero, storage class `2`, at the proven offsets 0, 4, and 8.
`section_infos_equal` is `true`; neither side owns any other runtime section.

## House/Berth audit

- Each global has a semantic locale name and ordinary C type.
- The four-byte character array is proven by the next symbol's offset and the
  exact XDK member, not inserted merely as padding.
- No packing, alignment override, raw offset, assembly, `volatile`, undefined
  behavior, byte forcing, compiler flag change, or comparator exception is
  used.
- This canonical C-locale data contains no original-game bug to annotate.

Admission requires exact data bytes, flags, alignment, all three symbol
identities/offsets/storage classes, full builds, zero semantic errors, a clean
forced-rebuild regression check, and all 179 project tests.
