# `glow.obj` delete recovery (Jonas, 2026-08-29)

## Result

`_glow_delete` is byte-exact and relocation-exact on its first production
candidate:

- 80 padded code bytes;
- six relocations;
- normalized SHA-256
  `cbe403b6bc67b7d867bee2c3dd8b6429d7e8eedaf0d2868f4089b57e350b202b`.

`source/objects/widgets/glow` advances from 7/25 to 8/25 exact functions and
from 640/7184 to 720/7184 strict padded code bytes. The repository advances to
4723/8246 strict functions and 653895/1922669 strict padded code bytes.

## Source authority and reconstruction

The January split object is the byte and relocation authority. Its disassembly
shows the glow datum lookup, a forward traversal beginning at `head_particle`,
deletion of each particle by its stored datum index, and final deletion of the
glow datum. The HCEA donor at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/glow_delete.c`
independently confirms that topology and the field meanings.

The retained implementation uses private, typed `glow_datum` and
`glow_particle` layouts with named fields. It contains no raw-address access,
pointer/integer reconstruction, assembly, intrinsic, pragma, compiler
attribute, volatile/register lever, or object patch.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/widgets/glow.obj
build/base/source/objects/widgets/glow.obj _glow_delete` reports
`all_equal: true`. All relocation addresses, types, identities, addends, and
owners agree:

- `_glow_globals+0` at `+0x04` and `+0x3F`;
- `_glow_globals+4` at `+0x26`;
- `_datum_get` at `+0x0F`;
- `_datum_delete` at `+0x30` and `+0x46`.

## Repository gates

- Full Ninja build/report/semantic/progress: pass with zero semantic unit
  errors.
- Semantic report: 470 units, 4868 evaluated functions, 4741 semantic-exact,
  129 hidden-exact, and 4751 accepted-exact.
- `_unit_preprocess_node_orientations`: still strict-exact at 1920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission: pass with zero candidates, contradictions, or revocations.
- Parked-function audit: pass with 12 active, zero stale, and zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.
