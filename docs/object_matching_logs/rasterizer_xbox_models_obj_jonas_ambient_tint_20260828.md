# `rasterizer_xbox_models.obj` ambient-tint recovery

## Result

`_rasterizer_model_ambient_reflection_tint` is strict semantic-COFF exact. The
unit advances from zero to one exact function and remains honestly
`NonMatching` with 13 functions still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_model_ambient_reflection_tint` | 54 / 64 | 4 | `5859860376536fd6aba18eb99f843f6770bbde9b46f3821ce76061cb879567f1` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`. The four
DIR32 relocations occur at function offsets `+0x04`, `+0x16`, `+0x1E`, and
`+0x2A`; every relocation addresses
`_global_rasterizer_model_ambient_reflection_tint` with addend zero. The
whole-unit gate reports one exact, zero residual, and 13 unwritten functions.

## Typed source and storage ownership

January's instruction stream checks the global color pointer and, when it is
non-null, copies the four `real` arguments into the named alpha, red, green,
and blue fields. The retained body expresses those operations directly.
VC7 naturally reproduces January's integer copy for alpha, x87 loads/stores
for the other components, global-pointer reload schedule, branch, epilogue,
and padding.

The pointer is already declared in `source/rasterizer/rasterizer.h` and owned
by `source/rasterizer/rasterizer.c`, which also allocates the pointed-to
`real_argb_color`. This translation unit only consumes that typed declaration;
it introduces no duplicate data owner or raw storage claim.

January object SHA-256 is
`994b40517a081b740bc93bb4f0b60617fd53b00657bdd175e8666a34e9eb1f15`;
the retained rebuilt raw object SHA-256 is
`220bee600ef9e59527dea6711cb6fd3b480f68b639b4d8afe79ce2e9a6f2b932`.

## Validation and policy

- Complete `halobetacache_build`, `libcmt_build`, and `all_source`: pass.
- Progress: 573,919 / 2,198,102 overall and 561,005 / 1,770,166 Halo code
  bytes; 4,575 / 11,060 overall and 4,408 / 7,574 Halo functions.
- Semantic audit: 470 units, 4,724 functions evaluated, 4,612 accepted, zero
  unit errors.
- Admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Strict board: 274/619 objects, 627,895 / 1,922,669 bytes, and 4,583 / 8,246
  functions after excluding parked and assembly-implemented credit.
- Tooling tests: 205/205 pass; `git diff --check` passes.

The retained implementation is readable typed C with every parameter on its
own line and an explicit terminal return. It introduces no assembly,
attribute, pragma, intrinsic, qualifier trick, raw address, undefined
behavior, compiler option, object patch, semantic exception, or comparator
exception. `matrix_math.obj`, Vehicles, Units, and all user-protected AI and
game-engine sources are untouched.
