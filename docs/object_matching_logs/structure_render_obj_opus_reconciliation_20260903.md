# `structure_render.obj` Opus reconciliation and strict closeout

## Result

The January `structure_render.obj` is complete. All 19 target functions are
strict-exact for 3,504 padded code bytes, the target-owned `.data` and `.bss`
families are ownership-exact, and the object is admitted as `Matching`.

This packet was reconciled on
`agent/structure-render-reconcile-20260903`, based on canonical
`c700b911f0f1f115d534415ab609d28496fc25a1`. The donor
`C:\halo-worktrees\opus-small-families-30k-20260902` was treated as read-only.
Its committed reconstruction came from `608c66380`, with the donor's final
dirty symbol-banner and BSS-linkage cleanup audited separately. January's XDK
3911 split object remained the byte and ownership oracle throughout.

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_structure_render_set_fog_offset` | 80 | exact |
| `_compare_surface_indices` | 32 | exact |
| `_structure_render_dynamic_triangles_from_bitvector` | 256 | exact |
| `_structure_render_dynamic_triangles_from_indices` | 128 | exact |
| `_structure_render_build_dynamic_triangles` | 192 | exact |
| `_structure_render_pass` | 608 | exact |
| `_structure_render_preprocess` | 288 | exact |
| `_structure_render_lightmaps` | 176 | exact |
| `_structure_render_diffuse_texture` | 128 | exact |
| `_structure_render_specular_lightmaps` | 112 | exact |
| `_structure_render_reflection_lightmap_masks` | 80 | exact |
| `_structure_render_reflection_mirrors` | 64 | exact |
| `_structure_render_reflections` | 128 | exact |
| `_structure_render_transparent_geometry` | 128 | exact |
| `_structure_render_fog` | 128 | exact |
| `_structure_render_fog_screen` | 112 | exact |
| `_structure_render_specular_light` | 304 | exact |
| `_structure_render_diffuse_light` | 304 | exact |
| `_structure_render_shadow` | 256 | exact |

The canonical base already had the 80-byte fog-offset setter. This packet
therefore contributes exactly 18 newly exact functions and 3,424 padded code
bytes.

## Names, linkage, and source authenticity

The five private January sections formerly named by address were mapped by
their unique file offset, section order, padded size, relocation count, and
strict normalized body:

| File offset | Size / relocs | Semantic private name |
| ---: | ---: | --- |
| 1,592,704 | 32 / 0 | `_compare_surface_indices` |
| 1,592,736 | 256 / 6 | `_structure_render_dynamic_triangles_from_bitvector` |
| 1,592,992 | 128 / 4 | `_structure_render_dynamic_triangles_from_indices` |
| 1,593,120 | 192 / 15 | `_structure_render_build_dynamic_triangles` |
| 1,593,312 | 608 / 14 | `_structure_render_pass` |

Their behavior and names are independently corroborated by the later HCEA
structure-render lifts under
`_research_cache/halocea/src/blam/structures/`; that source was used only as
semantic corroboration, never as the byte oracle. All five are ordinary
file-private functions and are marked `static` in both C and
`config/symbols.json`.

The address-style `_bss_004c0cd0` entry is renamed
`_structure_render_globals`. Its decoded layout is a 24-byte transient state
block containing environment-geometry validity, the dynamic-triangle handle,
fog-offset validity, and the fog-offset vector. The field offsets are asserted
at 0x0, 0x4, 0x8, and 0xC. The HCEA
`headers/structure_render_globals.h` lift independently corroborates the name,
field meanings, and total size. Every January reference is internal to this
translation unit; an explicit `static` definition is the natural linkage and
is the only ordinary-C form that emits January's real 24-byte `.bss` rather
than a COMMON or undefined symbol.

The retained implementation is typed, coherent C. It uses tag-block access
macros, semantic flag macros and enum constants, the project's `real`, `word`,
point, vector, plane, and rectangle types, explicit `void` parameter lists,
one parameter per line, and explicit terminal returns. It contains no
volatile/register steering, barriers, pragmas, fake dependencies, raw-address
access, undefined representation tricks, forced inline, or nonsensical
matching branches. It contains no inline assembly and emits no
`point_from_line3d` symbol or COMDAT, preserving the January inline schedule.

## Owner-header reconciliation

The donor's temporary cross-subsystem mirrors were removed:

- `structure_surface` now belongs to
  `structures/structure_bsp_definitions.h` as three `word` vertex indices;
  the six-byte layout is corroborated by the local HCEA and Demon sources.
- `leaf_portal` and `debug_leaf_portals` now belong to
  `structures/leaf_map.h`; the redundant C-file structure copy was removed.
- all public structure-render entry points and its two public debug indices
  now belong to `structures/structure_render.h`; duplicate declarations were
  removed from `render.c` and `render_objects.c`.
- `rasterizer_globals_definition.lightmap_mode` is a real `short` field at
  offset 0x60, carved out of the same reserved 0x68-byte layout and protected
  by an offset assertion. `hs_globals_external.c` independently authenticates
  that exact address and type.
- the environment lightmap enum and the seven previously missing environment
  wrapper declarations live in the narrow rasterizer-owned
  `rasterizer_environment.h` interface.
- the runtime bitmap lookup declaration lives in the narrow bitmap-owned
  `bitmap_group_runtime.h` interface rather than in this caller.

The narrow interfaces are load-bearing for compiler fidelity. A measured
header blast proved that inserting the seven rasterizer declarations into the
large shared `rasterizer.h` changed one exact draw-primitives function, while
inserting the enum there changed one exact decals function. Likewise, adding
the bitmap lookup to broad `bitmap_group.h` changed the retained measurement
of its parked large function. Moving only those declarations into subsystem-
owned narrow headers restored both exact functions and the parked evidence;
the real rasterizer field itself is safe in its true owner structure. This is
declaration-position isolation, not a code-generation carrier.

`rasterizer_xbox_environment.c` now consumes the owner definitions and drops
its duplicate lightmap enum, profile constants, partial rasterizer-global
overlay, and frame-parameter mirror. `rasterizer_xbox.c` is untouched.

## Whole-object ownership

| Family | January | Rebuilt | Decision |
| --- | ---: | ---: | --- |
| `.text` | 19 sections / 3,504 B / 287 relocs | identical | exact |
| `.data` | 1 section / 18,346 B / 12 relocs | identical | exact |
| `.bss` | 1 section / 24 B / 0 relocs | identical | exact |
| target-owned `.rdata` | 17 sections / 647 B / 0 relocs | identical owners | exact |

The rebuilt object additionally owns the ten-byte select-any string COMDAT
`"triangles"`. January's exact
`_structure_render_build_dynamic_triangles` has the same relocation to the
same decorated string symbol, but csplit records that symbol as undefined in
this object because the linked image selected another identical COMDAT owner.
The rebuilt copy is discardable, has no distinct runtime state, and is the
same reviewed ownership case already accepted for other complete objects. It
is not counted as target-owned data and does not block `Matching` admission.

## Verification

- Focused hardened gate: `19 exact, 0 residual, 0 unwritten`.
- Direct hardened comparison: all 19 function sections report
  `all_equal: true`, including normalized bytes and relocation destinations.
- Full XDK 3911 source rebuild and header blast: pass; 473 semantic units,
  6,110 evaluated functions, 5,800 accepted exact, and zero unit errors.
- Stable whole-tree sweep against the `c700b911f` canonical snapshot: exactly
  18 gained functions / 3,424 bytes and zero regressions across 8,245 target
  functions.
- Progress gate after `Matching` admission: pass; 181 parked compiler ties
  validated.
- Parked-function audit: 181 active, zero stale, zero invalid.
- Object-admission audit: this object is the sole new ownership-review
  candidate, with zero contradictions or revocations; the section census
  above closes that review. The two reported rejections belong to unrelated,
  inherited objects.
- Changed-source fake-match scan: zero review leads.
- Tool suite: 261 tests passed.
- `git diff --check`: clean.

Reproducibility SHA-256 values:

- `source/structures/structure_render.c`:
  `B1C7B1D382C6138D5F5127F852E6E539B41EC34527105290367E258AC965B977`
- `source/structures/structure_render.h`:
  `7A74C4ABB617EE7C59BB8D0F1AFF049B3C87177D728860ABFD438E8B6B943A82`
- `source/bitmaps/bitmap_group_runtime.h`:
  `1AF4FECB30F89ECCF01754E783F6A291E0FAF780A30B6F02AEFD2F1187D3B0F7`
- `source/rasterizer/rasterizer_environment.h`:
  `002C17845DB81F82E2E8CC0A3DF35D02499C84FA4D76110394121B8347DBCDB9`
- January split object:
  `BF5DB264D8988B0D7B4705950B9BFC513B23996238CD3C61A5F0677DF546E2B1`

The rebuilt COFF's raw SHA-256 is intentionally omitted: the Xbox compiler
embeds the current compile timestamp in the object header, so the raw file
hash is not reproducible across clean builds. The strict `coff_compare.py`
result and stable-verdict snapshot above are the reproducible code/data
evidence.

No canonical worktree was edited and no push was performed by this lane.
