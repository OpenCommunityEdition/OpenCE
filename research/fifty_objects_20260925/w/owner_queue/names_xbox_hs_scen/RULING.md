# RULING - non-first-party descriptive static names (rasterizer_xbox .bss, hs_scenario_definitions data)

Base: cdc8ebd3 (HEAD is now eb33baec, a docs-only commit; patches are still clean). Measurements: `MEASURED.md`.
Apply order: `patches/APPLY_ORDER.txt`.

## Summary

Both items are technically complete and apply-ready. In each one, every byte, relocation, alignment, storage class and
data section matches January. Each item leaves exactly one question: may **9 file-static names that no first-party
source attests** go into source and symbols.json?

The names are load-bearing only through the split, and this was measured for both items:

- **rasterizer_xbox:** without its symbols.json rows, 9 exact functions fall to reloc-identity residuals.
- **hs_scenario_definitions:** without its rows, .data drops to 48.93%.

So "yes to the split, no to the names" is not an option. A no on either item keeps the object where it is today.

## Question 1 - rasterizer_xbox

> Admit `source/rasterizer/xbox/rasterizer_xbox` as Matching, replacing the invented `rasterizer_xbox_d3d_globals`
> aggregate and its 23 `#define` redirects with 23 `= 0` file statics? 14 of the names are first-party; the 9 below are
> not. (Yes = apply 01-05. No = nothing lands; the object stays at FAIL(22) audit, NonMatching.)

Name by name. The owner can accept a subset only if replacement names are supplied: the split needs a name at every
offset.

| # | Name (offset in .bss 0x45E028) | Type | Evidence tier | Evidence |
|---|---|---|---|---|
| 1 | `node_matrix_constants` (+0x000) | `real[44][3][4]` | **invented** | Descriptive. No January string, no HCEX rasterizer_dx9 static, and no /Od string (the /Od "node_matrix" hits are model-animation strings in fn 0x6fd580). |
| 2 | `bitmap_dimensions_non_blocking` (+0x840) | `point2d` | **HCEX-convention stem** | HCEX has `Static Local union point2d dimensions` in `rasterizer_set_texture_non_blocking`. HCEX's own spelling (function-local static) is REFUTED for January: VC7 name-hash order swaps the two (lab D_uN_ord). The `bitmap_` prefix and `_non_blocking` suffix are descriptive. |
| 3 | `bitmap_dimensions` (+0x844) | `point2d` | **HCEX-convention stem** | As #2 (`rasterizer_set_texture`'s static local `dimensions`). |
| 4 | `global_d3d_texture_render_primary[2]` (+0x84C) | `D3DBaseTexture *[2]` | **convention stem** (January pattern + HCEX/Od target stem) | 13 January strings attest the `global_d3d_{texture,surface}_<target>` pattern. `render_primary` is the HCEX / later-/Od enum `_rasterizer_target_render_primary`. |
| 5 | `global_d3d_surface_render_primary` (+0x854) | `D3DSurface *` | **convention stem** (strongest of the 9) | January attests the sibling `global_d3d_surface_render_primary_z`. |
| 6 | `global_d3d_texture_render_secondary_z` (+0x860) | `D3DTexture *` | **convention stem** | `render_secondary` is attested (January + HCEX enum). The `_z` suffix is attested by `..._render_primary_z`. The texture half of a `_z` pair is inferred; the source comment says so. |
| 7 | `global_d3d_surface_render_secondary_z` (+0x868) | `D3DSurface *` | **convention stem** | Same as #6. |
| 8 | `global_d3d_texture_render_primary_copy` (+0x8A0) | `D3DBaseTexture *` | **invented** (suffix) | The `_copy` suffix has no first-party source. The tree's enum `_rasterizer_target_render_primary_copy` is itself descriptive ("the eighth target has no first-party name"). |
| 9 | `global_d3d_surface_render_primary_copy` (+0x8A4) | `D3DSurface *` | **invented** (suffix) | Same as #8. |

These names are first-party and are not part of the question:

- **14 bare names in January's IDirect3D*() strings:** `d3d`, `global_d3d_surface_render_primary_z`,
  `global_d3d_{texture,surface}_{render_secondary,shadow_primary,shadow_secondary,sun_glow_primary,sun_glow_secondary,water}`.
  No string contains `rasterizer_xbox_d3d_globals`, which proves the aggregate is invented.
- **`d3d_palette`:** 7 January strings.
- **`srcblend_table`/`destblend_table`/`blendop_table`:** HCEX Static Locals of `rasterizer_set_framebuffer_blend_function`,
  `const unsigned long[9]`, in January's source/dest/op order.
- **`rasterizer_filthy_bitmap_defaults_initialize`:** January-hash atlas, the Sept-2001 map, and HCEX `static void`.

**Evidence for yes.**

