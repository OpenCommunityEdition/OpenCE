# `bitmaps.obj` palette checkpoint

## Result

The original checkpoint and the later semantic-name closure are intentionally
partial; `source/bitmaps/bitmaps.c` remains `NonMatching`.

- `_bitmap_type_get_string`: strict exact, 112 meaningful/padded bytes and 11
  relocations.
- `_bitmap_format_get_string`: strict exact, 118 meaningful bytes, 128 padded
  bytes, and 11 relocations.
- `_global_vector_palette`: strict exact as the complete target-owned `.data`
  section, 1,116 bytes and 21 relocations.
- The original checkpoint gained 230 code bytes and two functions.  The
  2026-08-31 closure below adds another 107 meaningful code bytes and one
  function.  Exact partial data sections are recorded here but are not credited
  globally because the conservative progress ledger only credits a partial
  unit's entire remaining unmatched-data span.

The retained source contains no assembly, volatile access, force-inline
spelling, compiler pragma/barrier, raw offset, or type-punning device.

## Independent target evidence

The January split object and PDB/config symbol map are authoritative. Demon was
used only to identify a candidate 256-entry vector palette. Parsing those 256
words and comparing them with the first 1,024 bytes of the target section gave
the same SHA-256 on both sides:

`b58960eb51a53850a9145e1f7e8c1123af23a47c6e18bf6f723a40323dad1517`

The apparent 92-byte discrepancy is not unexplained palette padding. Target
relocations and the first two target functions prove this exact layout:

| Offset | Size | Meaning | Relocations |
|---:|---:|---|---:|
| `0x000` | 1,024 | 256 writable `pixel32` palette entries | 0 |
| `0x400` | 16 | three bitmap-type names plus a null sentinel | 3 |
| `0x410` | 76 | eighteen bitmap-format names plus a null sentinel | 18 |

`_bitmap_type_get_string` reads the table at `0x400` and checks the sentinel at
`0x40c`. `_bitmap_format_get_string` reads the table at `0x410` and checks the
sentinel at `0x458`. The rebuilt section consequently has the target's exact
size, normalized bytes, relocation addresses/types/destinations/addends, and
public owner `_global_vector_palette`:

`443063631e6958ff5026be3038f8785d61eee4073cb01b8eaa4fdfcc3b620002`

The compiler emits five referenced string literals as candidate COMDAT
definitions that csplit represents as undefined imports because the linked
image coalesced them under another object. They are not part of the target-owned
section claim. Eighteen additional target-owned read-only literal sections
(440 bytes) compare strict exact by their compiler symbol names.

## Semantic table-name closure (2026-08-31)

The former fail-closed boundary below is superseded by the campaign's current
private-symbol naming rule.  January's assertion literal directly preserves
the original identifier `bitmap_format_bits_per_pixel_table`, so the synthetic
csplit owner `_rdata_002544cc` was renamed to the semantic private symbol
`_bitmap_format_bits_per_pixel_table` rather than retaining an address label.

The reconstructed source is ordinary C: a signed `char` table with eighteen
format entries and the established `NONE` terminator, followed by the two
January assertions and the indexed return.  No inline assembly, volatile
access, cast, fake dependency, or control-flow device is present.  The target
function and candidate are strict exact at 107 meaningful / 112 padded bytes,
10 relocations, and normalized SHA-256
`0e1dd0463ca57c4a951d2c590afd0ee746c372c5e57135f372d17076d8ddab76`.
The private table is independently strict exact at 19 bytes, zero relocations,
and normalized SHA-256
`5d5588f9b27e3b0829d2e92bc5e72a8a4ba2ffedd62f9737b488f66845ecfe0f`.

The 19 target bytes are
`08 08 08 10 00 00 10 00 10 10 20 20 00 00 04 08 08 08 FF`.
The final `FF` is the signed `NONE` terminator, not alignment padding.  HCEA's
independent data-boundary reconstruction records the same 19-byte extent and
signed-byte access width, while Stian's independently readable reconstruction
corroborates the eighteen functional entries and assertion behavior.  The
exact source shape was also recovered from the historical bitmap-palette
campaign experiment that originally removed it solely because the old policy
could not reconcile the synthetic owner name.

The public prototype now resides in `bitmaps/bitmaps.h`; consumers include the
owner header instead of redeclaring the function locally.  The header cleanup
preserves the exact-function frontiers of the affected TIFF and Xbox hardware
bitmap translation units.

## Historical fail-closed boundary (superseded 2026-08-31)

`_bitmap_format_get_bits_per_pixel` was reconstructed experimentally with
identical padded bytes and relocation locations/types. It was removed from the
checkpoint because its readable local table symbol did not compare strictly to
csplit's synthetic `_rdata_002544cc` owner. No semantic credit or address-based
identifier was retained to hide that ownership mismatch.

The lane began at integration commit
`a5f816f2d862642e0304e5d488bd23b6c5d948e9`. Full Halo/libcmt builds, the
strict semantic report, object-admission audit, parked-function validation, and
all 179 tool tests passed. A clean one-unit regression snapshot is created only
after the reviewed source and this ledger are committed.
