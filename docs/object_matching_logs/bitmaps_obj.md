# `bitmaps.obj` palette checkpoint

## Result

The checkpoint is intentionally partial and `source/bitmaps/bitmaps.c` remains
`NonMatching`.

- `_bitmap_type_get_string`: strict exact, 112 meaningful/padded bytes and 11
  relocations.
- `_bitmap_format_get_string`: strict exact, 118 meaningful bytes, 128 padded
  bytes, and 11 relocations.
- `_global_vector_palette`: strict exact as the complete target-owned `.data`
  section, 1,116 bytes and 21 relocations.
- Ordinary progress gain: 230 code bytes and two functions. The exact partial
  data section is recorded here but is not credited globally because the
  conservative progress ledger only credits a partial unit's entire remaining
  unmatched-data span.

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

## Fail-closed boundary

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
