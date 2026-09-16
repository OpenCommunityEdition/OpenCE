# `light_volumes.obj`: Opus5 250K house-clean lane, wave w3d (structural pass, 2026-09-15)

## Result

Nothing landed. `source/objects/widgets/light_volumes.c` is **unchanged** (`git diff --stat` empty).
Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 9 / residual 1 /
unwritten 0**, row-for-row identical. Park drift 0, fake scan 0 leads.

The parked `_light_volume_render` was re-investigated in scratch only. This pass produced a complete
frame/relocation census, **three byte-proven source defects** (two of which together make January's
loop header reproduce instruction-for-instruction), and **three corrections to the recorded w2 evidence
and to the `laws_w3` C1 premise for this row**. No candidate reached strict exact; the retained parked
body is still the smallest body measured.

| | padded / relocs / frame / sha16 | instrs | alndiff hunks |
| --- | --- | ---: | ---: |
| January target | 912 / 47 / 0xa4 / `2e43dbc111ad1255` | 291 | - |
| retained (park base) | 928 / 47 / 0xa8 / `a4e273c7939882fc` | 296 | 64 |
| best this wave (v12) | 944 / 47 / 0xac | 306 | 58 |
| loop-only fix (v7) | 944 / 47 / 0xac | 309 | 59 |

## Duplicate checks

Ledgers read: `light_volumes_obj_opus5_150k_w2_20260914`, `..._credible_fuzzy_closeout_20260831`,
`..._jonas_exhaustive_census_20260830`, `..._jonas_distance_fade_schedule_fixed_point_20260826`,
`..._jonas_interpolator_recovery_20260826`, `..._jonas_partial`. Laws applied as detectors:
laws_w2 A4, A5, A6, A10, A12, A18, A20, A22; laws_w3 A24, A31, A35, A38, A44, plus the C1 third-pass
row for this function and R14/R15/R16. The w2 starting point
`scratch/workers/render_particles_rasterizer_xbox_decals_etc/lvJ.c` was re-measured in this tree
(944/48) rather than assumed.

## Census (mandatory, recorded before any shape)

**Relocations: 47 == 47.** No missing and no extra call or global. The gate tag is `[size, sha]` only.
Calls: `_datum_get`, `_tag_get`, `_light_volume_interpolate_frames`, `_object_get_marker_by_name`,
`_object_get_function_value`, `_rasterizer_widget_begin`, `_rasterizer_widget_set_texture`,
`__CIpow` x4, `_rgb_colors_interpolate`, `_real_argb_color_to_pixel32`,
`_rasterizer_widget_draw_sprite3d`, `_rasterizer_widget_end`. January's object contains no
`_dot_product3d` and no `_vector_from_points3d` COMDAT.

**Frame: `sub esp,0xa4` vs `sub esp,0xa8` - exactly one extra dword local.**

| object | target | ours (retained) |
| --- | --- | --- |
| `marker` (108 B) | `[ebp-0xa4 .. -0x39]` | `[ebp-0xa8 .. -0x3d]` |
| `color` (16 B) | `[ebp-0x38 .. -0x29]` | `[ebp-0x3c .. -0x2d]` |
| loop `position` (12 B) | `[ebp-0x18 .. -0x10]`, **shared with `delta`** | `[ebp-0x2c .. -0x24]` |
| `delta` (12 B) | the same cells | **no slot at all - x87-stack resident** |
| scalars | 7 dwords: radius `-0x28`, color_fraction `-0x24`, divisor `-0x20`, pow1#1 exponent `-0x1c`, sprite_index `-0xc`, distance_fade `-8`, pow1#2-4 exponent / brightness_fraction `-4` | 8 dwords: radius `-0x20`, pow1#1 exp `-0x1c`, pow1#2 exp `-0x18`, brightness_fraction `-0x14`, sprite_index `-0x10`, pow1#3-4 exp `-0xc`, `-8`, distance_fade / color_fraction `-4` |
| `[ebp+8]` (`object_index` home) | `function_value`, then the `count-1` temp, then `offset_fraction` (13 refs) | `parallel_factor`, the `count-1` temp, `offset_fraction` (20 refs) |

