# `rasterizer_xbox_water.obj` frame-visibility recovery

## Result

`_rasterizer_water_set_visibility_for_frame` is strict semantic-COFF exact.
The unit advances from 2/14 to 3/14 exact functions and from 32 to 64 exact
padded code bytes. Eleven functions remain unwritten, so the object stays
honestly `NonMatching`.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_water_set_visibility_for_frame` | 24 / 32 | 2 | `eb1f352ebb6c371c3d4de9a07f816ac25e84e135fe9da40f0a41ed9b5031ebb6` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`. The two
DIR32 relocations occur at function offsets `+0x0D` and `+0x12` and address
the existing aggregate at offsets zero and one. The whole-unit gate reports
three exact, zero residual, and eleven unwritten functions, proving that both
previously admitted window-visibility leaves remain exact.

## Source evidence and ownership boundary

The prior visibility-leaf ledger and HCEA semantic donor establish the named
two-byte state: frame visibility derives `needs_update`, and the original
visibility byte is retained for the window. January's decisive low-byte shape
is expressed directly by assigning `!visibility` before assigning
`visibility`; VC7 preserves the argument in AL, materializes the logical
negation in CL, and emits January's store order.

The existing source deliberately retains one external two-byte COMMON owner.
This wave makes no `.bss` data claim. A source-private aggregate and two
source-private bytes were checked only to resolve ownership provenance: they
reproduced January's byte-aligned section but failed strict storage/relocation
identity. Those variants were removed. No symbol metadata or comparator rule
was changed.

January object SHA-256 is
`0d6480e2b845e331b0919dcd5e7d49ba2e11987d54634102c7435353bceaca37`;
the final rebuilt raw object SHA-256 is
`0b3d697880b81c792404adcffbfd862dff4b3fa4439c1a994db0d127cd424d92`.

## Validation and policy

The combined three-leaf wave passed complete `halobetacache_build`,
`libcmt_build`, and `all_source` graphs. Fresh progress reports 573,826 /
2,198,102 overall and 560,912 / 1,770,166 Halo code bytes. Semantic audit
evaluates 4,722 functions, accepts 4,610, and reports zero unit errors.
Admission is 0 candidates / 0 contradictions / 0 revocations; parks are 13
active / 0 stale / 0 invalid; the strict board reports 627,783 / 1,922,669
bytes and 4,581 / 8,246 functions; and all 205 tooling tests pass.

The retained function is readable typed C with one parameter per line and an
explicit terminal return. It adds no assembly, qualifier trick, attribute,
pragma, intrinsic, raw address, undefined behavior, compiler option, object
patch, semantic exception, or comparator exception. `matrix_math.obj`,
Vehicles, Units, and all protected AI/game-engine sources are untouched.
