# `geometry.obj` register-cleanup fixed point

This bounded audit closes a previously undocumented retry path for two typed
2D convex-hull residuals. No production source is retained.

The current bodies entered history together in commit
`803bf7918f8ee837a501c54308177b2b70b36610` (`Reconstruct convex hull 2D
tests`). Both use `register` on pointer aliases. Because that qualifier is not
admissible in the strict campaign, exactly one natural policy-clean candidate
per independent function was compiled: remove only the function's `register`
qualifiers and leave every declaration, expression, branch, and lifetime
unchanged.

| Function | Target padded/relocs/hash | Candidate padded/relocs/hash | Result |
| --- | --- | --- | --- |
| `_convex_hull2d_test_point_indexed` | 160 / 0 / `b9502f691fc60f2e201916952a36fbc74ab668c9a6e909789434fdee8b87b494` | 160 / 0 / `dfd0ae8ec3c7a6c11b8ec1c9a3aec5fe5fd5e129e25e5591a47ae341b6970f8f` | Inert; identical to the pre-audit candidate. January pushes EBX/ESI/EDI before the zero-count branch and retains a different x87 expression stack. |
| `_convex_hull2d_test_circle` | 208 / 2 / `7785cf3f04fb5c7f61f3d9449ab75a08e9ef9b34208010754c8f9a8ac4e0bf45` | 208 / 2 / `f81ac4cfa81cb075b975521f82ce539a388c625f3d8262a7120819109c08bf8c` | Inert; identical to the pre-audit candidate. January pushes EBX/ESI/EDI before the zero-count branch and uses a different x87 stack schedule. The second zero-constant relocation remains at candidate `+126` versus target `+132`. |

Both edits were removed immediately. The final `source/math/geometry.c` Git
blob is exactly the integration-base blob
`6640c797bfbd2537630d442f78b55526b748de35`; all seven inherited strict
functions are therefore source-identical. This ledger grants no byte,
function, data, or object credit.

Do not retry qualifier removal, qualifier-free alias declarations, or the same
current expressions. Reopen either function only with a genuinely new
authenticated source topology or compiler provenance that predicts the
observed prologue and x87 transition. Compiler-control devices, expression
steering, undefined behavior, and byte forcing remain out of bounds.
