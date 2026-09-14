# HUD Draw: park quartet reopen and converter/ftol helper ownership (opus5 150K house-clean lane, w1, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/hud_draw.c`; January target `build/split/source/interface/hud_draw.obj`.
- Compiler XDK VC7 13.00.9254 through `tools/campaign/gate.py` (repository flags from `build.ninja`).
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Worker scope: this one `.c`; no header/config edit.
- **No change was made to the real source file** (`git diff --stat -- source/interface/hud_draw.c` is empty): every
  outcome below is a scratch candidate awaiting an orchestrator decision (park reopen, or owner ruling on assembly).
- Evidence: January COFF bytes and relocations (authority); January map atlas `scratch/atlas/hud_draw.obj.txt`
  (inline `i` records); `cachebeta.exe` inter-function padding bytes (read as data, never executed); HCEA
  `src/blam/interface/get_flash_color.c` and `hud_draw_bitmap_internal.c` (later build, semantics only).
- Ledgers read: `hud_draw_obj_small_helpers_proposal_20260904`, `hud_draw_obj_two_exact_leaves_integration_20260904`,
  `hud_draw_obj_jonas_flash_duration_rejection_20260829`, `hud_draw_obj_fable_independent_intake_20260906`,
  `hud_draw_obj_fable_return_address_reconciliation_20260906`, `hud_draw_obj_bitmap_owner_pair_reconciliation_20260909`,
  `hud_draw_obj_opus5_fresh_graphs_20260914`, `hud_draw_obj_opus5_100k_20260914`,
  `decals_obj_jonas_real_a_rgb_color_fixed_point_20260827`, `decals_obj_color_projection_packet_20260912`,
  `ai_obj_large_visibility_spatial_packet_20260909`, `ai_obj_482743d8a_reconciliation_audit_20260909`,
  `lane_reconciliation_batch5_fuzzy_ui_world_20260909`, the 100K consolidated handoff.
- Worker report: `scratch/workers/hud_draw.md`; probes `scratch/workers/hud_draw/`.

## 2. Gates

| Gate | exact | residual | unwritten | of |
| --- | ---: | ---: | ---: | ---: |
| baseline = final real file (`final.obj`) | 16 | 4 | 3 | 23 |
| park-reopen candidate `p4.c` (real file + 3 reopened bodies edited) | **20** | 0 | 3 | 23 |
| asm-helper candidate `v2.c` (real file + 3 helper bodies) | **19** | 4 | 0 | 23 |
| combined `all.c` (p4 + helpers) | **23** | 0 | 0 | 23 |

All candidates: `_point_from_line3d` guard passed; row-by-row listing diff shows no baseline-EXACT row lost;
owner census vs `build/base`: no new or removed owner except internal `$L` label renumbering, plus (helpers only) the
three target-defined helper sections, the target-defined `verify == result` literal, and literal COMDATs January
references undefined (`"color"`, the two `assert_valid_real_*_color` formats). Combined census vs the target: no
target-only owner; candidate-only code owners `_fast_ftol` and `_square_root` pre-exist in base. `/Zs /W3`: 26 warnings
before and after, no C4013. Fake scan: `p4.c` = 2 inherited `get_return_eip` leads (same as HEAD); `v2.c`/`all.c` add
inline-assembly leads for the three helpers and codegen-directive leads for one pragma pair.

## 3. Park reopen candidate (`scratch/workers/hud_draw/p4.c`)

| Function | Padded | Relocs | Normalized SHA-256 (16) | Edit |
| --- | ---: | ---: | --- | --- |
| `_hud_draw_bitmap_internal` | 544 | 16 | `afc1044b43f5f168` | corner test inline + store order |
| `_hud_draw_bitmap_direct` | 320 | 15 | `6ec7215284a2c224` | none (callee ABI) |
| `_hud_draw_bitmap_with_meter` | 384 | 17 | `e44c9d5bbd7d5279` | none (callee ABI) |
| `_get_flash_color` | 608 | 32 | `0717c2b85934e185` | interpolation component addressing |

### `hud_draw_bitmap_internal` (static; private ABI)
- January passes `point` in EDI and `xy_scale` in EAX (copied to ESI) with six stack arguments; the parked body
  enregistered only `xy_scale` (EDI) and pushed `point`. Both parked callers therefore pushed one extra argument
  (`add esp,24h` vs `20h`) and gained loop-alignment filler: the whole 16-byte gap of `direct` and `with_meter`.
- Established law applied (100K lane, measured on the exact sibling `hud_draw_multitexture_overlay` in this TU):
  a named `(vertex_index+1)&2` corner local takes a register and removes a pointer parameter from VC7's
  register-parameter set. The sibling's exact vertex loop spells the corner test inline in both selects; the
  copy-pasted loop here now uses the same spelling. That alone closes both callers.
- The last residual was a transposed store; January stores `framebuffer_blend_function` (`mov word [ebp-28h],7`)
  before `map[0]` (`mov [ebp-0A4h],edx`). Source statement order now follows January.

### `get_flash_color`
- With the callees settled, only address operands differed: January passes `&result`, `&base_color`,
  `&flash_color` at offset 0 to `vectors_interpolate` and reads/writes offset `+0Ch` for `scalars_interpolate`; the
  parked body passed `.rgb` (+4) and `.alpha` (+0). January blends components 0..2 as a vector and component 3 as a
  scalar, which still blends all four channels by the same fraction. HCEA's independent reconstruction has the same
  calls (`(real_vector3d *)&result`, `&result.n[3]`). A short comment records the layout.
- The park's premise ("residual/helper schedule" around the inline square root) is refuted; `_square_root` is unchanged.

### Park criteria
- `bitmap_internal`, `with_meter`: unclassified; their parked evidence text (a fog animation distance multiplied by
  zero) does not describe these HUD functions. Met by an established reusable compiler law plus January store order.
