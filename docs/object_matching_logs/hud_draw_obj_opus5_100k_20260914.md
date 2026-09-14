# HUD Draw: multitexture overlay graph (opus5 100K consolidated lane, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/hud_draw.c`; target `build/split/source/interface/hud_draw.obj`
  (SHA-256 `c59443010b644cf21f896a5822d18372e4a4853ecb74da19c140e8eced2988c5`).
- Compiler: XDK VC7 13.00.9254 through `tools/campaign/gate.py` (repository flags from `build.ninja`).
- Lane `opus/100k-consolidated-20260914` (worktree HEAD `b8cd50113`, canonical base `c3e257e9a`).
- Evidence: January COFF bytes and relocations (authority); HCEA
  `src/blam/interface/hud_draw_multitexture_overlay.c`,
  `hud_draw_multitexture_overlay_get_current_weapon_definition.c`, `src/data/theta.c`,
  `src/headers/rasterizer_dynamic_screen_geometry_parameters.h` (names, DB types, semantics only).
  No atlas file exists for this object; the January PDB publics do not list the private static.
- Prior ledgers read: `hud_draw_obj_opus5_fresh_graphs_20260914` (full, including its worker report and
  m1/m2/m3 probes), `hud_draw_obj_bitmap_owner_pair_reconciliation_20260909`,
  `hud_draw_obj_fable_independent_intake_20260906`, `hud_draw_obj_fable_return_address_reconciliation_20260906`,
  `hud_draw_obj_two_exact_leaves_integration_20260904`, `interface_obj_weapon_hud_best_fuzzy_20260912`.
- Per-function worker report: `scratch/workers/hud_draw.md`; probes in `scratch/workers/hud_draw/`.

## 2. Validated baseline and final state

| Gate | exact | residual | unwritten | of |
| --- | ---: | ---: | ---: | ---: |
| lane HEAD (before) | 13 | 7 | 3 | 23 |
| final real file (`scratch/workers/hud_draw/final.obj`) | **15** | 5 | 3 | 23 |
| final source with the accumulator spelled as January's split name (`probe_rename.c`, verification only) | 16 | 4 | 3 | 23 |

Newly strict exact in the real-file gate:

| Function | Padded | Meaningful | Relocs | Normalized SHA-256 (16) |
| --- | ---: | ---: | ---: | --- |
| `_hud_draw_multitexture_overlay_get_current_weapon_definition` | 480 | 468 | 29 | `9484b60b15a40128` |
| `_hud_draw_static_element` | 640 | 627 | 26 | `e4f4da2a62de9eee` |

EXACT-PENDING-RENAME:

| Function | Padded | Meaningful | Relocs | Normalized SHA-256 (16) | Remaining difference |
| --- | ---: | ---: | ---: | --- | --- |
| `_hud_draw_multitexture_overlay` | 2272 | 2264 | 74 | `874edd237080cff0` (equal) | two `.bss` relocations name `_bss_00453ab4` (January split) vs `?theta@?L@??hud_draw_multitexture_overlay@@9@9` |

Remaining non-exact rows: `_fast_ftol_C`, `_real_argb_color_to_pixel32`, `_real_rgb_color_to_pixel32`
(UNWRITTEN, owner-blocked `bitmaps_inlines.h` assembly) and the four PARKED bitmap functions
(`_get_flash_color`, `_hud_draw_bitmap_direct`, `_hud_draw_bitmap_internal`, `_hud_draw_bitmap_with_meter`),
whose size/relocations/normalized hash still equal `config/parked.json`.

Checks: point-from-line3d guard passed; no baseline-exact row regressed (full listing diff); park drift pass;
owner census: no new code owner, the only data-owner change is the renamed 4-byte `.bss` accumulator
(base `?animation_phase@?1??...`, target `_bss_00453ab4`); fake-match scan reports only the two inherited
`get_return_eip` leads; `/W3` shows no C4013.

## 3. Accepted controls

### `hud_draw_multitexture_overlay` (private ABI: overlay ECX, xy_scale EAX)
- Map-offset effectors write through the parameter block: `parameters.map_offset[i]->x += dest_value`
  (January `mov eax,[ebp-0xf4]; fadd [eax]; fstp [eax]`). The header field is `real_point2d const *`, so the
  landed source carries a `(real_point2d *)` qualifier cast; the DB type is non-const (proposal 2).
  Removing six `texture_offset` references is what re-ranks the frame to January's layout.
  BUG (original): `map_offset[i]` is only assigned when map `i` has a bitmap, so an offset effector on a
  missing map dereferences NULL; a corrected build would target `texture_offset[i]`.
- The accumulator is incremented before the effector lookup (`fstp` to `.bss` precedes the
  `tag_block_get_element_with_size` call). It is a block-scope `static real theta;` in the effector loop:
  HCEA names the data symbol `theta`, and the DB parameter is also `theta`, which rules out function scope.
