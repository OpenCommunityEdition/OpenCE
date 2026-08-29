# `stack_walk_windows.obj` Jonas wrapper recovery

## Result

The ordinary public `stack_walk` wrapper is strict exact. This advances
`source/cseries/stack_walk_windows.obj` from 4/16 to 5/16 exact functions;
the object remains incomplete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_stack_walk` | 22 / 32 | 1 | `8d97cbd0530207a81d551bac4bd3d7ef3d80056b18676e059305e595737a7a84` |

The sole zero-addend `REL32` relocation is at `+0x0D` and targets
`_stack_walk_with_context`. Direct hardened COFF comparison reports
`all_equal: true` for all 32 padded bytes, the relocation manifest, and the
normalized hash.

## Source and provenance

The retained source is a typed three-argument forwarding wrapper. It passes
no context, increments the requested number of ignored frames so the wrapper
does not appear in its own trace, and leaves symbol names enabled. A recovered
later-build implementation in `halo/src/halo/cseries/stack_walk_windows.c`
provides independent semantic and call-topology provenance. January's own
callee confirms the parameter ABI: disregard flag, signed 16-bit frame count,
then context pointer.

The current tree's only pre-existing declaration in `source/cseries/cseries.c`
was aligned with the recovered `long` wrapper parameter. Its existing callers
pass `FALSE`, so this corrects the typed interface without changing their call
shape.

## Policy boundary

The implementation is ordinary readable C. Every parameter is on its own
line, the void function ends with an explicit `return;`, and the call uses
typed values. It introduces no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced compiler option, raw address or offset, type
punning, synthetic caller, byte forcing, object patch, comparator exception,
or tool exception. `matrix_math.obj`, Vehicles, Units, and the protected AI
and game-engine sources were not edited.
