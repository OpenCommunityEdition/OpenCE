# `stack_walk_windows.obj` Jonas symbol lookup recovery

## Result

The symbol-to-base lookup helper is strict exact. This advances
`source/cseries/stack_walk_windows.obj` from 6/16 to 7/16 exact functions;
the object remains incomplete with zero residual functions.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_base_address_from_symbol_name` | 91 / 96 | 1 | `01c8e5c83906350d200888c1437b2825a1f74d90314da430cf9674c8a0f5638b` |

Direct hardened `tools/coff_compare.py` comparison reports `all_equal: true`
for all 96 padded bytes, the zero-addend `_csstrcmp` relocation at `+0x32`,
and the normalized hash.

## Source shape

The ordinary typed helper starts at symbol index one, compares each symbol's
stored name against the requested name, and retains the matching RVA base.
An explicit `symbol_name` local reproduces January's temporary lifetime and
register allocation without any representation or code-generation device.

The adjacent `_symbol_name_from_address` formatter was inspected but not
implemented. Its target owns a 0x4008-byte anonymous BSS allocation whose
semantic field layout is not yet established. It remains unwritten rather
than introducing an address-named global or raw-offset access.

## Policy boundary

The retained implementation is ordinary readable typed C with every parameter
on its own line. It introduces no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, compiler-option change, raw address
or offset, undefined behavior, type punning, synthetic caller, byte forcing,
object patch, comparator exception, or tool exception. `matrix_math.obj`,
Vehicles, Units, and the protected AI and game-engine sources were not edited.
