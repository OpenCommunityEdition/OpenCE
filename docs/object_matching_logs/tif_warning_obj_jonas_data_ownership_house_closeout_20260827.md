# `tif_warning.obj` Jonas data-ownership and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/bitmaps/libtiff/tif_warning.obj`. It is the warning-side mirror of the
immediately preceding `tif_error.obj` closeout: all three normalized function
payloads already matched, while the setter and variadic dispatcher relocated
through candidate-local `_warningHandler` instead of January's owning data symbol
`_data_002db510+92`.

The unit advances from 1/3 to 3/3 functions and from 96/160 to 160/160 padded
exact bytes. The repository advances from 261/619 to 262/619 fully strict
objects.

## Authority and bounded scope

- Published base: `506c1ebafd81792f2e9457442412021a7c6fe200`.
- Baseline source blob: `b3b0aea01fe367f323e2715ab7efa7bd5269af83`.
- Retained source blob before commit: `0d358552575e22f8ab1cd8e23c411f704865312a`.
- Retained physical source SHA-256:
  `80412a80e9d2322eebb78a910296e164422837ca38047d60f00eb4a6a9ed646b`.
- January split object SHA-256:
  `b46bc6924d11d7a27f4b60def15fc852f708e5d348bd5d72863ca9c2bb0a82b4`.
- Sole candidate object SHA-256:
  `d880583b4b29a00416e90ad4a2340d25c79552dc9c68d3dd44bc5ca649c250c6`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for bytes, padding, relocation identity/addends,
and data ownership. The checked-in vendored libtiff implementation is the
readable-behavior and source-topology authority. The cached Stian donor's
`CLAUDE.md`, `AGENTS.md`, and error-handler provenance were screened; they agree
that this is upstream libtiff's warning handler, setter, and variadic dispatcher.
Their raw-address functional-reimplementation technique was not used.

Only `source/bitmaps/libtiff/tif_warning.c` and this ledger change. No header,
config, compiler flag, comparator, admission rule, completion rule, semantic
allowlist, parked entry, or tool changes.

## Natural ownership recovery

The baseline `.data` layout already contained the exact 92-byte RCS string and
the handler pointer at offset 92. The candidate expressed the offset-zero object
as static `_rcsid` and the pointer as static `_warningHandler`; January owns the
same 96-byte section through external `_data_002db510`.

Renaming only the offset-zero array to `data_002db510` makes all three handler
references resolve through the target owner plus 92. No raw address, manual
offset, aliasing trick, or byte patch is present.

The full owned data section is exact:

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_data_002db510` | 96 | 1 | `07a41223e8996065989d6a942c0f9ab44da0c9850b19a9d6a1e220624059ada9` |

Its relocation is at address 92, type 6, to `_code_0005dfc0` with zero addend.

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_0005dfc0` | 96 | 11 | `e9eaca104c0e7b6ff2a0df199383c78f2f5f9d8a5b420921764c347e06a12dea` |
| `_TIFFSetWarningHandler` | 32 | 2 | `44c1c95e9ae6e7a755e7a42d0bbc78fe9482a71108139d60b394f8dd345ed51d` |
| `_TIFFWarning` | 32 | 1 | `bba7e2d1ec06877905c43c7b297589f68aa1b647cbfcdc1237e4415db32673cf` |

Every padded byte and every relocation address, type, symbolic destination,
addend, and owner matches. The setter relocations at addresses 7 and 13 and the
dispatcher relocation at address 4 all resolve to `_data_002db510+92`.

## House-style normalization

The frozen edit also replaces the legacy `DECLARE1`/`DECLARE3` and conditional
K&R definition syntax with ordinary C89 definitions, one parameter per line.
Both void functions end with explicit `return;`. These source-only cleanups leave
the January code and relocation schedule unchanged.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator/admission exception, or compiler/flag substitution.

## One-shot experiment and gates

Exactly one candidate compile ran:

```text
[1/1] CL build\base\source\bitmaps\libtiff\tif_warning.obj
tif_warning.c
```

It was immediately preserved at
`build/audit/tif_warning_ownership_house_first_shot_20260827.obj`. Direct
hardened comparison accepted all three functions and the complete owned data
section. No source tuning, retry, or retained replay occurred.

Full validation:

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 262/619 exact objects, 4,309/8,246 functions, and
  557,959/1,922,669 padded bytes.
- `tif_warning.obj`: 3/3 functions and 160/160 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,465 functions evaluated, 4,318 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `b24ee40066e2a603e2f6a7753d269ed38acc9a8b787274bba737472fb33e7c16`.
- `build/semantic_report.json` SHA-256:
  `ae7175d08e13e02239e274d128f6dd4807a043b8ee1558581705f33a687a187c`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
