# `tif_read.obj` Jonas data-owner and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/bitmaps/libtiff/tif_read.obj`. Ten functions were already strict exact;
the other five were normalized-equal but referenced function-local module
strings instead of January's owning data symbol `_data_002db040` plus their
natural offsets.

The unit advances from 10/15 to 15/15 functions and from 1,552/2,448 to
2,448/2,448 padded exact bytes. The repository advances from 264/619 to 265/619
fully strict objects, from 4,321 to 4,326 strict functions, and from 558,999 to
559,895 strict padded bytes.

## Authority and bounded scope

- Published base: `b573761441e58e6c85b9798cf7771005a3bfebcf`.
- Baseline source blob: `a3a3e081bc25ad99b670069654c88dca21f5a929`.
- Retained source blob before commit: `99371acecf02149923b37886835472a6dd5cf68a`.
- Retained source SHA-256:
  `43550b960fe78eaeec2ab48d1fbc1fce0ab5e1af79a75485376f7875e8ebbe10`.
- January object SHA-256:
  `3788ab2d1eff274f5446b020d3b9697e38038a6fafe98a052d201343eccb05fa`.
- Sole candidate object SHA-256:
  `2a80b5f9806277d3fed4291f0af2269ba9649a3426d56f54193c03941ad58b62`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocation identity/addends,
and data ownership. The checked-in vendored libtiff implementation is the
readable behavior/topology authority. The campaign's current Claude/AGENTS
house and tooling rules were applied across every definition and prototype.
Only `tif_read.c` and this ledger change; no header, config, compiler flag,
comparator, admission rule, semantic allowlist, parked entry, or tool changes.

## Natural ownership recovery

The candidate and January objects already contained the same 176-byte `.data`
layout: the 88-byte RCS string at offset zero followed by five function-local
module strings at offsets 88, 108, 124, 140, and 156. January owns the section
through external `_data_002db040`, while the candidate previously named the
offset-zero string local `_rcsid`.

Renaming that existing array to `data_002db040` restores the natural owner. No
address, manual offset, added payload, aliasing trick, or linker/compiler change
is present. The complete section is strict exact:

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_data_002db040` | 176 | 0 | `3ca6df6986dd074eca60b6a76dc0120ee48d7f225b39a6d9fe08672bdef597bb` |

The recovered functions are `TIFFReadRawStrip`, `TIFFFillStrip`,
`TIFFReadRawTile`, `TIFFFillTile`, and `TIFFReadBufferSetup`. Their module-string
relocations now resolve through `_data_002db040` at the exact January offsets.

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_TIFFReadRawStrip1` | 144 | 6 | `38d3303b15b1abda1daba6b274663c9ea4a29b75776503f0ac0f9b07e5c3f371` |
| `_TIFFReadRawTile1` | 160 | 6 | `bbaed6641aa56fca094b2152d94f156afd58f57b5f537b08f065871c803bd863` |
| `_TIFFReadBufferSetup` | 192 | 7 | `c98fcd2ce9712e1d949950bdb3939150a8ff93276f4c0ebb56bf56e2f37d4fd9` |
| `_TIFFStartStrip` | 80 | 0 | `da132d8ec308e62f26dd75f95f9d6a7d07ecad594c7011c169ac6bd2554c0d01` |
| `_TIFFStartTile` | 128 | 0 | `2f164189e4b48d3ac562a095970ea261e4c5fcd7f86f9e317d217adffee04ff2` |
| `_TIFFCheckRead` | 80 | 5 | `e4741e9dcf1cd9fbfbf62d3e9e62b1a27bce4b21a11ec83ef636f65c6babdb7a` |
| `_TIFFReadRawStrip` | 160 | 8 | `7952983752dc5204b32e3c57a3e4c6d75ecc1ad2e18910d6fff99dbaf87a7d6c` |
| `_TIFFFillStrip` | 192 | 8 | `a5c11641108d07317057aafc920d598d572b4f821f1b4ab9718170f79ddc94bd` |
| `_TIFFReadRawTile` | 160 | 8 | `c1aa3e281087c2c6a166972486c52a1862dc3fc2f7422abc1fd53f960afb8fc5` |
| `_TIFFFillTile` | 192 | 8 | `4c82ce60fb01adedd8a19421731740e3096678a78c0d8d498ec584da4288ab8e` |
| `_TIFFSeek` | 208 | 8 | `45477166e1f4ad7c98993fef16af248462b4c14ab289be64e7f89ad70ce25052` |
| `_TIFFReadEncodedStrip` | 192 | 8 | `9234cbc4b97ae76adbec98c47bf0bc25085b3dd6c4840abe5010630c03874330` |
| `_TIFFReadTile` | 224 | 9 | `54736e2fbc1a03b848dcae1efb6eede24afba6141a55a77934112fa7a44adf92` |
| `_TIFFReadEncodedTile` | 192 | 7 | `78fe18f45487f908d208f60e6d738ac9aaf73841800f98dae7897e6739b17526` |
| `_TIFFReadScanline` | 144 | 5 | `814d989ec76b5dc378f4bbf9fda38f8eccba44c0431bccd55de1233a1d207b29` |

Every padded byte and every relocation address, type, destination, owner, and
addend matches.

## House style and frozen compile

All legacy K&R definitions and conditional prototypes were converted to
ordinary C89 declarations with explicit `int` return types and one parameter per
line. Four inherited `register` qualifiers were removed. Every function already
returns a value on all terminal paths; this object has no void function.

Exactly one candidate compile ran:

```text
[1/1] CL build\base\source\bitmaps\libtiff\tif_read.obj
tif_read.c
```

It was immediately preserved at
`build/audit/tif_read_owner_house_first_shot_20260827.obj`. Direct hardened
comparison accepted all fifteen functions and the complete owned data section;
no tuning, retry, or retained replay occurred.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator/admission exception, or compiler/flag substitution.

## Full validation

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 265/619 exact objects, 4,326/8,246 functions, and
  559,895/1,922,669 padded bytes.
- `tif_read.obj`: 15/15 functions and 2,448/2,448 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,467 functions evaluated, 4,335 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `e75cbef4f35f453e1db52db538c799ed2febefc2263ca1cd51b58fb7ca444b15`.
- `build/semantic_report.json` SHA-256:
  `79e294479f311246e137a89dbdc7be2ae8c0f8563db4fb8a3be01a10b20d1f8d`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
