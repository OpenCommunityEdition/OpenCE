# `ui_widget.obj` plasma-color setter recovery

## Result

`_set_ui_plasma_effect_color` is strict semantic-COFF exact. The unit advances
from 15/102 to 16/102 exact functions and remains honestly `NonMatching` with
86 functions still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_set_ui_plasma_effect_color` | 39 / 48 | 4 | `0532065f62296e261c20e67ac5df49b5a195094f99ba552b2f6c545ae998fba1` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`. The four
DIR32 relocations occur at function offsets `+0x0D`, `+0x16`, `+0x1C`, and
`+0x21`; they address `_ui_plasma_effect_color` at addends zero, four, eight,
and twelve. The whole-unit gate reports 16 exact, zero residual, and 86
unwritten functions, so all fifteen inherited exact functions remain exact.

## Typed source and storage ownership

January's instruction stream bit-copies the four `real` arguments into the
alpha, red, green, and blue components of one 16-byte ARGB color. The retained
body assigns those named fields directly. VC7 naturally loads the first three
argument words into EAX/ECX/EDX, stores alpha, loads blue, and then stores the
remaining components in January's order.

The owner audit found `_ui_plasma_effect_color` in
`source/linker_common.obj` as one external 16-byte BSS definition.
`ui_widget.c` therefore declares the typed symbol `extern` and makes no data
claim. No duplicate COMMON/BSS owner is introduced, and no symbol metadata or
linker-common source is changed.

January object SHA-256 is
`783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`;
the retained rebuilt raw object SHA-256 is
`eea349b26e452f02875cd4419614f0f10ec46cb3f142ce4fc05d7b05c3e2bd8f`.

## Validation and policy

- Complete `halobetacache_build`, `libcmt_build`, and `all_source`: pass.
- Progress: 573,865 / 2,198,102 overall and 560,951 / 1,770,166 Halo code
  bytes; 4,574 / 11,060 overall and 4,407 / 7,574 Halo functions.
- Semantic audit: 470 units, 4,723 functions evaluated, 4,611 accepted, zero
  unit errors.
- Admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Strict board: 274/619 objects, 627,831 / 1,922,669 bytes, and 4,582 / 8,246
  functions after excluding parked and assembly-implemented credit.
- Tooling tests: 205/205 pass; `git diff --check` passes.

The retained implementation is readable typed C with every parameter on its
own line and an explicit terminal return. It introduces no assembly,
attribute, pragma, intrinsic, qualifier trick, raw address, undefined
behavior, compiler option, object patch, semantic exception, or comparator
exception. `matrix_math.obj`, Vehicles, Units, and all user-protected AI and
game-engine sources are untouched.
