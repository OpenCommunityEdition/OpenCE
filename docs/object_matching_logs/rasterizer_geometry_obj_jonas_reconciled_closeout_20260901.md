# `rasterizer_geometry.obj` reconciled closeout (2026-09-01)

## Result

Starting from canonical `660bf3421`, this packet reconciles the strongest
Claude/Fable rasterizer-geometry work with the current source, documentation,
header ownership, and house rules. It adds exactly seven strict functions and
2,288 padded bytes, with no lost exact identity anywhere on the board:

| Function | Padded bytes | Result |
|---|---:|---|
| `_compress_real_to_int8` | 96 | strict exact |
| `_compress_real_to_int8_clamp` | 80 | strict exact |
| `_compress_real_to_int16` | 112 | strict exact |
| `_compress_real_to_int16_clamp` | 96 | strict exact |
| `_rasterizer_geometry_get_vertex_size` | 80 | strict exact |
| `_rasterizer_geometry_compress_vertices` | 736 | strict exact |
| `_rasterizer_geometry_uncompress_vertices` | 1,088 | strict exact |

The object advances from `9 exact / 0 residual / 10 unwritten` to
`16 exact / 3 residual / 0 unwritten`. The retained residuals are the natural
vector packing/unpacking implementations already measured and parked as
instruction-scheduling ties:

| Function | Padded bytes | Fuzzy measurement |
|---|---:|---:|
| `_uncompress_int32_to_real_vector3d` | 144 | 95.0% |
| `_compress_real_vector3d_to_int32` | 544 | 96.34682% |
| `_compress_real_vector3d_to_int32_clamp` | 512 | 94.8% |

## Reconciliation and source credibility

The exact scalar compressors use the authenticated `fast_ftol` helper from
`cseries.h`. A donor-local duplicate was removed: current canonical already
owns the shared January helper, and a second definition both failed to compile
and would have been the wrong declaration owner. Inline assembly remains
confined to that small math conversion helper; no rasterizer function body
contains assembly or a code-generation directive. The translation unit still
emits a candidate-only 32-byte, zero-relocation `_fast_ftol` COMDAT from the
authenticated shared helper; it receives no strict credit and prevents a
whole-object completion claim independently of the three parked residuals.

All recovered arithmetic uses `real`, the existing CSeries macros, and named
rasterizer vertex enum constants. Four translation-unit-local vertex layouts
replace raw offsets. The compressed model vertex uses the project-local
`node_weight` spelling found in the sibling render layout. Its low-byte
uncompress operation is not a reconstruction accident: January's instructions
and the HCEA `model_vertex_compressed` donor independently establish the
quantized byte semantics.

January asserts twelve vertex types and indexes a twelve-entry short table.
The six removed enum members were later-engine imports with no source user.
The recovered table is a named file-scope owner,
`rasterizer_vertex_type_sizes`, and `config/symbols.json` marks it static;
there is no `_rdata_...` placeholder.

The cross-translation-unit `uncompress_int32_to_real_vector3d` declaration was
moved out of both C files into `rasterizer_geometry.h`, its actual owner. A
forward declaration of `union real_vector3d` keeps the header self-contained
without importing the broad real-math header. The two bulk converters and the
vertex-size query are declared in the same owner-header section. There are no
address-derived private names, address-derived globals, duplicate prototypes,
manual raw object/tag casts, `point_from_line3d` COMDATs, volatile accesses,
barriers, fake dependencies, or nonsensical control-flow carriers.

## Provenance

The packet was replayed selectively from the Claude/Fable lineage rather than
cherry-picking either broad campaign wave. The principal donor commits are
`37cc580a3`, `6c5cf2fce`, `26d53978d`, `826997e0c`, and `3b016c531`.
January COFF remains the authority for code, relocation, symbol, and owner
acceptance. HCEA and the existing rasterizer layouts corroborate the type,
field, switch, and conversion semantics. The historical donor ledger is kept
for its negative controls, but its intermediate unit-local helper and
`14/19` endpoint are explicitly superseded by this document.

## Verification

- Full configure and Ninja `progress`: pass.
- Strict board: `710,595 / 1,922,413` padded bytes,
  `4,999 / 8,245` functions, and `278 / 619` exact objects.
- Progress accounting: `650,614 / 2,198,102` meaningful code bytes and
  `4,980 / 11,060` functions.
- Rename-aware strict-set comparison against canonical: exactly seven gains,
  2,288 padded bytes, zero losses.
- `rasterizer_geometry.obj`: `16 exact / 3 residual / 0 unwritten`.
- Direct header consumers are unchanged:
  `rasterizer.obj` `131/131`, transparent geometry `17 exact / 2 residual`,
  and Xbox hardware geometry `15 exact / 2 residual`.
- The final owner-header state was replayed through a full incremental Ninja
  build of all 22 direct and transitive dependents reached through
  `structure_bsp_definitions.h`; the strict-set sweep still reports only the
  seven intended gains and zero losses.
- Protected `units.obj`: `189/189`.
- Park validator: `63 active / 0 stale / 0 invalid`.
- Changed-file fake-match scan: zero review leads.
