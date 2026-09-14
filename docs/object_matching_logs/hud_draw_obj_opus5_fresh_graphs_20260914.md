# HUD Draw: meter and numbers graphs (opus5 fresh-graphs lane, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/hud_draw.c`; target `build/split/source/interface/hud_draw.obj`
  (SHA-256 `c59443010b644cf21f896a5822d18372e4a4853ecb74da19c140e8eced2988c5`).
- Compiler: XDK VC7 13.00.9254 through `tools/campaign/gate.py` (repository flags `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/30k-fresh-graphs-20260914`, HEAD `47d9dd0da` (canonical base `c3e257e9a`).
- Evidence used: January COFF bytes and relocations (authority), Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/hud_draw.obj.decomp.c`, January map atlas `scratch/atlas/hud_draw.obj.txt`,
  HCEA `src/blam/interface/hud_draw_meter.c` / `hud_draw_numbers.c` (semantics and PDB enum/struct
  names only: `hud_meter_flags`, `hud_number_show_flags`, `hud_number`, `interface_tag_index`,
  `rasterizer_meter_parameters`), Stian build-2276 lift `src/halo/interface/hud_draw.c`
  (`FUN_000d3340`, `FUN_000d3860`).
- Prior ledgers read in full: `hud_draw_obj_bitmap_owner_pair_reconciliation_20260909`,
  `hud_draw_obj_fable_independent_intake_20260906`, `hud_draw_obj_fable_return_address_reconciliation_20260906`,
  `hud_draw_obj_jonas_flash_duration_rejection_20260829`, `hud_draw_obj_small_helpers_proposal_20260904`,
  `hud_draw_obj_two_exact_leaves_integration_20260904`.
- Per-function worker report: `scratch/workers/hud_draw.md`.

## 2. Validated baseline and final state

| Gate | exact | residual | unwritten | of |
| --- | ---: | ---: | ---: | ---: |
| lane HEAD (before) | 11 | 7 | 5 | 23 |
| final real file (`scratch/workers/hud_draw/final.obj`) | **13** | 7 | 3 | 23 |

Newly strict exact (both previously UNWRITTEN, public):

| Function | Padded | Meaningful | Relocs | Normalized SHA-256 (16) |
| --- | ---: | ---: | ---: | --- |
| `_hud_draw_meter` | 1312 | 1298 | 46 | `6f675c6830c15c17` |
| `_hud_draw_numbers` | 1856 | 1818 | 73 | `42711a6117260029` |

Remaining non-exact rows:

| Function | Tag | Classification |
| --- | --- | --- |
| `_fast_ftol_C` (56) | UNWRITTEN | asm-only helper; policy-blocked |
| `_real_argb_color_to_pixel32` (320) | UNWRITTEN | asm-only `bitmaps_inlines.h` converter; policy-blocked |
| `_real_rgb_color_to_pixel32` (192) | UNWRITTEN | asm-only `bitmaps_inlines.h` converter; policy-blocked |
| `_hud_draw_multitexture_overlay` (2272) | size 2224 | private ABI / frame ranking |
| `_hud_draw_multitexture_overlay_get_current_weapon_definition` (480) | size 464 | argument order (fixable) + register/CSE tie |
| `_hud_draw_static_element` (640) | sha | inherits the overlay's private ABI at one call site |
| `_get_flash_color`, `_hud_draw_bitmap_direct`, `_hud_draw_bitmap_internal`, `_hud_draw_bitmap_with_meter` | PARKED | locked, measurements unchanged |

Final checks: point-from-line3d guard passed; all four parks equal `config/parked.json` base
(size/relocs/normalized hash); owner census vs `build/base` + target shows no new code/data/BSS/COMMON
owner except `__real@40000000`, which January references as an undefined literal COMDAT; fake-match
scan reports only the two inherited `get_return_eip` leads; `/W3` shows no C4013 implicit declaration.

## 3. Accepted controls

### `hud_draw_meter`
- Ordinary typed C compiled exact on the first gate. The twelve `fast_ftol_C` calls are two
  expansions of `MAX(meter->minimum_value, PIN(fast_ftol_C(alpha_multiplier*value + alpha_bias), 0, UNSIGNED_CHAR_MAX))`
  (the cseries macros re-evaluate their operands, which is exactly January's repeated call pattern).
- Branch order follows January's CFG: disabled; `!switch_color_on_state_change` (flash fade, rgb scaled by
  `reference_time<0 ? 0 : PIN(1-reference_time,0,1)`); flashing with `interpolates_between_min_max`
  (`rgb_colors_interpolate`, optional inversion); flashing flat max color; flat min color.
- Background alpha inversion written `(UNSIGNED_CHAR_MAX - (empty_color>>24))<<24`; the `-1 -` spelling
  compiles identically, the named-constant form states the intent.
- TU-local `struct rasterizer_meter_parameters` (0x1C, PDB field names; identical to the local copy in
  `rasterizer_xbox_dynavobgeom.c`) and `enum hud_meter_flags`; includes `bitmaps/bitmap_utilities.h`
  (owner of `pixel32_to_real_rgb_color`) and `interface/interface.h`.

### `hud_draw_numbers`
- Pen position is a `point2d cursor` whose `x` field VC7 enregisters: this is the only tested shape that
  reproduces every 16-bit `mov si, ax` / `mov si, word ptr [origin.x]` store (a plain `short` pen
  produces 32-bit stores and a 16-byte-shorter body).
- The glyph number bitmap retrieved per glyph is `number_bitmap`; the sequence bitmap is `source_bitmap`
  (the only naming consistent with January's operand order for the literal assertion
  `source_bitmap==number_bitmap` at lines 515/539/556/575/595).
- Each glyph block declares its outputs and point, then assigns point then NULL outputs; in the integer
  loop `short digit = value % 10;` precedes the leading-zero `break` (January computes and spills the
  quotient before that test, which also accounts for the 0x234 frame).
- `value = abs(value)` (January uses the `cdq/xor/sub` intrinsic form, not the `ABS` macro's branch).
- Corner switch: `default:` asserts unreachable (line 493) and falls into the right-anchored cases,
  exactly as the jump table and block layout show.
- `hud_globals_get_scale(...)` is used for a non-positive override scale; it inlines to the 1.0f store.
- TU-local `number_hud_element_definition` (0x54), `hud_number_definition` (0x64), `'hud#'` tag and
  PDB-named enums, identical to the existing local copies in `hud_messaging.c`, `hud_nav_points.c`,
  `hud_weapon.c`.

## 4. Experiment matrix

| Function | Shape | Result | Kept |
| --- | --- | --- | --- |
| meter | natural C, cseries MAX/PIN, CFG branch order, `-1 -` inversion | 1312/46 EXACT | superseded by next |
| meter | same with `UNSIGNED_CHAR_MAX -` inversion | 1312/46 EXACT | yes |
| numbers | natural C, short pen, NULL initialisers, `ABS` | 1840/73, frame 0x230 | no |
| numbers | + bitmap names swapped, `abs()`, `digit` before break | 1840/73, frame 0x234 | no |
| numbers | + `(short)(long)` casts / compound `-=` (probes) | no change | no |
| numbers | + `point2d cursor` pen | 1856/73, same instruction count | no |
| numbers | + declare-then-assign glyph blocks | 1856/73 EXACT | yes |
| get_current_weapon_definition | nested seat `TAG_BLOCK_GET_ELEMENT(&unit_definition_get(unit_get(parent)->definition_index)->unit.seats, ...)` | 480/27, seat index CSE'd to a spill slot (frame 0x20c), player register EAX->EDI | no |
| get_current_weapon_definition | nested lookup folded into `TEST_FLAG` | identical to previous | no |
| multitexture_overlay | `player` local before weapon-definition call | call order matches, 2224 | no |
| multitexture_overlay | + pitch source keeps `NONE` | `or eax,eax` matches, 2224 | no |
| multitexture_overlay | + `parameters.map[map_index]` instead of a bitmap local | 2224, 139 diff blocks (was 143) | no |

All non-kept shapes were measured only on scratch copies (`scratch/workers/hud_draw/*.c`).

## 5. Do-not-repeat

- Short-typed pen spellings for `hud_draw_numbers` (`(short)`, `(short)(long)`, compound `-=`): all emit
  32-bit register stores.
- Natural-C `real_rgb_color_to_pixel32` (prior rounding counterexample) and `(long)value` `fast_ftol_C`
  (prior semantic rejection); the January bodies are hand-written x87 and cannot be produced by C.
- Re-sweeping nested/un-nested seat lookups in `get_current_weapon_definition` without new evidence.

## 6. Residual classification

- Converters and `fast_ftol_C`: measured fact: in-memory `fistp`/`and`/`shl` packets and FIST/FISUB
  corrections with operands loaded before a shared scale. Inference: authentic `__asm` bodies of
  `bitmaps_inlines.h` inlines (assert file literal) like the admitted `real_alpha_to_pixel32`. Not added:
  this lane forbids new inline-assembly findings in this TU.
- `get_current_weapon_definition`: fact: January's seat lookup is one nested expression; with it, the
  remaining difference is VC7 register choice/CSE. Classification: register tie after a structural fix.
- `hud_draw_multitexture_overlay`: fact: January passes overlay in ECX and xy_scale in EAX; the candidate
  passes overlay in EAX. The retained body also passes unit index 0 where January passes NONE to
  `unit_get_aiming_vector` (reconstruction defect, not an original bug). Classification: private ABI /
  frame ranking, unresolved.
- `hud_draw_static_element`: inherited from the overlay callee ABI (only its call site differs).

## 7. Reopen criteria

- Converters / `fast_ftol_C`: owner admission of these `bitmaps_inlines.h` asm helpers (donor: Fable
  `f9dd42218`), placed in their genuine header owner.
- `hud_draw_multitexture_overlay`: a source use-count change that makes VC7 enregister `xy_scale` and
  `overlay` (start from `scratch/workers/hud_draw/m3.c`), landing the `NONE` pitch fix with it;
  `hud_draw_static_element` should then close without edits.
- `get_current_weapon_definition`: evidence for a compare spelling that avoids the seat-index CSE, retried
  on the nested lookup.

## 8. Disposition

`hud_draw.obj` is still active, not Matching: 13/23 strict exact. Two new public functions (3,168 padded /
3,116 meaningful bytes) are strict exact in the real-file gate. No fuzzy landing, no header/config edit.

## Orchestrator proposals

1. Move `struct rasterizer_meter_parameters` to `source/rasterizer/rasterizer.h`; duplicates in
   `source/interface/hud_draw.c` and `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c`.
2. Move `number_hud_element_definition`, `hud_number_definition`, `hud_number_show_flags` and `hud_number`
   to an interface header (e.g. `unit_hud_interface_definition.h`); duplicates in `hud_draw.c`,
   `hud_messaging.c`, `hud_nav_points.c`, `hud_weapon.c`.
3. Owner decision on admitting the `bitmaps_inlines.h` converter asm bodies and `fast_ftol_C`.