- The aggregate plus #define redirects is the fake-stringification pattern. January's strings name the globals bare.
- `= 0` declaration order reproduces January's .bss layout exactly. The strip test (initialisers removed) fails the audit
  on 23 rows (name-hash order), so the initialisers are layout-attested, not decoration.
- There is no public in 0x45E028..0x45E8CF, so every name is static.
- Every other audit row is first-party.
- Counter-precedents:
  - rasterizer_xbox_shadows was admitted in batch 3 with 2 disclosed descriptive gap statics (HCEX `local_` pattern).
  - collision_debug's descriptive split landed, for function credit only.
  - eb33baec's hud_unit admission landed a descriptive macro name, `match_assert_stack_frame`.

**Evidence against.**

- 2026-09-15 owner ruling: "prefers no descriptive/invented static names in symbols.json". It held the env_fog (21) and
  models (13) .bss splits.
- 2026-09-24 canonical: path_obstacle_avoidance was held because its "BSS relabel lacks authenticated private identities".
- 3 of the 9 names are invented outright (#1, #8, #9).

**What a yes also carries (not name-gated, disclosed).**

- **Storage.** `d3d_palette`, `rasterizer_state_cache` and 20 XDK wrapper rows become static, per cachebeta publics.
- **Blend tables.** The three HCEX static-local tables replace the invented `framebuffer_blend_function_states[3][9]`
  and its enum. January's .rdata alignment of 4 rules out a 108-byte object.
- **Flicker calls.** Direct `D3DDevice_SetFlickerFilter`/`SetSoftDisplayFilter` calls, per January's relocations. This
  drops 2 surplus wrapper copies.
- **Renamed static.** `rasterizer_filthy_bitmap_defaults_initialize` is static and renamed. Its stale prototype leaves
  `rasterizer_xbox_internal.h`.
- **Park re-baseline, new at cdc8ebd3.** Removing that prototype moves the PARKED fuzzy `__rasterizer_model_draw`
  (rasterizer_xbox_models) from sha 89b0d7ea to cbfa8585 at the same 5168 bytes / 348 relocations, and objdiff from
  95.08453 to 95.07629. The cause is a declaration-count tie: models.c no longer includes hud_draw.h. There is no strict
  effect; the other 6 includers are byte-identical. `05_parked.json.patch` re-baselines the park. Without it the park
  check goes stale.

**House-rule classes touched.**

- Descriptive names (owner-gated class).
- Layout-attested `= 0` initialisers (strip test done).
- Header prototype removal with declaration-count blast radius (1 parked fuzzy hash).
- A static function rename backed by first-party evidence.
- HCEX-attested function-local statics.
- No BUG class, no view casts, no new COMDATs (the 3 surplus COMDATs are pre-existing, identical and pair-link clean).

**Comments exactly as they land (01 patch).**

```c
/* this object's private Direct3D state; the IDirect3D*() error strings name
 * these globals bare (not as members of an aggregate), and cachebeta.pdb has
 * no public in 0x0045E028..0x0045E8CF, so each is a file static. */
/* hand-built descriptors wrapping the two back buffers */
/* the render-secondary target has no depth buffer; both slots are
 * explicitly cleared at initialization.  The texture name is inferred
 * from the surface name immediately behind it. */
/* the shared 256-entry palette; January's own
 * IDirect3DDevice8_CreatePalette(global_d3d_device, D3DPALETTE_256,
 * &d3d_palette) error string names it. */
	/* three separate NONE-terminated tables (January .rdata is 4-byte aligned, so
	 * no single 108-byte array); names, scope and type follow HCEX.pdb's static
	 * locals of this function (const unsigned long srcblend_table[9], ...). */
```

The first comment slightly overstates things: 9 of the 23 names are NOT in the strings. If the owner says yes, I
recommend this optional byte-inert amendment. It is **not** in the patch.

```c
/* ... 14 of these names are named bare by January's IDirect3D*() error strings;
 * node_matrix_constants, bitmap_dimensions*, and the render_primary,
 * render_secondary_z and render_primary_copy slots have no first-party name
 * (descriptive). */
```

## Question 2 - hs_scenario_definitions

> Admit `source/hs/hs_scenario_definitions` as Matching, replacing the invented wrapper structs, `#pragma pack`,
> 11 `__declspec(align(4))` and the `field_table_alignment` padding member with 18 separate January data definitions?
> This adds the 9 static names below (8 convention-derived, 1 invented) and retires the source-layout-incomplete
> rejection. (Yes = route A: A01-A04, or route C: C01-C03 + A02-A04, which adds HCEX's scenario-limit enum to
> `hs_scenario_definitions.h`. No = nothing lands; the rejection stays; data stays 484/1112.)

| # | Name (offset in .data 0x2E49C0) | Type | Evidence tier | Evidence |
|---|---|---|---|---|
| 1 | `hs_scripts_block_fields` (+0x018) | `static struct tag_field[6]` | **HCEX-convention stem** | Stem = January public `hs_scripts_block` (cachebeta). Suffix = HCEX `<block>_fields` (10 HCEX statics, e.g. `map_leaf_face_block_fields`; landed leaf_map/bitmap_group precedent). |
| 2 | `hs_globals_block_fields` (+0x090) | `static struct tag_field[7]` | **HCEX-convention stem** | Same rule, with `hs_globals_block`. |
| 3 | `hs_references_block_reference` (+0x110) | `static struct tag_reference_definition` `{0, NONE, NULL}` | **invented** | HCEX has no convention for a TU-local any-tag reference; the only tag-reference names are public `global_<group>_reference`. |
| 4 | `hs_references_block_fields` (+0x11C) | `static struct tag_field[3]` | **HCEX-convention stem** | Same rule, with `hs_references_block`. |
| 5 | `hs_source_files_block_fields` (+0x17C) | `static struct tag_field[3]` | **HCEX-convention stem** | Same rule, with `hs_source_files_block`. |
| 6 | `data_array_header_bs_codes` (+0x1CC) | `static byte_swap_code[]` (14) | **HCEX-convention stem** | Stem = January's own byte_swap_definition name string "data_array_header". Suffix = HCEX `<name>_bs_codes` (23 distinct `_bs_definition` pairs; byte_swapping.c precedent). |
| 7 | `data_array_header_bs_definition` (+0x204) | `static struct byte_swap_definition` | **HCEX-convention stem** | Same as #6. |
| 8 | `syntax_node_bs_codes` (+0x218) | `static byte_swap_code[]` (10) | **HCEX-convention stem** | Stem = January string "syntax_node". |
| 9 | `syntax_node_bs_definition` (+0x240) | `static struct byte_swap_definition` | **HCEX-convention stem** | Same as #8. |

These are first-party and are not part of the question:

- the 9 public data names (exactly the cachebeta publics in 0x2E49C0..0x2E4C34);
- `byte_swap_script_syntax_data`, from the Sept-2001 Bungie map; it already landed in 6e3e2d35;
- route C's enum, which is HCEX's anonymous enum with the same names, values and order. January's hs_compile assert
  text `increase MAXIMUM_HS_STRING_DATA_PER_SCENARIO` also attests it.

The owner can rule name by name. #3 is the only invented name. Nothing first-party names the any-tag reference. If the
owner prefers another spelling for #3 (or any other), it is a pure rename in A01 and A02.

**Evidence for yes.**

- It retires the rejection's exact objections: invented aggregates, pack/align directives and the padding member.
- .data alignment 8 now arises naturally: the 72- and 84-byte field arrays take 8-byte alignment under VC7, and
  0x8C..0x90 is that alignment pad.
- .rdata literal order now equals January's. This attests the block-before-fields definition order; January's
  leaf_map shows the same idiom.
- The data credit becomes native, +628 bytes (objdiff 1112/1112), and needs no semantic_data_matches entry.
- 8 of the 9 names come mechanically from January's own names by HCEX-attested Bungie conventions.

**Evidence against.** The same 2026-09-15 ruling as Q1 applies. hs_globals_external was rejected because its 442 names
are attested only by a later build. The names here are not even attested by a later build: HCEX has no
hs_scenario_definitions compiland. One name is invented.

**House-rule classes touched.**

- Descriptive names (owner-gated).
- Forward tentative `static struct tag_field <block>_fields[N];` definitions, placing each block before its fields. The
  strip test was run: fields-first is also strict-exact, but it reverses 4 January literal pairs and drops objdiff .data
  to 92.84%. The order is ordinary C definition order and is attested by January's .rdata.
- Removal of the consumer-local `extern char *hs_*_names[]` (rule 9) in favour of `#include "hs/hs.h"`.
- Disclosure: 2 new C4090 warnings. hs.h's genuine `char const *[]` meets tag_groups.h's reconstructed `char **names`,
  and HCEX types it `const char *[0x31]`. They are byte-inert.
- Route C only: a shared-header enum. Its consumer sweep is byte-inert at cdc8ebd3; hs_runtime's `$L` names shift only.
- No BUG class, no casts, no pragmas, no COMDATs.

**Comments exactly as they land.** A01 and C03 add no comment beyond the regenerated header symbol list (the 9 names at
their offsets). The patch has no disclosure comment. Optional byte-inert amendment, NOT in the patch:

```c
/* file-static tag-definition data: the <block>_fields and <name>_bs_codes /
 * <name>_bs_definition names follow the Bungie conventions (no first-party
 * record); hs_references_block_reference is descriptive. */
```

## Independent item (not name-gated; no ruling needed)

`patches/independent/rasterizer_xbox_debug_static_wrappers_symbols.json.patch` was approved by the wave-2 reviewer and
never landed. It marks 8 XDK wrapper rows static (no cachebeta public).

- The split changes only rasterizer_xbox_debug.obj.
- gate 18/18.
- object_audit goes from FAIL(8) to PASS.

It can land with or without Q1.
