# `tif_compress.obj` Jonas provenance and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/bitmaps/libtiff/tif_compress.obj`. All ten January function payloads and
the 140-byte initialized data section were already normalized-equal. Strict
credit was blocked only by the candidate's local RCS/data owner and the readable
names of two private functions.

The unit advances from 0/10 to 10/10 functions and from 0/720 to 720/720 padded
exact bytes. The repository advances from 262/619 to 263/619 fully strict
objects, from 4,309 to 4,319 strict functions, and from 557,959 to 558,679 strict
padded bytes.

## Authority and bounded scope

- Published base: `6e54eff7e4f511e99c8beb1679a1926fb2258252`.
- Baseline source blob: `78b3dfe410929cd7bcb2ba460d9fdc97023d4704`.
- Retained source blob before commit: `02afd75c8d349fc2cd6a847270452ffb81ad0c33`.
- Retained source SHA-256:
  `199c9225cfd045cc874a59b35a32f31811b9e45179d481dee751d55a01ff1938`.
- January object SHA-256:
  `250dda10d498dbf3721f52f15a9ce8d63cf7f1f71c77f5644211d121795c858e`.
- Retained candidate object SHA-256:
  `3da40f961a083343e1314aacd338d5804e86bb283b8c709a5175de7983ae37a6`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocations, addends, symbol
identity, and data ownership. The checked-in vendored libtiff source is the
readable behavior/topology authority. The campaign's Claude/AGENTS tooling rules
and the adjacent strict libtiff ownership closeouts were screened before the
wave. Only `tif_compress.c` and this ledger change; no header, config, compiler
flag, comparator, admission rule, semantic allowlist, parked entry, or tool
changes.

## Natural provenance recovery

The candidate and January objects already contain the same 140-byte `.data`
payload and the same eight relocations. The first 92 bytes are the RCS string;
the four-entry `CompressionSchemes` table begins at offset 92. January owns that
section through external `_data_002b7ef8`, while the candidate previously named
the offset-zero string `_rcsid` and the table `_CompressionSchemes`.

Renaming only the offset-zero array to `data_002b7ef8` restores January's owner
without adding an address or offset. The complete section is strict exact:

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_data_002b7ef8` | 140 | 8 | `fce8978495e5846a1793f5da22a5f184b211c840928d413b1e3f43129a4613d4` |

The two private functions were uniquely identified inside the same object by
their sizes, normalized bytes, call topology, and relocation shapes:

- `findScheme` becomes January's `_code_000535f0`;
- `TIFFNoEncode` becomes January's `_code_00053610`.

All internal calls were renamed at source level. There is no wrapper, raw
address, manual byte placement, object patch, or cross-object substitution.

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000535f0` | 32 | 2 | `44c79cdcf484363777ba90f37cf8ad6943f53628f2765d70ade0d05d7f262830` |
| `_code_00053610` | 64 | 4 | `a8e8b631e0ce58a495784c704310dcb22a2c51c5b60ac90af100fa9a382a6ece` |
| `_TIFFNoRowEncode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFNoStripEncode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFNoTileEncode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFNoDecode` | 64 | 4 | `6b727c1fe518118b69dcd631d6547b4ccf18ec1938a96f946d48efd7a9f5264b` |
| `_TIFFNoRowDecode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFNoStripDecode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFNoTileDecode` | 64 | 5 | `0990b61fe460b2deeb08ae24041a3415d793e87a30e48f5f98f4c8cecc2d1860` |
| `_TIFFSetCompressionScheme` | 176 | 10 | `94b1e7a1a3fd7427497fbceec4c75d1ec140c1f52c6bf943fd95dc1f200552d6` |

Every padded byte and every relocation address, type, destination, owner, and
addend matches.

## House-style normalization and frozen compile

The same frozen edit converts all legacy K&R definitions to ordinary C89
definitions with one parameter per line and restores the explicit `int` return
type on `TIFFSetCompressionScheme`. A final house audit also removes the
inherited `register` qualifier from the private lookup local. No function in
this object returns `void`.

The provenance/definition wave ran one candidate compile:

```text
[1/1] CL build\base\source\bitmaps\libtiff\tif_compress.obj
tif_compress.c
```

It was immediately preserved at
`build/audit/tif_compress_provenance_house_first_shot_20260827.obj`. Direct
hardened comparison accepted all ten functions and the full owned data section.
After the separate house audit removed `register`, one fresh compliance replay
ran the same single production edge and was preserved at
`build/audit/tif_compress_provenance_house_noreg_first_shot_20260827.obj`. It
reconfirmed all ten functions and the owned data section without further source
change or retry.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator/admission exception, or compiler/flag substitution.

## Full validation

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 263/619 exact objects, 4,319/8,246 functions, and
  558,679/1,922,669 padded bytes.
- `tif_compress.obj`: 10/10 functions and 720/720 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,467 functions evaluated, 4,328 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `e75cbef4f35f453e1db52db538c799ed2febefc2263ca1cd51b58fb7ca444b15`.
- `build/semantic_report.json` SHA-256:
  `935145540f47ea79e9d997b16af57f0095d922700897c9c2ad79b43e2c25bd3f`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
