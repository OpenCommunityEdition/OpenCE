# `object_lights.obj` structure reconnect recovery (Jonas, 2026-08-29)

## Result

`_lights_reconnect_to_structure_bsp` is byte-exact and relocation-exact on
its first production compile:

- 96 padded code bytes;
- seven relocations;
- normalized SHA-256
  `573e6fdc91ddcdc96231440547fa2cbb6a255d2180b80241ba1a3d02ec86f01d`.

`source/objects/object_lights` advances from 10/43 to 11/43 exact functions
and from 560/12608 to 656/12608 strict padded code bytes. The repository
advances to 4724/8246 strict functions and 653991/1922669 strict padded code
bytes.

## Evidence and reconstruction

The January split disassembly is the byte and relocation authority. It shows
an ordinary `data_next_index` traversal over `light_data`; each datum whose
`_point_light_connected_to_map_bit` is set has that bit cleared before an
out-of-line `light_reconnect_to_map` call. The neighboring already-exact
`_lights_disconnect_from_structure_bsp` proves the same iterator and typed
datum-prefix ownership. HCEA's independently recovered
`lights_reconnect_to_structure_bsp.c` confirms the lifecycle intent, while
the January Xbox wrapper determines the compact retained topology.

The implementation uses only the established `light_datum_prefix`, flag
macros, data-array API, and public reconnect helper. It is readable C89 and
contains no assembly, volatile/register lever, pragma, intrinsic, attribute,
raw address, pointer/integer reconstruction, pun, undefined behavior, or
object patch.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/object_lights.obj
build/base/source/objects/object_lights.obj
_lights_reconnect_to_structure_bsp` reports `all_equal: true`. The seven
relocations match in address, type, identity, addend, and owner: three
`_light_data` references, two `_data_next_index` calls, one `_datum_get`, and
one `_light_reconnect_to_map`.

## Repository gates

- Full Ninja build/report/semantic/progress: pass with zero semantic unit
  errors.
- Semantic report: 470 units, 4869 evaluated functions, 4742 semantic-exact,
  129 hidden-exact, and 4752 accepted-exact.
- `_unit_preprocess_node_orientations`: remains strict-exact at 1920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission and parked-function audits: pass; the parked inventory is
  12 active, zero stale, zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.
