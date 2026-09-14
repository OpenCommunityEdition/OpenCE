# Rasterizer Lights: opus5 150K wave w2 park re-investigation and residual audit (2026-09-14)

Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Scope: `source/rasterizer/rasterizer_lights.c` only.

## Result

**No source change landed.** `git diff --stat -- source/rasterizer/rasterizer_lights.c` is empty.

| gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 9 | 4 | 0 |
| final | 9 | 4 | 0 |

Park drift: 0 (`_rasterizer_lens_flares_draw`, `_rasterizer_lens_flares_submit_occlusion_tests` OK).
Owner census vs `build/base`: no change. Fake-match scan: 0 leads. Guard: pass.

Two owner/orchestrator decisions are documented below with measured candidates; both are outside a worker's authority
(the `_point_from_line3d` guard and the park lock).

## Per-function outcomes

### `_rasterizer_lights_reset_for_new_map` (48 B) — owner-blocked, original buffer overrun

The only byte difference is the first `csmemset` length: January pushes `0x7722`, the natural body `0x7700`.
The January split `.bss` proves the array is exactly `0x7700` bytes: `local_lens_flare_occlusion_test_results2` at 0
(262,176 bytes), `local_lens_flare_occlusion_test_results` at 262,176, `local_lens_flare_parameters` at 292,640.
The parameters base is fixed by `lens_flare_submit_parameter_get` (`lea eax,[eax*8+parameters]`, addend 0) and the
results base by `rasterizer_lens_flare_submit` (addend 0) and `lens_flare_occlusion_test_results_get` (addend 2).
January therefore clears 34 bytes of `local_lens_flare_parameters[0]`. Every spelling that emits `0x7722` at the results
base is a buffer overrun, which brief section 5 forbids preserving. All ten history blobs that compile it exact use
`sizeof(array)+sizeof(array[0])`; the 2026-09-03 admission and 2026-09-13 donor audit already rejected that form.

Reopen: an explicit owner ruling admitting a preserved-overrun `BUG` comment for this reset.

### `_rasterizer_lens_flares_submit_occlusion_tests` (400 B, parked) — exact candidate gated on the point_from_line3d ruling

The residual is the store schedule of the `direction` copy: January stores `direction.i` before the window-index
compare and `.j/.k` after it; the candidate stores all three first. Sizes and all 22 relocations agree. January calls
`_point_from_line3d` out of line.

Every history blob that closes this body (`f1c3f67b04`, `d09f77f8f0`, `e4473191aa`, `9f83f23e94`, `9b1ebc491d`) fails the
guard: each emits the 48-byte `_point_from_line3d` COMDAT because none defines `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`.

Measured diagnostics (scratch only):

| candidate | change | occlusion_tests | other rows | guard |
| --- | --- | --- | --- | --- |
| `p1.c` | remove this TU's `#define`/`#undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` | **EXACT** | 9 baseline exact kept; draw park sha unchanged | fails (new `_point_from_line3d` .text owner only) |
| `d1.c` | define kept + unused extra `__inline` definition at top | residual | unchanged | pass |
| `d2.c` | define kept + unused extra prototype | residual | unchanged | pass |
| `d3.c` | define kept + TU-local copy of the same inline body before use | **EXACT** | unchanged | fails |

New measured law (VC7 13.00.9254): a call to a *visible* `__inline` definition that the compiler declines to inline is
optimized differently from a call to an external prototype (here: lazy stores of an address-taken local around a later
compare). Declaration count and header position are not the lever (d1/d2 negative, d3 positive). This implies January's
`rasterizer_lights.obj` was compiled with the inline body and emitted a `point_from_line3d` COMDAT that the linker folded;
csplit attributes the surviving copy to `action_charge.obj`.

Proposal: add this 400-byte function to the pending owner ruling on the `point_from_line3d` COMDAT policy (previous
handoff, remaining work item 1). If admitted, the change is TU-local (delete the define/undef lines 83 and 103) and the
park can be removed; candidate `scratch/workers/rasterizer_lights/p1.c`
(sha256 `14cdf5a1ce6ebde214f9343c2329e7dbf9de6c097c4b9e7335fd0409b1735b71`).

### `_rasterizer_lens_flare_submit_for_cluster` (400 B) — scheduler tie, five shapes exhausted

Same frame (0x54), 18 relocations and store order. Differences: January preloads the `j`/`k` signed direction bytes into
ECX/EDX before `i`, then does fild/fmul(1/127)/fstp per component; and in the loop tail January loads the marker count
after `add esp,0x3c; inc ebx`. January `structure_lens_flares.obj` shows the same preload pattern for direct
per-component statements, but that spelling does not reproduce it here.

| shape | result |
| --- | --- |
| c1 direct `direction.i= marker->i_direction*(1.0f/127.0f)` statements | loads interleave; tail unchanged |
| c2 c1 with `(real)` casts (structure_lens_flares spelling) | identical to c1 |
| c3 `set_real_vector3d` with `(real)` casts | identical to baseline |
| c4 c2 with a `char direction[3]` member | identical to c1 (HCEA DB names are i/j/k anyway) |
| c5 c2 with direction/up at outer block scope | identical to c1 |

The inline-visible `point_from_line3d` context (`p1`) does not change this function. Reopen only with new spelling
evidence for the byte-direction decode or a law explaining VC7 integer-load hoisting before `fild`.

### `_rasterizer_lens_flares_draw` (2,240 B, parked) — unchanged; data-literal correction proposed

Three residual sites remain: an x87 operand swap at `+0x202` (`occlusion_fraction` vs `light_brightness`), the i-term
operand order of the inlined `dot_product3d(&direction, &camera_offset)` at `+0x31c`, and an interleave of `fmul st(1)` /
`push edx` with the `rasterizer_widget_set_texture` argument loads at `+0x738`.

Shapes: swapping the two real declarations is inert; a separate `distance_fade` local grows the frame to 0xb8
(rejected); `light_brightness = occlusion_fraction*light_brightness*uncompress_int8_to_real(...)` changes the x87 shape
(rejected); an inlined `point_from_line3d` for the reflection point (`p2`) regresses many dot-product terms.

New finding: January `.rdata` proves the draw assertion texts are
`reflection->animation_period!=0.0f`, `animation_color.alpha>=0.0f && animation_color.alpha<=1.0f`,
`animation_color.red >=0.0f && animation_color.red <=1.0f`, `animation_color.green>=0.0f && animation_color.green<=1.0f`
and `animation_color.blue >=0.0f && animation_color.blue <=1.0f`. The parked body spells them with extra spaces, so the
object owns five wrong literal COMDATs. Candidate `scratch/workers/rasterizer_lights/D4.c`
(sha256 `03c1c82aceca3ec4df365b30b03a660ee6f1e0cc77d7c3ed82b558f0db6f75cf`) corrects only those texts: gate 9/4/0, guard
pass, park drift 0 (code sha unchanged), and the five literal owners then match January. The body is park-locked, so this
is an orchestrator edit.

## Evidence used

January split COFF (bytes, relocations, `.bss` labels, `.rdata` texts); `tools.campaign.alndiff`/`tinfo`;
January `structure_lens_flares.obj` disassembly for the byte-direction decode; HCEA dx9 reconstructions
(`rasterizer_lens_flare_submit_for_cluster.c`, `rasterizer_lights_reset_for_new_map.c`,
`rasterizer_lens_flares_submit_occlusion_tests.c`, `rasterizer_lens_flares_draw.c`) for semantics only; git history blobs
and `tools/campaign/branch_sweep.py`. No supplied binary was executed.
