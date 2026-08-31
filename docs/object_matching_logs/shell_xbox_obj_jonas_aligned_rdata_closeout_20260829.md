# `shell_xbox.obj` aligned-rdata closeout

## Disposition

`source/shell/shell_xbox.obj` is complete and `source/shell/shell_xbox.c` is
marked `Matching`. All eight functions are strict exact for 437 meaningful / 528
padded code bytes. The rebuilt object also owns all 40 aligned data bytes: 8
bytes of ordinary exact BSS plus 32 bytes in two string COMDATs and the
compiler-generated SEH scope-table COMDAT.

This closeout is based on canonical commit
`c4af544228c14b198bcdbf9397118434f1b37099`. The production source is
unchanged, with SHA-256
`e2595f5b4deb0218eaa1c0333bc71d6174c181c00f35ebf8e937d5112de74f9f`.
The split target object SHA-256 is
`a031df3f8221220242213a247345bf724f97e88e3b5ede1edadada87556764ff`;
after the 2026-08-31 authenticated `cseries.h` declaration restoration, the
rebuilt base object SHA-256 is
`d96e025c99eab103119620b8660f15719c7409f60f4be208d15d2a98a85353cb`.

## Function proof

Direct `tools/coff_compare.py` comparison reports `all_equal: true` for the
complete function inventory:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_shell_platform_initialize` | 93 / 96 | 7 | `c6a92fffb8acd6956bab92a861ac83b26c33d6e1c68a32dcf167b88e02b0bb1e` |
| `_shell_platform_verify` | 5 / 16 | 1 | `ba7d1522ead1d288cfb795c9d1f2bddb3fbcf76832ab6d74b38ba952d1c6a454` |
| `_shell_platform_dispose` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_shell_screen_pause` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_shell_get_command_line` | 6 / 16 | 1 | `9f90b76585a2e9d12439da053c9d502d27587d74cc7441c902dd9c68c789c2fc` |
| `_shell_idle` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_fuck_code_in_the_eye` | 197 / 208 | 6 | `96768235e98f6d99724f744f6bd42ab44b2bfb43bea7863b6b093b0a81d2458d` |
| `_main` | 133 / 144 | 10 | `95f187afcd93ff231b332230226f531403adae11cb1fc9b5d08897329fb966a3` |

The earlier
`shell_xbox_obj_jonas_seh_relocation_provenance_20260826.md` ledger correctly
refused `_main` under the comparator then available. Commit `c00d710e`
subsequently added target-independent SEH provenance handling: it removes only
known link-absolute-zero `__except_list` records after masking their encoded
values and resolves scope-table destinations relative to their containing
source function. Its negative tests preserve different handler destinations
and all unrelated relocations. Under that fail-closed rule `_main` is now
strict exact; this closeout does not add a Shell-specific function exception.

## Aligned data proof

Objdiff merges three aligned `.rdata` input sections into one 32-byte report
span. The semantic-data manifest records and re-verifies every constituent
section independently:

| Target owner | Base owner | Raw / padded bytes | Flags | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| `??_C@_06GAOAIMNN@xdemo?5?$AA@` | same | 7 / 8 | `1076891712` | 0 | `2d636beaf6ef18262eb352ea8a00f3b1b0ee3628666eb07e679892eca5f08b5e` |
| `??_C@_06PFFNFMJI@XDEMOS?$AA@` | same | 7 / 8 | `1076891712` | 0 | `798ec2f79c908eed7153344688b0def452386891f2deae50826a2658182e9bd5` |
| `_rdata_002a0a88` | `$T18267` | 12 / 16 | `1077940288` | 2 | `3414421b183188342209566fad4e0c0723070b4030630dde685ba70281e92725` |

The two scope-table relocations occur at table offsets `+4` and `+8`, with
equal type and source-relative destinations `_main+81` and `_main+99`.
Target and base deliberately retain their distinct producer owner snapshots:
the target owner has storage class 2 and the compiler-local base owner has
storage class 3. The existing 8-byte BSS span is already ordinary exact.

The 2026-08-31 `cseries.h` restoration changed only the compiler-local scope
table spelling from `$T18261` to `$T18267`. This is not inferred from the name:
the current rebuilt section was re-identified by its unique 12-byte `.rdata`
layout, flags, two relocations, and payload hash. Direct source-relative
comparison still proves the same relocation addresses `+4`/`+8`, type 6, and
destinations `_main+81`/`_main+99`. No data credit or exception was expanded.

The grouped verifier is an evidence check, not an equality waiver. It requires
a unique non-code owner at offset zero; equal normalized payload, relocation
shape, and source-relative destinations; equal section name, raw size,
alignment-derived padded size, and flags; exact target/base owner snapshots;
no repeated section; and complete coverage of both the report's unmatched
section inventory and its unmatched data total. Tests reject a changed member,
a changed SEH handler destination, a non-whole source function, a repeated
section, and incomplete report-section coverage.

## Validation

- Full Ninja report, semantic report, and progress: pass.
- Semantic report: 470 units, 4,885 evaluated functions, 4,769 semantic
  exact, 131 hidden exact, 4,779 accepted exact, and zero unit errors.
- Progress: 4,740 strict functions / 602,843 code bytes; 1,856,050 data
  bytes; 384 / 833 linked objects. Shell advances the linked count by one and
  the data total by 32 bytes without changing code credit.
- `build/report.json` SHA-256:
  `0dffae8be6a4fc128bcb9ec7bd318fc3420f5ab582f32dc3aa8076add7533fb6`.
- `build/semantic_report.json` SHA-256:
  `15c73316befdede03757a3d2211738d47122f8f7dfa48c415c57a2cef72fe4ae`.
- Targeted comparator/progress tests: 76 passed; full pytest: 212 passed.
- `_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object-admission audit, parked-function validation, full pytest, and
  `git diff --check`: pass.

No production C source, compiler setting, splitter input, target object, or
object patch changed. The only new mechanism presents complete constituent
COFF evidence that the merged objdiff data row cannot display on its own.
