# `flags.obj` typed accessor pair (Jonas, 2026-08-29)

## Result

Both adjacent flag-grid accessors are byte-exact and relocation-exact on the
first production compile:

- `_flag_datum_get_vertex`: 176 padded bytes, 12 relocations, normalized
  SHA-256
  `f32333f2f476504f3d2f0b3cb6583f27fcf535c92f9d1ce5896f3ebc38b36891`;
- `_flag_datum_get_cell`: 192 padded bytes, 12 relocations, normalized
  SHA-256
  `f157c11431ea141fe36a7ae2ba7f3715f3c40052fdb6f3f59bcc02feb7885b11`.

`source/objects/widgets/flags` advances from 7/16 to 9/16 exact functions
and from 432/5888 to 800/5888 strict padded code bytes. The repository
advances to 4730/8246 strict functions and 654999/1922669 strict padded code
bytes.

## Evidence and typed reconstruction

The January split object is the code, diagnostic, and relocation authority.
It proves column-major vertex indexing as `x * height + y`, cell indexing as
`x * (height - 1) + y`, the exact assertion identities and source lines, the
24-byte vertex stride, the two-byte cell stride, and the authenticated datum
layout: vertices at the named field after `first_attachment` and cells after
225 vertices. HCEA's recovered flag datum, vertex, cell, and definition
headers independently confirm all field meanings and array extents, although
it provides no standalone accessor bodies.

The retained source extends the already-used datum prefix into a complete
typed flag datum and supplies a typed definition prefix through `width` and
`height`. Access uses named arrays and fields only. It contains no raw-address
or byte-offset dereference, pointer/integer reconstruction, pun, assembly,
volatile/register lever, pragma, intrinsic, attribute, undefined behavior,
byte forcing, or object patch.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/widgets/flags.obj
build/base/source/objects/widgets/flags.obj _flag_datum_get_vertex
_flag_datum_get_cell _flag_render _flags_update` reports `all_equal: true`.
Each accessor has three natural assertions, with matching file/expression
COMDAT ownership plus matching `_display_assert` and `_system_exit` calls.
All 24 relocation addresses, types, identities, addends, and owners agree.

## Repository gates

- Full Ninja build/report/semantic/progress: pass with zero semantic unit
  errors.
- Semantic report: 470 units, 4875 evaluated functions, 4748 semantic-exact,
  129 hidden-exact, and 4758 accepted-exact.
- `_unit_preprocess_node_orientations`: remains strict-exact at 1920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission and parked-function audits: pass; parked inventory is 12
  active, zero stale, zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.