The extra dword is a pow1 exponent home: January reuses one home (`[ebp-4]`) for inlined pow1
instances #2/#3/#4; ours uses two.

## Byte-proven source defects found

**(a) `definition->count` is re-read inside the loop instead of using the `count` local (A5 class).**
January loads `[ebx+0x6e]` once (`mov ax,[ebx+0x6e]; test ax,ax; jle; movsx ecx,ax; dec ecx`), converts
`count - 1` to `real` once in the preheader (`mov [ebp+8],ecx; fild [ebp+8]; fstp [ebp-0x20]`) and
divides with `fdiv [ebp-0x20]`. The retained `(real)(definition->count - 1)` re-loads the field and
emits `fidiv [ebp+8]` in the body (the conversion is not hoisted). HCEA independently reconstructs a
named `float divisor = (float)(count - 1);` local.

**(b) the sprite loop is a `for` with a `short` index, not `do { } while (--count)`.**
January emits a 32-bit shadow index in `[ebp-0xc]` and a 16-bit trip counter in `DI`
(`movzx edi,ax` ... `inc ecx; dec edi; mov [ebp-0xc],ecx; jne`); the retained form emits `test di,di`
and no shadow.

**(a)+(b) together make January's loop header instruction-identical** (`scratch/workers/w3d_glow_light_volumes/v7.c`;
target[162..179] equal, only slot numbers and one `lea esp,[esp]` loop-head alignment nop differ, and
that nop is a downstream consequence of the earlier byte offset). Hunks 64 -> 59.

**(c) `real function_value = 1.f;`** - January `mov dword ptr [ebp+8],0x3f800000` at +0xd3, between the
far-fade `fld [ebx+0x38]` (+0xd0) and its `fcomp` (+0xda): the A38 signature of a declaration in the
block enclosing the far-fade `if`. The retained body has no initializer.

## Corrections to the recorded evidence (important for future waves)

**(d) The retained hand-written dot expressions are already January's association and, for the i term,
January's operand order.** January's rule here is "`fld` the SECOND source operand, `fmul` the first":
`marker.i*cam.i` -> `fld [render+28]; fmul [ebp-0x68]`, `cam.k*marker.k` -> `fld [ebp-0x60]; fmul
[render+36]`. The retained `marker.i*cam.i + (cam.j*marker.j + cam.k*marker.k)` predicts all three
target terms, and the `A + (B + C)` association is confirmed by January's `faddp` order ((k+j)+i).
Our build reproduces only the i term; the j/k terms come out global-first because the A22 younger-leaf
rule overrides source order while the delta values are alive on the x87 stack.
The w2 ledger's "wrong operand order: the HCEA hand-written dot-product order" is a mis-attribution.

