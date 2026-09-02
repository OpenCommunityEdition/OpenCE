# `structure_detail_objects.obj` small-family completion (2026-09-01)

## Outcome

The January object advances from 5/15 to 12/15 strict-exact functions. All ten
previously unwritten functions now have evidence-backed source; seven small
functions are exact and the three remaining routines are explicitly retained
as fuzzy reconstructions. Strict matched padded code rises from 176 to 672
bytes, a gain of 496 bytes. The remaining 1,968 padded bytes are defined C,
not empty shells.

| Newly exact function | Padded bytes | Relocations |
| --- | ---: | ---: |
| `_calculate_world_from_cell_index_and_offset` | 32 | 2 |
| `_get_local_player_datum` | 64 | 5 |
| `_detail_object_offset` | 48 | 4 |
| `_key_compare_cells_lower_bound` | 48 | 0 |
| `_key_compare_cells_upper_bound` | 48 | 0 |
| `_get_lower_bound_cell` | 128 | 0 |
| `_get_upper_bound_cell` | 128 | 0 |

The existing exact dispose, flush, `dot_product4d`, and new-map initializer
functions remain exact. No exact sibling was traded for the gain.

## Donor and name audit

The campaign census inspected every registered local worktree and branch and
every tracked source blob. None contained a more advanced January candidate
than the canonical source. The later HCEA corpus supplies named, typed
cross-build reconstructions for the initializer, offset helper, both cell
comparators, both binary searches, the renderer, and the debug traversal. In
particular, these filenames recover the six formerly address-derived private
names:

- `calculate_world_from_cell_index_and_offset.c`;
- `get_local_player_datum.c`;
- `key_compare_cells_lower_bound.c`;
- `key_compare_cells_upper_bound.c`;
- `get_lower_bound_cell.c`;
- `get_upper_bound_cell.c`.

The independent PC reconstruction at
`research-cache/pastudan-halo-20260828/src/halo/structures/structure_detail_objects.c`
preserves the January renderer's packed-cell topology, exact assertion line,
intrinsic `memset`/`abs` patterns, x87 rounding, and target-specific debug
drawing path. HCEA independently corroborates the visible-cell algorithm and
the debug function's gating, cell traversal, reference-vector update, and
one-shot state reset. No direct Marathon analogue exists; the open Marathon
corpus was still checked for naming, type, and house-style evidence.

`config/symbols.json` now uses the recovered private names and marks those six
functions static. The C file contains named cell, layer, view, runtime, packed
detail-object, and BSP detail-data layouts with compile-time size checks. Tag
block reads go through typed subsystem macros. Public declarations live in
the closest owning headers: `structures/structures.h` for this compiland's
public entry points and `render/render_debug.h` for
`render_debug_box_outline`. The duplicate use-site declaration in `render.c`
was removed, and the rasterizer detail-object wrappers now expose the actual
`detail_object_view_data *` ABI rather than invented opaque address-style
types.

## Retained fuzzy boundary

### `_structure_detail_objects_initialize`

Target and candidate are both 80 padded bytes with three relocations. The raw
normalized-byte diagnostic reports seven differing bytes. January cleans the
three allocation arguments from the stack before publishing the returned
runtime pointer; VC7 publishes the pointer first and then adjusts ESP. The
direct-global spelling grows to 96 bytes and six relocations, while natural
declaration, assignment, aggregate, and temporary-pointer variants either
return to the retained fixed point or perturb more code. This is parked as
`instruction-scheduling`.

### `_structure_render_detail_objects`

Target and candidate are both 1,136 padded bytes with 31 relocations. The
retained C reconstructs the complete single-player guard, camera-cell x87
rounding, 3x3 neighborhood search, vertical filter, per-layer population,
reference-vector selection, view compaction, vertex rebuild, draw, and end
calls. The six private helpers it uses are themselves strict exact. Remaining
differences are broad VC7 frame/register/x87 scheduling, including one
reciprocal-versus-count-access relocation schedule swap. Natural runtime/view
aliases, scopes, loop spellings, fallback-global access, and reciprocal
placement were boundedly tested; variants either return to this fixed point,
regress exact siblings, or require unsupported alias-only source. This is
parked as `tu-context-optimization`.

