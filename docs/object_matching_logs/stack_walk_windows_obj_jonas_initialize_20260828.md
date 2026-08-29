# `stack_walk_windows.obj` Jonas initializer recovery

## Result

The public stack-walker initializer is strict exact. This advances
`source/cseries/stack_walk_windows.obj` from 5/16 to 6/16 exact functions;
the object remains incomplete with zero residual functions.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_stack_walk_initialize` | 44 / 48 | 6 | `44667aa7e2551f12e73bf73f88ccba590f2f83ac05542faa386a2d51645c0bd6` |

Direct hardened `tools/coff_compare.py` comparison reports `all_equal: true`
for all 48 padded bytes, all six relocation records, and the normalized hash.
The relocations identify the build timestamp, `stack_walk_globals.symbol_table`,
the cache map path, `_load_symbol_table`, and the two fixup accesses.

## Provenance and source shape

January's relocation manifest establishes the three typed arguments and their
order: `"d:\\cachebeta.map"`, the address of the global symbol table, and
`"Mon Dec 17 12:49:36 2001"`. The ignored return value is followed by the
target's direct `NONE` check and zero fallback for the global fixup. The
independent recovered later-build stack-walker confirms the hardcoded map path
and the initializer's role, while January's COFF remains the byte authority.

## Policy boundary

The retained implementation is ordinary readable typed C. Every parameter is
on its own line, the no-argument function uses `void` on its own line, and the
function ends with an explicit `return;`. It introduces no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, optimizer barrier,
compiler-option change, raw address or offset, undefined behavior, type
punning, synthetic caller, byte forcing, object patch, comparator exception,
or tool exception. `matrix_math.obj`, Vehicles, Units, and the protected AI
and game-engine sources were not edited.
