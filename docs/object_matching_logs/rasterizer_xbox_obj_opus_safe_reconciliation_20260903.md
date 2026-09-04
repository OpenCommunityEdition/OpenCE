# `rasterizer_xbox.obj` selective Opus reconciliation (2026-09-03)

## Scope and authority

This lane selectively reconciles the defensible `rasterizer_xbox.c` work from
Opus branch `opus/small-families-30k-20260902` (source commits `bb0d690a3`,
`608c66380`, and return-type correction `eeedd72e5`) onto canonical base
`600855991`. It does not bulk-cherry-pick the donor. January's split COFF,
ordered relocations, assertion strings, stock Xbox XDK 3911 declarations, and
the established campaign tools remain the admission oracle. Later HaloCEA
sources and existing subsystem definitions were used only to corroborate
types, names, and behavior.

The baseline stable snapshot was
`scratch/before-rasterizer-xbox-20260903.json`: 8,245 evaluated functions and
5,733 strict exact. The admitted snapshot is
`scratch/after-transparent-prototype.json`: 8,245 evaluated and 5,780 strict
exact. The fail-closed stable diff reports **47 gained functions / 13,056
padded bytes / zero regressions**. Raw progress rises by 10,533 code bytes,
from 802,941 to 813,474, because the progress report counts meaningful bytes
rather than section padding.

## Exact admissions

The focused gate moves from 5 exact / 0 residual / 90 unwritten to **52 exact /
6 documented residual / 37 unwritten**. The 47 newly exact functions are:

- XDK-preserved wrappers: `_Direct3D_Release@0`, `_IDirect3D8_Release@4`,
  `_IDirect3D8_CreateDevice@28`, `_D3DDevice_SetRenderState`,
  `_D3DDevice_SetTextureStageState`, `_IDirect3DDevice8_Release@4`,
  `_IDirect3DDevice8_GetDeviceCaps@8`, `_IDirect3DDevice8_Present@20`,
  `_IDirect3DDevice8_GetBackBuffer@16`,
  `_IDirect3DDevice8_SetRenderTarget@12`, `_IDirect3DDevice8_Clear@28`,
  `_IDirect3DDevice8_SetViewport@8`,
  `_IDirect3DDevice8_SetRenderState@12`,
  `_IDirect3DDevice8_SetTextureStageState@16`,
  `_IDirect3DDevice8_SetVertexData2s@16`, `_IDirect3DDevice8_Begin@8`,
  `_IDirect3DDevice8_End@4`, `_IDirect3DSurface8_Release@4`,
  `_IDirect3DSurface8_GetDesc@8`, and `_IDirect3DSurface8_LockRect@16`.
- Lifecycle and windows: `_rasterizer_preinitialize__fill_you_up_with_the_devils_cock`,
  `__rasterizer_initialize`, `__rasterizer_dispose`,
  `__rasterizer_frame_begin`, `__rasterizer_frame_end`,
  `__rasterizer_present`, `__rasterizer_windows_begin`,
  `__rasterizer_windows_end`, `__rasterizer_window_get_fog`,
  `__rasterizer_window_set_fog`, and `__rasterizer_window_end`.
- State, texture, lighting, and targets:
  `_rasterizer_get_bitmap_default_hardware_format`,
  `_rasterizer_filthy_bitmap_default_initialize`,
  `_rasterizer_set_texture_bitmap_data`, `_rasterizer_set_texture_direct`,
  `_rasterizer_set_texture_direct_non_blocking`,
  `_rasterizer_set_texture_non_blocking`, `_rasterizer_get_target`,
  `_rasterizer_set_target`, `_rasterizer_set_target_as_texture`,
  `_rasterizer_set_framebuffer_blend_function`,
  `_rasterizer_set_vertex_shader`, `_rasterizer_set_pixel_shader`,
  `_rasterizer_set_model_lighting_point_light`,
  `_rasterizer_set_model_lighting_distant_light`,
  `_rasterizer_set_model_lighting`, and
  `_rasterizer_secondary_render_target_debug`.

The five pre-existing exact leaves remain exact:
`__rasterizer_reset_state`, `_rasterizer_spin_begin`,
`_rasterizer_spin_end`, `__rasterizer_set_vblank_callback`, and
`_rasterizer_get_stencil_mode`.

