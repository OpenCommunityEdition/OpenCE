# `flags.obj` render-wrapper recovery (Jonas, 2026-08-29)

## Result

`_flag_render` is byte-exact and relocation-exact on its first production
compile:

- 128 padded code bytes;
- six relocations;
- normalized SHA-256
  `2e5d366571419d844d1c918a6a6a10817fcad75bacc06a9cdd2bd247c1f62420`.

`source/objects/widgets/flags` advances from 6/16 to 7/16 exact functions
and from 304/5888 to 432/5888 strict padded code bytes. The repository
advances to 4728/8246 strict functions and 654631/1922669 strict padded code
bytes.

## Evidence and reconstruction

The January split object is the byte and relocation authority. It validates
the owning object, resolves the flag datum and `'flag'` tag definition,
records the current object index, refreshes stale or uninitialized simulation
state with a five-second update, resets the render-age counter, and calls the
proper renderer unless the datum's `noop` byte suppresses drawing. HCEA's
`flag_render.c` independently confirms the field semantics and branch
topology; January additionally proves the ordinary object validation call.

The retained source reuses the typed datum prefix introduced for the exact
update wrapper and canonical object, data-array, and tag APIs. It is readable
C89 and contains no assembly, volatile/register lever, pragma, intrinsic,
attribute, raw address, pointer/integer reconstruction, pun, undefined
behavior, byte forcing, or object patch.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/widgets/flags.obj
build/base/source/objects/widgets/flags.obj _flag_render _flags_update`
reports `all_equal: true`. The six render relocations match in address, type,
identity, addend, and owner: `_object_get_and_verify_type`, `_flag_data`,
`_datum_get`, `_tag_get`, `_flag_update`, and `_flag_render_proper`.

## Repository gates

- Full Ninja build/report/semantic/progress: pass with zero semantic unit
  errors.
- Semantic report: 470 units, 4873 evaluated functions, 4746 semantic-exact,
  129 hidden-exact, and 4756 accepted-exact.
- `_unit_preprocess_node_orientations`: remains strict-exact at 1920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission and parked-function audits: pass; parked inventory is 12
  active, zero stale, zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.
