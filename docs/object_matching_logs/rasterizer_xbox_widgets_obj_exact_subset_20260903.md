# `rasterizer_xbox_widgets.obj`: exact-only reconciliation (2026-09-03)

## Outcome

This packet reconciles the natural exact-code subset of the unfinished Opus
widgets reconstruction onto canonical commit `fb0f40762`. The Opus branch was
used only as a candidate atlas; January's split COFF is the matching authority.

The focused gate moves from 2 exact / 0 residual / 23 unwritten functions to
17 exact / 0 residual / 8 unwritten functions. The strict gain is 15 functions
and 2,192 padded text bytes. The three donor residuals were excluded rather
than parked or claimed:

- `__rasterizer_widget_begin` (1,152 bytes);
- `__rasterizer_widget_draw_sprite2d` (528 bytes);
- `__rasterizer_widget_submit_occlusion_test` (640 bytes).

## Exact admissions

The six admitted Halo bodies are:

- `__rasterizer_widget_draw_sprite3d` (576 bytes);
- `__rasterizer_widget_get_occlusion_test_result` (208 bytes);
- `__rasterizer_widget_set_texture` (192 bytes);
- `__rasterizer_widget_set_tint_factor` (64 bytes);
- `__rasterizer_widget_submit` (368 bytes);
- `_rasterizer_widget_project_billboard` (448 bytes).

Nine naturally emitted stock-XDK wrapper COMDATs add another 336 bytes:
`D3DDevice_SetTextureStageState`, `IDirect3DDevice8_Begin`,
`IDirect3DDevice8_End`, `IDirect3DDevice8_GetVisibilityTestResult`,
`IDirect3DDevice8_SetTextureStageState`, and the four typed vertex-data
wrappers used by the retained draw paths.

Five additional target XDK sections are authenticated and named from the same
stock `D3D8.h` declarations but remain unwritten because their only donor
callers were excluded. No byte credit is taken for them.

## Name and ownership evidence

All former `code + address` entries in this object are now either authenticated
stock XDK wrapper names or the semantic private helper name. HaloCEA names the
corresponding helper `rasterizer_widget_project_billboard`, and January's two
widget callers and its byte-identical sibling in the Xbox lights translation
unit corroborate that role. It remains translation-unit-private. Moving the
duplicated helper body into a broadly included header would change January's
inline/COMDAT schedule and is intentionally deferred.

The one-byte warning latch is now the private, convention-consistent
`warned_about_too_many_transparent_geometry_groups`, replacing the former
`bss_004662ea` identifier. No header prototype was added: every admitted Halo
entrypoint already has its owning rasterizer interface, while the helper is
private to this translation unit.

## Source-quality boundary

The retained bodies use `real`, typed points/vectors, named flags and renderer
structures, explicit final returns, and stock XDK calls. They add no assembly,
forced inline, pragma, volatile/register steering, raw address, raw byte
emission, type punning, fake dependency, or fabricated control flow. Including
`<xtl.h>` emits only the wrapper COMDATs naturally required by admitted calls;
it does not emit `point_from_line3d`.

The excluded donor bodies remain useful research leads but are not canonical
source until they become natural exact matches or receive separately reviewed
semantic parks.

## Verification

- focused gate after symbol regeneration: 17 exact / 0 residual / 8 unwritten;
- all 572 candidate translation units compile successfully;
- stable comparison covers 8,245 target sections and reports exactly 15 gained
  functions / 2,192 padded text bytes with zero regressions;
- tooling tests: 261/261 pass;
- focused fake-match scan: zero findings;
- candidate symbol scan finds no `point_from_line3d` COMDAT;
- `config/parked.json` is unchanged and contains no widgets entry, so the
  packet neither stales nor bypasses a parked verdict;
- JSON parse and `git diff --check`: pass.

## Current-HEAD integration cleanup

The combined integration branch also carries the reviewed transparent-geometry
subset. Backend widget declarations now live in the closest associated
`rasterizer_widgets.h`; `rasterizer.c` and the Xbox implementation include
that owner instead of maintaining consumer-local declarations. Public wrapper
and backend signatures now agree with the Xbox implementation and HCEA
evidence: tint is `real`, z-buffer enable is one `boolean`, sprite2d owns its
full typed argument list, and occlusion submission returns `long` from
point/radius/index arguments. The wrapper tail thunks remain exact.

`rasterizer_spin_begin(short profile)` and both nonblocking texture helpers
are owned by `rasterizer_xbox.h`. Slot 26 is spelled with
`_rasterizer_profile_screen_effect`, and the failure path uses
`_error_silent`. The high-bit assertion retains its original expression
because `match_assert` stringifies it: changing it to `TEST_FLAG` preserves
instructions but changes January's assertion-string relocation and therefore
is not exact. The private projection helper received indentation-only cleanup.

The owner header is included before the broad legacy `rasterizer.h` in the
three widget consumers. A compatibility guard leaves the legacy declarations'
preprocessed token stream unchanged for unrelated consumers; this is required
because VC7 allocation in `decals.obj` is definition-position sensitive. The
final combined stable diff gains 23 functions / 2,608 padded text bytes with
zero regressions.
