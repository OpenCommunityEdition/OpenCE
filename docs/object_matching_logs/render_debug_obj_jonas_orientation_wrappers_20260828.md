# `render_debug.obj` Jonas orientation wrappers

## Result

Two ordinary typed orientation wrappers are strict exact. This advances
`source/render/render_debug.obj` from 0/36 to 2/36 exact functions; the object
remains incomplete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_render_debug_quaternion` | 47 / 48 | 2 | `572785a46ecd3c9e01900911a0c30817148d4186da5986311a5307fdb907ff2f` |
| `_render_debug_vectors` | 51 / 64 | 2 | `719bd528dc90885b0817a245523e97c619dc163429b407bee15834662eeb17f8` |

Direct hardened COFF comparison reports `all_equal: true` for every padded
byte, both relocation manifests, and both normalized hashes.

`_render_debug_quaternion` relocates to
`_matrix4x3_from_point_and_quaternion` at `+0x13` and
`_render_debug_matrix` at `+0x24`. `_render_debug_vectors` relocates to
`_matrix4x3_from_point_and_vectors` at `+0x17` and
`_render_debug_matrix` at `+0x28`. All four are zero-addend `REL32` calls.

## Source shape and policy

Each wrapper constructs a typed `real_matrix4x3` local using the appropriate
existing math interface and forwards it to `render_debug_matrix`. Every
parameter is on its own line and both void functions end with an explicit
`return;`.

The implementation introduces no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced compiler option, raw address or offset, type
punning, synthetic caller, byte forcing, object patch, comparator exception,
or tool exception. No source under `source/math` was edited: in particular,
`matrix_math.obj` was not worked on or changed. Vehicles, Units, and the
protected AI and game-engine sources were also untouched.