**(e) `laws_w3` C1's "try genuine `dot_product3d` calls first" is MEASURED and NEGATIVE.**
`dot_product3d(&marker.matrix.forward, &render.camera.forward)` makes all three terms local-first -
it breaks the i term the hand expansion already gets right - and grows the body to 944. Swapping the
arguments is byte-identical (w2's "canonicalised by VC7" note confirmed). A35(1)'s "i global-first,
j/k local-first through dot_product3d parameters" does not reproduce here. **A35 does not fire on this
function; the C1 row should be retired.**

**(f) `vector_from_points3d` does NOT reproduce the delta memory stores** (contradicts w2 evidence #2).
It leaves `delta` entirely on the x87 stack, exactly like the hand-written subtraction, and so does a
`(real_vector3d const *)&delta` cast. The `_vector_from_points3d` surplus-COMDAT owner ruling (R14) is
therefore **not** a blocker for this function.

**(g) Mechanism for the delta home.** The only measured form that reproduces January's immediate
`fstp [ebp-0x18]/[ebp-0x14]/[ebp-0x10]` stores is the aggregate's address escaping to a real
(non-inlined) call - the camera delta and the loop's sprite position being one function-scope
`real_point3d` passed to `rasterizer_widget_draw_sprite3d`. That homes the values but at
`[ebp-0x20..-0x18]` (frame 0xac) and does not fix the j/k operand order, so it is not January's form.

## Shapes measured (all scratch, `scratch/workers/w3d_glow_light_volumes/`)

| shape | change | size / relocs / frame | instrs | hunks |
| --- | --- | --- | ---: | ---: |
| lv_base | retained parked body | 928 / 47 / 0xa8 | 296 | 64 |
| lvJ | w2 starting point re-measured here | 944 / 48 | - | - |
| v1 | `dot_product3d` x2 + `real_vector3d delta` | 944 / 47 | 310 | - |
| v2 | v1 with swapped arguments | 944 / 47 | - | - |
| v3 | v1 + `vector_from_points3d` | 928 / 47 | 301 | - |
| v4 | retained + `dot_product3d` x2 + point->vector cast | 944 / 47 | - | - |
| v5 | v4 with swapped arguments | 944 / 47 | - | - |
| v6 | v4 + one shared `real_point3d` (diagnostic) | 944 / 47 | 307 | - |
| v7 | retained + `count - 1` + `for`/`short sprite_index` | 944 / 47 / 0xac | 309 | 59 |
| v8 | v7 + `dot_product3d` x2 | 944 / 47 | - | 61 |
| v10 | v7 + `real function_value = 1.f;` | 944 / 47 / 0xb0 | 306 | 63 |
| v11 | v10 + `dot_product3d` x2 | 944 / 47 | 304 | 65 |
| v12 | v7 + one shared `real_point3d` | 944 / 47 / 0xac | 306 | 58 |
| v13 | lv_base + `#include "interface/hud_draw.h"` | 928 / 47, sha `a4e273c7939882fc` | 296 | byte-identical |
| v14 | v7 + the same include | 944 / 47 | 309 | - |

Every probe kept all nine sibling rows EXACT and the `_point_from_line3d` guard passing. The owner
census of the `dot_product3d` probes adds only `_dot_product3d`, which laws_w2 A4 accepts as systemic.

## Residual classification

Allocation / x87-scheduling tie sitting on top of the three identified source defects. The three
remaining differences move together and none of the twelve shapes separated them:
1. whether `delta` is memory- or x87-resident;
2. whether `parallel_factor` takes `[ebp+8]` (`fst [ebp+8]; fcomp`) or stays on the x87 stack
   (January's non-popping `fcom`);
3. which inlined pow1 instances share an exponent home, and whether that home is written by `fst`
   (January) or an integer `mov` pair (ours).

## Proposal (orchestrator decides; zero credit)

Add to `source/objects/widgets/light_volumes.c`, immediately after `#include "cseries/cseries.h"`:

    #include "interface/hud_draw.h"

This is the unit's only C4013 (`scratch/gate4/c4013.txt`: `objects\widgets\light_volumes.c(353):
warning C4013: 'real_argb_color_to_pixel32' undefined; assuming extern returning int`). The genuine
owner is `source/interface/hud_draw.h:44`, and the sibling widget file in the same directory,
`source/objects/widgets/lightning.c:50`, already includes it for the identical call. Measured
byte-identical (928/47, sha `a4e273c7939882fc`, all ten rows unchanged), so it is a pure fidelity fix.
It is a consumer-side `#include`, not a header edit. Not landed here because brief section 8 requires
an empty file diff in a unit where nothing lands (E18 class).

## Reopen criterion (replaces the w2 criterion)

Reopen on a source-level fact that gives `delta` a stack home at `[ebp-0x18..-0x10]` **without** adding
a frame slot - something other than sharing one aggregate with the loop's `position`, which homes it at
`[ebp-0x20..-0x18]` (frame 0xac). Apply (a), (b) and (c) together; each is independently byte-proven.

Do not retry: `dot_product3d` in either argument order; `vector_from_points3d`; the
`(real_vector3d const *)` cast; declaring `function_value` in the outer block; or any w2 probe
(declaration order, `function_value` scoping, two-statement versus nested pow1 arguments, `pow` versus
`power` inside `pow1`, ternary or result-variable `pow1`, the if-negate ABS form, making
`light_volume_globals` static).
