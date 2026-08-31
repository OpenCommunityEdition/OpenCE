# `bipeds.obj` export-function-values leaf (2026-08-31)

## Result

`_biped_export_function_values` is strict exact at 183 meaningful / 192
padded bytes, eight ordered relocations, and normalized SHA-256
`00969eae2e4d97a1b9bbeda3aa4e6a61e11ab52eb89b3aee27d60a8d80028c0c`.
`bipeds.obj` advances from 13 to 14 exact functions.

## Provenance and ownership

The readable HCEA body supplies the typed loop over four incoming object
function values. HCEA Release PDB records independently authenticate the enum
constants `_biped_function_none = 0` and
`_biped_function_flying_speed = 1`; this corrects the unauthenticated
`flying_velocity` spelling in the Claude draft. The target packet confirms
the magnitude, flying-speed normalization, and `PIN(..., 0.0f, 1.0f)` flow.

The two proven constants live in `source/units/biped_definitions.h`, the
public prototype lives in `source/units/bipeds.h`, and the duplicate foreign
prototype was removed from `source/objects/object_types.c`. Gates for all
direct consumers of those headers preserve their prior function packets,
including protected `units.obj` at 189/189.

The retained source is ordinary typed C using `real`, the canonical math
helper, named fields, named enum constants, and the `PIN` macro. It contains
no raw offsets, anonymous address names, representation punning, forced
inlining, schedule-only alias, compiler barrier, or inline assembly.
