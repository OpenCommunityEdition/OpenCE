# `contrails.obj` Jonas local-cone wrapper

## Result

The typed local-seed cone wrapper is strict exact. This advances
`source/effects/contrails.obj` from 7/19 to 8/19 exact functions; the object
remains incomplete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_local_random_vector_in_cone3d` | 35 / 48 | 2 | `14aed32fbbc92eb83753c9183582737959a5f03aeae1a37ac9bef0635bdcb084` |

The zero-addend `REL32` relocations target
`_get_global_local_random_seed_address` at `+0x14` and
`_seed_random_vector_in_cone3d` at `+0x1A`. Direct hardened comparison reports
`all_equal: true` for all 48 padded bytes, both relocations, and the normalized
hash.

## Provenance and source shape

The canonical inline in `source/math/real_math.h` and the independent Stian
lift agree on the same behavior: obtain the module-local seed and forward the
axis, two cone angles, and typed output vector to the seeded generator. The
inline name is narrowly macro-renamed around `contrails.h`, then undefined, so
the translation unit can own the required ordinary external definition
without changing a shared header.

An adjacent reconstruction of `_contrail_scale_random_value` was tested and
removed. The flattened donor logic emitted 96 rather than 112 padded bytes.
The authentic two-use `contrail_scale_value` helper topology also emitted 96
bytes and introduced an x87 branch absent from January. The retained replay
therefore contains no residual function.

## Policy boundary

The retained implementation is ordinary readable typed C with every parameter
on its own line. It introduces no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, compiler-option change, raw address or offset, type
punning, synthetic caller, byte forcing, object patch, comparator exception,
or tool exception. `matrix_math.obj`, Vehicles, Units, and the protected AI
and game-engine sources were not edited.