### `_render_debug_detail_objects`

The candidate is 736 padded bytes against January's 752, and both carry 33
relocations in the same semantic destination sequence. The complete retained
logic decodes packed positions, computes world coordinates and the reference
plane, flags vertical clipping, draws every point and cell box, overlays
clipped cells, and resets the debug pulse. A natural `dot_product4d` spelling
recovers the 752-byte envelope but adds an unsupported 34th multiply-by-one
relocation, so it is rejected. Direct-vector, declaration/scope,
bounds-arithmetic, comparison-order, view-access, and loop-shape variants do
not improve the strict topology. This is parked as
`tu-context-optimization`.

The manifest's `objdiff_percent` values are exactly the freshly regenerated
campaign report metric consumed by `tools.parked_functions`: 76.36364%,
76.39528%, and 78.78802% in the order above. The
seven-byte/888-byte/598-byte figures above and in the work ledger come from
the separate raw normalized-byte diagnostic; they are not park-manifest
percentages and must not be substituted for the campaign metric.

## Strict measurements

| Function | Artifact | Padded size | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_structure_detail_objects_initialize` | January | 80 | 3 | `99e687328f033302450f35257d1339d168279f9876fcd14b9180ddaa52f618d5` |
|  | candidate | 80 | 3 | `46c693d13661a9d71dd5516590b0c9d7c29e22dc1cc38eea6ff94d60c88ddd82` |
| `_structure_render_detail_objects` | January | 1,136 | 31 | `dbf237f0819e54a61d67102c174e6f79789603a62a8adbac4e97c21e01394658` |
|  | candidate | 1,136 | 31 | `eb6883e150ad20816c89d64a99075684ab290ad767736f7942e56317ac3d4fb1` |
| `_render_debug_detail_objects` | January | 752 | 33 | `3e4c30c13847d08060c08d64bd9114a3c6592eb827ba69d422c93f046d70a6a7` |
|  | candidate | 736 | 33 | `4457d66b4b59e307e3671bba697b89ab0c51f05f4c7b48284ef89a92b068a822` |

## House-rule audit

The retained implementation uses project `real`, `boolean`, point/vector,
rectangle, tag-block, and render types; semantic private/global names; typed
access macros; multiline prototypes; and explicit returns. It introduces no
`code_`/`bss_` placeholders, raw-address fields, manual tag casts, forced
inlining, volatile steering, barriers, inert carriers, undefined behavior, or
nonsensical exactness branches. The only assembly reached by these routines is
the already authenticated shared `fast_ftol` math helper. Intrinsic
`memset`/`abs` selection is directly supported by both target disassembly and
the independent PC donor. This translation unit neither calls nor emits
`point_from_line3d`, preserving the January inline schedule and avoiding its
COMDAT.

## Verification

The focused gate after reconstruction reports 12 exact, three residual, and
zero unwritten functions. The broad-header sentinels remain 189/189 exact for
`units.obj`, 131/131 exact for `rasterizer.obj`, and 13/13 exact for
`render.obj`.

- `ninja all_source`: pass across all 571 source/library translation units;
- same-symbol-map whole-tree strict sweep: 5,022/5,022 baseline-exact
  functions remain exact, seven newly exact, zero regressions, zero load
  errors;
- `python -m tools.parked_functions`: 80 active, zero stale, zero invalid;
- `python -m tools.audit_semantic_matches`: 472 units scanned, 5,172
  functions evaluated, 5,033 semantic exact, 134 hidden exact, zero unit
  errors;
- `python -m tools.audit_object_admission`: zero contradictions and zero
  revocations; its sole candidate is the pre-existing `player_rumble.obj`
  ownership review;
- fake-match scan of all six touched C/header files: zero findings;
- `python -m pytest`: 255 passed;
- `git diff --check`: pass;
- built-object symbol audit: no `point_from_line3d` symbol or COMDAT.

The original fail-closed unit snapshot naturally reports the six reviewed
private-symbol renames and newly emitted functions/constants/debug records as
evidence changes. It reports no usable same-name regression result because
the target symbol map itself changed. The independent same-symbol-map sweep
above removes that ambiguity and verifies every prior strict-exact function
across the full 571-unit build.