The XDK wrappers are emitted by the stock header's own `D3DINLINE` definitions.
No handwritten COM dispatchers, altered force-inline policy, raw bytes, or
inline assembly are used.

## Semantic ownership and headers

Address-derived global names were replaced with evidence-backed subsystem
names in source and `config/symbols.json`: `rasterizer_xbox_d3d_globals`,
`rasterizer_state_cache`, `framebuffer_blend_function_states`, `d3d_palette`,
`suppress_window_begin_end`, and `previous_window_index`. XDK wrapper symbols
are named from their standard decorated exports instead of `code_<address>`.
The large D3D private block remains one layout-checked aggregate so its split
COFF ownership and relocation anchors stay faithful.

Four return types in `rasterizer_xbox.h` are corrected from January's own
return sequences: preinitialization and the two direct bitmap setters return
`boolean`, while `rasterizer_set_texture` returns `union point2d *`. The
backend `_rasterizer_windows_begin` and its public wrapper are `void`; callers
discarded the old fictitious boolean result.

Cross-TU declarations live in owner or narrow associated headers. Broad
header experiments were explicitly rejected after stable sweeps showed VC7
definition-position regressions in unrelated consumers:

- adding bitmap query declarations to broad bitmap headers regressed
  `bitmap_group.obj` and `game_engine.obj`, so they live in
  `bitmap_group_internal.h` and `bitmap_strings_internal.h`;
- adding `main_get_window_count` to `main.h` perturbed shell semantic data, so
  it lives in `main_internal.h`;
- adding the full backend interface to the public Xbox header regressed
  `rasterizer_xbox_draw_primitives.obj`, so backend declarations live in
  `rasterizer_xbox_internal.h`;
- expanding `struct rasterizer_globals_definition` in `rasterizer.h` regressed
  an exact decals function solely through compiler allocation sensitivity, so
  this owner uses a private, offset-checked late-field overlay instead; and
- the transparent-geometry disposer is declared by its existing narrow owner
  header and included by this TU.

## Credible fuzzy boundary

Six complete ordinary-C bodies remain non-exact and are parked in
`config/parked.json` with current target/base hashes and objdiff scores:

| Function | Target/base size | Relocations | Similarity | Residual boundary |
| --- | ---: | ---: | ---: | --- |
| `_SetupSmartStates` | 128 / 128 | 14 / 15 | 75.89286% | January uses an aggregate-copy `rep movsd`; the honest array spelling calls `csmemcpy`. |
| `__rasterizer_window_begin` | 416 / 416 | 39 / 39 | 99.03226% | One dependency-independent load/store transposition. |
| `_rasterizer_set_frustum_z` | 320 / 320 | 22 / 22 | 96.521736% | Equivalent commutative x87 projection order. |
| `_rasterizer_set_model_skinning` | 320 / 320 | 18 / 18 | 95.92233% | Instruction-for-instruction EAX/ECX allocation exchange. |
| `_rasterizer_set_stencil_mode` | 784 / 784 | 78 / 78 | 95.382774% | Initial debug-gate load/if-conversion schedule. |
| `_rasterizer_set_texture` | 496 / 432 | 35 / 31 | 86.17284% | January compiler-clones a success tail; six natural CFGs retain one shared tail. |

Each retained body has complete semantic behavior and target-supported names,
types, calls, diagnostics, and enum constants. Exact-only representation casts,
one-use aggregate inventions, register/volatile forcing, fake dependencies,
and nonsensical control flow were rejected. The remaining 37 listed sections
stay unwritten; no speculative large body was admitted.

## Verification

- `python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox --all`:
  52 exact / 6 residual / 37 unwritten.
- Full `ninja`: clean build apart from the pre-existing accepted const warning
  at the blocking texture-cache boundary; semantic report has 5,789 semantic
  exact, 5,806 accepted exact, and zero unit errors.
- Raw progress: 813,474 / 2,198,102 code bytes, 5,738 / 11,060 functions;
  all progress 37.01%, 388 / 833 files.
- Stable diff: 47 gained / 13,056 padded bytes / zero regressions.
- `python -m tools.parked_functions`: 185 active / 0 stale / 0 invalid,
  including the six entries above.
- `python tools/fake_match_scan.py` over all changed C files: zero review leads.
- `python -m pytest -q`: 261 passed.
- `git diff --check`: clean.
