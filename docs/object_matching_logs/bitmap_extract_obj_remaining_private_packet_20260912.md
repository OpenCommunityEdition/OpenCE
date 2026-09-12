# `bitmap_extract.obj` remaining private packet (2026-09-12)

## Scope and result

This packet replaces the four remaining unwritten owners in
`source/bitmaps/bitmap_extract.c` with complete, source-level reconstructions.
All four remain fuzzy and receive **zero exact-byte credit**.

The final grouped gate reports 6 exact, 15 residual, and 0 unwritten owners.
The same six owners were exact before this packet, so the strict delta is
**+0 meaningful bytes / +0 padded bytes**, with zero inherited regressions.

| owner | target meaningful / padded | candidate meaningful / padded | target / candidate relocations | status |
| --- | ---: | ---: | ---: | --- |
| `_extract_build_texture_pages_by_sequence` | 599 / 608 | 662 / 672 | 22 / 22 | complete fuzzy |
| `_extract_plateless_cube_map` | 824 / 832 | 832 / 832 | 19 / 19 | complete fuzzy; exact padded size and relocation count |
| `_extract_sequence` | 720 / 720 | 654 / 656 | 28 / 27 | complete fuzzy |
| `_extract_sprites` | 1,089 / 1,104 | 1,091 / 1,104 | 49 / 49 | complete fuzzy; exact padded size and relocation count |

## Evidence and reconstruction notes

- The January split object is authoritative for control flow, constants,
  strings, call order, relocation identities, face transforms, and field
  access widths.
- The Pastudan Xbox reconstruction supplied an independent disassembly cross-
  check for all four January-relative routines (`FUN_00073b00`,
  `FUN_000747d0`, `FUN_00075a20`, and `FUN_00076410`). It was used only as
  supporting evidence, not copied as source.
- Existing `bitmap_extract.c`, `bitmap_group.h`, `texture_page.h`, `data.h`,
  and bitmap utility interfaces establish the native types and ownership
  boundaries. No public header change was needed.
- `_extract_plateless_cube_map` reconstructs the six directly evidenced face
  transforms and copies each face into a temporary 2D bitmap.
- `_extract_build_texture_pages_by_sequence` reconstructs the transactional
  page packing, retry, page-span accounting, and final page shrinking.
- `_extract_sprites` reconstructs budget validation, background selection,
  per-sequence page packing, normalized sprite metadata, page creation, and
  budget reporting. The January cdecl call shape is preserved with a
  translation-unit prototype for the page builder; it is not placed in an
  unrelated header.
- `_extract_sequence` retains the coherent bitmap-run state machine and border
  trimming. January contains a behaviorally redundant branch involving the
  bottom-reference and single-sequence flags. That redundant shape was not
  reproduced merely to steer code generation; the simpler equivalent remains
  explicitly fuzzy under the campaign's anti-fake-match rule.

## Fingerprints

Target normalized SHA-256 values:

- page builder: `70092abc226e6d9400baed2f856500896bf821f08e7ee9c8b9a7691013a4ce44`
- plateless cube map: `f55f10e75310fe5c98c6bff63288c76c13e64850a4ec6ffaa7ed03244d64c2e2`
- sequence: `5f106d5c4f2bb3024df015b0b10b2236b3ffc3818cff7588e4fd3a7c7a810e56`
- sprites: `b853b0998fdec2cdf39cce422d5baa701b09b807da62f236f7c409764437ee0b`

Candidate normalized SHA-256 values:

- page builder: `adcd5aa719f65d006cd4315cb2c70d8254fc9a2fa081c533827c7e65dd565f2a`
- plateless cube map: `a5286bb8ce75c089b2a60e4f1594da25f2df04742ba7fdfc8478539f9a978e54`
- sequence: `662a00511374710cd51cc4b0c7cec0d0d9d2b7248b8ab34656e5c162a725f548`
- sprites: `a400be2de7897a4d4d48395aeab2617f5f9e997fd923a9fa6e71e1b5319147cd`

The final candidate object is
`scratch/bitmap_extract_obj_remaining_private_packet_final.obj`, SHA-256
`0e30ff17736a69b6f72ac88382b7d679c15cd787900cd2ca1c6f2a332ec34778`.
The frozen January target object SHA-256 is
`6d186cfa0163f99d33659c0cef225e17010ca8b613bbd0f257789d705cb976ce`.

## Verification

Final command:

```text
python -B tools/campaign/gate.py source/bitmaps/bitmap_extract --all --forbid-emitted-symbol _point_from_line3d --out scratch/bitmap_extract_obj_remaining_private_packet_final.obj
```

Result:

- emitted-symbol guard passed;
- 6 exact / 15 residual / 0 unwritten;
- all 6 inherited exact owners preserved;
- no exact-byte credit claimed for the four new bodies;
- no configure, full build, commit, or push was performed in this packet.
