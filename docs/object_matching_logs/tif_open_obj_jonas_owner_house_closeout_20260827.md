# `tif_open.obj` Jonas owner and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/bitmaps/libtiff/tif_open.obj`. Ten functions were already strict exact;
the remaining three had normalized-equal code but referenced anonymous local
storage or the wrong CRT symbol spellings.

The unit advances from 10/13 to 13/13 functions and from 272/1,152 to
1,152/1,152 padded exact bytes. The repository advances from 265/619 to 266/619
fully strict objects, from 4,326 to 4,329 strict functions, and from 559,895 to
560,775 strict padded bytes.

## Authority and bounded scope

- Published base: `49d00e993099f4b14da4f7955c71c300cb29eb65`.
- Baseline source blob: `9dcd293579138a8429a4a7555b0d46a7eff99eca`.
- Retained source blob before commit: `38fd789a2fc8b99d102d9a9cd39ed10fa460aa43`.
- Retained source SHA-256:
  `7f338e936153f42d75270b655fac39de2c0ae7a3ab9a4f3d9efecd9ae3db94e6`.
- January object SHA-256:
  `49ec64bc5abba2751f521e8cc0db525b3692bcb8e0455b92ebddc6fe7f858306`.
- Sole candidate object SHA-256:
  `9a1eae1524aedfb424c3f7c4a367279fe178e914a41201b2dfd74b66c9cbf8f8`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocation identity/addends,
and data ownership. The checked-in vendored libtiff implementation is the
readable behavior/topology authority. The campaign's current Claude/AGENTS
house and tooling rules were applied across every definition and prototype.
Only `tif_open.c` and this ledger change; no header, config, compiler flag,
comparator, admission rule, semantic allowlist, parked entry, or tool changes.

## Natural ownership and CRT recovery

The candidate and January objects already contained the same 215-byte `.data`
layout: the 88-byte RCS string at offset zero, `typemask` at offset 88,
`bigTypeshift` at offset 140, and the two function-local module strings at
offsets 192 and 204. January owns the section through external
`_data_002dae60`; renaming the existing offset-zero RCS array restores that
natural owner.

The existing 52-byte all-zero little-endian type-shift table is likewise the
natural `_bss_0031bea0` owner. Renaming that array and retaining the readable
`litTypeshift` source name through a preprocessor alias restores the exact BSS
identity without adding storage or reconstructing an address.

`WriteOK` expanded to `write`, while January relocates to `__write`; the sibling
libtiff writers already use `#define write _write`. The descriptor failure path
similarly needed the Xbox-header `_close` identifier so its relocation becomes
`__close`. These are the only behavior-neutral spelling corrections.

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_data_002dae60` | 215 | 0 | `f4f0656cded1bd8c4a9403f223444b577a360bcd1583158a73147ffef77c30ae` |
| `_bss_0031bea0` | 52 | 0 | `7955cb2de90dd9efc6df9fdbf5f5d10c114f4135a9a6b52db1003be749e32f7a` |

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_TIFFInitOrder` | 64 | 3 | `bd3bd923690a629897d660a912fcc36be4defc308d77be2e84e47306781e8f2a` |
| `_getMode` | 80 | 2 | `e7a0447a62eafe90a35a65751850690d1f1ee5712b6b66de0deec426fadd0997` |
| `_TIFFFdOpen` | 656 | 30 | `db6b5df4f97a8ee472a951786a9bdec3d829f8cedc69978e5df80664140e0bf2` |
| `_TIFFScanlineSize` | 48 | 0 | `d074d45de339206e276d7cdb9d4303e81d5d3bc1882328613be359271dab272b` |
| `_TIFFFileName` | 16 | 0 | `2dca21ceb024874aeeb365385c844fa0cee54944f26abad6a81689c682640e13` |
| `_TIFFFileno` | 16 | 0 | `bfb9f796436aa79da58c8b1e0b3d0643e580fd854411b06a2e1c2ec0ffbccf3d` |
| `_TIFFGetMode` | 16 | 0 | `da25d2e0ea825fce59f94041808bb41d2d21d3edbaddf4468e91fae493d19454` |
| `_TIFFIsTiled` | 32 | 0 | `787a2e2db97c785db4ee6fd16bc2ba666e7b6420eee9dbaa032ce639fc637501` |
| `_TIFFCurrentRow` | 16 | 0 | `d400049e092b2775376ada0b87428d2f5e6ff7003608c1d7adb561ded84bb397` |
| `_TIFFCurrentDirectory` | 16 | 0 | `7b9840369c70bb521ae723525f0d0a6087c8e246b88bcfbff8e4dd133ba27091` |
| `_TIFFCurrentStrip` | 16 | 0 | `985147458020cfb5311ecdbacc6725f29fdd51427b6dda7526b9a7e900d725a2` |
| `_TIFFCurrentTile` | 16 | 0 | `52c3ae155c73beecd9a6bcfc260db467a65acef02f9cd52a4c1e264547b58ee2` |
| `_TIFFOpen` | 160 | 8 | `033057ed94a4ebc9e91ab6cf77b819af57b2c07f70b1e77ddc55ce0da2f5dfe7` |

Every padded byte and every relocation address, type, destination, owner, and
addend matches.

## House style and frozen compile

All thirteen legacy definitions and the conditional external prototype were
converted to ordinary C89 declarations with explicit return types and one
parameter per line. The inherited `register` qualifier was removed.
`TIFFInitOrder` is explicitly `void` and has an explicit terminal `return;`.

Exactly one candidate compile ran:

```text
[1/1] CL build\base\source\bitmaps\libtiff\tif_open.obj
tif_open.c
```

It was immediately preserved at
`build/audit/tif_open_owner_house_first_shot_20260827.obj`. Direct hardened
comparison accepted all thirteen functions and both complete owned data
sections; no tuning, retry, or retained replay occurred.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator/admission exception, or compiler/flag substitution.

## Full validation

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 266/619 exact objects, 4,329/8,246 functions, and
  560,775/1,922,669 padded bytes.
- `tif_open.obj`: 13/13 functions and 1,152/1,152 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,467 functions evaluated, 4,338 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `e75cbef4f35f453e1db52db538c799ed2febefc2263ca1cd51b58fb7ca444b15`.
- `build/semantic_report.json` SHA-256:
  `f76651f9661bb626a7ce3f886c26e18a7871d838c49389309aad4db4f741a5cc`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
