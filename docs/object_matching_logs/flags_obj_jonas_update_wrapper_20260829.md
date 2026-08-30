# `flags.obj` update-wrapper recovery (Jonas, 2026-08-29)

## Result

`_flags_update` is byte-exact and relocation-exact on its first production
compile:

- 160 padded code bytes;
- nine relocations;
- normalized SHA-256
  `0959db1e95c06bf6dbe512a190137d7a22fba6c3f24071a597c9a16f13b5582e`.

`source/objects/widgets/flags` advances from 5/16 to 6/16 exact functions
and from 144/5888 to 304/5888 strict padded code bytes. The repository
advances to 4727/8246 strict functions and 654503/1922669 strict padded code
bytes.

## Evidence and reconstruction

The January split object is the byte and relocation authority. Its wrapper
iterates `flag_data`, resolves each datum and its `'flag'` tag definition,
increments `updates_since_last_render`, and calls `_flag_update` only for an
attached flag rendered within the last five updates and a nonzero delta.
HCEA's `flags_update.c` independently confirms the topology and field
meanings.

The retained source adds the target-authenticated typed datum prefix through
`definition_index` and uses the canonical data-array and tag APIs. It is
readable C89 and contains no assembly, volatile/register lever, pragma,
intrinsic, attribute, raw address, pointer/integer reconstruction, pun,
undefined behavior, byte forcing, or object patch.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/widgets/flags.obj
build/base/source/objects/widgets/flags.obj _flags_update` reports
`all_equal: true`. All nine relocation addresses, types, identities, addends,
and owners match: three `_flag_data` references, two `_data_next_index`
calls, and one each to `_datum_get`, `_tag_get`, `__real@00000000`, and
`_flag_update`.

## Repository gates

- Full Ninja build/report/semantic/progress: pass with zero semantic unit
  errors.
- Semantic report: 470 units, 4872 evaluated functions, 4745 semantic-exact,
  129 hidden-exact, and 4755 accepted-exact.
- `_unit_preprocess_node_orientations`: remains strict-exact at 1920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission and parked-function audits: pass; parked inventory is 12
  active, zero stale, zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.
