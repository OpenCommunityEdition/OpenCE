# `tif_thunder.obj` Jonas data-owner and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/bitmaps/libtiff/tif_thunder.obj`. Two functions were already strict
exact. The sole residual, `_ThunderDecode`, already matched every normalized
byte and relocation address but referenced the two delta tables by their local
symbols instead of January's owning data symbol plus offsets.

The unit advances from 2/3 to 3/3 functions and from 112/880 to 880/880 padded
exact bytes. The repository advances from 266/619 to 267/619 fully strict
objects, from 4,329 to 4,330 strict functions, and from 560,775 to 561,543
strict padded bytes.

## Authority and bounded scope

- Published base: `d03c4ac00fdb9c80d2ea58c93a1499e21d8b70bd`.
- Baseline source blob: `5412e488b841eb41ffc70e14d7b069c6c1665252`.
- Retained source blob before commit: `992dc32232f3d4d29c9a18c6500c383d3cd9cde9`.
- Retained source SHA-256:
  `4686a17f94ebb633bd7af1dac835c619445f91e98cb7c66f5666e1c11bd80ddf`.
- January object SHA-256:
  `ac687bf67736aa79530fc44cba3630cb778f1f0390521555999a50330cc9c69a`.
- Sole candidate object SHA-256:
  `b733d7f1a260157fe73386993313aeb858812f674da2e65685b7b915ce9a76ad`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocation identity/addends,
and data ownership. The checked-in vendored libtiff implementation is the
readable behavior/topology authority. Historical Claude/Jonas recovery at
`45f8d684` was reviewed: it proved the same 140-byte aggregate but expressed it
as a synthetic covering structure. This wave instead restores the natural
offset-zero owner already present in the source. Only `tif_thunder.c` and this
ledger change; no header, config, compiler flag, comparator, admission rule,
semantic allowlist, parked entry, or tool changes.

## Natural data ownership

The candidate and January objects already contained the same 140-byte `.data`
layout: the 92-byte RCS string at offset zero, `twobitdeltas` at offset 92, and
`threebitdeltas` at offset 108. January owns the section through external
`_data_002db3a0`; renaming the existing offset-zero RCS array restores that
natural owner without adding an aggregate, address, offset, or payload.

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_data_002db3a0` | 140 | 0 | `1bc31b9885824709c305b0fdbf9485eacced4038f09bc9d034a9b946a3a6182b` |

All five table relocations in `_ThunderDecode` now resolve through the owner:
three to offset 92 and two to offset 108.

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_ThunderDecode` | 768 | 16 | `32c753e71ce15de79bbd6bb485c9e4b6b0b713ec4f0d86a2809d842f5c8e47f3` |
| `_ThunderDecodeRow` | 80 | 1 | `2389604501f2363d6123302892ba4835d4a89bd8fb7f3c4ff2150049d4641a36` |
| `_TIFFInitThunderScan` | 32 | 1 | `660b716f40214e66319a91c264d5bea119ef327ed97413579f351e84a6408175` |

Every padded byte and every relocation address, type, destination, owner, and
addend matches.

## House style and frozen compile

All three legacy K&R definitions were converted to ordinary C89 declarations
with explicit return types and one parameter per line. Three inherited
`register` qualifiers were removed. All functions return values on every
terminal path; this object has no void function.

Exactly one candidate compile ran:

```text
[1/1] CL build\base\source\bitmaps\libtiff\tif_thunder.obj
tif_thunder.c
```

It was immediately preserved at
`build/audit/tif_thunder_owner_house_first_shot_20260827.obj`. Direct hardened
comparison accepted all three functions and the complete data section; no
tuning, retry, or retained replay occurred.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator/admission exception, or compiler/flag substitution.

## Full validation

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 267/619 exact objects, 4,330/8,246 functions, and
  561,543/1,922,669 padded bytes.
- `tif_thunder.obj`: 3/3 functions and 880/880 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,467 functions evaluated, 4,339 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `014fad0e65b3e4c87e33c482e02a3549a2b4f292349e14ac980228ba343a66d9`.
- `build/semantic_report.json` SHA-256:
  `65e248d87dc77ba51e380fb4e7dc27dcc9a5c5ee794172b18983271162159d82`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- Tools publication audit: local and remote each contain the same 121 tracked
  `tools/**` paths, with zero changed or untracked tool paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