- `meter_parameters = NULL` is stored after the two map-scale stores (January store order).
- The vertex corner test `((vertex_index + 1) & 2)` is written in both selects; a named corner local takes
  EDI, spills `xy_scale` and removes it from the register-parameter set (measured: that single change moves
  overlay from EAX to ECX, `xy_scale` to EAX, and closes `hud_draw_static_element`).
- The non-power-of-two test is a `boolean non_power_of_two` local tested by `if`. VC7 lays out every direct
  `||`/`&&`/`!` spelling else-arm first; only a materialised boolean gives January's then-first layout
  (lab `scratch/workers/hud_draw/lab2.c`, 9 forms).
- The pitch source keeps NONE: `long unit_index = local_player_get_player_index(...) == NONE ? NONE :
  player_get(local_player_get_player_index(...))->unit_index;` (January `cmp eax,-1; jne; or eax,eax`,
  two calls). This replaces the old defective `unit_index = 0`.
- `player` local assigned before the weapon-definition call (January pushes `&weapon_state` after `datum_get`).
- Map loop reads `parameters.map[map_index]` directly (January reloads it for the height divide).

### `hud_draw_multitexture_overlay_get_current_weapon_definition`
- `struct unit_datum *parent_unit = unit_get(unit->object.parent_object_index);` followed by
  `TAG_BLOCK_GET_ELEMENT(&unit_definition_get(parent_unit->definition_index)->unit.seats, unit->unit.parent_seat_index, struct unit_seat)`.
  January calls `object_get_and_verify_type` first, then loads the seat index, then the definition index,
  with `tag_get` nested; the fully nested lookup makes VC7 load the index before the nested call and spill it.

### `hud_draw_static_element`
- No source change; closes once the callee's private ABI matches.

## 4. Experiment matrix

| Function | Shape | Result | Kept |
| --- | --- | --- | --- |
| overlay | m3 + map_offset writes + increment before lookup (v1) | 2272/74, ABI still EAX, frame matches | no |
| overlay | + meter store order (v2) | 2272/74 | no |
| overlay | + no corner local (v3) | ABI ECX/EAX; static_element EXACT; pow2 layout + bss name remain | no |
| overlay | pow2 as `== 0 &&` arms swapped (v4), bare operands (v5), `!(... ||...)` (v6), `w&(w-1)` (v7), implicit int->real (v8) | inert (else-first) | no |
| overlay | `boolean non_power_of_two` (v9) | only reloc-identity (bss name) | yes |
| overlay | `boolean power_of_two` with arms swapped (v10) | else-first again | no |
| overlay | accumulator renamed `theta` in loop block (v11) | identical bytes | yes |
| overlay | pitch ternary initializer (v12) / inline ternary (v13) | identical to redundant-store spelling | v12 yes |
| overlay | without `player` local (v14) | sha | no |
| overlay | file-scope `static real bss_00453ab4` (probe only) | EXACT, gate 16/4/3 | verification only |
| weapon definition | fully nested seat lookup (w1) | 480, index spilled, frame 0x20c, EDI player | no |
| weapon definition | `parent_unit` local + nested `tag_get` (w2) | EXACT | yes |

## 5. Do-not-repeat

- Direct condition spellings of the power-of-two test (all canonicalise to the else-first layout).
- A named corner local in the vertex loop (flips the private ABI).
- Fully nested seat lookups in `get_current_weapon_definition`.
- `--alias` cannot express the accumulator rename (block-scope statics are C-mangled); use a file-scope probe.

## 6. Residual classification

- `hud_draw_multitexture_overlay`: ownership/relocation naming only (measured: size, relocations,
  normalized hash equal; the `.bss` target name is the csplit placeholder).
- Converters / `fast_ftol_C`: unchanged, owner-blocked assembly.

## 7. Reopen criteria

- Overlay: none for code; strict EXACT after the symbols.json rename below.
- Converters / `fast_ftol_C`: owner admission of the `bitmaps_inlines.h` asm bodies.

## 8. Disposition

Active, not Matching: 15/23 strict exact in the real-file gate, 16/23 after the pending data rename.
Newly strict exact: 1,120 padded / 1,095 meaningful bytes; pending rename: 2,272 padded / 2,264 meaningful.
No fuzzy landing, no header/config edit.

## Orchestrator proposals

1. `config/symbols.json` (edit the line in place): `{ "file_offset": 4537012, "flags": 0, "name": "_bss_00453ab4" }`
   -> `{ "file_offset": 4537012, "flags": 0, "name": "?theta@?L@??hud_draw_multitexture_overlay@@9@9", "static": true }`.
   Consumer: `hud_draw.obj` only (two relocations in `_hud_draw_multitexture_overlay`). Precedent:
   `?overcharge_count@?1??hud_render_unit_interface@@9@9`.
2. `source/rasterizer/rasterizer.h:154` `real_point2d const *map_offset[3];` -> `real_point2d *map_offset[3];`
   (HCEA DB type; January writes through it), then drop the six qualifier casts in `hud_draw.c`. Field users:
   `hud_draw.c`, `ui_widget.c`, `rasterizer_xbox_dynavobgeom.c`; 57 files include `rasterizer.h` - a qualifier-only
   change, but measure the board.