- `bitmap_direct`: "authoritative source or natural donor" — the unexplained-gap premise is refuted (callee ABI;
  body unchanged).
- `get_flash_color`: premise refuted by January's `lea` offsets, HCEA corroboration.

## 4. Converter and truncation helpers (asm, proposed separately; `scratch/workers/hud_draw/v2.c`)

### Emitted ownership
January's atlas lists `_fast_ftol_C`, `_real_argb_color_to_pixel32`, `_real_rgb_color_to_pixel32` and the already exact
`_real_alpha_intensity_to_pixel32` as `i` (inline COMDAT) copies selected from `hud_draw.obj`, with
`..\bitmaps\bitmaps_inlines.h` assertion literals (argb lines 89/188, rgb 201). 26 other January objects reference them
undefined (fast_ftol_C only from hud_nav_points, hud_unit, hud_weapon); no other January object defines them; in
`build/base` nothing defines them. `hud_draw.c` is the genuine selected owner, the same arrangement as the admitted
`decals.c::real_a_rgb_color_to_pixel32` (asm) and `hud_draw.c::real_alpha_intensity_to_pixel32`. Prototypes already
exist in `hud_draw.h`.

### `real_rgb_color_to_pixel32` (192/8, `01d6a96ad9a03f25`) and `real_argb_color_to_pixel32` (320/12, `ae86b8fdd729000f`)
- Both are the in-memory dword `FISTP`/`AND`/`SHL` packet with a stack-local `255.0f`, the same kind as the admitted
  `real_alpha_to_pixel32` and `real_a_rgb_color_to_pixel32`. 32-bit inline `fistp` is never VC7 C output; the prior
  natural-C RGB form has a recorded rounding counterexample.
- ARGB contains two complete conversions and asserts `verify == result` (a January-defined literal): a register-packed
  `result` (returned) and an in-memory masked `verify`. The block-scoped pass-1 longs share `verify`'s slot (frame
  0x18). This is the original self-verifying source structure. Probe `c3`: pass-1 packing in C -> 304 bytes, refuted.
- Both strict EXACT with ordinary C validation (`match_vassert` + `csprintf` with January's format and `"color"`).

### `fast_ftol_C` (56/0, `6d480d71829c2f3a`)
- FIST / FISUB / sign-corrected truncation; not expressible in C (a cast calls `__ftol2`).
- Compiler-mode signature: in `cachebeta.exe` the eight bytes after its 0x38-byte body are `CC` linker fill, not
  VC7 /O2 `90` alignment NOPs. Across every `symbols.json` code symbol only two Halo game functions have `CC` tails:
  `_fast_ftol_C` and `_ai_handle_spatial_effect`, the admitted `#pragma optimize("", off)` function in `ai.c`
  (the other 44 are XDK/CRT library routines; 8,056 functions end in compiler NOPs). A second signature: January's
  `hud_draw_meter` calls it out of line at all 12 sites.
- Measured: the asm body at /O2 (`v1.c`) is 64 bytes (same 22 instructions + 8 NOPs) and VC7 inlines it into
  `hud_draw_meter`, which regresses 1312/46 -> 1536/40. Bracketed by `#pragma optimize("", off)` / `on` (`v2.c`) it is
  56/0 EXACT and `hud_draw_meter` stays EXACT.

## 5. Experiment matrix

| File | Shape | Result |
| --- | --- | --- |
| v1.c | 3 asm helpers at /O2 | argb, rgb EXACT; fast_ftol_C 64 (padding); meter regressed |
| v2.c | + `#pragma optimize("", off)` around fast_ftol_C only | 19/4/0, no loss |
| c3.c | argb pass-1 packing in C | argb 304/12 (refuted) |
| p1.c | internal: corner test inline (sibling spelling) | direct, with_meter EXACT; internal one store transposition |
| p2.c | + framebuffer_blend_function before map[0] | internal EXACT |
| p3.c/p4.c | get_flash_color whole-color vector + n[3] scalar (+comment) | EXACT; 20/0/3 |
| all.c | p4 + v2 helpers | 23/0/0 |

## 6. Do-not-repeat
- Named corner local in either HUD vertex loop (flips the private ABI).
- `.rgb`/`.alpha` addressing in `get_flash_color` interpolation (January addresses offset 0 and `n[3]`).
- `fast_ftol_C` at /O2 (padding + inlined into meter); C packing for the ARGB first pass.

## 7. Orchestrator decisions requested
1. **Park reopen** (4 functions, 1,856 padded bytes): apply `scratch/workers/hud_draw/park_reopen.diff`
   (real file -> `p4.c`), remove the four `source/interface/hud_draw` entries from `config/parked.json`.
2. **Helper admission** (568 padded bytes): apply `scratch/workers/hud_draw/asm_helpers.diff` (`p4.c` -> `all.c`), or
   against the real file the three bodies from `asm_bodies_v2.txt`, inserted before `real_alpha_intensity_to_pixel32`.
   Owner rulings: (a) unit-local asm converter bodies (precedent decals.c); (b) the tightly bracketed
   `#pragma optimize("", off)` around `fast_ftol_C` only (precedent ai.c, same January CC-padding signature).
   Without (b), (a) can land for the two converters alone: `v4.c` (real file + argb + rgb only) measures 18/4/1,
   guard pass, parks 0 drift, `hud_draw_meter` exact.

## 8. Disposition
Active, not Matching. Real file unchanged at 16/23. With both decisions the unit measures 23/23 strict exact in the
whole-TU gate (object-level completion still blocked by the pre-existing `_fast_ftol`/`_square_root` extra COMDATs and
the `.bss` theta rename proposed by the 100K lane).
